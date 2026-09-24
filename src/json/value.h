#ifndef SIEVE_JSON_VALUE_H
#define SIEVE_JSON_VALUE_H

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "json/scalar.h"
#include "json/string.h"

enum class ValueType { Null, Bool, Number, String, Array, Object };

struct Value {
  ValueType type;
  Scalar scalar;    
  StringResult str; 
  std::size_t consumed = 0;
};

std::optional<Value> parse_value(std::string_view in,
                                 std::string& strout) noexcept;
std::optional<std::size_t> skip_value(std::string_view in) noexcept;

#endif // SIEVE_JSON_VALUE_H
