if (EMSCRIPTEN)
  message(STATUS "Sanctify game server will not be generated for EMSCRIPTEN build")
  return()
endif ()

set(src_list "main.cc")

add_executable(sanctify-game-server ${src_list})