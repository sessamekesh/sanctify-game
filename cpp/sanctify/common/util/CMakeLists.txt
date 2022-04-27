set(HEADER_LIST
  "camera_util.h"
  "resource_registry.h")

set(SRC_LIST
  "camera_util.cc")

set(TEST_SRC_LIST
  "camera_util_test.cc"
  "resource_registry_test.cc")

add_library(sanctify-common-util STATIC ${HEADER_LIST} ${SRC_LIST})
target_include_directories(sanctify-common-util PUBLIC "${SANCTIFY_INCLUDE_ROOT}")

target_link_libraries(sanctify-common-util PUBLIC igcore)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME sanctify-common-util AS_LIB 1)
endif ()

if (IG_BUILD_TESTS)
  add_executable(sanctify-common-util-test ${TEST_SRC_LIST})
  target_link_libraries(sanctify-common-util-test gtest gtest_main sanctify-common-util)

  gtest_discover_tests(sanctify-common-util-test
    WORKING_DIRECTORY ${PROJECT_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}")

  set_target_properties(sanctify-common-util-test PROPERTIES FOLDER tests)
endif ()
