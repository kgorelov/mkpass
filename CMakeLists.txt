cmake_minimum_required (VERSION 3.10)
project (mkpass)

option(WITH_GUI "Build with Qt GUI" ON)

set(CMAKE_AUTOMOC ON)

################################################################################
# Libraries
################################################################################

# C++ part library, used both in the main executable and tests
set(SOURCES
  src/digest.cpp
  src/compose.cpp
  src/tui.cpp
  src/mkpass.cpp
  src/hkdf_argon2.cpp
  src/db.cpp
)
add_library(cpp_part STATIC ${SOURCES})
target_include_directories(cpp_part PUBLIC src src/linenoise-ng)
if (MSVC)
    target_compile_options(cpp_part PRIVATE /std:c++20)
else()
    target_compile_options(cpp_part PRIVATE -std=c++20)
    SET(BUILD_MODE Debug)
    SET(CMAKE_CXX_FLAGS "-Wall")
    SET(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()


# Link external OpenSSL only when cmake -DOPENSSL_SHA512=ON
if(OPENSSL_SHA512)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(cpp_part
    OpenSSL::SSL
    OpenSSL::Crypto
  )
endif()


# Prefer static linking
set(BUILD_SHARED_LIBS OFF)   # Prefer static libs globally
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")  # Only static


# SQLite3 library
add_library(sqlite3 STATIC src/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC src/sqlite3)
target_link_libraries(cpp_part PRIVATE sqlite3)


# Linenoise-ng library
add_library(linenoise STATIC src/linenoise-ng/linenoise.cpp src/linenoise-ng/ConvertUTF.cpp src/linenoise-ng/wcwidth.cpp)
target_include_directories(linenoise PUBLIC src/linenoise-ng)


# A pure C SHA512 implementation library
add_library(sha512 STATIC src/sha512.c)


# Argon2 library
set(ARGON2_SOURCES
  src/argon2/argon2.c
  src/argon2/core.c
  src/argon2/ref.c
  src/argon2/blake2/blake2b.c
  src/argon2/thread.c
  src/argon2/encoding.c
)
add_library(argon2 STATIC ${ARGON2_SOURCES})


################################################################################
# Main mkpass executable
################################################################################

add_executable(mkpass src/main.cpp)
if(WITH_GUI)
  # set(OPENGL_opengl_LIBRARY "/usr/lib/x86_64-linux-gnu/libGL.so")
  # set(OPENGL_glx_LIBRARY "/usr/lib/x86_64-linux-gnu/libGLX.so")
  # set(QT_QPA_DEFAULT_PLATFORM eglfs)
  # set(OpenGL_GL_PREFERENCE GLVND)
  # find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
  find_package(Qt5 REQUIRED COMPONENTS Widgets Concurrent)
  set(GUI_SOURCES
    src/gui.cpp
    src/password_dialog.cpp
    src/qrcode/qrcodegen.cpp
  )
  qt5_add_resources(GUI_SOURCES src/icons.qrc)
  add_library(gui_part STATIC ${GUI_SOURCES})
  #target_link_libraries(gui_part PUBLIC Qt6::Widgets)
  # target_link_libraries(gui_part PRIVATE Qt5::Widgets Qt5::Concurrent)
  target_link_libraries(gui_part PUBLIC Qt5::Widgets Qt5::Concurrent)
  target_include_directories(gui_part PUBLIC src src/qrcode)
  target_compile_definitions(mkpass PRIVATE WITH_GUI)
endif()

if (MSVC)
    target_compile_options(mkpass PRIVATE /std:c++20)
else()
    target_compile_options(mkpass PRIVATE -std=c++20)
endif()

target_link_libraries(mkpass PRIVATE
  cpp_part
  sha512
  argon2
  linenoise
)
if(WITH_GUI)
  target_link_libraries(mkpass PRIVATE gui_part)
endif()

################################################################################
# Testing support
################################################################################

enable_testing()
set(TEST_SOURCES
  src/digest_ut.cpp
  src/generator_ut.cpp
  src/uniform_ut.cpp
  src/compose_ut.cpp
  src/hkdf_argon2_ut.cpp
  src/db_ut.cpp
)
find_package(GTest REQUIRED)
add_executable(mkpass_tests ${TEST_SOURCES})
if (MSVC)
    target_compile_options(mkpass_tests PRIVATE /std:c++20)
else()
    target_compile_options(mkpass_tests PRIVATE -std=c++20)
endif()

target_link_libraries(mkpass_tests
  cpp_part
  sha512
  argon2
  linenoise
  GTest::GTest
  GTest::Main
)
include(GoogleTest)
gtest_discover_tests(mkpass_tests)

add_executable(e2e_tests src/e2e_test.cpp)
if (MSVC)
    target_compile_options(e2e_tests PRIVATE /std:c++20)
else()
    target_compile_options(e2e_tests PRIVATE -std=c++20)
endif()
target_link_libraries(e2e_tests
  cpp_part
  GTest::GTest
  GTest::Main
)
gtest_discover_tests(e2e_tests)
