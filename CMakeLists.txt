cmake_minimum_required(VERSION 3.21)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")   # must be before project()
project(qute VERSION 0.1.0 LANGUAGES CXX)
if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network)
if(UNIX AND NOT APPLE)
    find_package(Qt6 QUIET COMPONENTS DBus)   # optional: only needed by the KWin platform shell
endif()
qt_standard_project_setup(REQUIRES 6.5)

if(WIN32)
    # Adds the app icon + VERSIONINFO block to a Windows executable and builds it as a GUI app
    # (no console window when launched from Explorer). qute re-attaches to the parent console in
    # main.cpp so the CLI and the setup wizard still print to cmd/PowerShell.
    function(qute_add_windows_resource target icon description internal_name original_name)
        set(QUTE_RC_ICON "${CMAKE_SOURCE_DIR}/packaging/windows/${icon}")
        set(QUTE_RC_DESCRIPTION "${description}")
        set(QUTE_RC_INTERNAL_NAME "${internal_name}")
        set(QUTE_RC_ORIGINAL_NAME "${original_name}")
        set(QUTE_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${target}.rc")
        configure_file("${CMAKE_SOURCE_DIR}/packaging/windows/app.rc.in" "${QUTE_RC_FILE}" @ONLY)
        target_sources(${target} PRIVATE "${QUTE_RC_FILE}")
        set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE TRUE)
    endfunction()
endif()

option(QUTE_BUILD_TESTS "Build unit tests (needs Qt6::Test)" ON)

add_subdirectory(src/core)
add_subdirectory(src/platform)
add_subdirectory(src/setup)
add_subdirectory(src/overlay)
add_subdirectory(src/settings)

if(QUTE_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NOT APPLE)
    install(DIRECTORY assets/spritepacks DESTINATION ${CMAKE_INSTALL_DATADIR}/qute
            PATTERN "preview.png" EXCLUDE)
    install(FILES packaging/qute-settings.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/qute.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/icons/qute.svg packaging/icons/qute-settings.svg
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    # The GNOME Shell extension is installed as data, not into the system extensions folder:
    # the user enables it with packaging/gnome-shell/install.sh (or the copy under share/qute).
    install(DIRECTORY packaging/gnome-shell/qute@qute-agent
            DESTINATION ${CMAKE_INSTALL_DATADIR}/qute/gnome-shell)
    install(PROGRAMS packaging/gnome-shell/install.sh
            DESTINATION ${CMAKE_INSTALL_DATADIR}/qute/gnome-shell)
    install(FILES README.md docs/ARCHITECTURE.md docs/OPENCODE.md docs/PLATFORMS.md docs/MACOS.md
                  docs/SETUP_WIZARD.md docs/SPRITE_PACK.md
            DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()
