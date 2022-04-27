#include "arena_camera.h"

#include <gtest/gtest.h>

#include <glm/gtx/matrix_decompose.hpp>

using namespace sanctify;
using namespace logic;

TEST(ArenaCamera, TiltGoesFromTopDown) {
  ArenaCamera camera(glm::vec3(0.f, 0.f, 0.f), glm::radians(30.f), 0.f, 10.f);

  EXPECT_GT(glm::dot(camera.position(), glm::vec3(0.f, 1.f, 0.f)), 0.5f);
}

TEST(ArenaCamera, SpinStartsAtNegativeZ) {
  ArenaCamera camera(glm::vec3(0.f, 0.f, 0.f), glm::radians(90.f), 0.f, 10.f);

  EXPECT_LT(glm::length(camera.position() - glm::vec3(0.f, 0.f, -10.f)),
            0.00001f);
}

TEST(ArenaCamera, SpinGoesTowardsPositiveX) {
  ArenaCamera camera(glm::vec3(0.f, 0.f, 0.f), glm::radians(90.f),
                     glm::radians(30.f), 10.f);

  glm::vec3 position = camera.position();

  EXPECT_GT(position.x, 0.f);
  EXPECT_GT(position.z, -10.f);
}
