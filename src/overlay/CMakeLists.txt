qt_add_executable(qute
    main.cpp
    overlaycontroller.cpp
    overlaycontroller.h
)

qt_add_qml_module(qute
    URI Qute.Overlay
    VERSION 1.0
    QML_FILES
        Main.qml
        SpriteView.qml
        Composer.qml
        InlineList.qml
        MessageText.qml
        SelectableText.qml
        ThemedMenu.qml
        CodeHighlight.js
        ControlChip.qml
        ContextRing.qml
        PickerPopup.qml
        ConversationView.qml
        AttachmentStrip.qml
        AttachmentChip.qml
        Theme.qml
        MenuStrip.qml
        Hint.qml
        PillButton.qml
)

target_link_libraries(qute PRIVATE qute_setup qute_core Qt6::QuickControls2)
if(APPLE)
    # qute.app: accessory app (LSUIElement). The CLI is the same binary: qute.app/Contents/MacOS/qute
    set_target_properties(qute PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/macos/Info-qute.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.qute.overlay"
        MACOSX_BUNDLE_BUNDLE_NAME "Qute Agent"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    add_custom_command(TARGET qute POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets/spritepacks
                $<TARGET_BUNDLE_CONTENT_DIR:qute>/Resources/spritepacks
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/packaging/macos/io.github.qute.daemon.plist
                $<TARGET_BUNDLE_CONTENT_DIR:qute>/Resources/io.github.qute.daemon.plist)
    install(TARGETS qute BUNDLE DESTINATION .)
elseif(WIN32)
    qute_add_windows_resource(qute qute.ico "Qute Agent overlay" qute qute.exe)
    install(TARGETS qute RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    install(TARGETS qute RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
