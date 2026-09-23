#ifndef SIEVE_JSON_SCALAR_H
#define SIEVE_JSON_SCALAR_H

#include <cstddef>
#include <optional>
#include <string_view>

enum class ScalarType { Null, Number, Boolean, String };

struct Scalar {
  ScalarType type;
  union {
    double number;
    bool boolean;
    std::string_view view;
  };
  std::size_t consumed = 0;
};

std::optional<Scalar> parse_scalar(std::string_view in) noexcept;

#endif // SIEVE_JSON_SCALAR_H