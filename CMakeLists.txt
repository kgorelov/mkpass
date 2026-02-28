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
  set(MKPASS_QT_VERSION 5 CACHE STRING "Qt version to use (5 or 6)")
  set_property(CACHE MKPASS_QT_VERSION PROPERTY STRINGS 5 6)

  set(GUI_SOURCES
    src/gui.cpp
    src/password_dialog.cpp
    src/progress_dialog.cpp
    src/qrcode/qrcodegen.cpp
  )

  if (MKPASS_QT_VERSION EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
    qt_add_resources(GUI_SOURCES src/icons.qrc)
    set(QT_LIBRARIES Qt6::Widgets Qt6::Concurrent)
  elseif (MKPASS_QT_VERSION EQUAL 5)
    find_package(Qt5 REQUIRED COMPONENTS Widgets Concurrent)
    qt5_add_resources(GUI_SOURCES src/icons.qrc)
    set(QT_LIBRARIES Qt5::Widgets Qt5::Concurrent)
  else()
    message(FATAL_ERROR "Unsupported Qt version: ${MKPASS_QT_VERSION}. Please use 5 or 6.")
  endif()

  add_library(gui_part STATIC ${GUI_SOURCES})
  target_link_libraries(gui_part PUBLIC ${QT_LIBRARIES})
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

if (MSVC)
    target_sources(mkpass PRIVATE mkpass.rc)
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
)
if (NOT MSVC)
  list(APPEND TEST_SOURCES src/db_ut.cpp)
endif()
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

if (NOT MSVC)
  add_executable(e2e_tests src/e2e_test.cpp)
  target_compile_options(e2e_tests PRIVATE -std=c++20)
  target_link_libraries(e2e_tests
    cpp_part
    GTest::GTest
    GTest::Main
  )
  gtest_discover_tests(e2e_tests)
endif()

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    install(
        TARGETS mkpass
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(
        FILES icons/mkpass_256.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME mkpass.png
    )
    install(
        FILES mkpass.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
endif()
