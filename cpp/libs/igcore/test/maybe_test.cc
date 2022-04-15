#include <gtest/gtest.h>
#include <igcore/maybe.h>

#include <string>

using indigo::core::Maybe;

namespace {
struct NonCopyable {
  int Value;

  NonCopyable(int v) : Value(v) {}
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable(NonCopyable&& o) noexcept : Value(o.Value) { o.Value = 0u; }

  NonCopyable& operator=(int v) {
    Value = v;
    return *this;
  }
};

Maybe<int> half_if_even(int n) {
  if (n % 2 == 0) {
    return Maybe<int>(n / 2);
  }
  return Maybe<int>::empty();
}

}  // namespace

TEST(Maybe, InitializesEmpty) {
  Maybe<int> empty;
  EXPECT_FALSE(empty.has_value());
}

TEST(Maybe, InitializesWithCopyValue) {
  Maybe<int> copied(5);
  EXPECT_TRUE(copied.has_value());
}

TEST(Maybe, InitializesWithMovedValue) {
  NonCopyable nc(5);
  Maybe<NonCopyable> copied(std::move(nc));
  EXPECT_TRUE(copied.has_value());
  EXPECT_EQ(nc.Value, 0);
}

TEST(Maybe, GetsCopiedValue) {
  Maybe<int> val(5);
  EXPECT_EQ(val.get(), 5);
}

TEST(Maybe, GetsMovedValue) {
  Maybe<NonCopyable> val(NonCopyable(5));

  NonCopyable nc = val.move();
  EXPECT_EQ(nc.Value, 5);
  EXPECT_FALSE(val.has_value());
}

TEST(Maybe, MapsCopyableToCopyable) {
  Maybe<int> original = 5;
  Maybe<std::string> stringified =
      original.map<std::string>([](int i) { return std::to_string(i); });
  EXPECT_TRUE(original.has_value());
  EXPECT_TRUE(stringified.has_value());
  EXPECT_EQ(original.get(), 5);
  EXPECT_EQ(stringified.get(), "5");
}

TEST(Maybe, MapsCopyableToMovable) {
  Maybe<int> original = 5;
  Maybe<NonCopyable> wrapped =
      original.map<NonCopyable>([](int o) { return NonCopyable(o); });
  EXPECT_TRUE(original.has_value());
  EXPECT_TRUE(wrapped.has_value());
  EXPECT_EQ(original.get(), 5);
  EXPECT_EQ(wrapped.get().Value, 5);
}

TEST(Maybe, MapsEmptyToEmpty) {
  Maybe<int> original;
  auto copy = original.map<int>([](int i) { return i * 2; });

  EXPECT_FALSE(original.has_value());
  EXPECT_FALSE(copy.has_value());
}

TEST(Maybe, MoveMapsSuccessfully) {
  Maybe<NonCopyable> original = NonCopyable{5};
  auto copy = original.map_move<NonCopyable>(
      [](NonCopyable&& c) { return NonCopyable(c.Value * 2); });

  EXPECT_FALSE(original.has_value());
  EXPECT_TRUE(copy.has_value());
  EXPECT_EQ(copy.get().Value, 10);
}

TEST(Maybe, MoveMapsOnEmptySuccessfully) {
  Maybe<NonCopyable> original;
  auto copy = original.map_move<NonCopyable>(
      [](NonCopyable&& c) { return NonCopyable(c.Value * 2); });

  EXPECT_FALSE(original.has_value());
  EXPECT_FALSE(copy.has_value());
}

TEST(Maybe, OrElseReturnsValueOfFilledMaybe) {
  Maybe<int> original = 5;
  EXPECT_EQ(original.or_else(10), 5);
}

TEST(Maybe, OrElseReturnsDefaultOfEmptyMaybe) {
  Maybe<int> original;
  EXPECT_EQ(original.or_else(10), 10);
}

TEST(Maybe, FlatMapReturnsValue) {
  Maybe<int> original = 4;
  auto mapped = original.flat_map<int>(::half_if_even);

  EXPECT_TRUE(original.has_value());
  EXPECT_EQ(original.get(), 4);
  EXPECT_TRUE(mapped.has_value());
  EXPECT_EQ(mapped.get(), 2);
}

TEST(Maybe, FlatMapReturnsEmpty) {
  Maybe<int> original = 3;
  auto mapped = original.flat_map<int>(::half_if_even);

  EXPECT_TRUE(original.has_value());
  EXPECT_EQ(original.get(), 3);
  EXPECT_FALSE(mapped.has_value());
}