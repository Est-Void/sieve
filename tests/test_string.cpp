#include "json/string.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("empty string") {
  std::string out;
  auto o = parse_string("\"\"", out);
  REQUIRE(o.has_value());
  CHECK(o->view.empty());
  CHECK(o->escaped == false);
  CHECK(o->consumed == 2);
}

TEST_CASE("plain view, out untouched") {
  std::string out = "SENTINEL";
  auto o = parse_string("\"abc\"", out);
  REQUIRE(o.has_value());
  CHECK(o->view == "abc");
  CHECK(o->escaped == false);
  CHECK(o->consumed == 5);
  CHECK(out == "SENTINEL");
}

TEST_CASE("escaped quote") {
  std::string out;
  auto o = parse_string("\"a\\\"b\"", out);
  REQUIRE(o.has_value());
  CHECK(o->escaped == true);
  CHECK(o->view.empty());
  CHECK(out == "a\"b");
  CHECK(o->consumed == 6);
}

TEST_CASE("newline escape") {
  std::string out;
  auto o = parse_string("\"a\\nb\"", out);
  REQUIRE(o.has_value());
  CHECK(o->escaped == true);
  CHECK(out == "a\nb");
  CHECK(o->consumed == 6);
}

TEST_CASE("backslash escape") {
  std::string out;
  auto o = parse_string("\"\\\\\"", out);
  REQUIRE(o.has_value());
  CHECK(out == "\\");
  CHECK(o->consumed == 4);
}

TEST_CASE("unicode escape") {
  std::string out;
  auto o = parse_string("\"\\u0041\"", out);
  REQUIRE(o.has_value());
  CHECK(o->escaped == true);
  CHECK(out == "A");
  CHECK(o->consumed == 8);
}

TEST_CASE("surrogate pair is grinning face") {
  std::string out;
  auto o = parse_string("\"\\uD83D\\uDE00\"", out);
  REQUIRE(o.has_value());
  CHECK(o->escaped == true);
  CHECK(out.size() == 4);
  CHECK(out == "\xF0\x9F\x98\x80");
  CHECK(o->consumed == 14);
}

TEST_CASE("unterminated is empty") {
  std::string out;
  CHECK(!parse_string("\"ab", out).has_value());
}

TEST_CASE("raw newline inside is empty") {
  std::string out;
  CHECK(!parse_string("\"a\nb\"", out).has_value());
}

TEST_CASE("raw tab inside is empty") {
  std::string out;
  CHECK(!parse_string("\"a\tb\"", out).has_value());
}

TEST_CASE("bad hex is empty") {
  std::string out;
  CHECK(!parse_string("\"\\uxyz\"", out).has_value());
}

TEST_CASE("lone high surrogate is empty") {
  std::string out;
  CHECK(!parse_string("\"\\uD83D\"", out).has_value());
}

TEST_CASE("high surrogate without second escape is empty") {
  std::string out;
  CHECK(!parse_string("\"\\uD83D x\"", out).has_value());
}

TEST_CASE("lone low surrogate is empty") {
  std::string out;
  CHECK(!parse_string("\"\\uDE00\"", out).has_value());
}

TEST_CASE("unknown escape is empty") {
  std::string out;
  CHECK(!parse_string("\"\\x\"", out).has_value());
}

TEST_CASE("no opening quote is empty") {
  std::string out;
  CHECK(!parse_string("nul", out).has_value());
}

TEST_CASE("unterminated nul is empty") {
  std::string out;
  CHECK(!parse_string("\"nul", out).has_value());
}

TEST_CASE("leading spaces") {
  std::string out = "SENTINEL";
  auto o = parse_string("   \"x\"", out);
  REQUIRE(o.has_value());
  CHECK(o->view == "x");
  CHECK(o->escaped == false);
  CHECK(o->consumed == 6);
  CHECK(out == "SENTINEL");
}

TEST_CASE("leading spaces with escape") {
  std::string out;
  auto o = parse_string("  \"a\\nb\"", out);
  REQUIRE(o.has_value());
  CHECK(o->escaped == true);
  CHECK(out == "a\nb");
  CHECK(o->consumed == 8);
}
