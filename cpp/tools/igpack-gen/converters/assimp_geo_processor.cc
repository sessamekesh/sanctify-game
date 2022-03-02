#include <converters/assimp_geo_processor.h>
#include <igasset/draco_encoder.h>
#include <igasset/proto_converters.h>
#include <igcore/log.h>
#include <igcore/vector.h>

#include <glm/gtx/quaternion.hpp>

using namespace indigo;
using namespace igpackgen;

namespace {
const char* kLogLabel = "AssimpGeoProcessor";

struct BaseGeoData {
  core::PodVector<asset::PositionNormalVertexData> posNormData;
  core::PodVector<asset::TexcoordVertexData> texcoordData;
  core::PodVector<uint32_t> indexData;
};

core::Maybe<BaseGeoData> extract_base_geo(const aiMesh* mesh) {
  core::PodVector<asset::PositionNormalVertexData> pos_norm_data(
      mesh->mNumVertices);
  core::PodVector<asset::TexcoordVertexData> texcoord_data(mesh->mNumVertices);
  core::PodVector<uint32_t> index_data(mesh->mNumFaces * 3);

  for (unsigned int idx = 0; idx < mesh->mNumVertices; idx++) {
    asset::PositionNormalVertexData data{};
    data.Position.x = mesh->mVertices[idx].x;
    data.Position.y = mesh->mVertices[idx].y;
    data.Position.z = mesh->mVertices[idx].z;

    // Logic pulled from the Filament rendering engine - packTangentFrame
    // method of mat3.h
    if (mesh->HasTangentsAndBitangents()) {
      glm::mat3 tbn = glm::mat3(
          glm::vec3(mesh->mTangents[idx].x, mesh->mTangents[idx].y,
                    mesh->mTangents[idx].z),
          glm::vec3(mesh->mBitangents[idx].x, mesh->mBitangents[idx].y,
                    mesh->mBitangents[idx].z),
          glm::vec3(mesh->mNormals[idx].x, mesh->mNormals[idx].y,
                    mesh->mNormals[idx].z));

      auto normal_quat = glm::quat_cast(tbn);
      // Consistency among all quats - there are two quats that can represent
      //  any angle, pick the one with the positive rotation.
      if (normal_quat.w < 0.f) {
        normal_quat = -normal_quat;
      }

      data.NormalQuat =
          glm::vec4(normal_quat.x, normal_quat.y, normal_quat.z, normal_quat.w);
    } else {
      auto normal_quat =
          glm::rotation(glm::vec3(0.f, 0.f, 1.f),
                        glm::vec3(mesh->mNormals[idx].x, mesh->mNormals[idx].y,
                                  mesh->mNormals[idx].z));

      data.NormalQuat =
          glm::vec4(normal_quat.x, normal_quat.y, normal_quat.z, normal_quat.w);
    }

    pos_norm_data.push_back(data);

    if (mesh->HasTextureCoords(0)) {
      asset::TexcoordVertexData tc_data{};
      tc_data.Texcoord.x = mesh->mTextureCoords[0][idx].x;
      tc_data.Texcoord.x = mesh->mTextureCoords[0][idx].y;
      texcoord_data.push_back(tc_data);
    }
  }

  for (unsigned int idx = 0; idx < mesh->mNumFaces; idx++) {
    if (mesh->mFaces[idx].mNumIndices != 3) {
      core::Logger::err(kLogLabel)
          << "Encountered non-triangular face in mesh " << mesh->mName.C_Str();
      return core::empty_maybe{};
    }

    index_data.push_back(mesh->mFaces[idx].mIndices[0]);
    index_data.push_back(mesh->mFaces[idx].mIndices[1]);
    index_data.push_back(mesh->mFaces[idx].mIndices[2]);
  }

  return ::BaseGeoData{std::move(pos_norm_data), std::move(texcoord_data),
                       std::move(index_data)};
}

}  // namespace

