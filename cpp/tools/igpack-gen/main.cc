#include <google/protobuf/text_format.h>
#include <igcore/log.h>
#include <plan_executor.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <fstream>
#include <iostream>

namespace {
const char* kLogLabel = "main";
}

using Logger = indigo::core::Logger;

int main(int argc, char** argv) {
  //
  // Parse CLI arguments
  //
  CLI::App app{"igpack-gen - a tool for generating Indigo asset pack bundles"};

  std::string input_asset_path_root;
  app.add_option("-w,--input_asset_path_root", input_asset_path_root,
                 "Root directory for resolving input asset file paths")
      ->required(true)
      ->check(CLI::ExistingDirectory);

  std::string output_asset_path_root;
  app.add_option("-d,--output_asset_path_root", output_asset_path_root,
                 "Root directory used when writing asset pack files")
      ->required(true);

  std::string input_plan_file_path;
  app.add_option("-i,--input_plan_file", input_plan_file_path,
                 "Path of the Indigo asset pack plan file (*.igpack-plan) that "
                 "should be processed by this tool")
      ->required(true)
      ->check(CLI::ExistingFile);

  CLI11_PARSE(app, argc, argv);

  //
  // Execute plan
  //
  std::filesystem::path input_path = input_asset_path_root;
  if (!std::filesystem::is_directory(output_asset_path_root) ||
      !std::filesystem::exists(output_asset_path_root)) {
    if (!std::filesystem::create_directories(output_asset_path_root)) {
      Logger::err(kLogLabel)
          << "Failed to create output directory " << output_asset_path_root;
      return -1;
    }
  }
  std::filesystem::path output_path = output_asset_path_root;

  // Read in the actual plan
  indigo::igpackgen::pb::IgpackGenPlan plan;
  {
    std::ifstream fin(input_plan_file_path);
    if (!fin) {
      Logger::err(kLogLabel)
          << "Failed to read input plan at path " << input_plan_file_path;
      return -1;
    }

    std::string plan_text;
    fin.seekg(0, std::ios::end);
    plan_text.reserve(fin.tellg());
    fin.seekg(0, std::ios::beg);
    plan_text.assign(std::istreambuf_iterator<char>(fin),
                     std::istreambuf_iterator<char>());

    if (!google::protobuf::TextFormat::ParseFromString(plan_text, &plan)) {
      Logger::err(kLogLabel)
          << "Failed to parse execution plan at: " << input_plan_file_path;
      Logger::err(kLogLabel) << "Plan text:\n" << plan_text;
      return -1;
    }
  }

  // This could be exposed as a CLI argument with a default so that people could
  //  use a CMake argument to avoid over-using memory (or to allow higher memory
  //  use on high powered machines)
  uint32_t max_file_cache = 512 * 1024 * 1024;  // 512MB raw file memory cache
  indigo::igpackgen::PlanExecutor executor(max_file_cache);
  indigo::igpackgen::PlanInvocationDesc plan_desc{};
  plan_desc.InputAssetPathRoot = input_path;
  plan_desc.OutputAssetPathRoot = output_path;
  plan_desc.Plan = std::move(plan);

  if (!executor.execute_plan(plan_desc)) {
    Logger::err(kLogLabel) << "Plan execution failed!";
    return -1;
  }

  Logger::log(kLogLabel) << "Successfully executed plan "
                         << input_plan_file_path << "!";
  return 0;
}