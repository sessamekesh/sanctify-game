# General function to add protocol buffer libraries to C++ builds - abstracts implementation details
#  between web and native builds, or between client/server processes

function (add_proto_lib_2)
  set(options)
  set(oneValueArgs TARGET_NAME)
  set(multiValueArgs PROTO_INFILES PROTO_LIB_DEPS)
  cmake_parse_arguments(APL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT TARGET protoc)
    message(FATAL_ERROR "protoc binary not available - this is probably because a tools build was not done on WASM")
  endif ()

  set(proto_in_dir "${PROJECT_SOURCE_DIR}")
  set(proto_out_dir "${PROJECT_BINARY_DIR}/proto")

  file(MAKE_DIRECTORY "${proto_out_dir}")

  set(protoc_args "--cpp_out=\"${proto_out_dir}\"" "-I" "\"${proto_in_dir}\"")

  foreach (in_proto IN LISTS APL_PROTO_INFILES)
    get_filename_component(protopath_absolute "${in_proto}" ABSOLUTE)
    get_filename_component(protopath_dir "${protopath_absolute}" DIRECTORY)
    get_filename_component(protopath_name "${in_proto}" NAME_WE)
    file(RELATIVE_PATH protopath_absout "${proto_in_dir}" "${protopath_absolute}")
    get_filename_component(protopath_relout "${protopath_absout}" DIRECTORY)

    set(proto_hdr "${proto_out_dir}/${protopath_relout}/${protopath_name}.pb.h")
    set(proto_cc "${proto_out_dir}/${protopath_relout}/${protopath_name}.pb.cc")

    list (APPEND proto_hdrs "${proto_hdr}")
    list (APPEND proto_srcs "${proto_cc}")

    add_custom_command(
      OUTPUT "${proto_hdr}" "${proto_cc}"
      COMMAND protoc ${protoc_args} "${protopath_absolute}"
      DEPENDS "${protopath_absolute}"
    )
  endforeach ()

  add_library(${APL_TARGET_NAME} STATIC ${proto_hdrs} ${proto_srcs})
  target_link_libraries(${APL_TARGET_NAME} PUBLIC libprotobuf)

  foreach(IN_LIB IN LISTS APL_PROTO_LIB_DEPS)
    target_link_libraries(${APL_TARGET_NAME} PUBLIC "${IN_LIB}")
  endforeach()

  target_include_directories(${APL_TARGET_NAME} PUBLIC "${proto_out_dir}")
  set_target_properties(${APL_TARGET_NAME} PROPERTIES FOLDER proto_messages)
endfunction ()