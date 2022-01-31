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
  return (end == std::string::npos) ? s : s.substr(end + 9);
}

std::string replace_bracket_group_and_binding_syntax(std::string s) {
  if (s.find("[[block]]") != std::string::npos) {
    // Ignore this case because it's handled above...
    return s;
  }

  size_t open = s.find("[[");
  size_t end = s.find("]]");
  if (open != std::string::npos && end != std::string::npos) {
    size_t group_start = s.find("group");

    if (group_start != std::string::npos && group_start < end &&
        group_start > open) {
      s = s.replace(group_start, 5, "@group");
      end++;
    }

    size_t binding_start = s.find("binding");
    if (binding_start != std::string::npos && binding_start < end &&
        binding_start > open) {
      s = s.replace(binding_start, 7, "@binding");
      end++;
    }

    size_t location_start = s.find("location");
    if (location_start != std::string::npos && location_start < end &&
        location_start > open) {
      s = s.replace(location_start, 8, "@location");
      end++;
    }

    size_t builtin_start = s.find("builtin");
    if (builtin_start != std::string::npos && builtin_start < end &&
        builtin_start > open) {
      s = s.replace(builtin_start, 7, "@builtin");
      end++;
    }

    size_t stage_start = s.find("stage");
    if (stage_start != std::string::npos && stage_start < end &&
        stage_start > open) {
      s = s.replace(stage_start, 5, "@stage");
      end++;
    }

    size_t comma_start = s.find(",");
    if (comma_start != std::string::npos && comma_start < end &&
        comma_start > open) {
      s = s.replace(comma_start, 1, "");
      end--;
    }

    s = s.replace(open, 2, "");
    s = s.replace(end - 2, 2, "");
  }

  return s;
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

    // Always replace the old [[binding(n) group(m)]] syntax with the newer
    // @binding(n) @group(m) syntax
    line = ::replace_bracket_group_and_binding_syntax(line);

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

  std::string processed_source = output.str();
  wgsl_source_asset->set_shader_source(processed_source);

  return true;
}