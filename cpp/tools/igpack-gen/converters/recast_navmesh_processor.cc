#include <converters/recast_navmesh_processor.h>
#include <igcore/log.h>
#include <igcore/math.h>

using namespace indigo;
using namespace core;
using namespace igpackgen;

namespace {
const char* kLogLabel = "RecastNavmeshProcessor";

}

bool RecastNavmeshProcessor::export_recast_navmesh(
    asset::pb::AssetPack& output_asset_pack,
    const pb::AssembleRecastNavMeshAction& action, FileCache& file_cache,
    AssimpSceneCache& assimp_scene_cache) {
  //
  // Step 1: wrangle parameters
  //
  float cell_size = action.cell_size();
  if (cell_size <= 0.f) {
    cell_size = nav::kDefaultCellSize;
  }

  float cell_height = action.cell_height();
  if (cell_height <= 0.f) {
    cell_height = nav::kDefaultCellHeight;
  }

  float max_slope_degrees = action.max_slope_degrees();
  if (max_slope_degrees <= 0.f) {
    max_slope_degrees = nav::kDefaultMaxSlopeDegrees;
  }

  float walkable_climb = action.walkable_climb();
  if (walkable_climb <= 0.f) {
    walkable_climb = nav::kDefaultWalkableClimb;
  }

  float walkable_height = action.walkable_height();
  if (walkable_height <= 0.f) {
    walkable_height = nav::kDefaultWalkableHeight;
  }

  float agent_radius = action.agent_radius();
  if (agent_radius <= 0.f) {
    agent_radius = nav::kDefaultAgentRadius;
  }

  float min_region_area = action.min_region_area();
  if (min_region_area <= 0.f) {
    min_region_area = nav::kDefaultMinRegionArea;
  }

  float merge_region_area = action.merge_region_area();
  if (merge_region_area <= 0.f) {
    merge_region_area = nav::kDefaultMergeRegionArea;
  }

  float max_contour_error = action.max_contour_error();
  if (max_contour_error <= 0.f) {
    max_contour_error = nav::kDefaultMaxContourError;
  }

  float max_edge_length = action.max_edge_length();
  if (max_edge_length <= 0.f) {
    max_edge_length = nav::kDefaultMaxEdgeLength;
  }

  float detail_sample_distance = action.detail_sample_distance();
  if (detail_sample_distance <= 0.f) {
    detail_sample_distance = nav::kDefaultDetailSampleDistance;
  }

  float detail_max_error = action.detail_max_error();
  if (detail_max_error <= 0.f) {
    detail_max_error = nav::kDefaultDetailMaxError;
  }

  //
  // Step 2: Set up the Recast compiler with all the input parameters and any
  //  geometry used to build the navmesh (source from Assimp)
  //
  nav::RecastCompiler compiler(
      cell_size, cell_height, max_slope_degrees,
      (int)floorf(walkable_climb / cell_height),
      (int)ceilf(walkable_height / cell_height), agent_radius / cell_size,
      (int)floorf(min_region_area / (cell_size * cell_size)),
      (int)ceilf(merge_region_area / (cell_size * cell_size)),
      max_contour_error, (int)ceilf(max_edge_length / cell_size),
      detail_sample_distance, detail_max_error);

  for (int op_idx = 0; op_idx < action.recast_build_ops_size(); op_idx++) {
    const auto& build_op = action.recast_build_ops(op_idx);

    if (build_op.has_include_assimp_geo()) {
      std::string file_name = build_op.include_assimp_geo().assimp_file_name();
      std::string mesh_name = build_op.include_assimp_geo().assimp_mesh_name();
      Maybe<aiMesh*> maybe_mesh =
          assimp_scene_cache.load_mesh(file_cache, file_name, mesh_name);
      if (maybe_mesh.is_empty()) {
        Logger::err(kLogLabel)
            << "Assimp mesh not found for Recast geo - " << mesh_name
            << " in Assimp resource file " << file_name;
        return false;
      }

      aiMesh* mesh = maybe_mesh.get();

      PodVector<glm::vec3> positions(mesh->mNumVertices);
      PodVector<uint32_t> indices(mesh->mNumFaces * 3);

      const auto& def = build_op.include_assimp_geo();
      glm::vec3 pos(def.position().x(), def.position().y(), def.position().z());
      glm::vec3 scl(def.scale().x(), def.scale().y(), def.scale().z());
      if (!def.has_scale()) {
        scl = glm::vec3(1.f, 1.f, 1.f);
      }
      glm::vec3 rot_axis(def.rotation().x(), def.rotation().y(),
                         def.rotation().z());
      float rot_angle = def.rotation().angle();
      if (!def.has_rotation()) {
        rot_axis = glm::vec3(0.f, 1.f, 0.f);
        rot_angle = 0.f;
      }

      glm::mat4 T = igmath::transform_matrix(pos, rot_axis, rot_angle, scl);

      for (int i = 0; i < mesh->mNumVertices; i++) {
        glm::vec4 raw_pos = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                             mesh->mVertices[i].z, 1.f};
        glm::vec3 pos = T * raw_pos;

        positions.push_back({pos.x, pos.y, pos.z});
      }

      for (int i = 0; i < mesh->mNumFaces; i++) {
        if (mesh->mFaces[i].mNumIndices != 3) {
          Logger::err(kLogLabel)
              << "Encountered face with " << mesh->mFaces[i].mNumIndices
              << " indices (expected 3) - face index " << i << " on mesh "
              << mesh->mName.C_Str();
          continue;
        }
        indices.push_back(mesh->mFaces[i].mIndices[0]);
        indices.push_back(mesh->mFaces[i].mIndices[1]);
        indices.push_back(mesh->mFaces[i].mIndices[2]);
      }

      compiler.add_walkable_triangles(positions, indices);
    }
  }

  //
  // Step 3: Run the Recast build step to produce a raw navmesh object, and
  //  validate it to make sure that building can succeed using this source data.
  //
  auto recast_raw = compiler.build_recast();
  if (recast_raw.is_empty()) {
    Logger::err(kLogLabel) << "Failed to build Recast polygon mesh and detail "
                              "mesh - cannot construct Detour navmesh";
    return false;
  }

  auto raw_detour_bytes_opt = compiler.build_raw(recast_raw.get());
  if (raw_detour_bytes_opt.is_empty()) {
    Logger::err(kLogLabel)
        << "Failed to construct Detour bytes from Recast polygon mesh and "
           "detail mesh - serialization failed";
    return false;
  }
  RawBuffer raw_detour_bytes = raw_detour_bytes_opt.move();

  auto detour_navmesh =
      nav::RecastCompiler::navmesh_from_raw(raw_detour_bytes.clone());
  if (detour_navmesh.is_empty()) {
    Logger::err(kLogLabel) << "Failed to construct Detour navmesh from raw "
                              "bytes - validation failed, not writing data";
    return false;
  }

  //
  // Step 4: Write the validated raw navmesh bytes to the Igasset object
  //
  asset::pb::SingleAsset* new_asset = output_asset_pack.add_assets();
  asset::pb::DetourNavmeshDef* detour_asset =
      new_asset->mutable_detour_navmesh_def();
  new_asset->set_name(action.igasset_name());

  std::string* out_raw_data = detour_asset->mutable_raw_detour_data();
  out_raw_data->resize(raw_detour_bytes.size());
  memcpy(&(*out_raw_data)[0], raw_detour_bytes.get(), raw_detour_bytes.size());

  return true;
}