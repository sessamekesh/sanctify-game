if (NOT EMSCRIPTEN)
  add_subdirectory(igpack-gen)
  add_subdirectory(list-assimp-assets)

  if (WIN32)
    add_subdirectory(map-editor)
  endif ()
endif ()