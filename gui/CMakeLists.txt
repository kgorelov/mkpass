if(WITH_GUI)
  set(CMAKE_AUTOMOC ON)

  set(MKPASS_QT_VERSION 5 CACHE STRING "Qt version to use (5 or 6)")
  set_property(CACHE MKPASS_QT_VERSION PROPERTY STRINGS 5 6)

  set(GUI_SOURCES
    gui.cpp
    password_dialog.cpp
    progress_dialog.cpp
    db_management_dialog.cpp
    service_details_dialog.cpp
    manual_dialog.cpp
  )

  if (MKPASS_QT_VERSION EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
    qt_add_resources(GUI_SOURCES icons.qrc)
    set(QT_LIBRARIES Qt6::Widgets Qt6::Concurrent)
  elseif (MKPASS_QT_VERSION EQUAL 5)
    find_package(Qt5 REQUIRED COMPONENTS Widgets Concurrent)
    qt5_add_resources(GUI_SOURCES icons.qrc)
    set(QT_LIBRARIES Qt5::Widgets Qt5::Concurrent)
  else()
    message(FATAL_ERROR "Unsupported Qt version: ${MKPASS_QT_VERSION}. Please use 5 or 6.")
  endif()

  add_library(gui_part STATIC ${GUI_SOURCES})
  target_link_libraries(gui_part PUBLIC ${QT_LIBRARIES})
  target_include_directories(gui_part PUBLIC . ../libmkpass)
  if (MSVC)
    target_compile_options(gui_part PRIVATE /std:c++20)
  else()
    target_compile_options(gui_part PRIVATE -std=c++20)
  endif()

  if (MSVC)
    add_executable(mkpass-gui WIN32 main_gui.cpp)
  else()
    add_executable(mkpass-gui main_gui.cpp)
  endif()
  target_compile_definitions(mkpass-gui PRIVATE WITH_GUI)
  target_include_directories(mkpass-gui PRIVATE ../libmkpass)
  target_link_libraries(mkpass-gui PRIVATE gui_part libmkpass sha512 argon2 qrcodegen)
  if (MSVC)
    target_compile_options(mkpass-gui PRIVATE /std:c++20)
    target_sources(mkpass-gui PRIVATE mkpass.rc)
  else()
    target_compile_options(mkpass-gui PRIVATE -std=c++20)
  endif()

  # Deploy Qt dependencies on Windows
  if(WIN32)
    if (MKPASS_QT_VERSION EQUAL 6)
      find_program(WINDEPLOYQT_EXECUTABLE windeployqt6 HINTS "${Qt6_DIR}/../../../bin")
      set(QT_PLUGINS_DIR "${Qt6_DIR}/../../../plugins")
    else()
      find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt5_DIR}/../../../bin")
      set(QT_PLUGINS_DIR "${Qt5_DIR}/../../../plugins")
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
      add_custom_command(TARGET mkpass-gui POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<TARGET_FILE:mkpass-gui>"
        COMMENT "Deploying Qt dependencies for mkpass-gui"
      )
    else()
      # Manual deployment when windeployqt is not available (conda-forge Qt)
      message(STATUS "windeployqt not found - using manual Qt deployment")

      # Copy platform plugins (required for Qt apps to run)
      add_custom_command(TARGET mkpass-gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:mkpass-gui>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${QT_PLUGINS_DIR}/platforms/qwindows.dll"
          "$<TARGET_FILE_DIR:mkpass-gui>/platforms/"
        COMMENT "Copying Qt platform plugins"
      )

      # Copy image format plugins (optional but recommended)
      add_custom_command(TARGET mkpass-gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:mkpass-gui>/imageformats"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${QT_PLUGINS_DIR}/imageformats/qico.dll"
          "${QT_PLUGINS_DIR}/imageformats/qjpeg.dll"
          "${QT_PLUGINS_DIR}/imageformats/qgif.dll"
          "$<TARGET_FILE_DIR:mkpass-gui>/imageformats/"
        COMMENT "Copying Qt image format plugins"
      )
    endif()

    # Copy runtime dependencies (required for both windeployqt and manual deployment)
    # These are system libraries that Qt depends on but windeployqt doesn't copy
    if(DEFINED ENV{CONDA_PREFIX})
      set(CONDA_BIN_DIR "$ENV{CONDA_PREFIX}/Library/bin")
      add_custom_command(TARGET mkpass-gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CONDA_BIN_DIR}/libpng16.dll"
          "${CONDA_BIN_DIR}/zlib.dll"
          "${CONDA_BIN_DIR}/pcre2-16.dll"
          "${CONDA_BIN_DIR}/zstd.dll"
          "${CONDA_BIN_DIR}/freetype.dll"
          "${CONDA_BIN_DIR}/harfbuzz.dll"
          "${CONDA_BIN_DIR}/libbz2.dll"
          "$<TARGET_FILE_DIR:mkpass-gui>/"
        COMMENT "Copying Qt runtime dependencies"
      )
    endif()
  endif()


if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    install(
        TARGETS mkpass-gui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(
        FILES ../icons/mkpass_256.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME mkpass.png
    )
    install(
        FILES mkpass.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
endif()


endif()
