configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/igcore/config.h")

set (HEADER_LIST
  "include/igcore/any.h"
  "include/igcore/dirtyable.h"
  "include/igcore/either.h"
  "include/igcore/fps_meter.h"
  "include/igcore/fastmath.h"
  "include/igcore/ivec.h"
  "include/igcore/log.h"
  "include/igcore/math.h"
  "include/igcore/maybe.h"
  "include/igcore/pod_vector.h"
  "include/igcore/raw_buffer.h"
  "include/igcore/typeid.h"
  "include/igcore/vector.h")

set (SRC_LIST
  "src/fps_meter.cc"
  "src/fastmath.cc"
  "src/log.cc"
  "src/math.cc"
  "src/raw_buffer.cc"
  "src/typeid.cc")

set(VISUAL_STUDIO_EMPTY_SOURCES
  "src/ivec.cc"
  "src/maybe.cc"
  "src/pod_vector.cc"
  "src/vector.cc")

if (MSVC)
  list(APPEND SRC_LIST ${VISUAL_STUDIO_EMPTY_SOURCES})
endif()

add_library(igcore STATIC ${HEADER_LIST} ${SRC_LIST})

target_include_directories(igcore PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(igcore PUBLIC glm)

set_property(TARGET igcore PROPERTY CXX_STANDARD 17)
target_compile_features(igcore PUBLIC cxx_std_17)

if (IG_BUILD_TESTS)
  set(TEST_SRC_LIST
    "test/either_test.cc"
    "test/maybe_test.cc")
  
  add_executable(igcore_test ${TEST_SRC_LIST})
  target_link_libraries(igcore_test gtest gtest_main igcore)
  gtest_discover_tests(igcore_test
    # Set a working directory both for GTest and Visual Studio to be happy
    WORKING_DIRECTORY ${PROJECT_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
  )

  set_property(TARGET igcore_test PROPERTY CXX_STANDARD 17)
  target_compile_features(igcore_test PUBLIC cxx_std_17)
endif ()