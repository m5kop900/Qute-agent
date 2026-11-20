# Platform layer: all OS / desktop-environment specific code lives here and ONLY here.
target_sources(qute_core PRIVATE
    platformshell.h
    platformshell.cpp
    genericshell.h
    genericshell.cpp
    gnomeshell.h
    gnomeshell.cpp
    kwinshell.h
    kwinshell.cpp
    placementscript.h
    terminallauncher.h
    terminallauncher.cpp
)

if(APPLE)
    target_sources(qute_core PRIVATE macshell.h macshell.mm)
    target_link_libraries(qute_core PUBLIC "-framework Cocoa")
endif()
