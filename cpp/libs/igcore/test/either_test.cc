#include <gtest/gtest.h>
#include <igcore/either.h>

using namespace indigo::core;
using indigo::core::Either;

namespace {
struct NonCopyable {
  int Value;

  NonCopyable(int v) : Value(v) {}
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable(NonCopyable&& o) noexcept : Value(o.Value) { o.Value = 0u; }
};
}  // namespace

TEST(Either, InitializesLeft) {
  Either<int, std::string> lval = left(5);
  EXPECT_TRUE(lval.is_left());
  EXPECT_FALSE(lval.is_right());
  EXPECT_EQ(lval.get_left(), 5);
}

TEST(Either, InitializesRight) {
  Either<int, std::string> rval = right<std::string>("foobar");
  EXPECT_FALSE(rval.is_left());
  EXPECT_TRUE(rval.is_right());
  EXPECT_EQ(rval.get_right(), "foobar");
}

TEST(Either, MoveLeft) {
  Either<std::string, std::string> lval = left<std::string>("foobar");
  EXPECT_TRUE(lval.is_left());

  auto mv = lval.left_move();
  EXPECT_EQ(mv, "foobar");
  EXPECT_EQ(lval.get_left(), "");
}

TEST(Either, MoveRight) {
  Either<std::string, std::string> lval = right<std::string>("foobar");
  EXPECT_TRUE(lval.is_right());

  auto mv = lval.right_move();
  EXPECT_EQ(mv, "foobar");
  EXPECT_EQ(lval.get_right(), "");
}

TEST(Either, MaybeLeftOnValue) {
  Either<int, std::string> lval = left(1);
  auto maybe = lval.maybe_left();
  EXPECT_TRUE(maybe.has_value());
  EXPECT_EQ(maybe.get(), 1);
}

TEST(Either, MaybeLeftOnEmpty) {
  Either<int, std::string> rval = right<std::string>("hello");
  auto maybe = rval.maybe_left();
  EXPECT_FALSE(maybe.has_value());
}

TEST(Either, MaybeRightOnValue) {
  Either<int, std::string> lval = right<std::string>("hello");
  auto maybe = lval.maybe_right();
  EXPECT_TRUE(maybe.has_value());
  EXPECT_EQ(maybe.get(), "hello");
}

TEST(Either, MaybeRightOnEmpty) {
  Either<int, std::string> rval = left(0);
  auto maybe = rval.maybe_right();
  EXPECT_FALSE(maybe.has_value());
}

TEST(Either, MaybeLeftMoveOnValue) {
  Either<NonCopyable, std::string> lval = left(NonCopyable(5));
  auto maybe = lval.maybe_left_move();
  EXPECT_TRUE(maybe.has_value());
  EXPECT_EQ(maybe.get().Value, 5);
  EXPECT_EQ(lval.get_left().Value, 0);
}

TEST(Either, MaybeLeftMoveOnEmpty) {
  Either<NonCopyable, std::string> lval = right<std::string>("foobar");
  auto maybe = lval.maybe_left_move();
  EXPECT_FALSE(maybe.has_value());
}

TEST(Either, MaybeRightMoveOnValue) {
  Either<NonCopyable, NonCopyable> rval = right(NonCopyable(5));
  auto maybe = rval.maybe_right_move();
  EXPECT_TRUE(maybe.has_value());
  EXPECT_EQ(maybe.get().Value, 5);
  EXPECT_EQ(rval.get_right().Value, 0);
}

TEST(Either, MaybeRightMoveOnEmpty) {
  Either<NonCopyable, NonCopyable> rval = left(NonCopyable(5));
  auto maybe = rval.maybe_right_move();
  EXPECT_FALSE(maybe.has_value());
  EXPECT_EQ(rval.get_left().Value, 5);
}

TEST(Either, MapLeftOnValue) {
  Either<int, std::string> lval = left(5);
  auto doubled = lval.map_left<int>([](int d) { return d * 2; });
  EXPECT_TRUE(doubled.is_left());
  EXPECT_EQ(doubled.get_left(), 10);
}

TEST(Either, MapLeftOnEmpty) {
  Either<int, std::string> lval = right<std::string>("hello");
  auto doubled = lval.map_left<int>([](int d) { return d * 2; });
  EXPECT_FALSE(doubled.is_left());
}

TEST(Either, MapRightOnValue) {
  Either<int, std::string> rval = right<std::string>("hello");
  auto appended =
      rval.map_right<std::string>([](std::string v) { return v + ", world!"; });
  EXPECT_TRUE(appended.is_right());
  EXPECT_EQ(appended.get_right(), "hello, world!");
}

TEST(Either, MapRightOnEmpty) {
  Either<int, std::string> rval = left(4);
  auto appended =
      rval.map_right<std::string>([](std::string v) { return v + ", world!"; });
  EXPECT_FALSE(appended.is_right());
}