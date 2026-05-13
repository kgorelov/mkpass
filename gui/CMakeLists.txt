if(WITH_GUI)
  set(CMAKE_AUTOMOC ON)

  set(MKPASS_QT_VERSION 5 CACHE STRING "Qt version to use (5 or 6)")
  set_property(CACHE MKPASS_QT_VERSION PROPERTY STRINGS 5 6)

  set(GUI_SOURCES
    gui.cpp
    password_dialog.cpp
    progress_dialog.cpp
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
    add_executable(mkpass-gui WIN32 main_gui.cpp)
  else()
    add_executable(mkpass-gui main_gui.cpp)
  endif()
  target_compile_definitions(mkpass-gui PRIVATE WITH_GUI)
  target_include_directories(mkpass-gui PRIVATE ../libmkpass)
  target_link_libraries(mkpass-gui PRIVATE gui_part libmkpass sha512 argon2 qrcodegen)
  if (MSVC)
    target_sources(mkpass-gui PRIVATE mkpass.rc)
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
