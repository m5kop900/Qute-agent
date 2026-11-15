qt_add_library(qute_core STATIC
    appconfig.cpp        appconfig.h
    singleinstance.cpp   singleinstance.h
    sseparser.cpp        sseparser.h
    agentstate.cpp       agentstate.h
    opencodeclient.cpp   opencodeclient.h
    spritepackmanager.cpp spritepackmanager.h
    opencodelocator.cpp  opencodelocator.h
)

target_include_directories(qute_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../platform)

target_link_libraries(qute_core PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Network)

target_compile_definitions(qute_core PUBLIC
    QUTE_VERSION="${PROJECT_VERSION}"
    QUTE_DEV_PACKS_DIR="${CMAKE_SOURCE_DIR}/assets/spritepacks"
    QUTE_INSTALL_PACKS_DIR="${CMAKE_INSTALL_FULL_DATADIR}/qute/spritepacks")

if(TARGET Qt6::DBus AND UNIX AND NOT APPLE)
    target_link_libraries(qute_core PUBLIC Qt6::DBus)
    target_compile_definitions(qute_core PUBLIC QUTE_HAVE_DBUS)
endif()
