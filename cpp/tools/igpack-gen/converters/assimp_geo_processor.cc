#include <converters/assimp_geo_processor.h>
#include <igasset/draco_encoder.h>
#include <igcore/log.h>

#include <glm/gtx/quaternion.hpp>

namespace {
const char* kLogLabel = "AssimpGeoProcessor";
}

using namespace indigo;
using namespace igpackgen;

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

        data.NormalQuat = glm::vec4(normal_quat.x, normal_quat.y, normal_quat.z,
                                    normal_quat.w);
      } else {
        auto normal_quat = glm::rotation(
            glm::vec3(0.f, 0.f, 1.f),
            glm::vec3(mesh->mNormals[idx].x, mesh->mNormals[idx].y,
                      mesh->mNormals[idx].z));

        data.NormalQuat = glm::vec4(normal_quat.x, normal_quat.y, normal_quat.z,
                                    normal_quat.w);
      }

      pos_norm_data.push_back(data);

      if (has_texcoords) {
        asset::TexcoordVertexData tc_data{};
        tc_data.Texcoord.x = mesh->mTextureCoords[0][i].x;
        tc_data.Texcoord.x = mesh->mTextureCoords[0][i].y;
        texcoord_data.push_back(tc_data);
      }
    }

    for (unsigned int idx = 0; idx < mesh->mNumFaces; idx++) {
      if (mesh->mFaces[idx].mNumIndices != 3) {
        core::Logger::err(kLogLabel)
            << "Encountered non-triangular face in mesh " << mesh_name;
        return false;
      }

      index_data.push_back(mesh->mFaces[idx].mIndices[0] + index_offset);
      index_data.push_back(mesh->mFaces[idx].mIndices[1] + index_offset);
      index_data.push_back(mesh->mFaces[idx].mIndices[2] + index_offset);
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