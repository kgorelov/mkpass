# Linenoise-ng library
add_library(linenoise STATIC linenoise-ng/linenoise.cpp linenoise-ng/ConvertUTF.cpp linenoise-ng/wcwidth.cpp)
target_include_directories(linenoise PUBLIC linenoise-ng)

add_executable(mkpass main.cpp tui.cpp)
if (MSVC)
    target_compile_options(mkpass PRIVATE /std:c++20)
else()
    target_compile_options(mkpass PRIVATE -std=c++20)
endif()

target_include_directories(mkpass PRIVATE ../libmkpass)

target_link_libraries(mkpass PRIVATE
  libmkpass
  sha512
  argon2
  linenoise
)

if (MSVC)
    target_sources(mkpass PRIVATE ../mkpass.rc)
endif()

if (NOT MSVC)
  find_package(GTest REQUIRED)
  add_executable(e2e_tests e2e_test.cpp)
  target_compile_options(e2e_tests PRIVATE -std=c++20)
  target_include_directories(e2e_tests PRIVATE ../libmkpass)
  target_link_libraries(e2e_tests PRIVATE
    libmkpass
    GTest::GTest
    GTest::Main
  )
  gtest_discover_tests(e2e_tests)
  target_compile_definitions(e2e_tests PRIVATE "MKPASS_EXECUTABLE_PATH=\"$<TARGET_FILE:mkpass>\"")



if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    install(
        TARGETS mkpass
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

endif()
