###############################################################################
# @file    CMakeLists.txt
#
# @brief   Build configuration for the GUI Target Components library.
#
# @details
# This CMake file defines the build process for the 'components' object library,
# which contains reusable GUI building blocks used throughout the Mint-Bill
# application. These components wrap GTKMM widgets and implement higher-level
# behaviors such as stack management, printing, and general GUI interactions.
#
# Functionality:
#   - Ensures a minimum required version of CMake (3.25).
#   - Declares the 'target-components' project using C and C++.
#   - Detects whether the expected source directory contains implementation files.
#   - Builds an OBJECT library named 'components' when files are present.
#   - Adds include paths required by GTKMM and other project modules
#     (app, data, storage, features, utility, GUI invoice pages).
#
# Behavior:
#   - If GUI source files exist:
#         • stack.cpp
#         • printer.cpp
#         • gui_parts.cpp
#     then an object library is produced.
#
#   - If the directory is empty, a CMake STATUS message is printed instead
#     and no target is created.
#
# This file does not create a standalone library but an OBJECT library intended
# to be linked into other GUI-related targets within the project.
###############################################################################
cmake_minimum_required(VERSION 3.25)
project(target-components VERSION 0.0.1 LANGUAGES CXX C)


file(GLOB RESULT ${PROJECT_SOURCE_DIR}/source)
list(LENGTH RESULT FILES_IN_DIR)
if(${FILES_IN_DIR} GREATER 0)

        add_library(components
                OBJECT
                ${PROJECT_SOURCE_DIR}/source/stack.cpp
                ${PROJECT_SOURCE_DIR}/source/printer.cpp
                ${PROJECT_SOURCE_DIR}/source/gui_parts.cpp
        )

        target_include_directories(components
                PRIVATE
                ${GTKMM_INCLUDE_DIRS}
                ${PROJECT_SOURCE_DIR}/include
                ${CMAKE_SOURCE_DIR}/gui/include
                PUBLIC
                ${CMAKE_SOURCE_DIR}/app/include
                ${CMAKE_SOURCE_DIR}/data/include
                ${CMAKE_SOURCE_DIR}/storage/include
                ${CMAKE_SOURCE_DIR}/third_party/turso/include
                ${CMAKE_SOURCE_DIR}/features/include
                ${CMAKE_SOURCE_DIR}/utility/include
                ${PROJECT_SOURCE_DIR}/invoice_page/include
        )
else ()
        message(STATUS "There are no files in, gui/source.")
endif()
