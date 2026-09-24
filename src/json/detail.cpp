#include "json/detail.h"

#include <cstdint>
#include <cstring>

namespace {

constexpr std::uint64_t k01 = 0x0101010101010101ULL;
constexpr std::uint64_t k80 = 0x8080808080808080ULL;

inline std::uint64_t has_zero(std::uint64_t x) noexcept {
  return (x - k01) & ~x & k80;
}

} // namespace

namespace detail {

bool is_ws(char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::size_t find_qb(std::string_view in, std::size_t p) noexcept {
  const char *d = in.data();
  const std::size_t n = in.size();
  constexpr std::uint64_t q_bc = k01 * 0x22; // '"'
  constexpr std::uint64_t b_bc = k01 * 0x5C; // '\\'
  while (p + 8 <= n) {
    std::uint64_t x;
    std::memcpy(&x, d + p, 8);
    std::uint64_t q = x ^ q_bc;
    std::uint64_t b = x ^ b_bc;
    std::uint64_t m = has_zero(q) | has_zero(b);
    if (m) {
      return p + (static_cast<std::size_t>(__builtin_ctzll(m)) >> 3);
    }
    p += 8;
  }
  while (p < n) {
    char c = d[p];
    if (c == '"' || c == '\\') {
      return p;
    }
    ++p;
  }
  return std::string_view::npos;
}

std::size_t find_special(std::string_view in, std::size_t p) noexcept {
  const char *d = in.data();
  const std::size_t n = in.size();
  constexpr std::uint64_t q_bc = k01 * 0x22; // '"'
  constexpr std::uint64_t b_bc = k01 * 0x5C; // '\\'
  constexpr std::uint64_t c_bc = k01 * 0x20; // граница управляющих
  while (p + 8 <= n) {
    std::uint64_t x;
    std::memcpy(&x, d + p, 8);
    std::uint64_t q = x ^ q_bc;
    std::uint64_t b = x ^ b_bc;
    std::uint64_t m = has_zero(q) | has_zero(b) | ((x - c_bc) & ~x & k80);
    if (m) {
      return p + (static_cast<std::size_t>(__builtin_ctzll(m)) >> 3);
    }
    p += 8;
  }
  while (p < n) {
    unsigned char c = static_cast<unsigned char>(d[p]);
    if (c == '"' || c == '\\' || c < 0x20) {
      return p;
    }
    ++p;
  }
  return std::string_view::npos;
}

} // namespace detail
