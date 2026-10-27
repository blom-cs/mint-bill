# ============================================================================================
#  Utility Module CMake Configuration
# --------------------------------------------------------------------------------------------
#  This CMakeLists file declares the `utility` object library, which provides foundational
#  helper classes such as:
#      - file I/O utilities
#      - date computation utilities
#      - string slicing utilities (boundary_slicer, word_slicer)
#
#  The library is compiled as an OBJECT library so multiple upper-level targets may link
#  against the compiled object files without creating an independent shared/static library.
#
#  Directory Structure Expected:
#      utility/
#      ├── include/      # Public headers
#      └── source/       # .cpp implementation files
#
#  Notes:
#   • The presence of files in `source/` is checked dynamically.
#   • If no files exist, an informational CMake message is printed.
#   • Public include paths allow the application layer to consume this module.
#   • Private include paths hide implementation details from linking targets.
# ============================================================================================
cmake_minimum_required(VERSION 3.25)
project(target-utility VERSION 0.0.1 LANGUAGES CXX C)


file(GLOB RESULT ${PROJECT_SOURCE_DIR}/source)
list(LENGTH RESULT FILES_IN_DIR)
if(${FILES_IN_DIR} GREATER 0)
        add_library(utility
                OBJECT
                ${PROJECT_SOURCE_DIR}/source/file.cpp
                ${PROJECT_SOURCE_DIR}/source/word_slicer.cpp
                ${PROJECT_SOURCE_DIR}/source/date_manager.cpp
                ${PROJECT_SOURCE_DIR}/source/boundary_slicer.cpp
                ${PROJECT_SOURCE_DIR}/source/paths.cpp
        )

        target_include_directories(utility
                PRIVATE
                ${PROJECT_SOURCE_DIR}/include/
                PUBLIC
                ${CMAKE_SOURCE_DIR}/app/include
        )
else ()
        message(STATUS "There are no files in, utility/source.")
endif()
