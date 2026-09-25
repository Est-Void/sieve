#ifndef SIEVE_JSON_ESCAPE_H
#define SIEVE_JSON_ESCAPE_H

#include <string>
#include <string_view>

// Дописывает в out JSON-экранированное содержимое строки, БЕЗ кавычек:
//   '"' → \",  '\\' → \\,  \b \f \n \r \t,
//   прочие управляющие <0x20 → \u00XX (строчные hex).
// Байты >= 0x20 (в т.ч. UTF-8) проходят как есть — поведение jq.
// Зеркало к parse_string: parse_string("\"" + json_escape(s) + "\"") == s.
void json_escape(std::string_view in, std::string &out);

#endif // SIEVE_JSON_ESCAPE_H
