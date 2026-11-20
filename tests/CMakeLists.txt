qt_add_library(qute_mockserver STATIC
    mockserver/mockopencodeserver.cpp
    mockserver/mockopencodeserver.h
)
target_link_libraries(qute_mockserver PUBLIC Qt6::Network)
target_include_directories(qute_mockserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mockserver)

foreach(t tst_sseparser tst_agentstate tst_jsonc tst_opencodeconfig tst_clientparse tst_spritepacks
          tst_conversationmodel tst_sessionstore tst_catalog tst_modes tst_sessions
          tst_attachments tst_slashcommands tst_composertokens tst_live tst_compactionpolicy
          tst_theme tst_appconfig)

    qt_add_executable(${t} ${t}.cpp)
    target_link_libraries(${t} PRIVATE qute_setup qute_core qute_mockserver Qt6::Test)
    add_test(NAME ${t} COMMAND ${t})
endforeach()

# UI logic of the prompt box (QML, loaded from the source tree).
find_package(Qt6 REQUIRED COMPONENTS QuickTest)
qt_add_executable(tst_composer_ui tst_composer_ui.cpp)
target_link_libraries(tst_composer_ui PRIVATE qute_core Qt6::QuickTest Qt6::Qml)
add_test(NAME tst_composer_ui COMMAND tst_composer_ui -input ${CMAKE_CURRENT_SOURCE_DIR}/qml)

# Smart chatbox positioning: the geometry has no Qt types, so its test is a plain executable (it needs no Qt to run),
# and a parity check compares the C++ algorithm with its JavaScript port (KWin script, GNOME extension).
add_executable(tst_chatboxplacement tst_chatboxplacement.cpp ${CMAKE_SOURCE_DIR}/src/core/chatboxplacement.cpp)
target_include_directories(tst_chatboxplacement PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
add_test(NAME tst_chatboxplacement COMMAND tst_chatboxplacement)

find_program(QUTE_NODE node)
if(QUTE_NODE AND UNIX)
    add_executable(placement_parity placement/parity.cpp ${CMAKE_SOURCE_DIR}/src/core/chatboxplacement.cpp)
    target_include_directories(placement_parity PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
    add_test(NAME placement_parity COMMAND ${QUTE_NODE} ${CMAKE_CURRENT_SOURCE_DIR}/placement/parity.js $<TARGET_FILE:placement_parity>)
endif()
