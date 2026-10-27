###################################################################################################
# Mint-Bill: Top-Level CMake Configuration
#
# This CMake file defines the global build configuration for the Mint-Bill application.
# It controls:
#   • Project naming (with or without deployment version suffixes)
#   • Global build options (Debug/Release, C/C++ standard levels, warnings, defines)
#   • Optional feature toggles:
#         - BUILD_PROJECT_FOR_DEPLOY: Build the deploy-ready version of the application
#         - BUILD_PROJECT: Build the full project with all modules
#         - UNIT_TESTS: Enable unit-testing targets
#         - STATIC_CODE_ANALYSIS: Enable Cppcheck/static analysis integration
#         - CODE_COVERAGE: Enable gcovr code-coverage support
#
# When BUILD_PROJECT is enabled, this file:
#   • Finds and configures all required third-party dependencies
#   • Builds the main application executable
#   • Recursively adds subdirectories (gui, components, data, storage, models, features, utility)
#   • Links all modules and system libraries together
#
# This file acts as the root of the Mint-Bill build system, orchestrating every subsystem and
# enforcing consistent compilation settings across the entire project.
###################################################################################################
cmake_minimum_required(VERSION 3.25)

option(BUILD_PROJECT "Build the project" OFF)
option(INSTALL_APPLICATION "Installing Application" OFF)
option(UNIT_TESTS "Perform unit tests" OFF)
option(STATIC_CODE_ANALYSIS "Perform static code analysis" OFF)
option(CODE_COVERAGE "Perform code coverage" OFF)
set(APP_NAME "mint-bill" CACHE STRING "Executable name (mint-bill, mint-bill-tme, mint-bill-tce)")
set(APP_ID "org.app.mint-bill" CACHE STRING "Application identifier")
set(APP_DISPLAY_NAME "Mint Bill" CACHE STRING "Pretty application name shown in menus")

# The release version is PROJECT_VERSION_NUM in scripts/build.sh.
# -DMINT_BILL_VERSION overrides that for the RPM build.
if(NOT MINT_BILL_VERSION)
	file(STRINGS "${CMAKE_SOURCE_DIR}/scripts/build.sh" _mint_bill_version_line
		REGEX "^PROJECT_VERSION_NUM=[0-9]+\\.[0-9]+\\.[0-9]+$")
	string(REGEX REPLACE "^PROJECT_VERSION_NUM=" "" MINT_BILL_VERSION "${_mint_bill_version_line}")
