# General function to add protocol buffer libraries to C++ builds - abstracts implementation details
#  between web and native builds

function (add_proto_lib)
  set(options)
  set(oneValueArgs OUTDIR TARGET_NAME)
  set(multiValueArgs INCLUDE_DIRS PROTO_INFILES)
  cmake_parse_arguments(APL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT TARGET protoc)
    message(FATAL_ERROR "protoc binary not available - this is probably because a tools build was not done on WASM")
  endif()

  if (APL_OUTDIR)
    set(proto_out_dir "${CMAKE_CURRENT_BINARY_DIR}/proto/${APL_OUTDIR}")
  else()
    set(proto_out_dir "${CMAKE_CURRENT_BINARY_DIR}/proto")
  endif()

  file(MAKE_DIRECTORY ${proto_out_dir})

  foreach(IN_PROTO IN LISTS APL_PROTO_INFILES)
    get_filename_component(protopath_absolute "${IN_PROTO}" ABSOLUTE)
    get_filename_component(protopath "${protopath_absolute}" DIRECTORY)
    get_filename_component(protopath_name "${IN_PROTO}" NAME_WE)

    set(proto_hdr "${proto_out_dir}/${protopath_name}.pb.h")
    set(proto_src "${proto_out_dir}/${protopath_name}.pb.cc")
    list(APPEND proto_hdrs "${proto_hdr}")
    list(APPEND proto_srcs "${proto_src}")

    # TODO (sessamekesh): On Linux build, this is for some reason escaping the spaces and failing!
    set(protoc_args "--cpp_out=\"${proto_out_dir}\"" "-I" "\"${protopath}\"")
    foreach(INCLUDE_DIR IN LISTS APL_INCLUDE_DIRS)
      set(protoc_args ${protoc_args} "-I" "\"${INCLUDE_DIR}\"")
    endforeach()
    set(protoc_args ${protoc_args} "\"${protopath_absolute}\"")

    add_custom_command(
      OUTPUT "${proto_hdr}" "${proto_src}"
      COMMAND protoc
      ARGS ${protoc_args}
      DEPENDS "${protopath_absolute}")
  endforeach()

  add_library(${APL_TARGET_NAME} STATIC "${proto_hdrs}" "${proto_srcs}")
  target_link_libraries(${APL_TARGET_NAME} PUBLIC libprotobuf)

  target_include_directories(${APL_TARGET_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/proto")
  set_target_properties(${APL_TARGET_NAME} PROPERTIES FOLDER proto_messages)
endfunction()