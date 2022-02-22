#include <DetourNavMeshBuilder.h>
#include <igcore/log.h>
#include <ignav/recast_compiler.h>

using namespace indigo;
using namespace nav;
using namespace core;

namespace {
const char* kLogLabel = "RecastCompiler";

//
// All these raiiRecast* objects are nice raii wrappers around Recast stuff,
//  but unfortunately the way they're used with the Maybe type means that
//  there's wonky broken shit and they SHOULD NOT BE USED OUTSIDE OF THIS FILE.
// Also this file is brittle.
// Basically the Maybe doesn't clear memory, so the rvalue returns of these are
//  basically causing the new value to be cleared immediately when it's
//  returned.
//
class raiiRecastHeightfield {
 public:
  static Maybe<raiiRecastHeightfield> Create() {
    rcHeightfield* hf = rcAllocHeightfield();
    if (hf == nullptr) {
      Logger::err(kLogLabel) << "Failed to allocate rcHeightfield";
      return empty_maybe{};
    }

    return raiiRecastHeightfield(hf);
  }

  raiiRecastHeightfield() : heightfield_(nullptr) {}

  raiiRecastHeightfield(const raiiRecastHeightfield&) = delete;
  raiiRecastHeightfield& operator=(const raiiRecastHeightfield&) = delete;

  raiiRecastHeightfield(raiiRecastHeightfield&& o) noexcept {
    heightfield_ = o.heightfield_;
    o.heightfield_ = nullptr;
  }
  raiiRecastHeightfield& operator=(raiiRecastHeightfield&& o) noexcept {
    heightfield_ = o.heightfield_;
    o.heightfield_ = nullptr;
    return *this;
  }

  ~raiiRecastHeightfield() {
    if (heightfield_ != nullptr) {
      rcFreeHeightField(heightfield_);
      heightfield_ = nullptr;
    }
  }

  operator rcHeightfield&() { return *heightfield_; }

 private:
  raiiRecastHeightfield(rcHeightfield* heightfield)
      : heightfield_(heightfield) {}
  rcHeightfield* heightfield_;
};

class raiiRecastCompactHeightfield {
 public:
  static Maybe<raiiRecastCompactHeightfield> Create() {
    rcCompactHeightfield* cf = rcAllocCompactHeightfield();
    if (cf == nullptr) {
      Logger::err(kLogLabel) << "Failed to allocate rcCompactHeightfield";
      return empty_maybe{};
    }

    return raiiRecastCompactHeightfield(cf);
  }

  raiiRecastCompactHeightfield() : compact_heightfield_(nullptr) {}

  raiiRecastCompactHeightfield(const raiiRecastHeightfield&) = delete;
  raiiRecastCompactHeightfield& operator=(const raiiRecastHeightfield&) =
      delete;

  raiiRecastCompactHeightfield(raiiRecastCompactHeightfield&& o) noexcept {
    compact_heightfield_ = o.compact_heightfield_;
    o.compact_heightfield_ = nullptr;
  }
  raiiRecastCompactHeightfield& operator=(
      raiiRecastCompactHeightfield&& o) noexcept {
    compact_heightfield_ = o.compact_heightfield_;
    o.compact_heightfield_ = nullptr;
    return *this;
  }

  ~raiiRecastCompactHeightfield() {
    if (compact_heightfield_ != nullptr) {
      rcFreeCompactHeightfield(compact_heightfield_);
      compact_heightfield_ = nullptr;
    }
  }

  operator rcCompactHeightfield&() { return *compact_heightfield_; }

 private:
  raiiRecastCompactHeightfield(rcCompactHeightfield* compact_heightfield)
      : compact_heightfield_(compact_heightfield) {}
  rcCompactHeightfield* compact_heightfield_;
};

class raiiRecastContourSet {
 public:
  static Maybe<raiiRecastContourSet> Create() {
    rcContourSet* cs = rcAllocContourSet();
    if (cs == nullptr) {
      Logger::err(kLogLabel) << "Failed to allocate rcContourSet";
      return empty_maybe{};
    }

    return raiiRecastContourSet(cs);
  }

  raiiRecastContourSet() : cs_(nullptr) {}

  raiiRecastContourSet(const raiiRecastContourSet&) = delete;
  raiiRecastContourSet& operator=(const raiiRecastContourSet&) = delete;

  raiiRecastContourSet(raiiRecastContourSet&& o) noexcept {
    cs_ = o.cs_;
    o.cs_ = nullptr;
  }
  raiiRecastContourSet& operator=(raiiRecastContourSet&& o) noexcept {
    cs_ = o.cs_;
    o.cs_ = nullptr;
    return *this;
  }

