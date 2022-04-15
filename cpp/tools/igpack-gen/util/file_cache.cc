#include <igcore/log.h>
#include <util/file_cache.h>

#include <algorithm>
#include <fstream>

namespace {
const char* kLogLabel = "FileCache";
}

using namespace indigo;
using namespace igpackgen;

FileCache::FileCache(std::filesystem::path input_root_path,
                     uint32_t max_cache_size)
    : input_root_path_(input_root_path),
      max_cache_size_(max_cache_size),
      cache_size_(0u),
      empty_string_("") {}

const std::string& FileCache::load_file(std::string relative_path) {
  if (!std::filesystem::exists(input_root_path_ / relative_path)) {
    return empty_string_;
  }

  if (raw_file_contents_.count(relative_path) > 0) {
    auto existing_it = std::find(file_load_order_.begin(),
                                 file_load_order_.end(), relative_path);
    if (existing_it != file_load_order_.end()) {
      file_load_order_.erase(existing_it);
    }
    file_load_order_.push_back(relative_path);
    return raw_file_contents_[relative_path];
  }

  std::ifstream fin(input_root_path_ / relative_path, std::fstream::binary);
  fin.seekg(0, std::ios::end);
  uint32_t file_size = (uint32_t)fin.tellg();
  fin.seekg(0, std::ios::beg);

  while ((cache_size_ + file_size > max_cache_size_) &&
         raw_file_contents_.size() > 0) {
    std::string file_to_delete = *file_load_order_.begin();
    file_load_order_.pop_front();
    auto file_contents_it = raw_file_contents_.find(file_to_delete);
    if (file_contents_it != raw_file_contents_.end()) {
      cache_size_ -= file_contents_it->second.size();
      raw_file_contents_.erase(file_contents_it);
    }
  }

  std::string file_data;
  file_data.resize(file_size);
  if (!fin.read(reinterpret_cast<char*>(&file_data[0]), file_size)) {
    core::Logger::err(kLogLabel)
        << "load_file failed for " << input_root_path_ / relative_path
        << " - file could not be read";
    return empty_string_;
  }

  file_load_order_.push_back(relative_path);
  cache_size_ += file_data.size();
  raw_file_contents_.emplace(relative_path, std::move(file_data));
  return raw_file_contents_[relative_path];
}