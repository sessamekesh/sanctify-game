#include <draco/compression/decode.h>
#include <igasset/draco_decoder.h>
#include <igcore/log.h>

#include <map>

using namespace indigo;
using namespace asset;

namespace {
const char* kLogLabel = "DracoDecoder";

constexpr int kNormalQuatAttributeIndex = 1;
}  // namespace

std::string indigo::asset::to_string(DracoDecoderResult rsl) {
  switch (rsl) {
    case DracoDecoderResult::Ok:
      return "Ok";
    case DracoDecoderResult::DecodeFailed:
      return "DecodeFailed";
    case DracoDecoderResult::MeshDataMissing:
      return "MeshDataMissing";
    case DracoDecoderResult::NoData:
      return "NoData";
    case DracoDecoderResult::DrcPointCloudIsNotMesh:
      return "DrcPointCloudIsNotMesh";
    case DracoDecoderResult::TooManyBones:
      return "TooManyBones";
    case DracoDecoderResult::BoneDataNotLoaded:
      return "BoneDataNotLoaded";
    case DracoDecoderResult::BoneDataIncomplete:
      return "BoneDataIncomplete";
  }

  return "<<DracoDecoderResult::Undefined>>";
}

DracoDecoder::DracoDecoder() : num_bones_(-1) {}

DracoDecoderResult DracoDecoder::decode(const core::RawBuffer& buffer) {
  draco::Decoder decoder;
  draco::DecoderBuffer drc_buffer;

  drc_buffer.Init(reinterpret_cast<const char*>(buffer.get()), buffer.size());

  const auto type = draco::Decoder::GetEncodedGeometryType(&drc_buffer);
  if (!type.ok()) {
    return DracoDecoderResult::DecodeFailed;
  }

  if (type.value() != draco::TRIANGULAR_MESH) {
    return DracoDecoderResult::DrcPointCloudIsNotMesh;
  }

  auto mesh_rsl = decoder.DecodeMeshFromBuffer(&drc_buffer);
  if (!mesh_rsl.ok()) {
    return DracoDecoderResult::DecodeFailed;
  }

  mesh_ = std::move(mesh_rsl).value();

  // TODO (sessamekesh): Be a bit more scientific about this...
  const draco::PointAttribute* idx_attrib = nullptr;
  for (int i = 0; i < 8; i++) {
    auto* test_attrib = mesh_->GetNamedAttributeByUniqueId(
        draco::GeometryAttribute::Type::GENERIC, i);
    if (test_attrib != nullptr &&
        test_attrib->data_type() == draco::DataType::DT_UINT32 &&
        test_attrib->num_components() == 4) {
      idx_attrib = test_attrib;
      break;
    }
  }
  if (idx_attrib != nullptr) {
    for (draco::PointIndex vert_idx(0); vert_idx < mesh_->num_points();
         vert_idx++) {
      glm::u32vec4 bone_indices{};
      idx_attrib->ConvertValue(idx_attrib->mapped_index(vert_idx), 4,
                               &bone_indices.x);
      num_bones_ =
          std::max({(int)bone_indices.x, (int)bone_indices.y,
                    (int)bone_indices.z, (int)bone_indices.w, num_bones_});
    }
  }

  if (num_bones_ > 0) {
    num_bones_++;
  }

  return DracoDecoderResult::Ok;
}

DracoDecoderResult DracoDecoder::add_bone_data(std::string bone_name,
                                               glm::mat4 inv_bind_pos) {
  if (bone_data_.size() >= num_bones_) {
    return DracoDecoderResult::TooManyBones;
  }

  bone_data_.push_back({bone_name, inv_bind_pos});
  return DracoDecoderResult::Ok;
}

core::Either<core::PodVector<PositionNormalVertexData>, DracoDecoderResult>
DracoDecoder::get_pos_norm_data() const {
  if (mesh_ == nullptr) {
    return core::right(DracoDecoderResult::MeshDataMissing);
  }

  auto pos_attrib =
      mesh_->GetNamedAttribute(draco::GeometryAttribute::Type::POSITION);
  auto normal_attrib = mesh_->GetNamedAttributeByUniqueId(
      draco::GeometryAttribute::Type::GENERIC, kNormalQuatAttributeIndex);

  if (!pos_attrib || pos_attrib->num_components() != 3 || !normal_attrib ||
      normal_attrib->num_components() != 4) {
    return core::right(DracoDecoderResult::NoData);
  }

  core::PodVector<PositionNormalVertexData> vertices(mesh_->num_points());

  for (draco::PointIndex vert_idx(0); vert_idx < mesh_->num_points();
       vert_idx++) {
    PositionNormalVertexData vert{};

    pos_attrib->ConvertValue(pos_attrib->mapped_index(vert_idx), 3,
                             &vert.Position[0]);
    normal_attrib->ConvertValue(normal_attrib->mapped_index(vert_idx), 4,
                                &vert.NormalQuat[0]);

    vertices.push_back(vert);
  }

  return core::left<core::PodVector<PositionNormalVertexData>>(vertices);
}

core::Either<core::PodVector<TexcoordVertexData>, DracoDecoderResult>
DracoDecoder::get_texcoord_data() const {
  if (mesh_ == nullptr) {
    return core::right(DracoDecoderResult::MeshDataMissing);
  }

  auto texcoord_attrib =
      mesh_->GetNamedAttribute(draco::GeometryAttribute::Type::TEX_COORD);

  if (!texcoord_attrib || texcoord_attrib->num_components() != 2) {
    return core::right(DracoDecoderResult::NoData);
  }

  core::PodVector<TexcoordVertexData> vertices(mesh_->num_points());

  for (draco::PointIndex vert_idx(0); vert_idx < mesh_->num_points();
       vert_idx++) {
    TexcoordVertexData vert{};

    texcoord_attrib->ConvertValue(texcoord_attrib->mapped_index(vert_idx), 2,
                                  &vert.Texcoord[0]);

    vertices.push_back(vert);
  }

  return core::left<core::PodVector<TexcoordVertexData>>(vertices);
}

