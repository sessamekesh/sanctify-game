if (EMSCRIPTEN)
  message(STATUS "Sanctify game server will not be generated for EMSCRIPTEN build")
  return()
endif ()

find_package(Boost 1.67 REQUIRED COMPONENTS system thread)

set(header_list
  "app/game_server.h"
  "net/dummy_game_token_exchanger.h"
  "net/igametokenexchanger.h"
  "util/cli.h"
  "util/event_scheduler.h"
  "util/types.h")

set(src_list
  "app/game_server.cc"
  "net/dummy_game_token_exchanger.cc"
  "util/event_scheduler.cc"
  "main.cc")

add_executable(sanctify-game-server ${header_list} ${src_list})

# LEGAL NOTE - use libdatachannel and not libdatachannel-static. This is because
#  libdatachannel uses the LGPLv2 license and requires that a user be able to
#  freely link in their own build of the library in its place.
# Strictly speaking we could probably get around this a different way, but I'd
#  rather just avoid the headache entirely and link dynamically.
target_link_libraries(sanctify-game-server PUBLIC
  LibDataChannel::LibDataChannel CLI11 Boost::boost
  igcore igasync sanctify-game-common)

target_include_directories(sanctify-game-server PRIVATE
  . "${websocketpp_SOURCE_DIR}")

if (MSVC)
  add_custom_command(TARGET sanctify-game-server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:datachannel>/datachannel${CMAKE_DEBUG_POSTFIX}.dll"
    $<TARGET_FILE_DIR:sanctify-game-server>
  )
endif ()