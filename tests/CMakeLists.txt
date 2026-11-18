qt_add_library(qute_mockserver STATIC
    mockserver/mockopencodeserver.cpp
    mockserver/mockopencodeserver.h
)
target_link_libraries(qute_mockserver PUBLIC Qt6::Network)
target_include_directories(qute_mockserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mockserver)

foreach(t tst_sseparser tst_agentstate tst_jsonc tst_opencodeconfig tst_clientparse tst_spritepacks
          tst_conversationmodel tst_sessionstore tst_catalog tst_modes tst_sessions)
    qt_add_executable(${t} ${t}.cpp)
    target_link_libraries(${t} PRIVATE qute_setup qute_core qute_mockserver Qt6::Test)
    add_test(NAME ${t} COMMAND ${t})
endforeach()
