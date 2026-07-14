# DuiLib CMake wrapper
# Downloads DuiLib via FetchContent and builds it as a static library.
#
# No manual git submodule steps required — CMake fetches the source
# automatically during configure. The downloaded source is cached in
# the CMake build tree (or CMAKE_FETCHCONTENT_BASE_DIR if set).

cmake_minimum_required(VERSION 3.16)
project(DuiLib LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    duilib_src
    GIT_REPOSITORY https://github.com/duilib/duilib.git
    GIT_TAG        85658d47036445626f5a24d5287f348fff871604
    GIT_SHALLOW    FALSE   # shallow clone can't checkout a specific commit hash
    # Prevent FetchContent from calling add_subdirectory on duilib's own CMakeLists.txt
    # (it builds a shared DLL; we build our own static lib wrapper below)
    SOURCE_SUBDIR  "_no_cmake_"
)

FetchContent_MakeAvailable(duilib_src)

# duilib repo layout: <repo_root>/DuiLib/  contains the actual source headers/cpp
set(DUILIB_SRC_ROOT "${duilib_src_SOURCE_DIR}/DuiLib")

# Collect all source files
file(GLOB_RECURSE DUILIB_SOURCES
    "${DUILIB_SRC_ROOT}/*.cpp"
    "${DUILIB_SRC_ROOT}/*.c"
)
file(GLOB_RECURSE DUILIB_HEADERS
    "${DUILIB_SRC_ROOT}/*.h"
    "${DUILIB_SRC_ROOT}/*.hpp"
)

add_library(duilib STATIC ${DUILIB_SOURCES} ${DUILIB_HEADERS})

# stb_image.c is a C file — explicitly mark it so MSVC compiles it as C, not C++
set_source_files_properties(
    "${DUILIB_SRC_ROOT}/Utils/stb_image.c"
    PROPERTIES LANGUAGE C
)

# Public include directory so consumers can do #include <UIlib.h>
target_include_directories(duilib PUBLIC
    "${DUILIB_SRC_ROOT}"
)

# Preprocessor defines
target_compile_definitions(duilib PUBLIC
    UILIB_STATIC        # Build/link as static library
    UNICODE              # Unicode build
    _UNICODE
    _HAS_STD_BYTE=0     # Avoid std::byte vs Windows byte ambiguity in C++17
)

# MSVC-specific settings
if(MSVC)
    # Match vcpkg x64-windows-static-md triplet: use /MD (dynamic CRT)
    set_property(TARGET duilib PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )

    # Suppress common DuiLib warnings
    target_compile_options(duilib PRIVATE
        /W3
        /wd4996   # deprecated functions
        /wd4819   # code page warnings
        /wd4267   # size_t to int conversion
        /wd4244   # possible loss of data
    )
endif()

# Windows system libraries required by DuiLib
target_link_libraries(duilib PUBLIC
    comctl32
    gdi32
    user32
    winmm
    ole32
    oleaut32
    uuid
    imm32
)
