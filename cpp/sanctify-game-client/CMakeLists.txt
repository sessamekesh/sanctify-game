set(header_list
  "platform_src/app_base.h"
  "src/app_startup_scene/startup_shader_src.h"
  "src/app_startup_scene/app_startup_scene.h"
  "src/sanctify_client_app.h")

set(src_list
  "src/app_startup_scene/app_startup_scene.cc"
  "src/sanctify_client_app.cc")

if (NOT EMSCRIPTEN)
  set(platform_src_list
    "platform_src/app_base_native.cc"
    "platform_src/native_main.cc")
else ()
  set(platform_src_list
    "platform_src/app_base_web.cc"
    "platform_src/emscripten_bindings.cc")
endif ()

add_executable(sanctify-game-client ${header_list} ${src_list} ${platform_src_list})
target_link_libraries(sanctify-game-client PUBLIC igasync igplatform igcore iggpu)
target_include_directories(sanctify-game-client PRIVATE platform_src src)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-game-client EXPORT_NAME SanctifyGameClient)
else ()
  target_link_libraries(sanctify-game-client
    PUBLIC dawncpp glfw dawn_native dawn_wire
    dawn_utils dawn_proc dawn_common)  
endif ()

if (WIN32)
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(sanctify-game-client PRIVATE "/Zi")
    target_link_options(sanctify-game-client PRIVATE "/SAFESEH:NO")
  endif()
endif ()