add_library(linenoise STATIC linenoise-ng/linenoise.cpp linenoise-ng/ConvertUTF.cpp linenoise-ng/wcwidth.cpp)
target_include_directories(linenoise PUBLIC linenoise-ng)

option(STATIC_CLI "Link CLI mostly statically (-static-libstdc++ -static-libgcc)" OFF)
option(FULLY_STATIC_CLI "Link CLI fully statically (-static)" OFF)

add_executable(mkpass main.cpp cli.cpp)

if (FULLY_STATIC_CLI)
    set_target_properties(mkpass PROPERTIES LINK_FLAGS "-static")
elseif (STATIC_CLI)
    if (NOT MSVC)
        set_target_properties(mkpass PROPERTIES LINK_FLAGS "-static-libstdc++ -static-libgcc")
    endif()
endif()
if (MSVC)
    target_compile_options(mkpass PRIVATE /std:c++20)
else()
    target_compile_options(mkpass PRIVATE -std=c++20 -Wsign-compare)
endif()

target_include_directories(mkpass PRIVATE ../libmkpass)

target_link_libraries(mkpass PRIVATE
  libmkpass
  sha512
  argon2
  linenoise
  qrcodegen
)

if (NOT WIN32)
  find_package(GTest REQUIRED)
  add_executable(e2e_tests e2e_test.cpp)

  if(TARGET GTest::gtest)
    get_target_property(GTEST_INCLUDE_DIRS GTest::gtest INTERFACE_INCLUDE_DIRECTORIES)
    if(GTEST_INCLUDE_DIRS)
      target_include_directories(e2e_tests PRIVATE ${GTEST_INCLUDE_DIRS})
    endif()
  endif()

  target_compile_options(e2e_tests PRIVATE -std=c++20 -Wsign-compare)
  target_include_directories(e2e_tests PRIVATE ../libmkpass)
  target_link_libraries(e2e_tests PRIVATE
    libmkpass
    qrcodegen
    GTest::gtest
    GTest::gtest_main
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
