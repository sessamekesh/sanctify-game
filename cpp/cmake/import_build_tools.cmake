#
# Build tools wrangler
#
# For Emscripten builds, imports the CMake file that includes NATIVE binary targets for tools.
#

if (EMSCRIPTEN)
  if (NOT IG_TOOL_WRANGLE_PATH)
	message(FATAL_ERROR "Must provide tool wrangling path with -DIG_TOOL_WRANGLE_PATH=\"some_path\"")
  endif ()
  include(${IG_TOOL_WRANGLE_PATH} RESULT_VARIABLE tool_wrangle_rsl)
  if (tool_wrangle_rsl EQUAL NOTFOUND)
	message(FATAL_ERROR "Tool wrangling failed! Could not find tool wrangler script at ${IG_TOOL_WRANGLE_PATH}")
  endif()

  message(STATUS "Tool wrangling succeeded! Tools have been read from path ${IG_TOOL_WRANGLE_PATH}")
endif ()