endif()
if(NOT MINT_BILL_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
	message(FATAL_ERROR "PROJECT_VERSION_NUM in scripts/build.sh must be MAJOR.MINOR.PATCH")
endif()

project(${APP_NAME} VERSION ${MINT_BILL_VERSION} LANGUAGES CXX C ASM)


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(APP_DATA_DIR "share/${APP_ID}")
set(MINTBILL_UI_PATH_DEFAULT  "${CMAKE_SOURCE_DIR}/gui/mint-bill.ui")
set(MINTBILL_CSS_PATH_DEFAULT "${CMAKE_SOURCE_DIR}/gui/mint-bill.css")
set(MINTBILL_DB_PATH_DEFAULT "${CMAKE_SOURCE_DIR}/mint-bill.db")
set(MINTBILL_EMAIL_TXT_PATH_DEFAULT "${CMAKE_SOURCE_DIR}/email.txt")
set(MINTBILL_EMAIL_HTML_PATH_DEFAULT "${CMAKE_SOURCE_DIR}/email.html")

set(MINTBILL_UI_PATH  "${MINTBILL_UI_PATH_DEFAULT}"  CACHE STRING "Path to main UI file")
set(MINTBILL_CSS_PATH "${MINTBILL_CSS_PATH_DEFAULT}" CACHE STRING "Path to CSS file")
set(MINTBILL_DB_PATH "${MINTBILL_DB_PATH_DEFAULT}" CACHE STRING "Development database path")
set(MINTBILL_DB_TEMPLATE "${MINTBILL_DB_PATH_DEFAULT}" CACHE STRING "Packaged template database path")
set(MINTBILL_EMAIL_TXT_PATH "${MINTBILL_EMAIL_TXT_PATH_DEFAULT}" CACHE STRING "Path to plain-text email template")
set(MINTBILL_EMAIL_HTML_PATH "${MINTBILL_EMAIL_HTML_PATH_DEFAULT}" CACHE STRING "Path to HTML email template")

if(BUILD_PROJECT)
        find_package(CURL REQUIRED)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)
        pkg_check_modules(CAIROMM REQUIRED cairomm-1.16)
        pkg_check_modules(POPPLER REQUIRED poppler-cpp)
	if(EXISTS "/tmp/turso-src/target/release/libturso_sqlite3.a")
		set(_turso_library_default "/tmp/turso-src/target/release/libturso_sqlite3.a")
	elseif(EXISTS "/opt/turso/libturso_sqlite3.a")
		set(_turso_library_default "/opt/turso/libturso_sqlite3.a")
	else()
		set(_turso_library_default "/tmp/turso-src/target/release/libturso_sqlite3.a")
	endif()
	set(TURSO_LIBRARY "${_turso_library_default}" CACHE FILEPATH "Path to the Turso sqlite3 static library")
	if(NOT EXISTS "${TURSO_LIBRARY}")
		message(FATAL_ERROR "Turso library not found at ${TURSO_LIBRARY}. Build it with: cd /tmp/turso-src && cargo build -p turso_sqlite3 --release")
	endif()

	if(INSTALL_APPLICATION)
		include(GNUInstallDirs)
		set(MINTBILL_INSTALL_BINDIR "${CMAKE_INSTALL_FULL_BINDIR}")
		set(MINTBILL_INSTALL_LIBEXECDIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}")
		add_compile_definitions(MINTBILL_USE_USER_DATA_DIR=1)
	endif()

        add_compile_options(
                -Wall
                -Werror
                -Wextra
                -Wunused
                -Wpedantic
                -Wconversion
                -Wuninitialized
                -Wredundant-move
                -Wenum-conversion
                $<$<CONFIG:DEBUG>:-g3>
                $<$<CONFIG:DEBUG>:-Og>
        )

        add_compile_definitions(
                $<$<CONFIG:DEBUG>:DEBUG>
                $<$<CONFIG:DEBUG>:TRACE_ENABLED>
		MINT_BILL_APP_ID="${APP_ID}"
		MINT_BILL_APP_NAME="${APP_NAME}"
		MINTBILL_UI_PATH="${MINTBILL_UI_PATH}"
		MINTBILL_CSS_PATH="${MINTBILL_CSS_PATH}"
		MINTBILL_DB_PATH="${MINTBILL_DB_PATH}"
		MINTBILL_DB_TEMPLATE="${MINTBILL_DB_TEMPLATE}"
		MINTBILL_EMAIL_TXT_PATH="${MINTBILL_EMAIL_TXT_PATH}"
		MINTBILL_EMAIL_HTML_PATH="${MINTBILL_EMAIL_HTML_PATH}"
        )

	add_executable(${PROJECT_NAME}
                ${CMAKE_SOURCE_DIR}/app/source/main.cpp
        )

        target_include_directories(${PROJECT_NAME} PRIVATE
                ${CURL_INCLUDE_DIR}
                ${GTKMM_INCLUDE_DIRS}
                ${CAIROMM_INCLUDE_DIRS}
                ${POPPLER_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/third_party/turso/include

                ${CMAKE_SOURCE_DIR}/app/include
                ${CMAKE_SOURCE_DIR}/gui/include
                ${CMAKE_SOURCE_DIR}/gui/components/include
                ${CMAKE_SOURCE_DIR}/data/include
                ${CMAKE_SOURCE_DIR}/storage/include
                ${CMAKE_SOURCE_DIR}/features/include
                ${CMAKE_SOURCE_DIR}/models/include
                ${CMAKE_SOURCE_DIR}/utility/include
        )

        target_link_options(${PROJECT_NAME} PRIVATE
                LINKER:-lpthread
                LINKER:-lrt
        )

        target_link_libraries(${PROJECT_NAME}
                PRIVATE
                ${CURL_LIBRARIES}
                ${GTKMM_LIBRARIES}
                ${CAIROMM_LIBRARIES}
                ${POPPLER_LIBRARIES}
		${TURSO_LIBRARY}
		dl
		pthread
		m
        )

        target_link_directories(${PROJECT_NAME}
                PRIVATE
                ${GTKMM_LIBRARY_DIRS}
                ${CAIROMM_LIBRARY_DIRS}
        )

	set_target_properties(${PROJECT_NAME} PROPERTIES
		OUTPUT_NAME "${APP_NAME}"
	)

        if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/gui)
                add_subdirectory(${CMAKE_SOURCE_DIR}/gui)
                target_link_libraries(${PROJECT_NAME} PRIVATE gui)
        else()
                message(STATUS "Directory gui, does not exist")
        endif()

        if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/gui/components)
                add_subdirectory(${CMAKE_SOURCE_DIR}/gui/components)
                target_link_libraries(${PROJECT_NAME} PRIVATE components)
        else()
                message(STATUS "Directory components, does not exist")
        endif()

        if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/data)
                add_subdirectory(${CMAKE_SOURCE_DIR}/data)
                target_link_libraries(${PROJECT_NAME} PRIVATE data)
        else()
                message(STATUS "Directory data, does not exist")
        endif()

        if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/storage)
                add_subdirectory(${CMAKE_SOURCE_DIR}/storage)
                target_link_libraries(${PROJECT_NAME} PRIVATE storage)
        else()
                message(STATUS "Directory storage, does not exist")
        endif()

        if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/models)
                add_subdirectory(${CMAKE_SOURCE_DIR}/models)
                target_link_libraries(${PROJECT_NAME} PRIVATE models)
        else()
                message(STATUS "Directory controllers, does not exist")
        endif()

        if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/features)
                add_subdirectory(${CMAKE_SOURCE_DIR}/features)
                target_link_libraries(${PROJECT_NAME} PRIVATE features ${POPPLER_LIBRARIES})
        else()
                message(STATUS "Directory features, does not exist")
        endif()

        if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/utility)
                add_subdirectory(${CMAKE_SOURCE_DIR}/utility)
                target_link_libraries(${PROJECT_NAME} PRIVATE utility)
        else()
                message(STATUS "Directory utility, does not exist")
        endif()

	if (INSTALL_APPLICATION)
		# Payload layout inside the RPM. The installer copies these into
		# ~/.local. The live database is <app>.db; this template is a
		# different filename so upgrades never overwrite customer data.
		install(FILES
			${CMAKE_CURRENT_SOURCE_DIR}/gui/mint-bill.css
			${CMAKE_CURRENT_SOURCE_DIR}/gui/mint-bill.ui
			${CMAKE_CURRENT_SOURCE_DIR}/email.txt
			${CMAKE_CURRENT_SOURCE_DIR}/email.html
			DESTINATION ${CMAKE_INSTALL_DATADIR}/${APP_ID}
		)
		install(FILES
			${CMAKE_CURRENT_SOURCE_DIR}/mint-bill.db
			DESTINATION ${CMAKE_INSTALL_DATADIR}/${APP_ID}
			RENAME ${APP_NAME}.db.template
		)

		install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

		set(PACKAGING_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/packaging)
		configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/packaging/systemd/mint-bill.service.in
			${PACKAGING_GEN_DIR}/${APP_NAME}.service
			@ONLY
		)
		configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/packaging/systemd/mint-bill-db-backup.service.in
			${PACKAGING_GEN_DIR}/${APP_NAME}-db-backup.service
			@ONLY
		)
		configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/packaging/systemd/mint-bill-db-backup.timer.in
			${PACKAGING_GEN_DIR}/${APP_NAME}-db-backup.timer
			@ONLY
		)
		configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/packaging/desktop/mint-bill.desktop.in
			${PACKAGING_GEN_DIR}/${APP_ID}.desktop
			@ONLY
		)
		configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/packaging/scripts/mint-bill-db-backup.sh.in
			${PACKAGING_GEN_DIR}/${APP_NAME}-db-backup
			@ONLY
		)

		install(FILES
			${PACKAGING_GEN_DIR}/${APP_NAME}.service
			${PACKAGING_GEN_DIR}/${APP_NAME}-db-backup.service
			${PACKAGING_GEN_DIR}/${APP_NAME}-db-backup.timer
			DESTINATION lib/systemd/user
		)
		install(FILES
			${PACKAGING_GEN_DIR}/${APP_ID}.desktop
			DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
		)
		install(PROGRAMS
			${PACKAGING_GEN_DIR}/${APP_NAME}-db-backup
			DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
		)
	else ()
		message("Application is not installed.")
	endif()
endif()
