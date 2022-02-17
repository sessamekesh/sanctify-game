#include <util/recast_builder.h>

using namespace mapeditor;

RecastBuilder::RecastBuilder() : is_valid_state_(true) {}

bool RecastBuilder::is_valid_state() const { return is_valid_state_; }

void RecastBuilder::rebuild_from(std::shared_ptr<RecastParams> params,
                                 std::shared_ptr<AssimpLoader> loader) {
  is_valid_state_ = !is_valid_state_;

  // TODO (sessamekesh): Use some shared library (between this and the
  //  igpack-gen tool) to actually generate the Recast/Detour meshes.
  // TODO (sessamekesh): Generate simple renderables for any included
  //  and excluded Assimp geometry, and prepare them for use in the viewport.
  // TODO (sessamekesh): Generate an overlay for the recast navmesh as well

  // TODO (sessamekesh): use this opportunity to set up all the Assimp mesh
  //  preview stuff needed for the preview viewport
  // TODO (sessamekesh): Also, the recast navmesh building logic should be
  //  shared so that the igpack-gen tool can use this logic as well, right?
}