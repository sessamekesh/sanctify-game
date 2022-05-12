set (HEADER_LIST
  "include/igecs/ctti_type_id.h"
  "include/igecs/evt_queue.h"
  "include/igecs/scheduler.h"
  "include/igecs/world_view.h")

set (SRC_LIST
  "src/ctti_type_id.cc"
  "src/scheduler.cc"
  "src/world_view.cc")

set (TEST_SRC_LIST
  "test/ctti_type_id_test.cc"
  "test/scheduler_test.cc"
  "test/world_view_test.cc")

add_library(igecs STATIC ${HEADER_LIST} ${SRC_LIST})
target_include_directories(igecs PUBLIC include)
target_link_libraries(igecs PUBLIC igcore igasync EnTT)
target_link_libraries(igecs PRIVATE dglib)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME igecs AS_LIB 1)
endif ()

if (IG_BUILD_TESTS)
  add_executable(igecs-test ${TEST_SRC_LIST})
  target_link_libraries(igecs-test PUBLIC gtest gtest_main igecs)

  add_test(igecs-test igecs-test)

  gtest_discover_tests(igecs-test
    WORKING_DIRECTORY ${PROJECT_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}")

  set_target_properties(igecs-test PROPERTIES FOLDER tests)
endif ()
