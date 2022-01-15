set (HEADER_LIST
  "include/igplatform/file_promise.h")

set(COMMON_SRC_LIST
  "src/file_promise_common.cc")

if (EMSCRIPTEN)
  set (SRC_LIST
    "src/web/file_promise.cc")
else ()
  set (SRC_LIST
    "src/native/file_promise.cc")
endif ()

add_library(igplatform STATIC ${HEADER_LIST} ${COMMON_SRC_LIST} ${SRC_LIST})

target_include_directories(igplatform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(igplatform PUBLIC igcore igasync)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME igplatform AS_LIB 1)
endif ()