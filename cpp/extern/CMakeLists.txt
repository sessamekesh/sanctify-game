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

if (MSVC)
  add_library(draco_lib ALIAS draco)
else ()
  add_library(draco_lib ALIAS draco_static)
endif ()

# add_executable(protoc ALIAS protobuf::protoc)