#include <igasset/proto_converters.h>

using namespace indigo;
using namespace asset;

bool asset::write_pb_mat4(pb::Mat4* o_mat, const glm::mat4& mat) {
  o_mat->clear_values();
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      o_mat->add_values(mat[i][j]);
    }
  }
  return true;
}

bool asset::read_pb_mat4(glm::mat4& o_mat, const pb::Mat4& mat) {
  if (mat.values_size() != 16) {
    return false;
  }

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      o_mat[i][j] = mat.values(i * 4 + j);
    }
  }

  return true;
}