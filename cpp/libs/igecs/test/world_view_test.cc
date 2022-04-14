#define IG_ENABLE_ECS_VALIDATION

#include <gtest/gtest-death-test.h>
#include <gtest/gtest.h>
#include <igecs/world_view.h>

using namespace indigo;
using namespace igecs;

namespace {
struct FooT {
  int a;
};

struct BarT {
  int a;
  int b;
};
}  // namespace

TEST(IgECS_WorldView, UnrestrictedWorldViewAllowsAccesses) {
  entt::registry registry;
  registry.set<FooT>(1);
  registry.set<BarT>(2, 3);

  auto decl = WorldView::Decl::Thin();
  auto world_view = decl.create(&registry);

  EXPECT_EQ(world_view.ctx<FooT>().a, 1);
  EXPECT_EQ(world_view.ctx<BarT>().b, 3);

  world_view.mut_ctx<BarT>().b = 5;
  EXPECT_EQ(world_view.ctx<BarT>().b, 5);

  entt::entity e = registry.create();
  world_view.attach<FooT>(e, 5);
  world_view.attach<BarT>(e, 10, 15);

  EXPECT_EQ(world_view.read<FooT>(e).a, 5);
  EXPECT_EQ(world_view.read<BarT>(e).a, 10);
  EXPECT_EQ(world_view.read<BarT>(e).b, 15);

  world_view.write<FooT>(e).a = 222;
  EXPECT_EQ(world_view.read<FooT>(e).a, 222);
}

TEST(IgECS_WorldView, CtxReadWriteSucceeds) {
  entt::registry registry;
  registry.set<FooT>(1);

  WorldView::Decl decl;
  decl.ctx_writes<FooT>();

  auto world_view = decl.create(&registry);

  EXPECT_EQ(world_view.ctx<FooT>().a, 1);

  world_view.mut_ctx<FooT>().a = 2;
  EXPECT_EQ(world_view.ctx<FooT>().a, 2);
}

TEST(IgECS_WorldView, ComponentReadWriteSucceeds) {
  entt::registry registry;

  entt::entity e = registry.create();
  registry.emplace<FooT>(e, 1);

  WorldView::Decl decl;
  decl.writes<FooT>();

  auto world_view = decl.create(&registry);

  EXPECT_EQ(world_view.read<FooT>(e).a, 1);
  world_view.write<FooT>(e).a = 2;
  EXPECT_EQ(world_view.read<FooT>(e).a, 2);
}

TEST(IgECS_WorldView, IterateSucceedsAndTouchesEverything) {
  entt::registry registry;

  entt::entity e1 = registry.create();
  entt::entity e2 = registry.create();

  registry.emplace<FooT>(e1, 1);
  registry.emplace<FooT>(e2, 2);
  registry.emplace<BarT>(e1, 10, 20);
  registry.emplace<BarT>(e2, 30, 40);

  WorldView::Decl decl;
  decl.writes<FooT>().reads<BarT>();

  auto wv = decl.create(&registry);

  auto view = wv.view<FooT, const BarT>();
  bool has_1 = false, has_2 = false;
  int ct = 0;
  for (auto [e, f, b] : view.each()) {
    ct++;

    if (f.a == 1 && b.a == 10 && b.b == 20) {
      has_1 = true;
      continue;
    }

    if (f.a == 2 && b.a == 30 && b.b == 40) {
      has_2 = true;
      continue;
    }
  }

  EXPECT_EQ(ct, 2);
  EXPECT_TRUE(has_1);
  EXPECT_TRUE(has_2);
}

TEST(IgECS_WorldViewDeathTest, BadCtxReadFails) {
  entt::registry registry;
  registry.set<FooT>(1);

  WorldView::Decl decl;

  auto world_view = decl.create(&registry);

  EXPECT_DEATH({ EXPECT_EQ(world_view.ctx<FooT>().a, 0); },
               "ECS validation failure: method ctx failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadCtxWriteFails) {
  entt::registry registry;
  registry.set<FooT>(1);

  WorldView::Decl decl;
  decl.ctx_reads<FooT>();

  auto world_view = decl.create(&registry);

  EXPECT_EQ(world_view.ctx<FooT>().a, 1);
  EXPECT_DEATH({ EXPECT_EQ(world_view.mut_ctx<FooT>().a, 1); },
               "ECS validation failure: method mut_ctx failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadEntityReadFails) {
  entt::registry registry;

  entt::entity e = registry.create();
  registry.emplace<FooT>(e, 1);

  WorldView::Decl decl;

  auto world_view = decl.create(&registry);

  EXPECT_DEATH({ EXPECT_EQ(world_view.read<FooT>(e).a, 1); },
               "ECS validation failure: method read failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadEntityWriteFails) {
  entt::registry registry;

  entt::entity e = registry.create();
  registry.emplace<FooT>(e, 1);

  WorldView::Decl decl;
  decl.reads<FooT>();

  auto wv = decl.create(&registry);

  EXPECT_EQ(wv.read<FooT>(e).a, 1);
  EXPECT_DEATH({ EXPECT_EQ(wv.write<FooT>(e).a, 1); },
               "ECS validation failure: method write failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadViewReadFails) {
  entt::registry registry;

  WorldView::Decl decl;
  decl.writes<BarT>();
  auto wv = decl.create(&registry);

  EXPECT_DEATH({ auto view = wv.view<FooT>(); },
               "MUTABLE view_test failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadViewWriteFails) {
  entt::registry registry;

  WorldView::Decl decl;
  decl.reads<FooT>().writes<BarT>();
  auto wv = decl.create(&registry);

  // No death
  auto v1 = wv.view<BarT, const FooT>();

  // For some reason just doing wv.view<FooT, BarT>() inside EXPECT_DEATH
  //  doesn't compile? It's weird. This works though.
  auto get = [&wv]() { return wv.view<BarT, FooT>(); };
  EXPECT_DEATH({ get(); }, "IMMUTABLE view_test failed for type .*FooT");
}
