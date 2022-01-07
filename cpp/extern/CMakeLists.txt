include(FetchContent)

if (IG_BUILD_TESTS)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY "https://github.com/google/googletest"
    GIT_TAG "6b74da4757a549563d7c37c8fae3e704662a043b"
  )

  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  include(CTest)
  include(GoogleTest)
endif ()

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY "https://github.com/g-truc/glm"
  GIT_TAG "6ad79aae3eb5bf809c30bf1168171e9e55857e45"
)
FetchContent_MakeAvailable(glm)

# Protobuf (for building/using protocol buffer libraries)
FetchContent_Declare(
  protobuf
  GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf"
  GIT_TAG "b73f78d32cc6d1013986de76789481dcaec2d064"
  SOURCE_SUBDIR cmake
)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
if (EMSCRIPTEN)
  set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(protobuf)

# Draco (3D geometry compression library)
FetchContent_Declare(
  draco
  GIT_REPOSITORY "https://github.com/google/draco"
  GIT_TAG "1a64bb27129d1dcf5825b368c484441d2e3ec039"
)
set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(draco)

# STB
add_subdirectory(stb EXCLUDE_FROM_ALL)

# Dawn (only on native builds)
IF (WIN32 OR UNIX)
  FetchContent_Declare(
    depot_tools
    GIT_REPOSITORY "https://chromium.googlesource.com/chromium/tools/depot_tools.git"
    GIT_TAG "9552069c06705e149b156bcc70b84c1636f22f5e"
  )
  FetchContent_Declare(
    dawn
    GIT_REPOSITORY "https://dawn.googlesource.com/dawn"
    GIT_TAG "e009ad7edd6882ca0f749bf1d8d52b844ffb56ae"
  )
  FetchContent_MakeAvailable(depot_tools)

  message(STATUS "Depot tools directory: ${depot_tools_SOURCE_DIR}")
  if (NOT dawn_POPULATED)
    FetchContent_Populate(dawn)

    if (NOT EXISTS "${dawn_SOURCE_DIR}/.gclient")
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy
          "${dawn_SOURCE_DIR}/scripts/standalone.gclient"
          "${dawn_SOURCE_DIR}/.gclient"
          RESULT_VARIABLE copy_rsl)
      if (NOT ${copy_rsl} EQUAL 0)
        message(FATAL "Error copying gclient config for DAWN!")
      endif ()
    endif ()

    set(ENV{DEPOT_TOOLS_WIN_TOOLCHAIN} 0)
    if (WIN32)
      execute_process(
        COMMAND cmd /C "${depot_tools_SOURCE_DIR}/gclient.bat" "sync"
        WORKING_DIRECTORY "${dawn_SOURCE_DIR}"
        RESULT_VARIABLE gclient_sync_rsl)
    elseif (UNIX)
      execute_process(
        COMMAND bash -c "${depot_tools_SOURCE_DIR}/gclient.py" "sync"
        WORKING_DIRECTORY "${dawn_SOURCE_DIR}"
        RESULT_VARIABLE gclient_sync_rsl)
    endif ()
    if (NOT ${gclient_sync_rsl} EQUAL 0)
      message(FATAL "Error syncing Dawn dependencies")
    endif ()

    add_subdirectory(${dawn_SOURCE_DIR} ${dawn_BINARY_DIR})
  endif ()
endif ()

if (MSVC)
  add_library(draco_lib ALIAS draco)
else ()
  add_library(draco_lib ALIAS draco_static)
endif ()

# add_executable(protoc ALIAS protobuf::protoc)