#ifndef LIBS_IGASSET_INCLUDE_IGASSET_PROTO_CONVERTERS_H
#define LIBS_IGASSET_INCLUDE_IGASSET_PROTO_CONVERTERS_H

#include <igasset/proto/igasset.pb.h>

#include <glm/glm.hpp>

namespace indigo::asset {

bool write_pb_mat4(pb::Mat4* o_mat, const glm::mat4& mat);

bool read_pb_mat4(glm::mat4& o_mat, const pb::Mat4& mat);

}  // namespace indigo::asset

#endif
