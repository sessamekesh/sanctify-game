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
  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/dawn/.gclient")
    message(STATUS "Configuring DAWN")

    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy
          "${PROJECT_SOURCE_DIR}/extern/dawn/scripts/standalone.gclient"
          "${PROJECT_SOURCE_DIR}/extern/dawn/.gclient"
      RESULT_VARIABLE gclient_copy_rsl)
    if (NOT ${gclient_copy_rsl} EQUAL 0)
      message(FATAL "Error copying gclient config for Dawn library")
    endif ()
  endif ()

  if (IG_CHECK_SUBMODULES_ON_BUILD OR NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/dawn/third_party/tint")
    message(STATUS "Running depot_tools sync for Dawn deps")
    if (WIN32)
	  execute_process(
		  COMMAND cmd /C "${PROJECT_SOURCE_DIR}/extern/depot_tools/gclient.bat" "sync"
		  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/extern/dawn")
    elseif (UNIX)
      execute_process(
		  COMMAND bash -c "${PROJECT_SOURCE_DIR}/extern/depot_tools/gclient.py" "sync"
		  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/extern/dawn")
    endif ()
  endif ()

  add_subdirectory(dawn)
endif ()

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
  set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
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

  add_library(http-request INTERFACE)
  target_include_directories(http-request INTERFACE "HTTPRequest/include")

  FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY "https://github.com/zaphoyd/websocketpp"
    GIT_TAG "1b11fd301531e6df35a6107c1e8665b1e77a2d8e"
  )
  FetchContent_MakeAvailable(websocketpp)
endif ()

# Digraph library, used for ECS scheduling verification
add_library(dglib INTERFACE)
target_include_directories(dglib INTERFACE "digraph")

# EnTT (Entity Component System library that powers core game logic)
if (MSVC)
  set(ENTT_INCLUDE_NATVIS OFF CACHE BOOL "" FORCE)
endif ()
add_subdirectory(entt)

# Dear ImGUI (used in certain tooling builds)
add_library(imgui
  "imgui/imgui.cpp"
  "imgui/imgui.h"
  "imgui/imgui_draw.cpp"
  "imgui/imgui_demo.cpp"
  "imgui/imgui_tables.cpp"
  "imgui/imgui_widgets.cpp"
  "imgui/misc/cpp/imgui_stdlib.cpp"
  "imgui/backends/imgui_impl_wgpu.cpp"
  "imgui/backends/imgui_impl_wgpu.h"
  "imgui/backends/imgui_impl_glfw.cpp"
  "imgui/backends/imgui_impl_glfw.h")
target_include_directories(imgui PUBLIC imgui imgui/backends)
target_link_libraries(imgui PUBLIC glfw dawncpp)

# Native file dialog (useful in GUI tooling builds)
if (NOT EMSCRIPTEN)
  add_library(nativefiledialog
    "nativefiledialog/src/nfd_common.c"
    "nativefiledialog/src/nfd_win.cpp")
  target_include_directories(nativefiledialog PUBLIC
    "nativefiledialog/src/include")
endif ()

# Recast/Detour navigation library
set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(recastnavigation)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ozz_build_samples OFF CACHE BOOL "" FORCE)
set(ozz_build_tests OFF CACHE BOOL "" FORCE)
set(ozz_build_fbx OFF CACHE BOOL "" FORCE)
set(ozz_build_gltf OFF CACHE BOOL "" FORCE)
set(ozz_build_tools OFF CACHE BOOL "" FORCE)
set(ozz_build_msvc_rt_dll ON CACHE BOOL "" FORCE)
add_subdirectory(ozz-animation)

if (EMSCRIPTEN)
  set_wasm_target_properties(TARGET_NAME libprotobuf AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME Recast AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME Detour AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME DetourCrowd AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME draco_static AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME stb AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME imgui AS_LIB 1)
  set_wasm_target_properties(TARGET_NAME ozz_animation AS_LIB 1)
endif ()
