#include <igasset/draco_encoder.h>
#include <igcore/log.h>

using namespace indigo;
using namespace asset;

namespace {
const char* kLogLabel = "DracoEncoder";

constexpr int kNormalQuatAttributeIndex = 1;
}  // namespace

DracoEncoder::EncodeSettings::EncodeSettings()
    : EncodeSpeed(0), DecodeSpeed(0) {}

DracoEncoder::DracoEncoder() {}

DracoEncoderResult DracoEncoder::validate_size(size_t size) {
  if (mesh_.num_points() == 0u) {
    mesh_.set_num_points(size);
  } else if (mesh_.num_points() != size) {
    core::Logger::err(kLogLabel)
        << "Mesh has " << mesh_.num_points() << " points - input data has "
        << size << " points. Vertex count must match!";
    return DracoEncoderResult::BadVertexCount;
  }

  if (size == 0u) {
    core::Logger::err(kLogLabel)
        << "Input pos_norm data has 0 points, cannot use";
    return DracoEncoderResult::NoData;
  }

  return DracoEncoderResult::Ok;
}

DracoEncoderResult DracoEncoder::add_pos_norm_data(
    const core::PodVector<PositionNormalVertexData>& data) {
  auto size_rsl = validate_size(data.size());
  if (size_rsl != DracoEncoderResult::Ok) {
    return size_rsl;
  }

  draco::DataBuffer raw_vert_data;
  raw_vert_data.Update(&data[0], data.raw_size());

  draco::GeometryAttribute pos_attribute;
  pos_attribute.Init(draco::GeometryAttribute::POSITION, &raw_vert_data, 3,
                     draco::DT_FLOAT32, false, sizeof(PositionNormalVertexData),
                     offsetof(PositionNormalVertexData, Position));
  auto pos_attribute_idx = mesh_.AddAttribute(pos_attribute, true, data.size());

  draco::GeometryAttribute normal_quat_attribute;
  normal_quat_attribute.Init(draco::GeometryAttribute::GENERIC, &raw_vert_data,
                             4, draco::DT_FLOAT32, false,
                             sizeof(PositionNormalVertexData),
                             offsetof(PositionNormalVertexData, NormalQuat));
  normal_quat_attribute.set_unique_id(kNormalQuatAttributeIndex);
  auto norm_quat_attribute_idx =
      mesh_.AddAttribute(normal_quat_attribute, true, data.size());

  for (int i = 0; i < data.size(); i++) {
    mesh_.attribute(pos_attribute_idx)
        ->SetAttributeValue(draco::AttributeValueIndex(i),
                            &data[i].Position[0]);
    mesh_.attribute(norm_quat_attribute_idx)
        ->SetAttributeValue(draco::AttributeValueIndex(i), &data[i].NormalQuat);
  }

  return DracoEncoderResult::Ok;
}

DracoEncoderResult DracoEncoder::add_texcoord_data(
    const core::PodVector<TexcoordVertexData>& data) {
  auto size_rsl = validate_size(data.size());
  if (size_rsl != DracoEncoderResult::Ok) {
    return size_rsl;
  }

  draco::DataBuffer raw_vert_data;
  raw_vert_data.Update(&data[0], data.raw_size());

  draco::GeometryAttribute uv_attribute;
  uv_attribute.Init(draco::GeometryAttribute::TEX_COORD, &raw_vert_data, 2,
                    draco::DT_FLOAT32, false, sizeof(TexcoordVertexData),
                    offsetof(TexcoordVertexData, Texcoord));
  auto uv_attribute_id = mesh_.AddAttribute(uv_attribute, true, data.size());

  for (int i = 0; i < data.size(); i++) {
    mesh_.attribute(uv_attribute_id)
        ->SetAttributeValue(draco::AttributeValueIndex(i),
                            &data[i].Texcoord[0]);
  }

  return DracoEncoderResult::Ok;
}

