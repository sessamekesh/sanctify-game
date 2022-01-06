#include <draco/compression/decode.h>
#include <igasset/draco_decoder.h>
#include <igcore/log.h>

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
  }

  return "<<DracoDecoderResult::Undefined>>";
}

DracoDecoder::DracoDecoder() {}

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

  core::PodVector<PositionNormalVertexData> vertices;

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

  core::PodVector<TexcoordVertexData> vertices;

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