  ~raiiRecastContourSet() {
    if (cs_ != nullptr) {
      rcFreeContourSet(cs_);
      cs_ = nullptr;
    }
  }

  operator rcContourSet&() { return *cs_; }

 private:
  raiiRecastContourSet(rcContourSet* cs) : cs_(cs) {}
  rcContourSet* cs_;
};

class raiiRecastPolyMesh {
 public:
  static Maybe<raiiRecastPolyMesh> Create() {
    rcPolyMesh* pm = rcAllocPolyMesh();
    if (pm == nullptr) {
      Logger::err(kLogLabel) << "Failed to allocate rcPolyMesh";
      return empty_maybe{};
    }

    return raiiRecastPolyMesh(pm);
  }

  raiiRecastPolyMesh() : pm_(nullptr) {}

  raiiRecastPolyMesh(const raiiRecastPolyMesh&) = delete;
  raiiRecastPolyMesh& operator=(const raiiRecastPolyMesh&) = delete;

  raiiRecastPolyMesh(raiiRecastPolyMesh&& o) noexcept {
    pm_ = o.pm_;
    o.pm_ = nullptr;
  }
  raiiRecastPolyMesh& operator=(raiiRecastPolyMesh&& o) noexcept {
    pm_ = o.pm_;
    o.pm_ = nullptr;
    return *this;
  }

  ~raiiRecastPolyMesh() {
    if (pm_ != nullptr) {
      rcFreePolyMesh(pm_);
      pm_ = nullptr;
    }
  }

  operator rcPolyMesh&() { return *pm_; }

  rcPolyMesh* operator->() { return pm_; }

  rcPolyMesh* move() {
    auto tr = pm_;
    pm_ = nullptr;
    return tr;
  }

 private:
  raiiRecastPolyMesh(rcPolyMesh* pm) : pm_(pm) {}
  rcPolyMesh* pm_;
};

class raiiRecastPolyMeshDetail {
 public:
  static Maybe<raiiRecastPolyMeshDetail> Create() {
    rcPolyMeshDetail* pm = rcAllocPolyMeshDetail();
    if (pm == nullptr) {
      Logger::err(kLogLabel) << "Failed to allocate rcPolyMesh";
      return empty_maybe{};
    }

    return raiiRecastPolyMeshDetail(pm);
  }

  raiiRecastPolyMeshDetail() : pm_(nullptr) {}

  raiiRecastPolyMeshDetail(const raiiRecastPolyMeshDetail&) = delete;
  raiiRecastPolyMeshDetail& operator=(const raiiRecastPolyMeshDetail&) = delete;

  raiiRecastPolyMeshDetail(raiiRecastPolyMeshDetail&& o) noexcept {
    pm_ = o.pm_;
    o.pm_ = nullptr;
  }
  raiiRecastPolyMeshDetail& operator=(raiiRecastPolyMeshDetail&& o) noexcept {
    pm_ = o.pm_;
    o.pm_ = nullptr;
    return *this;
  }

  ~raiiRecastPolyMeshDetail() {
    if (pm_ != nullptr) {
      rcFreePolyMeshDetail(pm_);
      pm_ = nullptr;
    }
  }

  operator rcPolyMeshDetail&() { return *pm_; }

  rcPolyMeshDetail* operator->() { return pm_; }

  rcPolyMeshDetail* move() {
    auto tr = pm_;
    pm_ = nullptr;
    return tr;
  }

 private:
  raiiRecastPolyMeshDetail(rcPolyMeshDetail* pm) : pm_(pm) {}
  rcPolyMeshDetail* pm_;
};

}  // namespace

RecastCompiler::RecastNavmeshRawData::RecastNavmeshRawData()
    : polyMesh(nullptr), polyMeshDetail(nullptr), minBb({}), maxBb({}) {}

RecastCompiler::RecastNavmeshRawData::RecastNavmeshRawData(
    rcPolyMesh* poly_mesh, rcPolyMeshDetail* poly_detail, glm::vec3 min_bb,
    glm::vec3 max_bb)
    : polyMesh(poly_mesh),
      polyMeshDetail(poly_detail),
      minBb(min_bb),
      maxBb(max_bb) {}

RecastCompiler::RecastNavmeshRawData::~RecastNavmeshRawData() {
  if (polyMesh != nullptr) {
    rcFreePolyMesh(polyMesh);
    polyMesh = nullptr;
  }

  if (polyMeshDetail != nullptr) {
    rcFreePolyMeshDetail(polyMeshDetail);
    polyMeshDetail = nullptr;
  }
}

