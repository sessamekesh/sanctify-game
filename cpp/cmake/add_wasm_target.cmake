function (set_wasm_target_properties)
  set(options)
  set(oneValueArgs TARGET_NAME AS_LIB EXPORT_NAME)
  set(multiValueArgs)
  cmake_parse_arguments(AWT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (${AWT_TARGET_NAME} STREQUAL "")
    message(WARNING "WASM target declared but not given a name - skipping")
    return()
  endif ()

  if (NOT EMSCRIPTEN)
    message(WARNING "WASM target ${AWT_TARGET_NAME} declared on a non-WASM build - skipping")
    return()
  endif ()

  if (NOT AS_LIB)
    set_target_properties(
      ${AWT_TARGET_NAME}
      PROPERTIES
        SUFFIX ".js")
  endif ()

  target_link_options(
    ${AWT_TARGET_NAME}
      PUBLIC "SHELL:--bind -s WASM=1 -s MODULARIZE=1 -s ENVIRONMENT=web,worker -s WASM_BIGINT=1"
      PUBLIC "SHELL:-s USE_GLFW=3 -s FETCH=1 -s INITIAL_MEMORY=268435456 -s EXIT_RUNTIME=0"
      PUBLIC "SHELL:-s MALLOC=emmalloc -s FILESYSTEM=0 -s USE_WEBGPU=1 -lwebsocket.js")

  if (NOT ${AWT_EXPORT_NAME} STREQUAL "")
    target_link_options(
      ${AWT_TARGET_NAME}
      PUBLIC "SHELL:-s EXPORT_NAME=${AWT_EXPORT_NAME}")
  endif ()

  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${AWT_TARGET_NAME} PUBLIC -gsource-map -O0)
    target_link_options(${AWT_TARGET_NAME} PUBLIC "SHELL:-s-gsource-map -O0")
  else ()
    target_compile_options(${AWT_TARGET_NAME} PUBLIC -O3)
    target_link_options(${AWT_TARGET_NAME} PUBLIC "SHELL:-O3")
  endif ()

  if (IG_ENABLE_THREADS)
    target_compile_options(${AWT_TARGET_NAME} PUBLIC "-pthread")
    target_link_options(
      ${AWT_TARGET_NAME}
        PUBLIC "SHELL:-s ASSERTIONS=1 -s USE_PTHREADS=1 -pthread"
        PUBLIC "SHELL:-s PTHREAD_POOL_SIZE=\"Math.max(((navigator&&navigator.hardwareConcurrency)||4),6)\"")
  endif ()

endfunction ()