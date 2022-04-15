#ifndef LIBS_IGECS_SRC_CTTI_TYPE_ID_H
#define LIBS_IGECS_SRC_CTTI_TYPE_ID_H

#include <igcore/config.h>

#include <atomic>
#include <string>

namespace {
std::string trim_type_name_msvc(const char* tn) {
  const size_t off = 135;
  const size_t tail = 7;

  std::string stn(tn);
  return stn.substr(off, stn.length() - off - tail);
}

std::string trim_type_name_gcc(const char* tn) {
  const size_t off = 135;
  const size_t tail = 8;

  std::string stn(tn);
  return stn.substr(off, stn.length() - off - tail);
}
}  // namespace

namespace indigo::igecs {

struct CttiTypeId {
  uint32_t id;

  bool operator==(const CttiTypeId& o) const { return id == o.id; }
  bool operator!=(const CttiTypeId& o) const { return id != o.id; }
  bool operator<(const CttiTypeId& o) const { return id < o.id; }

  template <typename T>
  static CttiTypeId of() {
    static uint32_t tid = next_type_id_++;
    return CttiTypeId{tid};
  }

  template <typename T>
  static std::string name() {
#ifdef IG_ENABLE_ECS_VALIDATION
#ifdef _MSC_VER
    return trim_type_name_msvc(__FUNCSIG__);
#else
    return trim_type_name_gcc(__PRETTY_FUNCTION__);
#endif
#else
    return std::string("T-") + std::to_string(CttiTypeId::of<T>().id);
#endif
  }

 private:
  static std::atomic_uint32_t next_type_id_;
};

}  // namespace indigo::igecs

#endif
