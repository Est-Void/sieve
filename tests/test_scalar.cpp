#include "json/scalar.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("null") {
  auto o = parse_scalar("null");
  REQUIRE(o.has_value());
  CHECK(o->type == ScalarType::Null);
  CHECK(o->consumed == 4);
}

TEST_CASE("true with leading spaces") {
  auto o = parse_scalar("  true");
  REQUIRE(o.has_value());
  CHECK(o->type == ScalarType::Boolean);
  CHECK(o->boolean == true);
  CHECK(o->consumed == 6);
}

TEST_CASE("false") {
  auto o = parse_scalar("false");
  REQUIRE(o.has_value());
  CHECK(o->type == ScalarType::Boolean);
  CHECK(o->boolean == false);
  CHECK(o->consumed == 5);
}

TEST_CASE("zero") {
  auto o = parse_scalar("0");
  REQUIRE(o.has_value());
  CHECK(o->type == ScalarType::Number);
  CHECK(o->number == Catch::Approx(0.0));
  CHECK(o->consumed == 1);
}

TEST_CASE("negative zero") {
  auto o = parse_scalar("-0");
  REQUIRE(o.has_value());
  CHECK(o->type == ScalarType::Number);
  CHECK(o->number == Catch::Approx(0.0));
  CHECK(o->consumed == 2);
}

TEST_CASE("integer") {
  auto o = parse_scalar("123");
  REQUIRE(o.has_value());
  CHECK(o->type == ScalarType::Number);
  CHECK(o->number == Catch::Approx(123.0));
  CHECK(o->consumed == 3);
}

TEST_CASE("negative frac with exponent") {
  auto o = parse_scalar("-12.5e3");
  REQUIRE(o.has_value());
  CHECK(o->type == ScalarType::Number);
  CHECK(o->number == Catch::Approx(-12500.0));
  CHECK(o->consumed == 7);
}

TEST_CASE("fraction") {
  auto o = parse_scalar("0.5");
  REQUIRE(o.has_value());
  CHECK(o->type == ScalarType::Number);
  CHECK(o->number == Catch::Approx(0.5));
  CHECK(o->consumed == 3);
}

TEST_CASE("negative exponent") {
  auto o = parse_scalar("1e-2");
  REQUIRE(o.has_value());
  CHECK(o->type == ScalarType::Number);
  CHECK(o->number == Catch::Approx(0.01));
  CHECK(o->consumed == 4);
}

TEST_CASE("truncated literal is empty") {
  CHECK(!parse_scalar("nulx").has_value());
  CHECK(!parse_scalar("tru").has_value());
}

TEST_CASE("literal boundary is classic") {
  CHECK(!parse_scalar("nullx").has_value());
  CHECK(!parse_scalar("true_").has_value());
  CHECK(!parse_scalar("false1").has_value());
}

TEST_CASE("leading zero is classic") {
  CHECK(!parse_scalar("01").has_value());
  CHECK(parse_scalar("0").has_value());
  CHECK(parse_scalar("0.5").has_value());
}

TEST_CASE("dot without fraction digits is empty") {
  CHECK(!parse_scalar("1.").has_value());
}

TEST_CASE("plus is not a number") {
  CHECK(!parse_scalar("+1").has_value());
}

TEST_CASE("empty input is empty") {
  CHECK(!parse_scalar("").has_value());
}

TEST_CASE("only spaces is empty") {
  CHECK(!parse_scalar("  ").has_value());
}
