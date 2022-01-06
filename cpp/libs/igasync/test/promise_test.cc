#include <gtest/gtest.h>
#include <igasync/promise.h>
#include <igcore/typeid.h>

#include <functional>

using namespace indigo;
using namespace core;

TEST(PromiseCombiners, combine2_combines) {
  auto task_list = std::make_shared<core::TaskList>();

  auto p1 = core::Promise<int>::create();
  auto p2 = core::Promise<double>::create();

  auto combined = PromiseCombiners::peek_combine(task_list, p1, p2);

  bool is_resolved = false;
  int p1_rsl = 0;
  double p2_rsl = 0.;
  combined->on_success(
      [&is_resolved, &p1_rsl, &p2_rsl](const auto& rsl) {
        is_resolved = true;
        p1_rsl = std::get<0>(rsl);
        p2_rsl = std::get<1>(rsl);
      },
      task_list);

  while (task_list->execute_next()) {
  }

  // No resolution = no value yet
  EXPECT_FALSE(is_resolved);

  p1->resolve(5);
  p2->resolve(10.);

  while (task_list->execute_next()) {
  }

  EXPECT_TRUE(is_resolved);
  EXPECT_EQ(p1_rsl, 5);
  EXPECT_DOUBLE_EQ(p2_rsl, 10.);
}

TEST(PromiseCombiners, combine3_combines) {
  auto task_list = std::make_shared<core::TaskList>();

  auto p1 = core::Promise<int>::create();
  auto p2 = core::Promise<double>::create();
  auto p3 = core::Promise<int>::create();

  auto combined = PromiseCombiners::peek_combine(task_list, p1, p2, p3);

  bool is_resolved = false;
  int p1_rsl = 0;
  double p2_rsl = 0.;
  int p3_rsl = 0;
  combined->on_success(
      [&is_resolved, &p1_rsl, &p2_rsl, &p3_rsl](const auto& rsl) {
        is_resolved = true;
        p1_rsl = std::get<0>(rsl);
        p2_rsl = std::get<1>(rsl);
        p3_rsl = std::get<2>(rsl);
      },
      task_list);

  while (task_list->execute_next()) {
  }

  // No resolution = no value yet
  EXPECT_FALSE(is_resolved);

  p1->resolve(5);
  p2->resolve(10.);
  p3->resolve(15);

  while (task_list->execute_next()) {
  }

  EXPECT_TRUE(is_resolved);
  EXPECT_EQ(p1_rsl, 5);
  EXPECT_DOUBLE_EQ(p2_rsl, 10.);
  EXPECT_EQ(p3_rsl, 15);
}