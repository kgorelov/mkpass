cmake_minimum_required (VERSION 3.10)
project (mkpass VERSION 0.1.0 LANGUAGES C CXX)
add_definitions(-DMKPASS_VERSION="${PROJECT_VERSION}")

option(WITH_GUI "Build with Qt GUI" ON)
option(WITH_WEBASSEMBLY "Build with WebAssembly" OFF)
option(WITH_TESTS "Build tests" ON)

if(WITH_TESTS)
    enable_testing()
endif()

message(STATUS "CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES: ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}")

add_subdirectory(libmkpass)
add_subdirectory(cli)
if(WITH_GUI)
    add_subdirectory(gui)
endif()
if(WITH_WEBASSEMBLY)
    add_subdirectory(mkpass_webasm)
endif()

# CPack packaging configuration
set(CPACK_PACKAGE_NAME "mkpass")
set(CPACK_PACKAGE_VENDOR "Kirill Gorelov")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Deterministic password and passphrase generator using Argon2")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/kgorelov/mkpass")
set(CPACK_PACKAGE_CONTACT "Kirill Gorelov <kgorelov@gmail.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/debian/copyright")

# DEB generator configuration
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kirill Gorelov <kgorelov@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# RPM generator configuration
set(CPACK_RPM_PACKAGE_GROUP "Applications/Utilities")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
set(CPACK_RPM_PACKAGE_URL "https://github.com/kgorelov/mkpass")

include(CPack)
