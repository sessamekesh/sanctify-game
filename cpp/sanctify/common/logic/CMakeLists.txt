set (HEADER_LIST
  "locomotion/locomotion_components.h"
  "locomotion/locomotion_system.h")

set (SRC_LIST
  "locomotion/locomotion_system.cc")

set (TEST_SRC_LIST
  "locomotion/locomotion_system_test.cc")

add_library(sanctify-common-logic STATIC ${HEADER_LIST} ${SRC_LIST})
target_include_directories(sanctify-common-logic PUBLIC "${SANCTIFY_INCLUDE_ROOT}")

target_link_libraries(sanctify-common-logic PUBLIC igcore EnTT glm)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-common-logic AS_LIB 1)
endif ()

if (IG_BUILD_TESTS)
  add_executable(sanctify-common-logic-test ${TEST_SRC_LIST})
  target_link_libraries(sanctify-common-logic-test gtest gtest_main sanctify-common-logic)
  
  gtest_discover_tests(sanctify-common-logic-test
    WORKING_DIRECTORY ${PROJECT_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}")

  set_target_properties(sanctify-common-logic-test PROPERTIES FOLDER tests)
endif ()
