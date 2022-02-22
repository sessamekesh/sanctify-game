#ifndef LIBS_IGNAV_INCLUDE_IGNAV_DETOUR_NAVMESH_H
#define LIBS_IGNAV_INCLUDE_IGNAV_DETOUR_NAVMESH_H

#include <DetourNavMesh.h>

namespace indigo::nav {

class DetourNavmesh {
 public:
  DetourNavmesh(dtNavMesh* mesh);

  DetourNavmesh(const DetourNavmesh&) = delete;
  DetourNavmesh& operator=(const DetourNavmesh&) = delete;
  DetourNavmesh(DetourNavmesh&&) noexcept;
  DetourNavmesh& operator=(DetourNavmesh&&) noexcept;

  const dtNavMesh* operator->() const { return mesh_; }
  const dtNavMesh* raw() const { return mesh_; }

  ~DetourNavmesh();

 private:
  dtNavMesh* mesh_;
};

}  // namespace indigo::nav

#endif
