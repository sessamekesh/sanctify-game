if (EMSCRIPTEN)
  message(STATUS "Sanctify game server will not be generated for EMSCRIPTEN build")
  return()
endif ()

set(src_list "main.cc")

add_executable(sanctify-game-server ${src_list})

# LEGAL NOTE - use libdatachannel and not libdatachannel-static. This is because
#  libdatachannel uses the LGPLv2 license and requires that a user be able to
#  freely link in their own build of the library in its place.
# Strictly speaking we could probably get around this a different way, but I'd
#  rather just avoid the headache entirely and link dynamically.
target_link_libraries(sanctify-game-server PUBLIC LibDataChannel::LibDataChannel)

if (MSVC)
  add_custom_command(TARGET sanctify-game-server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:datachannel>/datachannel${CMAKE_DEBUG_POSTFIX}.dll"
    $<TARGET_FILE_DIR:sanctify-game-server>
  )
endif ()