RecastCompiler::RecastCompiler(float cs, float ch, float max_slope_degrees,
                               int walkable_climb, int walkable_height,
                               float agent_radius, int min_region_area,
                               int merge_region_area, float max_contour_error,
                               int max_edge_length, float detail_sample_dist,
                               float detail_max_error)
    : cs_(cs),
      ch_(ch),
      max_slope_degrees_(max_slope_degrees),
      walkable_climb_(walkable_climb),
      walkable_height_(walkable_height),
      agent_radius_(agent_radius),
      min_region_area_(min_region_area),
      merge_region_area_(merge_region_area),
      max_contour_error_(max_contour_error),
      max_edge_length_(max_edge_length),
      detail_sample_dist_(detail_sample_dist),
      detail_max_error_(detail_max_error) {}

RecastCompiler::~RecastCompiler() {}

void RecastCompiler::add_walkable_triangles(
    const core::PodVector<glm::vec3>& vertices,
    const core::PodVector<uint32_t>& indices) {
  uint32_t index_offset = all_indices_.size();

  for (int i = 0; i < vertices.size(); i++) {
    const auto& vert = vertices[i];
    all_vertices_.push_back(vert);
  }

  for (int i = 0; i < indices.size() / 3; i++) {
    all_indices_.push_back(indices[i * 3] + index_offset);
    all_indices_.push_back(indices[i * 3 + 1] + index_offset);
    all_indices_.push_back(indices[i * 3 + 2] + index_offset);

    all_areas_.push_back(0);
  }
}

Maybe<RecastCompiler::RecastNavmeshRawData> RecastCompiler::build_recast() {
  rcContext ctx(false);

  auto hf_opt = ::raiiRecastHeightfield::Create();
  if (hf_opt.is_empty()) {
    return empty_maybe{};
  }

  auto heightfield = hf_opt.move();

  //
  // Step 1: Gather heightfield parameters
  //
  float bmin[3] = {}, bmax[3] = {};
  int w = -1, h = -1;
  rcCalcBounds(&all_vertices_[0].x, all_vertices_.size(), bmin, bmax);
  rcCalcGridSize(bmin, bmax, cs_, &w, &h);
  glm::vec3 min_bb(bmin[0], bmin[1], bmin[2]);
  glm::vec3 max_bb(bmax[0], bmax[1], bmax[2]);

  if (!rcCreateHeightfield(&ctx, heightfield, w, h, bmin, bmax, cs_, ch_)) {
    Logger::err(kLogLabel) << "Failed to create heightfield";
    return empty_maybe{};
  }

  //
  // Step 2: mark walkable area and rasterize tris to heightfield
  //
  rcMarkWalkableTriangles(&ctx, max_slope_degrees_, &all_vertices_[0].x,
                          all_vertices_.size(), &all_indices_[0],
                          all_areas_.size(), &all_areas_[0]);

  if (!rcRasterizeTriangles(&ctx, &all_vertices_[0].x, all_vertices_.size(),
                            &all_indices_[0], &all_areas_[0], all_areas_.size(),
                            heightfield, walkable_climb_)) {
    Logger::err(kLogLabel) << "Failed to rasterize triangles";
    return empty_maybe{};
  }

  //
  // Step 3: filter walkable surfaces post-rasterization to remove artifacts
  //  like unwanted overhangs and spans where the character cannot possibly
  //  stand.
  //
  rcFilterLowHangingWalkableObstacles(&ctx, walkable_climb_, heightfield);
  rcFilterLedgeSpans(&ctx, walkable_height_, walkable_climb_, heightfield);
  rcFilterWalkableLowHeightSpans(&ctx, walkable_height_, heightfield);

  //
  // Step 4: partition walkable surface to simple regions
  //
  auto ch_opt = raiiRecastCompactHeightfield::Create();
  if (ch_opt.is_empty()) {
    Logger::err(kLogLabel) << "Failed to allocate compact heightfield";
    return empty_maybe{};
  }

  auto compact_heightfield = ch_opt.move();

  if (!rcBuildCompactHeightfield(&ctx, walkable_height_, walkable_climb_,
                                 heightfield, compact_heightfield)) {
    Logger::err(kLogLabel) << "Failed to build compact heightfield";
    return empty_maybe{};
  }

  if (!rcErodeWalkableArea(&ctx, agent_radius_, compact_heightfield)) {
    Logger::err(kLogLabel)
        << "Failed to erode compact heightfield to agent radius";
    return empty_maybe{};
  }

  // TODO (sessamekesh): Mark additional areas as having certain properties
  // (exclude, etc)

  //
  // Step 5: Partition the heightfield in preparation for navmesh triangulation
  //
  if (!rcBuildDistanceField(&ctx, compact_heightfield)) {
    Logger::err(kLogLabel) << "Could not build distance field";
    return empty_maybe{};
  }

  if (!rcBuildRegions(&ctx, compact_heightfield, 0, min_region_area_,
                      merge_region_area_)) {
    Logger::err(kLogLabel) << "Failed to build watershed regions";
    return empty_maybe{};
  }

  //
  // Step 5: Trace and simplify region contours
  //
  auto cs_opt = raiiRecastContourSet::Create();
  if (cs_opt.is_empty()) {
    return empty_maybe{};
  }
  auto contour_set = cs_opt.move();

  if (!rcBuildContours(&ctx, compact_heightfield, max_contour_error_,
                       max_edge_length_, contour_set)) {
    Logger::err(kLogLabel) << "Failed to build contour set";
    return empty_maybe{};
  }

  //
  // Step 6: Build polygon mesh from contours + detail
  //
  auto pm_opt = raiiRecastPolyMesh::Create();
  if (pm_opt.is_empty()) {
    return empty_maybe{};
  }
  auto polymesh = pm_opt.move();

  if (!rcBuildPolyMesh(&ctx, contour_set, 6, polymesh)) {
    Logger::err(kLogLabel) << "Failed to triangulate contours";
    return empty_maybe{};
  }

  auto pmd_opt = raiiRecastPolyMeshDetail::Create();
  if (pmd_opt.is_empty()) {
    return empty_maybe{};
  }
  auto polymesh_detail = pmd_opt.move();

  if (!rcBuildPolyMeshDetail(&ctx, polymesh, compact_heightfield,
                             detail_sample_dist_, detail_max_error_,
                             polymesh_detail)) {
    Logger::err(kLogLabel) << "Could not build detail mesh";
    return empty_maybe{};
  }

  for (int i = 0; i < polymesh->npolys; i++) {
    if (polymesh->areas[i] != RC_NULL_AREA) {
      polymesh->flags[i] = 0x1;
    }
  }

  return RecastNavmeshRawData(polymesh.move(), polymesh_detail.move(), min_bb,
                              max_bb);
}

