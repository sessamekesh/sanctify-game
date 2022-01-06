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

set_property(TARGET igplatform PROPERTY CXX_STANDARD 17)
target_compile_features(igplatform PUBLIC cxx_std_17)