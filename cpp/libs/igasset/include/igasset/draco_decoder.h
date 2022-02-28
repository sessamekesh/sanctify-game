#ifndef LIBS_IGASSET_DRACO_DECODER_H
#define LIBS_IGASSET_DRACO_DECODER_H

#include <draco/mesh/mesh.h>
#include <igasset/vertex_formats.h>
#include <igcore/either.h>
#include <igcore/pod_vector.h>
#include <igcore/raw_buffer.h>
#include <igcore/vector.h>
#include <ozz/animation/runtime/skeleton.h>

namespace indigo::asset {
enum class DracoDecoderResult {
  Ok = 0,
  DecodeFailed,
  MeshDataMissing,
  NoData,
  DrcPointCloudIsNotMesh,

  BoneDataNotLoaded,
  TooManyBones,
  BoneDataIncomplete,
};
std::string to_string(DracoDecoderResult rsl);

class DracoDecoder {
 public:
  struct BoneData {
    std::string boneName;
    glm::mat4 invBindPose;
  };

  DracoDecoder();

  DracoDecoderResult decode(const core::RawBuffer& buffer);
  DracoDecoderResult add_bone_data(std::string bone_name,
                                   glm::mat4 inv_bind_pos);

  core::Either<core::PodVector<PositionNormalVertexData>, DracoDecoderResult>
  get_pos_norm_data() const;

  core::Either<core::PodVector<TexcoordVertexData>, DracoDecoderResult>
  get_texcoord_data() const;

  core::Either<indigo::core::PodVector<SkeletalAnimationVertexData>,
               DracoDecoderResult>
  get_skeletal_animation_vertices(
      const ozz::animation::Skeleton& ozz_skeleton) const;

  core::Either<indigo::core::PodVector<glm::mat4>, DracoDecoderResult>
  get_inv_bind_poses(const ozz::animation::Skeleton& ozz_skeleton) const;

  core::Either<core::PodVector<uint32_t>, DracoDecoderResult> get_index_data()
      const;

 private:
  std::unique_ptr<draco::Mesh> mesh_;
  int num_bones_;
  indigo::core::Vector<BoneData> bone_data_;
};
}  // namespace indigo::asset

#endif