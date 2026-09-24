#include "json/value.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("dispatch null") {
  std::string buf;
  auto v = parse_value("null", buf);
  REQUIRE(v.has_value());
  CHECK(v->type == ValueType::Null);
  CHECK(v->scalar.type == ScalarType::Null);
  CHECK(v->consumed == 4);
}

TEST_CASE("dispatch bool") {
  std::string buf;
  auto v = parse_value("  true", buf);
  REQUIRE(v.has_value());
  CHECK(v->type == ValueType::Bool);
  CHECK(v->scalar.boolean == true);
  CHECK(v->consumed == 6);
}

TEST_CASE("dispatch number") {
  std::string buf;
  auto v = parse_value("-12.5", buf);
  REQUIRE(v.has_value());
  CHECK(v->type == ValueType::Number);
  CHECK(v->scalar.number == Catch::Approx(-12.5));
  CHECK(v->consumed == 5);
}

TEST_CASE("string with brackets inside is string") {
  std::string buf = "SENTINEL";
  auto v = parse_value("\"x[{}]\"", buf);
  REQUIRE(v.has_value());
  CHECK(v->type == ValueType::String);
  CHECK(v->str.view == "x[{}]");
  CHECK(v->str.escaped == false);
  CHECK(v->scalar.view == "x[{}]");
  CHECK(v->consumed == 7);
  CHECK(buf == "SENTINEL");
}

TEST_CASE("escaped string lands in strout") {
  std::string buf;
  auto v = parse_value("\"a\\nb\"", buf);
  REQUIRE(v.has_value());
  CHECK(v->type == ValueType::String);
  CHECK(v->str.escaped == true);
  CHECK(v->str.view.empty());
  CHECK(buf == "a\nb");
  CHECK(v->consumed == 6);
}

TEST_CASE("skip flat array") {
  auto n = skip_value("[1,2]");
  REQUIRE(n.has_value());
  CHECK(*n == 5);
}

TEST_CASE("skip flat object") {
  auto n = skip_value("{\"a\":1}");
  REQUIRE(n.has_value());
  CHECK(*n == 7);
}

TEST_CASE("skip nested with brackets in string") {
  std::string_view in = "{\"a\":[1,{\"b\":\"x[{}]\"}]}";
  auto n = skip_value(in);
  REQUIRE(n.has_value());
  CHECK(*n == in.size());
  std::string buf;
  auto v = parse_value(in, buf);
  REQUIRE(v.has_value());
  CHECK(v->type == ValueType::Object);
  CHECK(v->consumed == in.size());
}

TEST_CASE("unclosed array is empty") {
  CHECK(!skip_value("[1,").has_value());
  std::string buf;
  CHECK(!parse_value("[1,", buf).has_value());
}

TEST_CASE("unclosed object is empty") {
  CHECK(!skip_value("{\"a\":").has_value());
  std::string buf;
  CHECK(!parse_value("{\"a\":", buf).has_value());
}

TEST_CASE("lone closer is empty") {
  CHECK(!skip_value("]").has_value());
  CHECK(!skip_value("}").has_value());
  std::string buf;
  CHECK(!parse_value("]", buf).has_value());
}

TEST_CASE("depth over limit is empty") {
  std::string in(70, '[');
  in += std::string(70, ']');
  CHECK(!skip_value(in).has_value());
}

TEST_CASE("depth at limit passes") {
  std::string in(64, '[');
  in += std::string(64, ']');
  auto n = skip_value(in);
  REQUIRE(n.has_value());
  CHECK(*n == 128);
}

TEST_CASE("parse_value eats leading spaces only") {
  std::string buf;
  auto v = parse_value("  [1]  ", buf);
  REQUIRE(v.has_value());
  CHECK(v->type == ValueType::Array);
  CHECK(v->consumed == 5);
}

TEST_CASE("skip_value eats both edges") {
  auto n = skip_value("  [1]  ");
  REQUIRE(n.has_value());
  CHECK(*n == 7);
}

TEST_CASE("skip scalar with edges") {
  auto n = skip_value("  null  ");
  REQUIRE(n.has_value());
  CHECK(*n == 8);
}

TEST_CASE("garbage is empty") {
  CHECK(!skip_value("").has_value());
  CHECK(!skip_value("   ").has_value());
  std::string buf;
  CHECK(!parse_value("", buf).has_value());
  CHECK(!parse_value("nul", buf).has_value());
}
