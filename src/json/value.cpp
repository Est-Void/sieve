#include "json/value.h"

#include "json/detail.h"

namespace {

constexpr std::size_t kMaxDepth = 64;

constexpr std::size_t npos = std::string_view::npos;

struct ByteClass {
  unsigned char v[256];
  ByteClass() : v{} {
    v[static_cast<unsigned char>('"')] = 1;
    v[static_cast<unsigned char>('[')] = 2;
    v[static_cast<unsigned char>('{')] = 2;
    v[static_cast<unsigned char>(']')] = 3;
    v[static_cast<unsigned char>('}')] = 3;
    for (int c = 0; c < 0x20; ++c) {
      if (!detail::is_ws(static_cast<char>(c))) {
        v[c] = 4;
      }
    }
  }
};
const ByteClass kByteClass{};

std::size_t string_end(std::string_view in, std::size_t p) noexcept {
  std::size_t from = p + 1;
  while (true) {
    std::size_t q = detail::find_qb(in, from);
    if (q == npos) {
      return npos;
    }
    if (in[q] == '"') {
      return q + 1;
    }
    from = q + 2; // перепрыгнуть backslash + следующий символ
  }
}

std::optional<std::size_t> skip_composite(std::string_view in,
                                          std::size_t open) noexcept {
  std::size_t depth = 0;
  std::size_t p = open;
  while (p < in.size()) {
    switch (kByteClass.v[static_cast<unsigned char>(in[p])]) {
    case 0:
      ++p;
      continue;
    case 1: {
      p = string_end(in, p);
      if (p == npos) {
        return std::nullopt; 
      }
      continue;
    }
    case 2:
      ++depth;
      if (depth > kMaxDepth) {
        return std::nullopt;
      }
      ++p;
      continue;
    case 3:
      if (depth == 0) {
        return std::nullopt; 
      }
      --depth;
      ++p;
      if (depth == 0) {
        return p - open; 
      }
      continue;
    default:
      return std::nullopt; 
    }
  }
  return std::nullopt;
}

} // namespace

std::optional<std::size_t> skip_value(std::string_view in) noexcept {
  std::size_t off = 0;
  while (off < in.size() && detail::is_ws(in[off])) {
    ++off;
  }
  if (off >= in.size()) {
    return std::nullopt;
  }
  std::size_t len = 0;
  char c = in[off];
  if (c == '"') {
    std::size_t e = string_end(in, off);
    if (e == npos) {
      return std::nullopt;
    }
    len = e - off;
  } else if (c == '[' || c == '{') {
    auto l = skip_composite(in, off);
    if (!l) {
      return std::nullopt;
    }
    len = *l;
  } else {
    auto s = parse_scalar(in.substr(off));
    if (!s) {
      return std::nullopt;
    }
    len = s->consumed;
  }
  std::size_t p = off + len;
  while (p < in.size() && detail::is_ws(in[p])) {
    ++p;
  }
  return p;
}

std::optional<Value> parse_value(std::string_view in,
                                 std::string& strout) noexcept {
  std::size_t off = 0;
  while (off < in.size() && detail::is_ws(in[off])) {
    ++off;
  }
  if (off >= in.size()) {
    return std::nullopt;
  }
  char c = in[off];
  if (c == '[' || c == '{') {
    auto l = skip_composite(in, off);
    if (!l) {
      return std::nullopt;
    }
    Value v{};
    v.type = (c == '[') ? ValueType::Array : ValueType::Object;
    v.consumed = off + *l; 
    return v;
  }
  if (c == '"') {
    auto s = parse_string(in.substr(off), strout);
    if (!s) {
      return std::nullopt;
    }
    Value v{};
    v.type = ValueType::String;
    v.str = *s;
    v.scalar.type = ScalarType::String;
    v.scalar.view = s->view; 
    v.consumed = off + s->consumed;
    return v;
  }
  auto s = parse_scalar(in.substr(off));
  if (!s) {
    return std::nullopt;
  }
  Value v{};
  switch (s->type) {
    case ScalarType::Null:
      v.type = ValueType::Null;
      break;
    case ScalarType::Boolean:
      v.type = ValueType::Bool;
      break;
    case ScalarType::Number:
      v.type = ValueType::Number;
      break;
    case ScalarType::String:
      return std::nullopt;
  }
  v.scalar = *s;
  v.consumed = off + s->consumed;
  return v;
}
