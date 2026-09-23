#include "json/scalar.h"

#include <charconv>
#include <system_error>

namespace {

constexpr bool is_ws(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

constexpr bool is_word_char(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') || c == '_';
}

bool match_lit(std::string_view r, std::string_view lit) noexcept {
    if (r.size() < lit.size() || !r.starts_with(lit)) {
        return false;
    }
    return r.size() == lit.size() || !is_word_char(r[lit.size()]);
}

} // namespace

std::optional<Scalar> parse_scalar(std::string_view in) noexcept {
    std::size_t off = 0;
    while (off < in.size() && is_ws(in[off])) {
        ++off;
    }
    std::string_view r = in.substr(off);

    if (match_lit(r, "null")) {
        Scalar s{};
        s.type = ScalarType::Null;
        s.consumed = off + 4;
        return s;
    }
    if (match_lit(r, "true")) {
        Scalar s{};
        s.type = ScalarType::Boolean;
        s.boolean = true;
        s.consumed = off + 4;
        return s;
    }
    if (match_lit(r, "false")) {
        Scalar s{};
        s.type = ScalarType::Boolean;
        s.boolean = false;
        s.consumed = off + 5;
        return s;
    }

    std::size_t p = 0;
    if (p < r.size() && r[p] == '-') {
        ++p;
    }
    if (p < r.size() && r[p] == '0') {
        ++p;
        if (p < r.size() && is_digit(r[p])) {
            return std::nullopt;
        }
    } else if (p < r.size() && r[p] >= '1' && r[p] <= '9') {
        do {
            ++p;
        } while (p < r.size() && is_digit(r[p]));
    } else {
        return std::nullopt;
    }
    if (p < r.size() && r[p] == '.') {
        ++p;
        if (p >= r.size() || !is_digit(r[p])) {
            return std::nullopt;
        }
        do {
            ++p;
        } while (p < r.size() && is_digit(r[p]));
    }
    if (p < r.size() && (r[p] == 'e' || r[p] == 'E')) {
        ++p;
        if (p < r.size() && (r[p] == '+' || r[p] == '-')) {
            ++p;
        }
        if (p >= r.size() || !is_digit(r[p])) {
            return std::nullopt;
        }
        do {
            ++p;
        } while (p < r.size() && is_digit(r[p]));
    }

    std::string_view tok = r.substr(0, p);
    double value = 0.0;
    const auto [ptr, ec] =
        std::from_chars(tok.data(), tok.data() + tok.size(), value);
    if (ec != std::errc() || ptr != tok.data() + tok.size()) {
        return std::nullopt;
    }

    Scalar s{};
    s.type = ScalarType::Number;
    s.number = value;
    s.consumed = off + p;
    return s;
}
