include(FetchContent)

#
# Make sure submodules are up to date...
#
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git" AND IG_CHECK_SUBMODULES_ON_BUILD)
  message(STATUS "Submodule update...")
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  RESULT_VARIABLE GIT_SUBMOD_RESULT)
  if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
  endif()
endif()

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
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
if (EMSCRIPTEN)
  set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(
  protobuf
  GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf"
  GIT_TAG "41e22cde8d8a44c35127a26c19e08b180e0b30a4"
  SOURCE_SUBDIR cmake
  CMAKE_ARGS
    -protobuf_BUILD_TESTS:BOOL=OFF
    -protobuf_BUILD_SHARED_LIBS:BOOL=OFF
    -protobuf_WITH_ZLIB:BOOL=OFF
)
FetchContent_MakeAvailable(protobuf)

# Draco (3D geometry compression library)
FetchContent_Declare(
  draco
  GIT_REPOSITORY "https://github.com/google/draco"
  GIT_TAG "1a64bb27129d1dcf5825b368c484441d2e3ec039"
)
set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(draco)

if (MSVC)
  add_library(draco_lib ALIAS draco)
else ()
  add_library(draco_lib ALIAS draco_static)
endif ()

# STB
add_subdirectory(stb EXCLUDE_FROM_ALL)

# GLFW (only on native builds)
if (NOT EMSCRIPTEN)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw/"
    GIT_TAG "df8d7bc892937a8b0f7c604c92a9f64f383cf48c"
  )
  FetchContent_MakeAvailable(glfw)
endif ()

# Dawn (only on native builds)
IF (NOT EMSCRIPTEN)
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

# Recast Navigation (used for navigation)

set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  recastnavigation
  GIT_REPOSITORY "https://github.com/recastnavigation/recastnavigation"
  GIT_TAG "c5cbd53024c8a9d8d097a4371215e3342d2fdc87"
)
FetchContent_MakeAvailable(recastnavigation)

# CLIUtils (used in tooling builds)
if (NOT EMSCRIPTEN)
  FetchContent_Declare(
    cli11
    GIT_REPOSITORY "https://github.com/CLIUtils/CLI11"
    GIT_TAG "5ba63620dc478bcdeb0e0feabc9990ac523c7643"
  )
  FetchContent_MakeAvailable(cli11)
endif ()

# Assimp (used in tooling builds)
if (NOT EMSCRIPTEN)
  set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    assimp
    GIT_REPOSITORY "https://github.com/assimp/assimp"
    GIT_TAG "3cca102531466f5e5f0af0d90858a23ad83ce662"
  )
  FetchContent_MakeAvailable(assimp)
endif ()

# WebRTC / WebSocket support (not used in Web builds - those use browser APIs)
if (NOT EMSCRIPTEN)
  set(NO_MEDIA OFF CACHE BOOL "" FORCE)
  add_subdirectory(libdatachannel)

  FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY "https://github.com/zaphoyd/websocketpp"
    GIT_TAG "1b11fd301531e6df35a6107c1e8665b1e77a2d8e"
  )
  FetchContent_MakeAvailable(websocketpp)
endif ()

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME libprotobuf AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME Recast AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME Detour AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME DetourCrowd AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME draco_static AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME stb AS_LIB 1)
endif ()
