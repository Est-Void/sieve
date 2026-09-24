#include "args.h"

ParseError parse_args(int argc, char *argv[], Args &args,
                      std::string_view &bad_token) {
  bool only_files = false;
  bool seen_c = false;
  bool seen_p = false;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (!only_files) {
      if (arg == "-h" || arg == "--help") {
        args.help = true;
        continue;
      } else if (arg == "-c" || arg == "--compact") {
        args.mode = Mode::Compact;
        seen_c = true;
        continue;
      } else if (arg == "-p" || arg == "--pretty") {
        args.mode = Mode::Pretty;
        seen_p = true;
        continue;
      } else if (arg == "--") {
        only_files = true;
        continue;
      } else if (arg.size() > 1 && arg[0] == '-') {
        bad_token = arg;
        return ParseError::UnknownFlag;
      }
    }
    if (args.filter.empty()) {
      args.filter = arg;
    } else {
      args.files.emplace_back(arg);
    }
  }
  if (args.help) {
    return ParseError::None;
  }
  if (args.filter.empty()) {
    bad_token = {};
    return ParseError::MissingFilter;
  }
  if (seen_c && seen_p) {
    bad_token = {};
    return ParseError::Conflict;
  }
  if (args.filter != ".") {
    bad_token = args.filter;
    return ParseError::BadFilter;
  }
  return ParseError::None;
}

void print_usage(std::ostream &os) {
  os << "Usage: sj [-c|--compact|-p|--pretty] [-h|--help] <filter> [file...]\n";
}