#ifndef LIBS_IGASSET_DRACO_DECODER_H
#define LIBS_IGASSET_DRACO_DECODER_H

#include <draco/mesh/mesh.h>
#include <igasset/vertex_formats.h>
#include <igcore/either.h>
#include <igcore/pod_vector.h>
#include <igcore/raw_buffer.h>

namespace indigo::asset {
enum class DracoDecoderResult {
  Ok = 0,
  DecodeFailed,
  MeshDataMissing,
  NoData,
  DrcPointCloudIsNotMesh,
};
std::string to_string(DracoDecoderResult rsl);

class DracoDecoder {
 public:
  DracoDecoder();

  DracoDecoderResult decode(const core::RawBuffer& buffer);

  core::Either<core::PodVector<PositionNormalVertexData>, DracoDecoderResult>
  get_pos_norm_data() const;

  core::Either<core::PodVector<TexcoordVertexData>, DracoDecoderResult>
  get_texcoord_data() const;

  core::Either<core::PodVector<uint32_t>, DracoDecoderResult> get_index_data()
      const;

 private:
  std::unique_ptr<draco::Mesh> mesh_;
};
}  // namespace indigo::asset

#endif