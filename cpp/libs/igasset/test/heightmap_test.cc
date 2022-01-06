#include <gtest/gtest.h>
#include <igasset/heightmap.h>

using namespace indigo;
using namespace asset;

// TODO (sessamekesh): Write unit tests for the heightmap in here, especially
//  around navigation.
// TODO (sessamekesh): You might want some "smoothing" too, because you'll be
//  sampling geometry on top of the heightmap? Or some sort of heightmap rescale
//  algorithm? Dunno.

TEST(Heightmap, CorrectValuesOnExactPixelValues) {
  pb::HeightmapData data;
  data.set_min_x(-10.f);
  data.set_max_x(10.f);
  data.set_min_y(-15.f);
  data.set_max_y(15.f);
  data.set_min_z(-20.f);
  data.set_max_z(20.f);
  data.set_data_width(2);
  data.set_data_height(2);

  {
    std::string raw_data;
    raw_data.resize(4);
    raw_data[0] = 0;
    raw_data[1] = 50;
    raw_data[2] = 100;
    raw_data[3] = 150;
    data.set_data(std::move(raw_data));
  }

  auto rsl = Heightmap::Create(data);
  ASSERT_TRUE(rsl.is_left());

  auto heightmap = rsl.left_move();

  float ul_corner_expected = -15.f;
  float ul_corner_actual = heightmap.height_at({-10.f, -20.f});
  EXPECT_FLOAT_EQ(ul_corner_actual, ul_corner_expected);

  float ur_corner_expected = (50.f / 255.f) * 30.f - 15.f;
  float ur_corner_actual = heightmap.height_at({10.f, -20.f});
  EXPECT_FLOAT_EQ(ur_corner_expected, ur_corner_actual);

  float ll_corner_expected = (100.f / 255.f) * 30.f - 15.f;
  float ll_corner_actual = heightmap.height_at({-10.f, 20.f});
  EXPECT_FLOAT_EQ(ll_corner_expected, ll_corner_actual);

  float lr_corner_expected = (150.f / 255.f) * 30.f - 15.f;
  float lr_corner_actual = heightmap.height_at({10.f, 20.f});
  EXPECT_FLOAT_EQ(lr_corner_expected, lr_corner_actual);
}

TEST(Heightmap, CorrectlyLerpsValues) {
  pb::HeightmapData data;
  data.set_min_x(-10.f);
  data.set_max_x(10.f);
  data.set_min_y(-15.f);
  data.set_max_y(15.f);
  data.set_min_z(-20.f);
  data.set_max_z(20.f);
  data.set_data_width(2);
  data.set_data_height(2);

  {
    std::string raw_data;
    raw_data.resize(4);
    raw_data[0] = 0;
    raw_data[1] = 50;
    raw_data[2] = 100;
    raw_data[3] = 150;
    data.set_data(std::move(raw_data));
  }

  auto rsl = Heightmap::Create(data);
  ASSERT_TRUE(rsl.is_left());

  auto heightmap = rsl.left_move();

  float top_middle_expected = (25.f / 255.f) * 30.f - 15.f;
  float top_middle_actual = heightmap.height_at({0.f, -20.f});
  EXPECT_FLOAT_EQ(top_middle_actual, top_middle_expected);

  float bottom_middle_expected = (125.f / 255.f) * 30.f - 15.f;
  float bottom_middle_actual = heightmap.height_at({0.f, 20.f});
  EXPECT_NEAR(bottom_middle_actual, bottom_middle_expected, 0.0001f);

  float left_middle_expected = (50.f / 255.f) * 30.f - 15.f;
  float left_middle_actual = heightmap.height_at({-10.f, 0.f});
  EXPECT_FLOAT_EQ(left_middle_actual, left_middle_expected);

  float right_middle_expected = (100.f / 255.f) * 30.f - 15.f;
  float right_middle_actual = heightmap.height_at({10.f, 0.f});
  EXPECT_FLOAT_EQ(right_middle_expected, right_middle_actual);
}

TEST(Heightmap, LerpsMiddleValues) {
  pb::HeightmapData data;
  data.set_min_x(-10.f);
  data.set_max_x(10.f);
  data.set_min_y(-15.f);
  data.set_max_y(15.f);
  data.set_min_z(-20.f);
  data.set_max_z(20.f);
  data.set_data_width(2);
  data.set_data_height(2);

  {
    std::string raw_data;
    raw_data.resize(4);
    raw_data[0] = 0;
    raw_data[1] = 50;
    raw_data[2] = 100;
    raw_data[3] = 150;
    data.set_data(std::move(raw_data));
  }

  auto rsl = Heightmap::Create(data);
  ASSERT_TRUE(rsl.is_left());

  auto heightmap = rsl.left_move();

  float true_middle_expected = (75.f / 255.f) * 30.f - 15.f;
  float true_middle_actual = heightmap.height_at({0.f, 0.f});
  EXPECT_FLOAT_EQ(true_middle_actual, true_middle_expected);

  float top_left_quarter_offset_expected = (37.5f / 255.f) * 30.f - 15.f;
  float top_left_quarter_offset_actual = heightmap.height_at({-5.f, -10.f});
  EXPECT_FLOAT_EQ(top_left_quarter_offset_actual,
                  top_left_quarter_offset_expected);
}