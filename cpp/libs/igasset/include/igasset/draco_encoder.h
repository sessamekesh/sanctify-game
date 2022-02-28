#ifndef LIB_IGASSET_DRACO_ENCODER_H
#define LIB_IGASSET_DRACO_ENCODER_H

#include <draco/compression/encode.h>
#include <igasset/vertex_formats.h>
#include <igcore/either.h>
#include <igcore/pod_vector.h>
#include <igcore/raw_buffer.h>

/**
 * DracoEncoder - encodes a collection of vertices into the Draco 3D file format
 */

namespace indigo::asset {

enum class DracoEncoderResult {
  Ok = 0,
  BadVertexCount,
  BadTriangleIndexCount,
  NoData,
  EncodeFailed,
};

class DracoEncoder {
 public:
  struct EncodeSettings {
    EncodeSettings();

    uint8_t EncodeSpeed;
    uint8_t DecodeSpeed;
  };

 public:
  DracoEncoder();

  DracoEncoderResult add_pos_norm_data(
      const core::PodVector<PositionNormalVertexData>& data);
  DracoEncoderResult add_texcoord_data(
      const core::PodVector<TexcoordVertexData>& data);
  DracoEncoderResult add_skeletal_animation_data(
      const core::PodVector<SkeletalAnimationVertexData>& data);

  // Eventually, custom attribute encoding is going to be needed here too...
  // Probably can include it with a descriptor and raw buffer - e.g., "this
  //  raw buffer contains vertices with 3 float32s separated by 60 bytes each,
  //  with an initial offset of 12 bytes.

  DracoEncoderResult add_index_data(const core::PodVector<uint32_t>& data);

  core::Either<core::RawBuffer, DracoEncoderResult> encode(
      EncodeSettings settings = EncodeSettings());

 private:
  draco::Mesh mesh_;

  DracoEncoderResult validate_size(size_t size);
};

}  // namespace indigo::asset

#endif
