#include <gtest/gtest.h>
#include <igasync/promise_combiner.h>

using namespace indigo;
using namespace core;

TEST(PromiseCombiner, basic_combine_works) {
  auto task_list = std::make_shared<core::TaskList>();

  auto p1 = core::Promise<int>::create();
  auto p2 = core::Promise<int>::create();

  auto combiner = PromiseCombiner::Create();

  auto key_1 = combiner->add(p1, task_list);
  auto key_2 = combiner->add(p2, task_list);

  auto combine_rsl_promise = combiner->combine();

  bool has_resolved = false;
  int r1 = 0, r2 = 0;
  combine_rsl_promise->on_success(
      [&r1, &r2, &has_resolved, key_1,
       key_2](const PromiseCombiner::PromiseCombinerResult& rsl) {
        has_resolved = true;

        const auto& v1 = rsl.get(key_1);
        const auto& v2 = rsl.get(key_2);

        r1 = v1;
        r2 = v2;
      },
      task_list);

  // Step 1: No execution should have happened at this point!
  ASSERT_FALSE(has_resolved);

  while (task_list->execute_next()) {
    FAIL() << "Task list should be empty at this point";
  }

  // Step 2 - incomplete combiner
  // Resolve one promise - that should not cause the combiner to finish
  p1->resolve(10);

  if (!task_list->execute_next()) {
    FAIL() << "Promise resolution should have triggered a Combiner update";
  }

  if (task_list->execute_next()) {
    FAIL()
        << "Promise resolution should have only triggered one Combiner update";
  }

  ASSERT_FALSE(has_resolved);

  // Step 3 - finish the combiner.
  // The combiner should finish, and the results should be available.
  p2->resolve(20);
  while (task_list->execute_next()) {
  }

  ASSERT_TRUE(has_resolved);
  EXPECT_EQ(r1, 10);
  EXPECT_EQ(r2, 20);
}