core::Either<core::PodVector<uint32_t>, DracoDecoderResult>
DracoDecoder::get_index_data() const {
  if (mesh_ == nullptr) {
    return core::right(DracoDecoderResult::MeshDataMissing);
  }

  core::PodVector<uint32_t> result(mesh_->num_faces() * 3);

  for (draco::FaceIndex face_idx(0); face_idx < mesh_->num_faces();
       face_idx++) {
    auto face = mesh_->face(face_idx);

    if (face.size() != 3) {
      core::Logger::log(kLogLabel) << "Skipping non-triangular face";
      continue;
    }

    for (auto idx : face) {
      result.push_back((uint32_t)idx.value());
    }
  }

  return core::left<core::PodVector<uint32_t>>(result);
}

core::Either<core::PodVector<SkeletalAnimationVertexData>, DracoDecoderResult>
DracoDecoder::get_skeletal_animation_vertices(
    const ozz::animation::Skeleton& ozz_skeleton) const {
  //
  // Step 1: verify that skeleton data is loaded and correct
  //
  if (bone_data_.size() == 0) {
    return core::right(DracoDecoderResult::BoneDataNotLoaded);
  }

  if (num_bones_ == 0 && bone_data_.size() == 0) {
    return core::right(DracoDecoderResult::NoData);
  }

  if (num_bones_ != bone_data_.size()) {
    return core::right(DracoDecoderResult::BoneDataIncomplete);
  }

  std::map<std::string, int> name_to_skeleton_index;
  {
    auto joint_names = ozz_skeleton.joint_names();

    for (int i = 0; i < joint_names.size(); i++) {
      const char* joint_name = joint_names[i];
      name_to_skeleton_index.emplace(joint_name, i);
    }
  }
  std::function<int(std::string)> get_joint_index =
      [&name_to_skeleton_index](std::string name) -> int {
    auto idx_it = name_to_skeleton_index.find(name);
    if (idx_it == name_to_skeleton_index.end()) {
      return -1;
    }
    return idx_it->second;
  };

  //
  // Step 2: Extract non-transformed skeleton data from Draco source
  //
  if (mesh_ == nullptr) {
    return core::right(DracoDecoderResult::MeshDataMissing);
  }

  // TODO (sessamekesh): Grab this from the Draco asset instead, because these
  // values are WRONG!
  auto bone_weights_attrib =
      mesh_->GetNamedAttribute(draco::GeometryAttribute::Type::GENERIC, 1);
  auto bone_indices_attrib =
      mesh_->GetNamedAttribute(draco::GeometryAttribute::Type::GENERIC, 2);

  if (bone_weights_attrib->data_type() != draco::DataType::DT_FLOAT32 ||
      bone_indices_attrib->data_type() != draco::DataType::DT_UINT32 ||
      bone_weights_attrib->num_components() != 4 ||
      bone_indices_attrib->num_components() != 4) {
    return core::right(DracoDecoderResult::NoData);
  }

  core::PodVector<SkeletalAnimationVertexData> vertices(mesh_->num_points());

  for (draco::PointIndex vert_idx(0); vert_idx < mesh_->num_points();
       vert_idx++) {
    SkeletalAnimationVertexData vert{};

    bone_weights_attrib->ConvertValue(
        bone_weights_attrib->mapped_index(vert_idx), 4, &vert.BoneWeights[0]);
    bone_indices_attrib->ConvertValue(
        bone_indices_attrib->mapped_index(vert_idx), 4, &vert.BoneIndices[0]);

    vertices.push_back(vert);
  }

  //
  // Step 3: Transform all bone indices to transformed skeleton values
  //
  for (int vert_idx = 0; vert_idx < vertices.size(); vert_idx++) {
    for (int idx_idx = 0; idx_idx < 4; idx_idx++) {
      int in_idx = vertices[vert_idx].BoneIndices[idx_idx];
      std::string in_name = bone_data_[in_idx].boneName;
      int out_idx = get_joint_index(in_name);

      if (out_idx < 0) {
        return core::right(DracoDecoderResult::BoneDataIncomplete);
      }

      vertices[vert_idx].BoneIndices[idx_idx] = out_idx;
    }
  }

  // Finished, return
  return core::left(std::move(vertices));
}

core::Either<core::PodVector<glm::mat4>, DracoDecoderResult>
DracoDecoder::get_inv_bind_poses(
    const ozz::animation::Skeleton& ozz_skeleton) const {
  if (num_bones_ == 0 || bone_data_.size() != num_bones_) {
    return core::right(DracoDecoderResult::BoneDataIncomplete);
  }

  auto joint_names = ozz_skeleton.joint_names();

  core::PodVector<glm::mat4> inv_bind_poses(joint_names.size());
  for (int i = 0; i < joint_names.size(); i++) {
    inv_bind_poses[i] = glm::mat4(1.f);

    for (int j = 0; j < bone_data_.size(); j++) {
      if (bone_data_[j].boneName == joint_names[i]) {
        inv_bind_poses[i] = bone_data_[j].invBindPose;
        break;
      }
    }
  }

  return core::left(std::move(inv_bind_poses));
}
