# C++ part library, used both in the main executable and tests
set(SOURCES
  digest.cpp
  compose.cpp
  mkpass.cpp
  hkdf_argon2.cpp
  db.cpp
  )
add_library(libmkpass STATIC ${SOURCES})
target_include_directories(libmkpass PUBLIC .)
if (MSVC)
    target_compile_options(libmkpass PRIVATE /std:c++20)
else()
    target_compile_options(libmkpass PRIVATE -std=c++20)
    SET(BUILD_MODE Debug)
    SET(CMAKE_CXX_FLAGS "-Wall")
    SET(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()


# Link external OpenSSL only when cmake -DOPENSSL_SHA512=ON
if(OPENSSL_SHA512)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(libmkpass
    OpenSSL::SSL
    OpenSSL::Crypto
  )
endif()


# Prefer static linking
set(BUILD_SHARED_LIBS OFF)   # Prefer static libs globally
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")  # Only static


# SQLite3 library
add_library(sqlite3 STATIC sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC sqlite3)
target_link_libraries(libmkpass PRIVATE sqlite3)


# A pure C SHA512 implementation library
add_library(sha512 STATIC sha512.c)


# Argon2 library
set(ARGON2_SOURCES
  argon2/argon2.c
  argon2/core.c
  argon2/ref.c
  argon2/blake2/blake2b.c
  argon2/thread.c
  argon2/encoding.c
)
add_library(argon2 STATIC ${ARGON2_SOURCES})
if (EMSCRIPTEN)
    target_compile_definitions(argon2 PRIVATE ARGON2_NO_THREADS=1)
endif()
target_include_directories(argon2 PUBLIC
    argon2
    argon2/blake2
)

# QR-code generator
set(qrcodegen_SOURCES
  qrcode/qrcodegen.cpp
)
add_library(qrcodegen STATIC ${qrcodegen_SOURCES})
target_include_directories(qrcodegen PUBLIC qrcode)


################################################################################
# Testing support
################################################################################

if(NOT EMSCRIPTEN)
  enable_testing()
  set(TEST_SOURCES
    digest_ut.cpp
    generator_ut.cpp
    uniform_ut.cpp
    compose_ut.cpp
    hkdf_argon2_ut.cpp
  )
  if (NOT MSVC)
    list(APPEND TEST_SOURCES db_ut.cpp)
  endif()
  find_package(GTest REQUIRED)
  add_executable(mkpass_tests ${TEST_SOURCES})
  if (MSVC)
      target_compile_options(mkpass_tests PRIVATE /std:c++20)
  else()
      target_compile_options(mkpass_tests PRIVATE -std=c++20)
  endif()

  target_include_directories(mkpass_tests PRIVATE .)

  target_link_libraries(mkpass_tests PRIVATE
    libmkpass
    sha512
    argon2
    GTest::GTest
    GTest::Main
  )
  include(GoogleTest)
  gtest_discover_tests(mkpass_tests)
endif()