bool AssimpGeoProcessor::export_static_draco_geo(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssimpToStaticDracoGeoAction& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  std::string file_name = action.input_file_path();
  asset::pb::SingleAsset* new_asset = output_asset_pack.add_assets();
  asset::pb::DracoGeometryDef* draco_pbr_geo_asset =
      new_asset->mutable_draco_geo();

  new_asset->set_name(action.mesh_igasset_name());

  bool has_texcoords = false;
  uint32_t num_vertices = 0u;
  uint32_t num_faces = 0u;

  uint32_t index_offset = 0u;
  for (int i = 0; i < action.assimp_mesh_names_size(); i++) {
    std::string mesh_name = action.assimp_mesh_names(i);

    core::Maybe<aiMesh*> maybe_mesh =
        assimp_scene_cache.load_mesh(file_cache, file_name, mesh_name);
    if (maybe_mesh.is_empty()) {
      core::Logger::err(kLogLabel)
          << "Assimp mesh not found for bundle " << action.mesh_igasset_name();
      return false;
    }

    aiMesh* mesh = maybe_mesh.get();
    if (i == 0) {
      has_texcoords = mesh->HasTextureCoords(0);
    } else {
      if (has_texcoords != mesh->HasTextureCoords(0)) {
        core::Logger::err(kLogLabel)
            << "Texcoord presence mismatch - aborting mesh "
            << action.mesh_igasset_name();
        return false;
      }
    }
    num_vertices += mesh->mNumVertices;
    num_faces += mesh->mNumFaces;
  }

  core::PodVector<asset::PositionNormalVertexData> pos_norm_data(num_vertices);
  core::PodVector<asset::TexcoordVertexData> texcoord_data(
      has_texcoords ? num_vertices : 1u);
  core::PodVector<uint32_t> index_data(num_faces * 3u);
  for (int i = 0; i < action.assimp_mesh_names_size(); i++) {
    std::string mesh_name = action.assimp_mesh_names(i);

    core::Maybe<aiMesh*> maybe_mesh =
        assimp_scene_cache.load_mesh(file_cache, file_name, mesh_name);
    if (maybe_mesh.is_empty()) {
      // This check should not be necessary, since it's done above
      return false;
    }

    aiMesh* mesh = maybe_mesh.get();

    auto geo_data = ::extract_base_geo(mesh);
    if (geo_data.is_empty()) {
      return false;
    }

    for (int i = 0; i < geo_data.get().posNormData.size(); i++) {
      pos_norm_data.push_back(geo_data.get().posNormData[i]);
    }
    for (int i = 0; i < geo_data.get().texcoordData.size(); i++) {
      texcoord_data.push_back(geo_data.get().texcoordData[i]);
    }
    for (int i = 0; i < geo_data.get().indexData.size(); i++) {
      index_data.push_back(geo_data.get().indexData[i] + index_offset);
    }

    index_offset += mesh->mNumFaces * 3;
  }

  asset::DracoEncoder encoder;
  encoder.add_pos_norm_data(std::move(pos_norm_data));
  if (has_texcoords) {
    encoder.add_texcoord_data(std::move(texcoord_data));
  }
  encoder.add_index_data(std::move(index_data));

  asset::DracoEncoder::EncodeSettings settings{};
  if (action.has_draco_params()) {
    settings.EncodeSpeed = action.draco_params().compression_speed();
    settings.DecodeSpeed = action.draco_params().decompression_speed();
  } else {
    settings.EncodeSpeed = 0;
    settings.DecodeSpeed = 0;
  }

  auto encode_rsl = encoder.encode(settings);
  if (encode_rsl.is_right()) {
    core::Logger::err(kLogLabel) << "DracoEncoder failed to encode data";
    return false;
  }

  auto& raw = encode_rsl.get_left();

  std::string* raw_data = draco_pbr_geo_asset->mutable_data();
  raw_data->resize(raw.size());
  memcpy(&(*raw_data)[0], raw.get(), raw.size());

  return true;
}

