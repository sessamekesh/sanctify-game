# NOTICE - keep this version as is - moving above 3.14 breaks protocol buffer integration in a remarkably weird way
# for Windows builds. One day I'd love to figure out what the hell is going on, but today is not that day.
cmake_minimum_required(VERSION 3.14)

project(
  sanctify-game-cxx
  VERSION 0.1
  DESCRIPTION "Collection of C++ libraries and executables for the Sanctify MoBA game"
  LANGUAGES C CXX
)

#
# Global configuration flags
#
set(IG_BUILD_TESTS "ON" CACHE BOOL "Build unit tests")
set(IG_TOOL_WRANGLE_PATH "" CACHE FILEPATH "Where to import/export the tool wrangling cmake file (native builds should export a tool wrangle file, and web builds import a wrangle file for pre-build native binaries)")
set(IG_ENABLE_THREADS "ON" CACHE BOOL "Enable multithreaded build")
set(IG_ENABLE_GRAPHICS_DEBUGGING "OFF" CACHE BOOL "Enable graphics debugging options (e.g. export HLSL debug symbols)")
set(IG_ENABLE_LOGGING "ON" CACHE BOOL "Include logging statements in the build")
set(IG_CHECK_SUBMODULES_ON_BUILD "OFF" CACHE BOOL "Check external submodules on build - off by default, but use with fresh builds")
set(IG_BUILD_SERVER "ON" CACHE BOOL "Include the server build (worth disabling if you don't want to worry about a Boost dependency)")
set(IG_ENABLE_ECS_VALIDATION "ON" CACHE BOOL "Include asserts that validate ECS concurrency safety (useful in debugging, but creates loud errors)")

#
# Global project settings
#
set(CMAKE_CXX_STANDARD 17)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

#
# Testing support
#
if (IG_BUILD_TESTS)
  include(FetchContent)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY "https://github.com/google/googletest"
    GIT_TAG "6b74da4757a549563d7c37c8fae3e704662a043b"
  )

  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  include(GoogleTest)
endif ()

#
# Include subdirectories that include library and executable outputs
#

# The ordering here is very specific, and exposes a rough dependency graph
#  among the different categories of CPP project that must not be violated!
# 1) External dependencies (extern AND cmake/wrangle_build_tools.cmake)
# 2) Common CMake utilities (macros to generate protobuf libraries, etc)
# 3) Common (indigo) libraries
# 4) Tools (e.g. asset bundling tool)
# 5) Exported applications (game server, game client)

include(cmake/add_wasm_target.cmake)
add_subdirectory(extern)
include(cmake/import_build_tools.cmake)
include(cmake/add_proto_lib.cmake)
include(cmake/add_proto_lib_2.cmake)
add_subdirectory(libs)
include(cmake/build_ig_asset_pack_plan.cmake)
add_subdirectory(tools)
include(cmake/export_build_tools.cmake)
add_subdirectory(sanctify-game)
add_subdirectory(sanctify)
