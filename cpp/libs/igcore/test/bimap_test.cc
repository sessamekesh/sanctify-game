#include <gtest/gtest.h>
#include <igcore/bimap.h>

using namespace indigo;
using namespace core;

struct TestL {
  int Value;

  bool operator<(const TestL& o) const { return Value < o.Value; }
};

struct TestR {
  int Value;

  bool operator<(const TestR& o) const { return Value < o.Value; }
};

TEST(Bimap, InitializesLeft) {
  Bimap<TestL, TestR> bimap;

  EXPECT_TRUE(bimap.find_l(TestL{1}) == bimap.end());
  EXPECT_TRUE(bimap.find_r(TestR{5}) == bimap.end());

  bimap.insert(TestL{1}, TestR{5});
  auto lrsl = bimap.find_l(TestL{1});
  auto rrsl = bimap.find_r(TestR{5});

  ASSERT_TRUE(lrsl != bimap.end());
  ASSERT_TRUE(rrsl != bimap.end());

  EXPECT_EQ(lrsl->Value, 5);
  EXPECT_EQ(rrsl->Value, 1);
}

TEST(Bimap, IteratesAllElements) {
  Bimap<TestL, TestR> bimap;

  bimap.insert(TestL{1}, TestR{2});
  bimap.insert(TestL{2}, TestR{4});
  bimap.insert(TestL{3}, TestR{6});

  bool has_1 = false, has_2 = false, has_3 = false;
  uint32_t size = 0;

  for (auto& [l, r] : bimap) {
    size++;
    if (l.Value == 1 && r.Value == 2) {
      has_1 = true;
    } else if (l.Value == 2 && r.Value == 4) {
      has_2 = true;
    } else if (l.Value == 3 && r.Value == 6) {
      has_3 = true;
    }
  }

  EXPECT_TRUE(has_1);
  EXPECT_TRUE(has_2);
  EXPECT_TRUE(has_3);
  EXPECT_EQ(size, 3);
}

TEST(Bimap, LeftIterateWorks) {
  Bimap<TestL, TestR> bimap;

  bimap.insert(TestL{1}, TestR{2});
  bimap.insert(TestL{2}, TestR{4});
  bimap.insert(TestL{3}, TestR{6});

  bool has_1 = false, has_2 = false, has_3 = false;
  uint32_t size = 0;

  for (auto& l : bimap.left_values()) {
    size++;
    if (l.Value == 1) {
      has_1 = true;
    } else if (l.Value == 2) {
      has_2 = true;
    } else if (l.Value == 3) {
      has_3 = true;
    }
  }

  EXPECT_TRUE(has_1);
  EXPECT_TRUE(has_2);
  EXPECT_TRUE(has_3);
  EXPECT_EQ(size, 3);
}

TEST(Bimap, RightIterateWorks) {
  Bimap<TestL, TestR> bimap;

  bimap.insert(TestL{1}, TestR{2});
  bimap.insert(TestL{2}, TestR{4});
  bimap.insert(TestL{3}, TestR{6});

  bool has_1 = false, has_2 = false, has_3 = false;
  uint32_t size = 0;

  for (auto& l : bimap.right_values()) {
    size++;
    if (l.Value == 2) {
      has_1 = true;
    } else if (l.Value == 4) {
      has_2 = true;
    } else if (l.Value == 6) {
      has_3 = true;
    }
  }

  EXPECT_TRUE(has_1);
  EXPECT_TRUE(has_2);
  EXPECT_TRUE(has_3);
  EXPECT_EQ(size, 3);
}

TEST(Bimap, ErasesLeft) {
  Bimap<TestL, TestR> bimap;

  bimap.insert(TestL{1}, TestR{2});
  bimap.insert(TestL{2}, TestR{4});
  bimap.insert(TestL{3}, TestR{6});

  ASSERT_EQ(bimap.size(), 3);

  size_t empty_erase = bimap.erase_l(TestL{5});
  ASSERT_EQ(bimap.size(), 3);
  ASSERT_EQ(empty_erase, 0);

  size_t middle_erase = bimap.erase_l(TestL{2});
  ASSERT_EQ(bimap.size(), 2);
  ASSERT_EQ(middle_erase, 1);
  ASSERT_TRUE(bimap.find_l(TestL{1}) != bimap.end());
  ASSERT_FALSE(bimap.find_l(TestL{2}) != bimap.end());
  ASSERT_TRUE(bimap.find_l(TestL{3}) != bimap.end());

  size_t beg_erase = bimap.erase_l(TestL{1});
  size_t end_erase = bimap.erase_l(TestL{3});
  ASSERT_EQ(bimap.size(), 0);
  ASSERT_EQ(beg_erase, 1);
  ASSERT_EQ(end_erase, 1);

  bimap.insert(TestL{1}, TestR{10});
  EXPECT_EQ(bimap.size(), 1);
}

