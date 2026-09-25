#include "json/escape.h"

#include "json/string.h"

#include <catch2/catch_test_macros.hpp>
#include <random>
#include <string>
#include <string_view>

TEST_CASE("empty string") {
  std::string out = "SENTINEL";
  // через std::string, а не литерал: у SSO-буфера нет известного
  // компилятору размера в 1 байт — нет ложного -Wstringop-overread
  const std::string empty;
  json_escape(empty, out);
  CHECK(out == "SENTINEL");
}

TEST_CASE("plain bytes pass through") {
  std::string out;
  json_escape("abc XYZ 018 .,:;", out);
  CHECK(out == "abc XYZ 018 .,:;");
}

TEST_CASE("quote and backslash") {
  std::string out;
  json_escape("a\"b\\c", out);
  CHECK(out == "a\\\"b\\\\c");
}

TEST_CASE("named escapes") {
  std::string out;
  std::string_view in = "a\bb\fc\nd\re\tf";
  json_escape(in, out);
  CHECK(out == "a\\bb\\fc\\nd\\re\\tf");
}

TEST_CASE("control chars become u00XX") {
  std::string out;
  json_escape(std::string_view("\x01\x0b\x1f", 3), out);
  CHECK(out == "\\u0001\\u000b\\u001f");
}

TEST_CASE("0x20 space is not escaped") {
  std::string out;
  json_escape("a b", out);
  CHECK(out == "a b");
}

TEST_CASE("utf-8 passes through") {
  std::string out;
  json_escape("é😀", out); // \xC3\xA9 \xF0\x9F\x98\x80
  CHECK(out == "é😀");
}

TEST_CASE("mixed content") {
  std::string out;
  json_escape("line1\nline2\t\"end\"", out);
  CHECK(out == "line1\\nline2\\t\\\"end\\\"");
}

TEST_CASE("round-trip: parse_string(escape(s)) == s") {
  std::mt19937 rng(777);
  // Алфавит: обычные байты, кавычка, backslash, управляющие, UTF-8
  const char *alpha = "abX '\"\\\x01\x02\x1f\t\n\r\xC3\xA9\xF0\x9F\x98\x80";
  const auto alpha_len = std::strlen(alpha);
  std::uniform_int_distribution<std::size_t> len_d(0, 60);
  std::uniform_int_distribution<std::size_t> ch_d(0, alpha_len - 1);

  for (int i = 0; i < 5000; ++i) {
    std::string s;
    std::size_t n = len_d(rng);
    for (std::size_t j = 0; j < n; ++j) {
      s.push_back(alpha[ch_d(rng)]);
    }

    std::string enc;
    json_escape(s, enc);
    std::string quoted = "\"" + enc + "\"";

    std::string dec;
    auto r = parse_string(quoted, dec);
    REQUIRE(r.has_value());
    std::string got = r->escaped ? dec : std::string(r->view);
    CHECK(got == s);
    CHECK(r->consumed == quoted.size());
  }
}
