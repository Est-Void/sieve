#ifndef SIEVE_JSON_STRING_H
#define SIEVE_JSON_STRING_H

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

struct StringResult {
    std::string_view view;
    std::size_t consumed = 0;
    bool escaped = false;
};

std::optional<StringResult> parse_string(std::string_view in, std::string& out) noexcept;

#endif // SIEVE_JSON_STRING_H