TEST(Bimap, ErasesRight) {
  Bimap<TestL, TestR> bimap;

  bimap.insert(TestL{1}, TestR{2});
  bimap.insert(TestL{2}, TestR{4});
  bimap.insert(TestL{3}, TestR{6});

  ASSERT_EQ(bimap.size(), 3);

  size_t empty_erase = bimap.erase_r(TestR{5});
  ASSERT_EQ(bimap.size(), 3);
  ASSERT_EQ(empty_erase, 0);

  size_t middle_erase = bimap.erase_r(TestR{4});
  ASSERT_EQ(bimap.size(), 2);
  ASSERT_EQ(middle_erase, 1);
  ASSERT_TRUE(bimap.find_r(TestR{2}) != bimap.end());
  ASSERT_FALSE(bimap.find_r(TestR{4}) != bimap.end());
  ASSERT_TRUE(bimap.find_r(TestR{6}) != bimap.end());

  size_t beg_erase = bimap.erase_r(TestR{2});
  size_t end_erase = bimap.erase_r(TestR{6});
  ASSERT_EQ(bimap.size(), 0);
  ASSERT_EQ(beg_erase, 1);
  ASSERT_EQ(end_erase, 1);

  bimap.insert(TestL{1}, TestR{10});
  EXPECT_EQ(bimap.size(), 1);
}

TEST(Bimap, InsertDoesNotUpdateValue) {
  Bimap<TestL, TestR> bimap;
  bimap.insert(TestL{1}, TestR{2});

  bimap.insert(TestL{2}, TestR{2});
  ASSERT_NE(bimap.find_l(TestL{1}), bimap.end());
  EXPECT_EQ(bimap.find_l(TestL{1})->Value, 2);
  EXPECT_EQ(bimap.find_l(TestL{2}), bimap.end());

  bimap.insert(TestL{1}, TestR{3});
  ASSERT_NE(bimap.find_r(TestR{2}), bimap.end());
  EXPECT_EQ(bimap.find_r(TestR{2})->Value, 1);
  EXPECT_EQ(bimap.find_r(TestR{3}), bimap.end());

  ASSERT_EQ(bimap.size(), 1);
}

TEST(Bimap, InsertOrUpdateUpdatesValues) {
  Bimap<TestL, TestR> bimap;
  bimap.insert(TestL{1}, TestR{10});
  bimap.insert(TestL{2}, TestR{20});

  bimap.insert_or_update(TestL{100}, TestR{10});
  ASSERT_EQ(bimap.size(), 2);
  ASSERT_EQ(bimap.find_l(TestL{1}), bimap.end());
  ASSERT_NE(bimap.find_l(TestL{100}), bimap.end());
  EXPECT_EQ(bimap.find_l(TestL{100})->Value, 10);

  bimap.insert_or_update(TestL{2}, TestR{200});
  ASSERT_EQ(bimap.size(), 2);
  ASSERT_EQ(bimap.find_r(TestR{20}), bimap.end());
  ASSERT_NE(bimap.find_r(TestR{200}), bimap.end());
  EXPECT_EQ(bimap.find_r(TestR{200})->Value, 2);
}

TEST(Bimap, InsertOrUpdateHandlesDoubleConflict) {
  Bimap<TestL, TestR> bimap;
  bimap.insert(TestL{1}, TestR{10});
  bimap.insert(TestL{2}, TestR{20});

  // Double conflic - entries already exist for L1 and R20
  bimap.insert_or_update(TestL{1}, TestR{20});
  EXPECT_EQ(bimap.size(), 1);
  EXPECT_EQ(bimap.find_l(TestL{2}), bimap.end());
  EXPECT_EQ(bimap.find_r(TestR{10}), bimap.end());

  ASSERT_NE(bimap.find_l(TestL{1}), bimap.end());
  ASSERT_NE(bimap.find_r(TestR{20}), bimap.end());

  EXPECT_EQ(bimap.find_l(TestL{1})->Value, 20);
  EXPECT_EQ(bimap.find_r(TestR{20})->Value, 1);
}