// Taken from an old proof of concept version of this game which has almost
//  certainly been lost to the cruel and unforgiving last breath of much of
//  my code: a long-abandoned side project in a private Github repo
// Wow I shouldn't be writing stuff at 1:30 AM
// https://github.com/sessamekesh/sanctify-min/blob/main/tools/modelencoder/modelencoder.cc
bool AssimpGeoProcessor::export_skinned_draco_geo(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssimpExtractSkinnedMeshToDraco& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  std::string file_name = action.input_file_path();
  std::string mesh_name = action.assimp_mesh_name();

  core::Maybe<aiMesh*> maybe_mesh =
      assimp_scene_cache.load_mesh(file_cache, file_name, mesh_name);

  if (maybe_mesh.is_empty()) {
    core::Logger::err(kLogLabel) << "Failed to extract skinned geo at file "
                                 << file_name << ", mesh " << mesh_name;
    return false;
  }

  const aiMesh* mesh = maybe_mesh.get();

  if (!mesh->HasBones()) {
    core::Logger::err(kLogLabel)
        << "Mesh " << mesh_name << " does not have bones!";
  }

  auto maybe_base_geo = ::extract_base_geo(mesh);
  if (maybe_base_geo.is_empty()) {
    core::Logger::err(kLogLabel) << "Failed to extract base geometry from file "
                                 << file_name << ", mesh " << mesh_name;
    return false;
  }
  auto base_geo = maybe_base_geo.move();

  core::PodVector<asset::SkeletalAnimationVertexData> skinning_vertices(
      mesh->mNumVertices);
  for (int i = 0; i < mesh->mNumVertices; i++) {
    skinning_vertices.push_back(asset::SkeletalAnimationVertexData{
        glm::vec4(0.f), glm::u32vec4(0xFFFFu)});
  }

  core::Vector<std::string> bone_names(mesh->mNumBones);
  core::PodVector<glm::mat4> inv_bind_poses(mesh->mNumBones);

  for (int bone_idx = 0; bone_idx < mesh->mNumBones; bone_idx++) {
    const aiBone* bone = mesh->mBones[bone_idx];
    std::string bone_name = bone->mName.C_Str();
    bone_names.push_back(bone_name);

    // Associate this bone with vertices...
    for (int weight_idx = 0; weight_idx < bone->mNumWeights; weight_idx++) {
      const aiVertexWeight& weight = bone->mWeights[weight_idx];

      if (weight.mVertexId >= skinning_vertices.size()) {
        core::Logger::err(kLogLabel)
            << "Error compiling bone associations: weight for vertex "
            << weight.mVertexId << " on bone " << bone_name
            << "is above max vertex " << skinning_vertices.size() - 1;
        return false;
      }

      asset::SkeletalAnimationVertexData& v =
          skinning_vertices[weight.mVertexId];
      if (v.BoneIndices[0] == 0xFFFFu) {
        v.BoneIndices[0] = bone_idx;
        v.BoneWeights[0] = weight.mWeight;
      } else if (v.BoneIndices[1] == 0xFFFFu) {
        v.BoneIndices[1] = bone_idx;
        v.BoneWeights[1] = weight.mWeight;
      } else if (v.BoneIndices[2] == 0xFFFFu) {
        v.BoneIndices[2] = bone_idx;
        v.BoneWeights[2] = weight.mWeight;
      } else if (v.BoneIndices[3] == 0xFFFFu) {
        v.BoneIndices[3] = bone_idx;
        v.BoneWeights[3] = weight.mWeight;
      } else {
        core::Logger::err(kLogLabel)
            << "Error compiling bone associations: vertex " << weight.mVertexId
            << " has too many bone associations";
        return false;
      }
    }

    // Export bone data...
    const auto& inv = bone->mOffsetMatrix;
    glm::mat4 o_inv{};
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        o_inv[r][c] = inv[c][r];
      }
    }
    inv_bind_poses.push_back(o_inv);
  }

  // Remove extras...
  for (int i = 0; i < skinning_vertices.size(); i++) {
    for (int j = 0; j < 4; j++) {
      if (skinning_vertices[i].BoneIndices[j] == 0xFFFFu) {
        skinning_vertices[i].BoneIndices[j] = 0u;
        skinning_vertices[i].BoneWeights[j] = 0.f;
      }
    }
  }

  asset::DracoEncoder encoder;
  if (encoder.add_pos_norm_data(base_geo.posNormData) !=
      asset::DracoEncoderResult::Ok) {
    return false;
  }
  if (base_geo.texcoordData.size() > 0) {
    if (encoder.add_texcoord_data(base_geo.texcoordData) !=
        asset::DracoEncoderResult::Ok) {
      return false;
    }
  }
  if (encoder.add_index_data(base_geo.indexData) !=
      asset::DracoEncoderResult::Ok) {
    return false;
  }

  if (encoder.add_skeletal_animation_data(skinning_vertices) !=
      asset::DracoEncoderResult::Ok) {
    return false;
  }

  auto encode_rsl = encoder.encode();
  if (encode_rsl.is_right()) {
    return false;
  }

  core::RawBuffer raw = encode_rsl.left_move();

  asset::DracoEncoder::EncodeSettings settings{};
  if (action.has_draco_params()) {
    settings.EncodeSpeed = action.draco_params().compression_speed();
    settings.DecodeSpeed = action.draco_params().decompression_speed();
  } else {
    settings.EncodeSpeed = 0;
    settings.DecodeSpeed = 0;
  }

  asset::pb::SingleAsset* new_asset = output_asset_pack.add_assets();
  asset::pb::DracoGeometryDef* draco_pbr_geo_asset =
      new_asset->mutable_draco_geo();
  new_asset->set_name(action.mesh_igasset_name());

  std::string* raw_data = draco_pbr_geo_asset->mutable_data();
  raw_data->resize(raw.size());
  memcpy(&(*raw_data)[0], raw.get(), raw.size());

  draco_pbr_geo_asset->set_has_bone_data(true);
  draco_pbr_geo_asset->set_has_pos_norm(true);
  draco_pbr_geo_asset->set_has_texcoords(base_geo.texcoordData.size() > 0);

  for (int i = 0; i < bone_names.size(); i++) {
    draco_pbr_geo_asset->add_ozz_bone_names(bone_names[i]);
    auto* pb_inv_bind = draco_pbr_geo_asset->add_inv_bind_pose();
    if (!asset::write_pb_mat4(pb_inv_bind, inv_bind_poses[i])) {
      return false;
    }
  }

  return true;
}
