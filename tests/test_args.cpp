#include "args.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("missing filter") {
  Args args;
  std::string_view bad;
  char *argv[] = {(char *)"sj"};
  CHECK(parse_args(1, argv, args, bad) == ParseError::MissingFilter);
}

TEST_CASE("identity filter, no files") {
  Args args;
  std::string_view bad;
  char *argv[] = {(char *)"sj", (char *)"."};
  CHECK(parse_args(2, argv, args, bad) == ParseError::None);
  CHECK(args.filter == ".");
  CHECK(args.files.empty());
  CHECK(args.mode == Mode::Compact);
}

TEST_CASE("conflicting -c and -p") {
  Args args;
  std::string_view bad;
  char *argv[] = {(char *)"sj", (char *)"-c", (char *)"-p", (char *)"."};
  CHECK(parse_args(4, argv, args, bad) == ParseError::Conflict);
}

TEST_CASE("unknown flag keeps token") {
  Args args;
  std::string_view bad;
  char *argv[] = {(char *)"sj", (char *)"--unknown", (char *)"."};
  CHECK(parse_args(3, argv, args, bad) == ParseError::UnknownFlag);
  CHECK(bad == "--unknown");
}

TEST_CASE("compact with two files") {
  Args args;
  std::string_view bad;
  char *argv[] = {(char *)"sj", (char *)"-c", (char *)".", (char *)"f1",
                  (char *)"f2"};
  CHECK(parse_args(5, argv, args, bad) == ParseError::None);
  CHECK(args.mode == Mode::Compact);
  CHECK(args.filter == ".");
  REQUIRE(args.files.size() == 2);
  CHECK(args.files[0] == "f1");
  CHECK(args.files[1] == "f2");
}

TEST_CASE("after --, -c is a file, not a flag") {
  Args args;
  std::string_view bad;
  char *argv[] = {(char *)"sj", (char *)".", (char *)"--", (char *)"-c"};
  CHECK(parse_args(4, argv, args, bad) == ParseError::None);
  CHECK(args.filter == ".");
  REQUIRE(args.files.size() == 1);
  CHECK(args.files[0] == "-c");
}
