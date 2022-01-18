#include <converters/wgsl_processor.h>
#include <igcore/log.h>

using namespace indigo;
using namespace igpackgen;

namespace {
const char* kLogLabel = "WgslProcessor";

asset::pb::WgslSource::ShaderType convert_shader_type(
    pb::CopyWgslSourceAction::ShaderType in_type) {
  switch (in_type) {
    case pb::CopyWgslSourceAction::ShaderType::
        CopyWgslSourceAction_ShaderType_VERTEX:
      return asset::pb::WgslSource::ShaderType::WgslSource_ShaderType_VERTEX;
    case pb::CopyWgslSourceAction::ShaderType::
        CopyWgslSourceAction_ShaderType_FRAGMENT:
      return asset::pb::WgslSource::ShaderType::WgslSource_ShaderType_FRAGMENT;
    case pb::CopyWgslSourceAction::ShaderType::
        CopyWgslSourceAction_ShaderType_COMPUTE:
      return asset::pb::WgslSource::ShaderType::WgslSource_ShaderType_COMPUTE;
  }

  return asset::pb::WgslSource::ShaderType::WgslSource_ShaderType_UNKNOWN;
}

}  // namespace

bool WgslProcessor::copy_wgsl_source(asset::pb::AssetPack& output_asset_pack,
                                     const pb::CopyWgslSourceAction& action,
                                     const std::string& wgsl_source) const {
  if (wgsl_source.empty()) {
    core::Logger::err(kLogLabel) << "Cannot copy WGSL source - input is empty";
    return false;
  }

  asset::pb::SingleAsset* new_asset = output_asset_pack.add_assets();
  asset::pb::WgslSource* wgsl_source_asset = new_asset->mutable_wgsl_source();

  new_asset->set_name(action.igasset_name());
  wgsl_source_asset->set_entry_point(action.entry_point());
  wgsl_source_asset->set_shader_type(
      ::convert_shader_type(action.shader_type()));
  wgsl_source_asset->set_shader_source(wgsl_source);

  return true;
}