#include "json/string.h"

#include "json/detail.h"

#include <cstring>

namespace {

struct EscapeTable {
  unsigned char v[256];
  constexpr EscapeTable() : v{} {
    v[static_cast<unsigned char>('"')] = '"';
    v[static_cast<unsigned char>('\\')] = '\\';
    v[static_cast<unsigned char>('/')] = '/';
    v[static_cast<unsigned char>('b')] = '\b';
    v[static_cast<unsigned char>('f')] = '\f';
    v[static_cast<unsigned char>('n')] = '\n';
    v[static_cast<unsigned char>('r')] = '\r';
    v[static_cast<unsigned char>('t')] = '\t';
  }
};
constexpr EscapeTable kEsc{};

struct HexTable {
  unsigned char v[256];
  constexpr HexTable() : v{} {
    for (int i = 0; i < 256; ++i) {
      v[i] = 0xFF;
    }
    for (int i = '0'; i <= '9'; ++i) {
      v[i] = static_cast<unsigned char>(i - '0');
    }
    for (int i = 'a'; i <= 'f'; ++i) {
      v[i] = static_cast<unsigned char>(i - 'a' + 10);
    }
    for (int i = 'A'; i <= 'F'; ++i) {
      v[i] = static_cast<unsigned char>(i - 'A' + 10);
    }
  }
};
constexpr HexTable kHex{};

bool parse_hex4(std::string_view in, std::size_t p, unsigned &out) noexcept {
  if (p + 4 > in.size()) {
    return false;
  }
  unsigned a = kHex.v[static_cast<unsigned char>(in[p])];
  unsigned b = kHex.v[static_cast<unsigned char>(in[p + 1])];
  unsigned c = kHex.v[static_cast<unsigned char>(in[p + 2])];
  unsigned d = kHex.v[static_cast<unsigned char>(in[p + 3])];
  if ((a | b | c | d) & 0x80u) {
    return false;
  }
  out = (a << 12) | (b << 8) | (c << 4) | d;
  return true;
}

inline char *encode_utf8_to(char *w, unsigned cp) noexcept {
  if (cp < 0x80) {
    *w++ = static_cast<char>(cp);
  } else if (cp < 0x800) {
    *w++ = static_cast<char>(0xC0u | (cp >> 6));
    *w++ = static_cast<char>(0x80u | (cp & 0x3Fu));
  } else if (cp < 0x10000) {
    *w++ = static_cast<char>(0xE0u | (cp >> 12));
    *w++ = static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
    *w++ = static_cast<char>(0x80u | (cp & 0x3Fu));
  } else {
    *w++ = static_cast<char>(0xF0u | (cp >> 18));
    *w++ = static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu));
    *w++ = static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
    *w++ = static_cast<char>(0x80u | (cp & 0x3Fu));
  }
  return w;
}

} // namespace

std::optional<StringResult> parse_string(std::string_view in,
                                         std::string &out) noexcept {
  std::size_t off = 0;
  while (off < in.size() && detail::is_ws(in[off])) {
    ++off;
  }
  if (off >= in.size() || in[off] != '"') {
    return std::nullopt;
  }

  std::size_t p = off + 1;
  for (;;) {
    std::size_t q = detail::find_special(in, p);
    if (q == std::string_view::npos) {
      return std::nullopt; 
    }
    unsigned char c = static_cast<unsigned char>(in[q]);
    if (c == '"') {
      StringResult r;
      r.view = in.substr(off + 1, q - (off + 1));
      r.consumed = q + 1;
      r.escaped = false;
      return r;
    }
    if (c == '\\') {
      p = q;
      break;     }
    if (c < 0x20) {
      return std::nullopt;    }
    p = q + 1;  
  }

  out.clear();
  const std::size_t cap = in.size() - off; 
  out.resize(cap);
  char *w = out.data();
  std::memcpy(w, in.data() + off + 1, p - (off + 1)); 
  w += p - (off + 1);
  for (;;) {

    std::size_t q = p;
    while (q < in.size()) {
      unsigned char c = static_cast<unsigned char>(in[q]);
      if (c == '"' || c == '\\' || c < 0x20) {
        break;
      }
      *w++ = static_cast<char>(c);
      ++q;
    }
    if (q >= in.size()) {
      return std::nullopt; 
    }
    if (q - p >= 16) {

      std::size_t e = detail::find_special(in, q);
      if (e == std::string_view::npos) {
        return std::nullopt;
      }
      if (e > q) {
        std::memcpy(w, in.data() + q, e - q);
        w += e - q;
      }
      p = e;
    } else {
      p = q;
    }

    unsigned char c = static_cast<unsigned char>(in[p]);
    if (c == '"') {
      out.resize(static_cast<std::size_t>(w - out.data()));
      StringResult r;
      r.view = {};
      r.consumed = p + 1;
      r.escaped = true;
      return r;
    }
    if (c == '\\') {
      ++p;
    } else if (c < 0x20) {
      return std::nullopt;
    } else {

      *w++ = static_cast<char>(c);
      ++p;
      continue;
    }
    if (p >= in.size()) {
      return std::nullopt;
    }
    unsigned char e = static_cast<unsigned char>(in[p]);
    unsigned simple = kEsc.v[e];
    if (simple != 0) {
      *w++ = static_cast<char>(simple);
      ++p;
      continue;
    }
    if (e != 'u') {
      return std::nullopt;
    }
    ++p;
    unsigned cp = 0;
    if (!parse_hex4(in, p, cp)) {
      return std::nullopt;
    }
    p += 4;
    if (cp >= 0xD800 && cp <= 0xDBFF) {

      if (p + 6 > in.size() || in[p] != '\\' || in[p + 1] != 'u') {
        return std::nullopt;
      }
      unsigned lo = 0;
      if (!parse_hex4(in, p + 2, lo) || lo < 0xDC00 || lo > 0xDFFF) {
        return std::nullopt;
      }
      p += 6;
      cp = ((cp - 0xD800u) << 10 | (lo - 0xDC00u)) + 0x10000u;
    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
      return std::nullopt; 
    }
    w = encode_utf8_to(w, cp);
  }
}
