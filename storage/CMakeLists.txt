############################################################################
# Contents: Mass Storage Interface CMakeLists.txt file
# Author: Dawid Blom
# Date: November 13, 2024
#
# NOTE:
############################################################################
cmake_minimum_required(VERSION 3.25)
project(target-storage VERSION 0.0.1 LANGUAGES CXX C)

file(GLOB RESULT ${PROJECT_SOURCE_DIR}/source)
list(LENGTH RESULT FILES_IN_DIR)
if(${FILES_IN_DIR} GREATER 0)
        add_library(storage
                OBJECT
                ${PROJECT_SOURCE_DIR}/source/sqlite.cpp
        )

        target_include_directories(storage
                PUBLIC
                ${CMAKE_SOURCE_DIR}/app/include
                ${CMAKE_SOURCE_DIR}/third_party/turso/include
                PRIVATE
                ${PROJECT_SOURCE_DIR}/include
        )
else ()
        message(STATUS "There are no files in, gui/source.")
endif()
