#include <converters/wgsl_processor.h>
#include <igcore/log.h>

#include <sstream>

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

const std::string kWhitespaceCharacters = " \n\r\t";

std::string ltrim(std::string s) {
  size_t start = s.find_first_not_of(kWhitespaceCharacters);
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(std::string s) {
  size_t end = s.find_last_not_of(kWhitespaceCharacters);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(std::string s) { return ltrim(rtrim(s)); }

std::string strip_trailing_comment(std::string s) {
  size_t start = s.find("//");
  return (start == std::string::npos) ? s : s.substr(0, start);
}

std::string content_after_block_annotation(std::string s) {
  size_t end = s.rfind("[[block]]");
  return (end == std::string::npos) ? s : s.substr(end);
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

  std::stringstream input(wgsl_source);
  std::stringstream output;

  for (std::string line; std::getline(input, line);) {
    // Always comment out [[block]], this is deprecated in the WGSL language
    // but some tooling still likes to include it.
    auto after_block = ::content_after_block_annotation(line);
    if (after_block != line) {
      if (::trim(after_block) == "") {
        // [[block]] annotation by itself - replace the line with a commented
        // line
        line = "// [[block]] <--- WgslProcessor removed deprecated annotation";
      } else {
        if (action.strip_comments_and_such()) {
          // Annotation was not by itself and comments are removed anyways,
          // carry on
          line = after_block;
        } else {
          // Not stripping comments, add in an extra line and carry on
          output << "// [[block]] <- WgslProcessor removed deprecated "
                 << "annotation\n";
          line = after_block;
        }
      }
    }

    // If comments and such are being removed, apply whitespace removal:
    if (action.strip_comments_and_such()) {
      line = trim(strip_trailing_comment(line));
      if (line == "") {
        continue;
      }
    }

    // Line has been processed, append it to the output
    output << line << "\n";
  }

  wgsl_source_asset->set_shader_source(output.str());

  return true;
}