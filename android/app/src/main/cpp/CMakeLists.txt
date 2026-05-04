cmake_minimum_required(VERSION 3.22.1)

project("mkpass")

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../../../../../)

# C++ part library
set(CPP_SOURCES
  ${PROJECT_ROOT}/libmkpass/digest.cpp
  ${PROJECT_ROOT}/libmkpass/compose.cpp
  ${PROJECT_ROOT}/libmkpass/mkpass.cpp
  ${PROJECT_ROOT}/libmkpass/hkdf_argon2.cpp
  ${PROJECT_ROOT}/libmkpass/db.cpp
  ${PROJECT_ROOT}/libmkpass/character_classes.cpp
  ${PROJECT_ROOT}/libmkpass/passphrase_patterns.cpp
  ${PROJECT_ROOT}/libmkpass/wordlists/eff_large_words.cpp
  ${PROJECT_ROOT}/libmkpass/wordlists/wordnet_nouns.cpp
  ${PROJECT_ROOT}/libmkpass/wordlists/wordnet_verbs.cpp
  ${PROJECT_ROOT}/libmkpass/wordlists/wordnet_adjs.cpp
  ${PROJECT_ROOT}/libmkpass/wordlists/wordnet_advs.cpp
)
add_library(cpp_part STATIC ${CPP_SOURCES})
target_include_directories(cpp_part PUBLIC ${PROJECT_ROOT}/libmkpass)
target_compile_options(cpp_part PRIVATE -std=c++20)

# SQLite3 library
add_library(sqlite3 STATIC ${PROJECT_ROOT}/libmkpass/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${PROJECT_ROOT}/libmkpass/sqlite3)

target_link_libraries(cpp_part PRIVATE sqlite3)

# QR-code generator
set(qrcodegen_SOURCES
  ${PROJECT_ROOT}/libmkpass/qrcode/qrcodegen.cpp
)
add_library(qrcodegen STATIC ${qrcodegen_SOURCES})
target_include_directories(qrcodegen PUBLIC ${PROJECT_ROOT}/libmkpass/qrcode)



# A pure C SHA512 implementation library
add_library(sha512 STATIC ${PROJECT_ROOT}/libmkpass/sha512.c)
target_include_directories(sha512 PUBLIC ${PROJECT_ROOT}/libmkpass)


# Argon2 library
set(ARGON2_SOURCES
  ${PROJECT_ROOT}/libmkpass/argon2/argon2.c
  ${PROJECT_ROOT}/libmkpass/argon2/core.c
  ${PROJECT_ROOT}/libmkpass/argon2/ref.c
  ${PROJECT_ROOT}/libmkpass/argon2/blake2/blake2b.c
  ${PROJECT_ROOT}/libmkpass/argon2/thread.c
  ${PROJECT_ROOT}/libmkpass/argon2/encoding.c
)
add_library(argon2 STATIC ${ARGON2_SOURCES})
target_include_directories(argon2 PUBLIC
    ${PROJECT_ROOT}/libmkpass/argon2
    ${PROJECT_ROOT}/libmkpass/argon2/blake2
)


# Add JNI wrapper
add_library(mkpass SHARED native-lib.cpp)
target_compile_options(mkpass PRIVATE -std=c++20)

target_include_directories(mkpass PRIVATE
    ${PROJECT_ROOT}/libmkpass
)

# Link to NDK libraries
find_library(log-lib log)

target_link_libraries(mkpass PRIVATE
  cpp_part
  sha512
  argon2
  qrcodegen
  ${log-lib}
)