DracoEncoderResult DracoEncoder::add_skeletal_animation_data(
    const core::PodVector<SkeletalAnimationVertexData>& data) {
  auto size_rsl = validate_size(data.size());
  if (size_rsl != DracoEncoderResult::Ok) {
    return size_rsl;
  }

  draco::DataBuffer raw_vert_data;
  raw_vert_data.Update(&data[0], data.raw_size());

  draco::GeometryAttribute bone_weights_attrib;
  bone_weights_attrib.Init(draco::GeometryAttribute::GENERIC, &raw_vert_data, 4,
                           draco::DT_FLOAT32, false,
                           sizeof(SkeletalAnimationVertexData),
                           offsetof(SkeletalAnimationVertexData, BoneWeights));
  auto bone_weight_attrib_id =
      mesh_.AddAttribute(bone_weights_attrib, true, data.size());

  draco::GeometryAttribute bone_indices_attrib;
  bone_indices_attrib.Init(draco::GeometryAttribute::GENERIC, &raw_vert_data, 4,
                           draco::DT_UINT8, false,
                           sizeof(SkeletalAnimationVertexData),
                           offsetof(SkeletalAnimationVertexData, BoneIndices));
  auto bone_indices_attrib_id =
      mesh_.AddAttribute(bone_indices_attrib, true, data.size());

  for (int i = 0; i < data.size(); i++) {
    mesh_.attribute(bone_weight_attrib_id)
        ->SetAttributeValue(draco::AttributeValueIndex(i),
                            &data[0].BoneWeights);
    mesh_.attribute(bone_indices_attrib_id)
        ->SetAttributeValue(draco::AttributeValueIndex(i),
                            &data[0].BoneIndices);
  }

  return DracoEncoderResult::Ok;
}

DracoEncoderResult DracoEncoder::add_index_data(
    const core::PodVector<uint32_t>& data) {
  if (data.size() % 3 != 0) {
    core::Logger::err(kLogLabel) << "Invalid number of triangle indices "
                                 << data.size() << " - must be divisible by 3";
    return DracoEncoderResult::BadTriangleIndexCount;
  }

  if (data.size() == 0) {
    core::Logger::err(kLogLabel)
        << "Cannot set triangles with empty index array";
    return DracoEncoderResult::NoData;
  }

  mesh_.SetNumFaces(data.size() / 3);

  for (int i = 0; i < data.size() / 3; i++) {
    draco::Mesh::Face face;
    face[0] = data[i * 3];
    face[1] = data[i * 3 + 1];
    face[2] = data[i * 3 + 2];
    mesh_.SetFace(draco::FaceIndex(i), face);
  }

  return DracoEncoderResult::Ok;
}

core::Either<core::RawBuffer, DracoEncoderResult> DracoEncoder::encode(
    EncodeSettings settings) {
#ifdef DRACO_ATTRIBUTE_VALUES_DEDUPLICATION_SUPPORTED
  mesh_.DeduplicateAttributeValues();
#endif
#ifdef DRACO_ATTRIBUTE_INDICES_DEDUPLICATION_SUPPORTED
  mesh_.DeduplicatePointIds();
#endif

  draco::Encoder encoder;

  encoder.SetSpeedOptions(settings.EncodeSpeed, settings.DecodeSpeed);

  // TODO (sessamekesh): Explore other encoder settings

  draco::EncoderBuffer out_buffer;
  auto status = encoder.EncodeMeshToBuffer(mesh_, &out_buffer);
  if (!status.ok()) {
    core::Logger::err(kLogLabel)
        << "Failure in Draco encoder: " << status.error_msg();
    return core::right<DracoEncoderResult>(DracoEncoderResult::EncodeFailed);
  }

  core::RawBuffer buffer(out_buffer.size());
  memcpy((void*)buffer.get(), (void*)out_buffer.data(), out_buffer.size());
  return core::left<core::RawBuffer>(std::move(buffer));
}