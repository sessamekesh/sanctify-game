#ifndef _LIB_IGCORE_TYPEID_H_
#define _LIB_IGCORE_TYPEID_H_

#include <atomic>

namespace indigo::core {
class TypeId {
 public:
  template <class T>
  inline static uint32_t of() {
    static const int type_id = next_type_id_++;
    return type_id;
  }

 private:
  static std::atomic_uint32_t next_type_id_;
};
}  // namespace indigo::core

#endif
