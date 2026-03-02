cmake_minimum_required (VERSION 3.10)
project (mkpass)

option(WITH_GUI "Build with Qt GUI" ON)

enable_testing()

add_subdirectory(libmkpass)
add_subdirectory(mkpass_cli)
add_subdirectory(mkpass_gui)
