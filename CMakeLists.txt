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
endif()
