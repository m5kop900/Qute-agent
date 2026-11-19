# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "src/core/CMakeFiles/qute_core_autogen.dir/AutogenUsed.txt"
  "src/core/CMakeFiles/qute_core_autogen.dir/ParseCache.txt"
  "src/core/qute_core_autogen"
  "src/overlay/CMakeFiles/qute_autogen.dir/AutogenUsed.txt"
  "src/overlay/CMakeFiles/qute_autogen.dir/ParseCache.txt"
  "src/overlay/qute_autogen"
  "src/settings/CMakeFiles/qute_settings_autogen.dir/AutogenUsed.txt"
  "src/settings/CMakeFiles/qute_settings_autogen.dir/ParseCache.txt"
  "src/settings/qute_settings_autogen"
  "src/setup/CMakeFiles/qute_setup_autogen.dir/AutogenUsed.txt"
  "src/setup/CMakeFiles/qute_setup_autogen.dir/ParseCache.txt"
  "src/setup/qute_setup_autogen"
  )
endif()
