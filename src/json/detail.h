#ifndef SIEVE_JSON_DETAIL_H
#define SIEVE_JSON_DETAIL_H

#include <cstddef>
#include <string_view>

namespace detail {

bool is_ws(char c) noexcept;

std::size_t find_qb(std::string_view in, std::size_t p) noexcept;

std::size_t find_special(std::string_view in, std::size_t p) noexcept;

} // namespace detail

#endif // SIEVE_JSON_DETAIL_H
