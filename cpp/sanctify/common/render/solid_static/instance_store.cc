#include "instance_store.h"

using namespace sanctify;
using namespace render;
using namespace solid_static;

InstanceKey::InstanceKey(ReadonlyResourceRegistry<Geo>::Key geo_key)
    : geo_key_(geo_key), str_key_(std::to_string(geo_key.get_raw_key())) {}

ReadonlyResourceRegistry<Geo>::Key InstanceKey::geo_key() const {
  return geo_key_;
}

std::string InstanceKey::get_key() const { return str_key_; }

int InstanceKey::get_lifetime() const { return 5; }