Maybe<RawBuffer> RecastCompiler::build_raw(
    const RecastCompiler::RecastNavmeshRawData& data) {
  unsigned char* nav_data = nullptr;
  int nav_data_size = 0;

  // TODO (sessamekesh): Handle converting params of navmesh into something the
  //  sample can use (e.g., have a TERRAIN, GRASS types that can be queried)

  dtNavMeshCreateParams params{};
  memset(&params, 0x00, sizeof(params));

  params.verts = data.polyMesh->verts;
  params.vertCount = data.polyMesh->nverts;
  params.polys = data.polyMesh->polys;
  params.polyAreas = data.polyMesh->areas;
  params.polyFlags = data.polyMesh->flags;
  params.polyCount = data.polyMesh->npolys;
  params.nvp = data.polyMesh->nvp;
  params.detailMeshes = data.polyMeshDetail->meshes;
  params.detailVerts = data.polyMeshDetail->verts;
  params.detailVertsCount = data.polyMeshDetail->nverts;
  params.detailTris = data.polyMeshDetail->tris;
  params.detailTriCount = data.polyMeshDetail->ntris;

  // Off mesh connection data would go in here... if we had any!

  params.walkableHeight = walkable_height_;
  params.walkableClimb = walkable_climb_;
  params.walkableRadius = agent_radius_;
  memcpy(params.bmin, data.polyMesh->bmin, sizeof(float) * 3);
  memcpy(params.bmax, data.polyMesh->bmax, sizeof(float) * 3);
  params.cs = cs_;
  params.ch = ch_;
  params.buildBvTree = true;

  if (!dtCreateNavMeshData(&params, &nav_data, &nav_data_size)) {
    Logger::err(kLogLabel) << "Could not build Detour navmesh";
    return empty_maybe{};
  }

  return RawBuffer(nav_data, nav_data_size, true);
}

Maybe<DetourNavmesh> RecastCompiler::navmesh_from_raw(RawBuffer raw_data) {
  dtNavMesh* nav_mesh = dtAllocNavMesh();
  if (nav_mesh == nullptr) {
    Logger::err(kLogLabel) << "Failed to allocate nav mesh";
    return empty_maybe{};
  }

  dtStatus status{};

  uint8_t* dat = nullptr;
  size_t sz = 0;
  if (!raw_data.detach(&dat, &sz)) {
    Logger::err(kLogLabel)
        << "Could not detach provided RawBuffer - must provide owned data!";
    return empty_maybe{};
  }

  status = nav_mesh->init(dat, sz, DT_TILE_FREE_DATA);
  if (dtStatusFailed(status)) {
    Logger::err(kLogLabel) << "Could not initialize Detour navmesh";
    return empty_maybe{};
  }

  return DetourNavmesh(nav_mesh);
}
