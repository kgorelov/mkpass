cmake_minimum_required(VERSION 3.22.1)

project("mkpass")

# Add JNI wrapper
add_library(mkpass SHARED native-lib.cpp)

# Include directories for headers
target_include_directories(mkpass PRIVATE
    ../../../../src
    ../../../../src/argon2
    ../../../../src/argon2/blake2
    ../../../../src/linenoise-ng
)

# Add C++ sources from the main project, excluding tests and GUI/TUI related files
file(GLOB_RECURSE MKPASS_SOURCES
    ../../../../src/*.cpp
    ../../../../src/*.c
)

list(FILTER MKPASS_SOURCES EXCLUDE REGEX ".*_ut\.cpp$")
list(FILTER MKPASS_SOURCES EXCLUDE REGEX ".*e2e_test\.cpp$")
list(FILTER MKPASS_SOURCES EXCLUDE REGEX ".*tui\.cpp$")
list(FILTER MKPASS_SOURCES EXCLUDE REGEX ".*gui\.cpp$")
list(FILTER MKPASS_SOURCES EXCLUDE REGEX ".*main\.cpp$")


target_sources(mkpass PRIVATE ${MKPASS_SOURCES})

# Link to NDK libraries
find_library(log-lib log)
target_link_libraries(mkpass ${log-lib})

