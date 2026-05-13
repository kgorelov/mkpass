cmake_minimum_required (VERSION 3.10)
project (mkpass)

option(WITH_GUI "Build with Qt GUI" ON)
option(WITH_WEBASSEMBLY "Build with WebAssembly" OFF)

enable_testing()

add_subdirectory(libmkpass)
add_subdirectory(cli)
if(WITH_GUI)
    add_subdirectory(gui)
endif()
if(WITH_WEBASSEMBLY)
    add_subdirectory(mkpass_webasm)
endif()
