###############################################################################
# CMakeLists.txt
#
# This file defines the build configuration for the `target-models` component.
# The module builds an OBJECT library containing all model and serialization
# sources used throughout the system. These models act as the primary bridge
# between domain logic, serialization layers, and storage queries.
#
# Key Features:
#   • Automatically detects source files under models/source.
#   • Builds an OBJECT library (`models`) for efficient reuse by other targets.
#   • Exposes required include directories for dependencies such as:
#       - CURL
#       - GTKMM
#       - Poppler
#       - Cairomm
#       - Turso (sqlite3.h under third_party/turso/include)
#   • Makes model and serialization headers available to external modules via
#     PUBLIC include paths.
#
# The CMake configuration ensures that higher-level components (GUI, app, data,
# storage, features, utility) can link against `models` without duplicating
# include paths or build logic.
###############################################################################
cmake_minimum_required(VERSION 3.25)
project(target-models VERSION 0.0.1 LANGUAGES CXX C)


file(GLOB RESULT ${PROJECT_SOURCE_DIR}/source)
list(LENGTH RESULT FILES_IN_DIR)
if(${FILES_IN_DIR} GREATER 0)
        add_library(models
                OBJECT
                ${PROJECT_SOURCE_DIR}/source/admin_model.cpp
                ${PROJECT_SOURCE_DIR}/source/admin_serialize.cpp
                ${PROJECT_SOURCE_DIR}/source/client_model.cpp
                ${PROJECT_SOURCE_DIR}/source/client_serialize.cpp
                ${PROJECT_SOURCE_DIR}/source/invoice_model.cpp
                ${PROJECT_SOURCE_DIR}/source/invoice_serialize.cpp
                ${PROJECT_SOURCE_DIR}/source/statement_model.cpp
                ${PROJECT_SOURCE_DIR}/source/statement_serialize.cpp
                ${PROJECT_SOURCE_DIR}/source/business_serialize.cpp
        )

        target_include_directories(models
                PRIVATE
                ${PROJECT_SOURCE_DIR}/include/
                ${CURL_INCLUDE_DIR}
                ${GTKMM_INCLUDE_DIRS}
                ${POPPLER_INCLUDE_DIRS}
                ${CAIROMM_INCLUDE_DIRS}
                PUBLIC
                ${CMAKE_SOURCE_DIR}/app/include
                ${CMAKE_SOURCE_DIR}/data/include
                ${CMAKE_SOURCE_DIR}/storage/include
                ${CMAKE_SOURCE_DIR}/third_party/turso/include
                ${CMAKE_SOURCE_DIR}/utility/include
                ${CMAKE_SOURCE_DIR}/features/include
        )
else ()
        message(STATUS "There are no files in, models/source.")
endif()
