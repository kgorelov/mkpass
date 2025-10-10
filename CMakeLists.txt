cmake_minimum_required (VERSION 3.10)
project (mkpass)

# C++ part library, used both in the main executable and tests
set(SOURCES
  src/digest.cpp
  src/compose.cpp
)
add_library(cpp_part STATIC ${SOURCES})
if (MSVC)
    target_compile_options(cpp_part PRIVATE /std:c++20)
else()
    target_compile_options(cpp_part PRIVATE -std=c++20)
endif()

# Link external OpenSSL only when cmake -DOPENSSL_SHA512=ON
if(OPENSSL_SHA512)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(cpp_part
    OpenSSL::SSL
    OpenSSL::Crypto
  )
endif()

# A pure C SHA512 implementation library
add_library(c_part STATIC src/sha512.c)

# Main mkpass executable
add_executable(mkpass src/main.cpp)
if (MSVC)
    target_compile_options(mkpass PRIVATE /std:c++20)
else()
    target_compile_options(mkpass PRIVATE -std=c++20)
endif()

target_link_libraries(mkpass PRIVATE
  cpp_part
  c_part
)

# Testing support
enable_testing()
set(TEST_SOURCES
  src/digest_ut.cpp
  src/generator_ut.cpp
  src/uniform_ut.cpp
  src/compose_ut.cpp
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
  c_part
  GTest::GTest
  GTest::Main
)
include(GoogleTest)
gtest_discover_tests(mkpass_tests)
