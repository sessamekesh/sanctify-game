#ifndef TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_BUILDER_H
#define TOOLS_MAP_EDITOR_SRC_UTIL_RECAST_BUILDER_H

#include <util/assimp_loader.h>
#include <util/recast_params.h>
#include <webgpu/webgpu_cpp.h>

namespace mapeditor {

class RecastBuilder {
 public:
  RecastBuilder();

  void rebuild_from(std::shared_ptr<RecastParams> params,
                    std::shared_ptr<AssimpLoader> loader);

  bool is_valid_state() const;

 private:
  bool is_valid_state_;
};

}  // namespace mapeditor

#endif
