#include "json/string.h"

namespace {

constexpr bool is_ws(char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

constexpr int hex_val(char c) noexcept {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

unsigned parse_hex4(std::string_view in, std::size_t p, bool &ok) noexcept {
  if (p + 4 > in.size()) {
    ok = false;
    return 0;
  }
  unsigned v = 0;
  for (int i = 0; i < 4; ++i) {
    int h = hex_val(in[p + static_cast<std::size_t>(i)]);
    if (h < 0) {
      ok = false;
      return 0;
    }
    v = (v << 4) | static_cast<unsigned>(h);
  }
  ok = true;
  return v;
}

void encode_utf8(std::string &o, unsigned cp) {
  if (cp < 0x80) {
    o.push_back(static_cast<char>(cp));
  } else if (cp < 0x800) {
    o.push_back(static_cast<char>(0xC0u | (cp >> 6)));
    o.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
  } else if (cp < 0x10000) {
    o.push_back(static_cast<char>(0xE0u | (cp >> 12)));
    o.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
    o.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
  } else {
    o.push_back(static_cast<char>(0xF0u | (cp >> 18)));
    o.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
    o.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
    o.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
  }
}

} // namespace

std::optional<StringResult> parse_string(std::string_view in,
                                         std::string &out) noexcept {
  std::size_t off = 0;
  while (off < in.size() && is_ws(in[off])) {
    ++off;
  }
  if (off >= in.size() || in[off] != '"') {
    return std::nullopt;
  }

  std::size_t p = off + 1;
  for (;;) {
    if (p >= in.size()) {
      return std::nullopt;
    }
    unsigned char c = static_cast<unsigned char>(in[p]);
    if (c == '"') {
      StringResult r;
      r.view = in.substr(off + 1, p - (off + 1));
      r.consumed = p + 1;
      r.escaped = false;
      return r;
    }
    if (c == '\\') {
      break;
    }
    if (c < 0x20) {
      return std::nullopt;
    }
    ++p;
  }

  out.clear();
  out.append(in.data() + off + 1, p - (off + 1));
  while (p < in.size()) {
    unsigned char c = static_cast<unsigned char>(in[p]);
    if (c == '"') {
      StringResult r;
      r.view = {};
      r.consumed = p + 1;
      r.escaped = true;
      return r;
    }
    if (c == '\\') {
      ++p;
      if (p >= in.size()) {
        return std::nullopt;
      }
      switch (in[p]) {
      case '"':
        out.push_back('"');
        ++p;
        break;
      case '\\':
        out.push_back('\\');
        ++p;
        break;
      case '/':
        out.push_back('/');
        ++p;
        break;
      case 'b':
        out.push_back('\b');
        ++p;
        break;
      case 'f':
        out.push_back('\f');
        ++p;
        break;
      case 'n':
        out.push_back('\n');
        ++p;
        break;
      case 'r':
        out.push_back('\r');
        ++p;
        break;
      case 't':
        out.push_back('\t');
        ++p;
        break;
      case 'u': {
        ++p;
        bool ok = false;
        unsigned cp = parse_hex4(in, p, ok);
        if (!ok) {
          return std::nullopt;
        }
        p += 4;
        if (cp >= 0xD800 && cp <= 0xDBFF) {

          if (p + 6 > in.size() || in[p] != '\\' || in[p + 1] != 'u') {
            return std::nullopt;
          }
          bool ok2 = false;
          unsigned lo = parse_hex4(in, p + 2, ok2);
          if (!ok2 || lo < 0xDC00 || lo > 0xDFFF) {
            return std::nullopt;
          }
          p += 6;
          cp = ((cp - 0xD800u) << 10 | (lo - 0xDC00u)) + 0x10000u;
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
          return std::nullopt;
        }
        encode_utf8(out, cp);
        break;
      }
      default:
        return std::nullopt;
      }
      continue;
    }
    if (c < 0x20) {
      return std::nullopt;
    }
    out.push_back(in[p]);
    ++p;
  }
  return std::nullopt;
}
