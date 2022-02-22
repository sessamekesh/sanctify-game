#ifndef LIBS_IGNAV_INCLUDE_IGNAV_COMPILER_H
#define LIBS_IGNAV_INCLUDE_IGNAV_COMPILER_H

#include <Recast.h>
#include <igcore/maybe.h>
#include <igcore/pod_vector.h>
#include <ignav/detour_navmesh.h>

#include <glm/glm.hpp>

namespace indigo::nav {

constexpr float kDefaultCellSize = 0.3f;
constexpr float kDefaultCellHeight = 0.2f;
constexpr float kDefaultMaxSlopeDegrees = 45.f;
constexpr float kDefaultWalkableClimb = 0.8f;
constexpr float kDefaultWalkableHeight = 20.f;
constexpr float kDefaultAgentRadius = 0.6f;
constexpr float kDefaultMinRegionArea = 5.76f;
constexpr float kDefaultMergeRegionArea = 36.f;
constexpr float kDefaultMaxContourError = 1.3f;
constexpr float kDefaultMaxEdgeLength = 20.f;
constexpr float kDefaultDetailSampleDistance = 1.8f;
constexpr float kDefaultDetailMaxError = 0.2f;

class RecastCompiler {
 public:
  struct RecastNavmeshRawData {
   public:
    RecastNavmeshRawData();
    RecastNavmeshRawData(rcPolyMesh* poly_mesh, rcPolyMeshDetail* poly_detail,
                         glm::vec3 minBb, glm::vec3 maxBb);
    ~RecastNavmeshRawData();

    RecastNavmeshRawData(const RecastNavmeshRawData&) = delete;
    RecastNavmeshRawData& operator=(const RecastNavmeshRawData&) = delete;

    RecastNavmeshRawData(RecastNavmeshRawData&& o)
        : polyMesh(std::exchange(o.polyMesh, nullptr)),
          polyMeshDetail(std::exchange(o.polyMeshDetail, nullptr)),
          minBb(std::move(o.minBb)),
          maxBb(std::move(o.maxBb)) {}
    RecastNavmeshRawData& operator=(RecastNavmeshRawData&& o) {
      polyMesh = std::exchange(o.polyMesh, nullptr);
      polyMeshDetail = std::exchange(o.polyMeshDetail, nullptr);
      minBb = std::move(o.minBb);
      maxBb = std::move(o.maxBb);
      return *this;
    }

    rcPolyMesh* polyMesh;
    rcPolyMeshDetail* polyMeshDetail;

    glm::vec3 minBb;
    glm::vec3 maxBb;
  };

 public:
  RecastCompiler(float cell_size, float cell_height, float max_slope_degrees,
                 int walkable_climb, int walkable_height, float agent_radius,
                 int min_region_area, int merge_region_area,
                 float max_contour_error, int max_edge_length,
                 float detail_sample_dist, float detail_max_error);
  ~RecastCompiler();

  void add_walkable_triangles(const core::PodVector<glm::vec3>& vertices,
                              const core::PodVector<uint32_t>& indices);

  indigo::core::Maybe<RecastNavmeshRawData> build_recast();
  indigo::core::Maybe<indigo::core::RawBuffer> build_raw(
      const RecastNavmeshRawData& data);
  static indigo::core::Maybe<DetourNavmesh> navmesh_from_raw(
      indigo::core::RawBuffer raw_buffer);

 private:
  float max_slope_degrees_;
  float cs_;
  float ch_;
  int walkable_climb_;
  int walkable_height_;
  float agent_radius_;
  int min_region_area_;
  int merge_region_area_;
  float max_contour_error_;
  int max_edge_length_;
  float detail_sample_dist_;
  float detail_max_error_;

  core::PodVector<glm::vec3> all_vertices_;
  core::PodVector<int> all_indices_;
  core::PodVector<unsigned char> all_areas_;
};

}  // namespace indigo::nav

#endif
