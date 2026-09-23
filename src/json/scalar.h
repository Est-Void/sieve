#ifndef JSON_SCALAR_H
#define JSON_SCALAR_H

#include <cstddef>
#include <optional>
#include <string_view>

enum class ScalarType {
    Null,
    Number,
    Boolean,
    //Invalid не нужен так как он не может быть возвращен
};

struct Scalar {
    ScalarType type;
    union {
        double number;
        bool boolean;
    };
    std::size_t consumed = 0;
};

std::optional<Scalar> parse_scalar(std::string_view in) noexcept;



#endif