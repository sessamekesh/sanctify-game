set(header_list
  "include/iggpu/texture.h"
  "include/iggpu/ubo_base.h"
  "include/iggpu/util.h")

set(src_list
  "src/util.cc")

add_library(iggpu STATIC ${header_list} ${src_list})
target_include_directories(iggpu PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(iggpu PUBLIC igcore glm igasset)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME iggpu AS_LIB 1)
else ()
  target_link_libraries(iggpu PUBLIC dawncpp dawn_common)
endif ()