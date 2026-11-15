# Terminal setup wizard: `qute opencode ...` / `qute setup`. Pure Qt Core + Network, no GUI.
qt_add_library(qute_setup STATIC
    jsonc.cpp            jsonc.h
    tty.cpp              tty.h
    installer.cpp        installer.h
    opencodeconfig.cpp   opencodeconfig.h
    mcppresets.cpp       mcppresets.h
    providerprobe.cpp    providerprobe.h
    secrets.cpp          secrets.h
    setupcli.cpp         setupcli.h
)
target_include_directories(qute_setup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qute_setup PUBLIC qute_core Qt6::Core Qt6::Network)
