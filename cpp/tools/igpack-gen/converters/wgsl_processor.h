#ifndef TOOLS_IGPACK_GEN_CONVERTERS_WGSL_PROCESSOR_H
#define TOOLS_IGPACK_GEN_CONVERTERS_WGSL_PROCESSOR_H

#include <igasset/proto/igasset.pb.h>
#include <igpack-gen/proto/igpack-plan.pb.h>

#include <string>

namespace indigo::igpackgen {

class WgslProcessor {
 public:
  bool copy_wgsl_source(asset::pb::AssetPack& output_asset_pack,
                        const pb::CopyWgslSourceAction& action,
                        const std::string& wgsl_source) const;
};

}  // namespace indigo::igpackgen

#endif
