cmake_minimum_required(VERSION 3.10)
project(WordNet C)

# Set the default search path for WordNet data files if not set
add_definitions(-DDEFAULTPATH="/usr/local/WordNet-3.0/dict")
add_definitions(-DHAVE_CONFIG_H)

# Include directories
include_directories(include)
include_directories(.)

# Library source files
set(LIB_SOURCES
    lib/binsrch.c
    lib/morph.c
    lib/search.c
    lib/wnglobal.c
    lib/wnhelp.c
    lib/wnrtl.c
    lib/wnutil.c
)

# Build the WordNet library
add_library(WN STATIC ${LIB_SOURCES})

# Build the word list utility
add_executable(wn_list_words wn_list_words.c)
target_link_libraries(wn_list_words WN)

# Installation (optional, but good to have)
install(TARGETS wn_list_words RUNTIME DESTINATION bin)
install(TARGETS WN ARCHIVE DESTINATION lib)
install(FILES include/wn.h DESTINATION include)
