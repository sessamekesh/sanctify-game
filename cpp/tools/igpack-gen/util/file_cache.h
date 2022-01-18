#ifndef TOOLS_IGPACK_GEN_UTIL_FILE_CACHE_H
#define TOOLS_IGPACK_GEN_UTIL_FILE_CACHE_H

#include <filesystem>
#include <list>
#include <map>
#include <string>

namespace indigo::igpackgen {

class FileCache {
 public:
  FileCache(std::filesystem::path input_root_path, uint32_t max_cache_size);
  const std::string& load_file(std::string relative_path);

 private:
  std::filesystem::path input_root_path_;

  uint32_t max_cache_size_;
  uint32_t cache_size_;

  std::map<std::string, std::string> raw_file_contents_;
  std::list<std::string> file_load_order_;
  std::string empty_string_;
};

}  // namespace indigo::igpackgen

#endif
