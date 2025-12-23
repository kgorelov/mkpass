cmake_minimum_required (VERSION 3.10)
project (mkpass)

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
target_include_directories(cpp_part PUBLIC src)
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

find_package(SQLite3 REQUIRED)
target_link_libraries(cpp_part PUBLIC SQLite::SQLite3)


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
if (MSVC)
    target_compile_options(mkpass PRIVATE /std:c++20)
else()
    target_compile_options(mkpass PRIVATE -std=c++20)
endif()

target_link_libraries(mkpass PRIVATE
  cpp_part
  sha512
  argon2
)

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
  GTest::GTest
  GTest::Main
)
include(GoogleTest)
gtest_discover_tests(mkpass_tests)
