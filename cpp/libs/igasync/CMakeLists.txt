set (HEADER_LIST
  "include/igasync/promise.h"
  "include/igasync/frame_task_scheduler.h"
  "include/igasync/promise_combiner.h"
  "include/igasync/promise_combiner_old.h"
  "include/igasync/task_list.h")

set (SRC_LIST
  "src/promise_combiner.cc"
  "src/promise_combiner_old.cc"
  "src/task_list.cc")

set(VISUAL_STUDIO_EMPTY_SOURCES "src/promise.cc")

if (MSVC)
  list(APPEND SRC_LIST ${VISUAL_STUDIO_EMPTY_SOURCES})
endif()

if (IG_ENABLE_THREADS)
  list(APPEND HEADER_LIST "include/igasync/executor_thread.h")
  list(APPEND SRC_LIST "src/executor_thread.cc")
endif ()

add_library(igasync STATIC ${HEADER_LIST} ${SRC_LIST})
target_include_directories(igasync PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(igasync PUBLIC igcore)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME igasync AS_LIB 1)
endif ()

if (IG_BUILD_TESTS)
  set(TEST_SRC_LIST
    "test/promise_combiner_test.cc"
    "test/promise_test.cc")
  
  add_executable(igasync_test ${TEST_SRC_LIST})
  target_link_libraries(igasync_test gtest gtest_main igasync)
  gtest_discover_tests(igasync_test
    # Set a working directory both for GTest and Visual Studio to be happy
    WORKING_DIRECTORY ${PROJECT_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
  )

  set_property(TARGET igasync_test PROPERTY CXX_STANDARD 17)
  target_compile_features(igasync_test PUBLIC cxx_std_17)
endif ()