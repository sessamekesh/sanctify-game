#ifndef SANCTIFY_COMMON_RENDER_SOLID_STATIC_INSTANCE_STORE_H
#define SANCTIFY_COMMON_RENDER_SOLID_STATIC_INSTANCE_STORE_H

#include <common/util/resource_registry.h>
#include <iggpu/instance_buffer_store.h>

#include "solid_static_geo.h"

/**
 * Utility class to maintain an instance buffer for solid static geometry
 *  without having to constantly create/destroy CPU/GPU buffers
 */

namespace sanctify::render::solid_static {

class InstanceKey {
 public:
  InstanceKey() = default;
  InstanceKey(ReadonlyResourceRegistry<Geo>::Key geo_key);
  InstanceKey(const InstanceKey&) = default;
  InstanceKey(InstanceKey&&) = default;
  InstanceKey& operator=(const InstanceKey&) = default;
  InstanceKey& operator=(InstanceKey&&) = default;
  ~InstanceKey() = default;

  // InstanceBufferStore key type
  ReadonlyResourceRegistry<Geo>::Key geo_key() const;
  std::string get_key() const;
  int get_lifetime() const;

 private:
  ReadonlyResourceRegistry<Geo>::Key geo_key_;
  std::string str_key_;
};

typedef indigo::iggpu::InstanceBufferStore<InstanceKey, InstanceData>
    InstanceStore;

}  // namespace sanctify::render::solid_static

#endif
