#
# Build tools exporter
#
# For native builds, assembles a CMake file that includes binary targets for tools.
#

if (NOT EMSCRIPTEN AND IG_TOOL_WRANGLE_PATH)
  message(STATUS "Writing import-igtools.cmake file to ${IG_TOOL_WRANGLE_PATH}")
  export(TARGETS protoc FILE "${IG_TOOL_WRANGLE_PATH}")
endif()