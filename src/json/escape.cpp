#include "json/escape.h"

#include "json/detail.h"

namespace {

// Простой escape: байт → символ X в паре '\X'; 0 = экранировать не нужно,
// 1 = нет имени, экранируем через \u00XX (ни одно имя не равно 0/1).
struct EncTable {
  unsigned char v[256];
  constexpr EncTable() : v{} {
    v[static_cast<unsigned char>('"')] = '"';
    v[static_cast<unsigned char>('\\')] = '\\';
    v[static_cast<unsigned char>('\b')] = 'b';
    v[static_cast<unsigned char>('\f')] = 'f';
    v[static_cast<unsigned char>('\n')] = 'n';
    v[static_cast<unsigned char>('\r')] = 'r';
    v[static_cast<unsigned char>('\t')] = 't';
    for (int c = 0; c < 0x20; ++c) {
      if (v[c] == 0) {
        v[c] = 1;
      }
    }
  }
};
constexpr EncTable kEnc{};

constexpr char kHexDigits[] = "0123456789abcdef";

} // namespace

void json_escape(std::string_view in, std::string &out) {
  // Нижняя оценка: строка без спецсимволов не растёт; расширения
  // (экраны) добьёт геометрический рост — 6x не резервируем.
  out.reserve(out.size() + in.size());

  std::size_t p = 0;
  for (;;) {
    // Кусок обычных байтов до следующего '"' | '\\' | <0x20 — целиком.
    // find_special — тот же SWAR-скан, что в parse_string.
    std::size_t q = detail::find_special(in, p);
    if (q == std::string_view::npos) {
      out.append(in.data() + p, in.size() - p);
      return;
    }
    if (q > p) {
      out.append(in.data() + p, q - p);
    }
    unsigned char c = static_cast<unsigned char>(in[q]);
    unsigned char e = kEnc.v[c];
    if (e == 1) {
      // Управляющий без имени: \u00XX
      const char buf[6] = {'\\', 'u', '0', '0', kHexDigits[c >> 4],
                           kHexDigits[c & 0x0F]};
      out.append(buf, 6);
    } else { // именованный: '"' '\\' 'b' 'f' 'n' 'r' 't'
      out.push_back('\\');
      out.push_back(static_cast<char>(e));
    }
    p = q + 1;
  }
}
