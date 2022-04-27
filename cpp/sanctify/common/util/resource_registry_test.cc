#include "resource_registry.h"

#include <gtest/gtest.h>

using namespace sanctify;

namespace {

struct MovableOnlyResource {
  int value;
  bool* setOnDestroy;

  MovableOnlyResource() = delete;
  MovableOnlyResource(int v, bool* set_on_destroy = nullptr)
      : value(v), setOnDestroy(set_on_destroy) {}
  ~MovableOnlyResource() {
    if (setOnDestroy) {
      *setOnDestroy = true;
    }
  }

  MovableOnlyResource(const MovableOnlyResource&) = delete;
  MovableOnlyResource& operator=(const MovableOnlyResource&) = delete;

  MovableOnlyResource(MovableOnlyResource&& o) noexcept
      : value(o.value), setOnDestroy(std::exchange(o.setOnDestroy, nullptr)) {}
  MovableOnlyResource& operator=(MovableOnlyResource&& o) noexcept {
    value = o.value;
    setOnDestroy = std::exchange(o.setOnDestroy, nullptr);
    return *this;
  }
};

}  // namespace

TEST(ReadonlyResourceRegistry, AddsAndGetsResource) {
  ReadonlyResourceRegistry<MovableOnlyResource> registry;

  bool first_destroyed = false;
  auto first_key =
      registry.add_resource(MovableOnlyResource(5, &first_destroyed));

  bool second_destroyed = false;
  auto second_key =
      registry.add_resource(MovableOnlyResource(10, &second_destroyed));

  EXPECT_NE(first_key.get_raw_key(), second_key.get_raw_key());

  {
    auto* r1 = registry.get(first_key);
    auto* r2 = registry.get(second_key);

    EXPECT_EQ(r1->value, 5);
    EXPECT_EQ(r2->value, 10);
  }

  EXPECT_FALSE(first_destroyed);
  EXPECT_FALSE(second_destroyed);

  registry.remove_resource(first_key);

  EXPECT_TRUE(first_destroyed);

  EXPECT_EQ(registry.get(first_key), nullptr);

  EXPECT_FALSE(second_destroyed);
  EXPECT_NE(registry.get(second_key), nullptr);
}
