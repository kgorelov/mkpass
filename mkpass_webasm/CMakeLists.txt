cmake_minimum_required(VERSION 3.13)
project(mkpass_webasm)

add_subdirectory(../libmkpass libmkpass_build)

add_executable(mkpass_webasm mkpass_webasm.cpp)

target_compile_definitions(mkpass_webasm PRIVATE ARGON2_NO_THREADS=1)
target_link_options(mkpass_webasm PRIVATE "--bind")
target_link_options(mkpass_webasm PRIVATE "-sMODULARIZE=1")
target_link_options(mkpass_webasm PRIVATE "-sEXPORT_NAME='mkpass_wasm'")
target_link_options(mkpass_webasm PRIVATE "-sALLOW_MEMORY_GROWTH=1")
target_link_options(mkpass_webasm PRIVATE "-sINITIAL_MEMORY=134217728") # 128 MiB
target_link_options(mkpass_webasm PRIVATE "-sNO_DISABLE_EXCEPTION_CATCHING")

target_link_libraries(mkpass_webasm PRIVATE libmkpass sha512 argon2 qrcodegen sqlite3)
