############################################################
#
# CMake Configuration – Features Library
#
# Summary
# --------
# Builds the `features` object library, which provides
# application-level services such as email delivery and
# PDF generation for invoices and statements.
#
# Details
# -------
# - Project:
#   - Name: `target-features`
#   - Version: 0.0.1
#   - Languages: C and C++
#
# - Source Layout:
#   - Expects implementation files in:
#       `${PROJECT_SOURCE_DIR}/source`
#   - Specifically compiles:
#       - `email.cpp`          : SMTP email feature
#       - `invoice_pdf.cpp`    : Invoice PDF generator
#       - `statement_pdf.cpp`  : Statement PDF generator
#
# - Library:
#   - Target name: `features`
#   - Type: `OBJECT` library, for reuse inside other
#     top-level targets (e.g. GUI or service binaries).
#
# - Include Paths:
#   - Private includes:
#       - Local feature headers under `include/`
#       - External dependencies:
#           - CURL
#           - GTKMM
#           - Poppler (C++ bindings)
#           - Cairomm
#           - Libsecret
#   - Public includes (exposed to dependents):
#       - `${CMAKE_SOURCE_DIR}/app/include`
#       - `${CMAKE_SOURCE_DIR}/data/include`
#       - `${CMAKE_SOURCE_DIR}/storage/include`
#       - `${CMAKE_SOURCE_DIR}/utility/include`
#
# - Conditional Build:
#   - If `features/source` is empty, the `features`
#     target is not created and a status message is
#     printed instead. This prevents configuring an
#     empty object library.
#
############################################################
cmake_minimum_required(VERSION 3.25)
project(target-features VERSION 0.0.1 LANGUAGES CXX C)


file(GLOB RESULT ${PROJECT_SOURCE_DIR}/source)
list(LENGTH RESULT FILES_IN_DIR)
if(${FILES_IN_DIR} GREATER 0)
        add_library(features
                OBJECT
                ${PROJECT_SOURCE_DIR}/source/email.cpp
                ${PROJECT_SOURCE_DIR}/source/invoice_pdf.cpp
                ${PROJECT_SOURCE_DIR}/source/statement_pdf.cpp
        )

        target_include_directories(features
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
        )
else ()
        message(STATUS "There are no files in, features/source.")
endif()
