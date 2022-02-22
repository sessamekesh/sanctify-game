#include <ignav/detour_navmesh.h>

#include <utility>

using namespace indigo;
using namespace nav;

DetourNavmesh::DetourNavmesh(dtNavMesh* mesh) : mesh_(mesh) {}

DetourNavmesh::DetourNavmesh(DetourNavmesh&& o) noexcept
    : mesh_(std::exchange(o.mesh_, nullptr)) {}

DetourNavmesh& DetourNavmesh::operator=(DetourNavmesh&& o) noexcept {
  mesh_ = std::exchange(o.mesh_, nullptr);
  return *this;
}

DetourNavmesh::~DetourNavmesh() {
  if (mesh_ != nullptr) {
    dtFreeNavMesh(mesh_);
    mesh_ = nullptr;
  }
}