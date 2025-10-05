cmake_minimum_required (VERSION 3.10)
project (mkpass)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

add_definitions(-std=c++20)
#add_definitions(-std=c++23)
enable_testing()

#add_subdirectory(src)
#add_executable(mkpass src/main.cpp src/digest.cpp src/composer.cpp)
add_executable(mkpass src/main.cpp src/digest.cpp src/compose.cpp)

# Link OpenSSL
target_link_libraries(mkpass OpenSSL::SSL OpenSSL::Crypto)

# Testing support
find_package(GTest REQUIRED)
add_executable(mkpass_tests src/digest.cpp src/digest_ut.cpp src/generator_ut.cpp src/uniform_ut.cpp src/compose.cpp src/compose_ut.cpp)
target_link_libraries(mkpass_tests GTest::GTest GTest::Main OpenSSL::SSL OpenSSL::Crypto)
enable_testing()
include(GoogleTest)
gtest_discover_tests(mkpass_tests)
