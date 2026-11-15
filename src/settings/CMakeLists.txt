qt_add_executable(qute_settings
    main.cpp
)
set_target_properties(qute_settings PROPERTIES OUTPUT_NAME "qute-settings")

qt_add_qml_module(qute_settings
    URI Qute.Settings
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(qute_settings PRIVATE qute_core Qt6::QuickControls2)
if(APPLE)
    set_target_properties(qute_settings PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/macos/Info-settings.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.qute.settings"
        MACOSX_BUNDLE_BUNDLE_NAME "Qute Agent Settings"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    add_custom_command(TARGET qute_settings POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/spritepacks
                $<TARGET_BUNDLE_CONTENT_DIR:qute_settings>/Resources/spritepacks)
    install(TARGETS qute_settings BUNDLE DESTINATION .)
else()
    install(TARGETS qute_settings RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
