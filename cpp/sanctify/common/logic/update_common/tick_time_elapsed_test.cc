#include "tick_time_elapsed.h"

#include <gtest/gtest.h>

using namespace sanctify;
using namespace logic;
using namespace indigo;

/**
 * Simple test for a trivial helper. Mostly meant to be a tiny example of a unit
 * test.
 */
TEST(FrameTimeElapsedUtil, UpdatesElapsedTime) {
  entt::registry world;

  // Writes the appropriate component up front
  {
    auto wv = igecs::WorldView::Thin(&world);
    FrameTimeElapsedUtil::mark_time_elapsed(&wv, 1.f);
    FrameTimeElapsedUtil::mark_time_elapsed(&wv, 1.f);

    EXPECT_TRUE(wv.has_ctx_written<CtxFrameTimeElapsed>());
    EXPECT_TRUE(wv.has_ctx_written<CtxSimTime>());
    EXPECT_FALSE(wv.has_ctx_read<CtxFrameTimeElapsed>());
  }

  // Gets the correct time elapsed without writing
  {
    auto wv = igecs::WorldView::Thin(&world);
    EXPECT_FLOAT_EQ(FrameTimeElapsedUtil::dt(&wv), 1.f);
    EXPECT_FALSE(wv.has_ctx_written<CtxFrameTimeElapsed>());
    EXPECT_FALSE(wv.has_ctx_written<CtxSimTime>());
    EXPECT_TRUE(wv.has_ctx_read<CtxFrameTimeElapsed>());
    EXPECT_FALSE(wv.has_ctx_read<CtxSimTime>());
  }

  {
    auto wv = igecs::WorldView::Thin(&world);
    EXPECT_FLOAT_EQ(FrameTimeElapsedUtil::get_sim_time(&wv), 2.f);
    EXPECT_FALSE(wv.has_ctx_written<CtxFrameTimeElapsed>());
    EXPECT_FALSE(wv.has_ctx_read<CtxFrameTimeElapsed>());
    EXPECT_TRUE(wv.has_ctx_read<CtxSimTime>());
  }
}
