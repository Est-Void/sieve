#pragma once
#ifndef ARGS_H
#define ARGS_H

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

enum class Mode { Compact, Pretty };

struct Args {
  std::string_view filter;
  Mode mode = Mode::Compact;
  std::vector<std::string> files;
  bool help = false;
};

enum class ParseError { None, MissingFilter, BadFilter, UnknownFlag, Conflict };

ParseError parse_args(int argc, char *argv[], Args &args,
                      std::string_view &bad_token);

void print_usage(std::ostream &os);

#endif // ARGS_H    