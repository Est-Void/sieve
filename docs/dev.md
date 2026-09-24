Название: Sieve (sj)
Тип: CLI-инструмент для обработки JSON/NDJSON
Язык: C++20
Цель: Высокопроизводительная замена jq с многопоточной обработкой NDJSON и расширяемой архитектурой для поддержки других форматов данных.

1. Цели и non-goals

1.1 Цели

    Быстрее jq 1.7 в 5-10 раз на проекциях/фильтрах NDJSON (мерить по разделу 8)
    Многопоточная обработка NDJSON (линейное ускорение на многоядерных CPU)
    Zero-copy парсинг где возможно (этап оптимизации, не MVP)
    Совместимость с подмножеством синтаксиса jq (см. раздел 6)
    Модульная архитектура для добавления новых форматов (CSV, YAML, XML)

1.2 Non-goals (что НЕ делаем)

    Полная совместимость со всеми edge-case jq (фокус на 95% use cases)
    GUI, REPL, интерактивный режим
    Поддержка всех форматов с первой версии
    SQL-подобный синтаксис в MVP

2. Архитектура

2.1 Ключевые компоненты
1. Reader (DataSource)

    Читает сырые данные из stdin/file
    Поддерживает streaming (чанками) и slurp (весь файл в память, флаг -s)
    Для NDJSON: разделение на строки с сохранением границ

2. JSON Parser (данные)

    MVP: корректный скалярный парсер, SAX/streaming стиль, без построения полного DOM
    Оптимизация (после MVP): SIMD-ускоренный лексер (AVX2 через x86intrin.h)
    Zero-copy: string_view на исходный буфер (только пока буфер жив, строки с escape — копировать)

3. Query Parser (язык запросов)

    Рекурсивный спуск или Pratt parser для подмножества jq (см. раздел 6)
    Строит AST запроса (не DOM данных)

4. Query Engine

    Выполняет AST запроса на потоке данных
    Поддерживает lazy evaluation
    Фильтры, проекции, агрегации

5. Writer (DataSink)

    Сериализует результат в stdout
    Поддерживает pretty/compact/raw режимы (-p/-c/-r)
    Буферизованный вывод для производительности

6. Thread Pool (для NDJSON)

    Разделяет входной поток на чанки
    Параллельная обработка строк
    По умолчанию вывод с сохранением порядка; опция --unordered для скорости

3. Технологический стек
3.1 Язык и стандарт

    C++20 (concepts, ranges, string_view, std::span, coroutines для streaming)
    Компилятор: GCC 13+ или Clang 16+ (минимум — нужен std::format; CI на актуальных GCC/Clang)

3.2 Зависимости (минимизировать)

    Build system: CMake 3.20+
    CLI parsing: самописный (MVP) или p-ranav/argparse (C++, header-only)
    Testing: Catch2 (header-only, проще интеграция чем GTest)
    Benchmarking: Google Benchmark
    JSON parsing: самописный (для контроля производительности); simdjson — только как референс в бенчмарках

3.3 Форматирование и логирование

    Форматирование: std::format (GCC 13+ / Clang 16+), без fmtlib
    Логирование: stderr + коды возврата, без spdlog (для CLI не нужен)

3.4 SIMD (только этап оптимизации, не MVP)

    x86intrin.h (AVX2), интринсики изолировать в отдельном модуле с fallback на скалярный код
    std::simd НЕ использовать (это C++26, не C++20)

4. Структура репозитория

    sieve/
      src/
        main.cpp        # CLI, разбор флагов
        reader/         # DataSource: file/stdin, chunker для NDJSON
        json/           # SAX-парсер JSON (скалярный MVP, simd/ позже)
        query/          # парсер языка + AST + engine
        writer/         # DataSink: compact/pretty/raw
        threads/        # thread pool, ordered/unordered вывод
      include/sieve/    # публичные заголовки модулей
      tests/            # Catch2: json_parser, query, e2e (сравнение с jq)
      benches/          # Google Benchmark + скрипты vs jq
      CMakeLists.txt

5. CLI интерфейс (совместимость с jq)

    sj [options] 'filter' [file...]
    sj '.foo.bar' data.json
    cat data.ndjson | sj -c '.foo'
    sj -r '.name' users.json
    sj -s '[.[] | select(.age > 18)]' shard*.json

    Флаги v0.1: -c/--compact, -p/--pretty, -r/--raw-output, -s/--slurp, --unordered
    Ошибки: сообщение в stderr, ненулевой exit code; невалидный JSON — номер строки/chunk'а

6. MVP scope (v0.1)

    Поддержать: `.`, `.foo`, `.foo.bar`, `.[N]`, `.[]`, `|`, `,`,
      сравнения (== != < > <= >=), `select()`, `length`, `keys`, `+` для строк/чисел/массивов
    Типы JSON: object, array, string (UTF-8, escape), number, true/false/null
    Режимы: compact по умолчанию для NDJSON, pretty по флагу, raw для строк (-r)
    Не в MVP: деструктуризация сложная, reduce/foreach, regex, @-форматы, модули jq, streaming-парсер больших single-JSON (>RAM)

7. Этапы разработки

    0. Скелет: CMake, CLI-stub (identity фильтр `.`), Catch2, CI
    1. JSON: скалярный SAX-парсер + тесты (включая fuzz на corpus jq)
    2. Query: парсер подмножества (раздел 6) + engine (per-line DOM для NDJSON, streaming без полного DOM для single-JSON)
    3. NDJSON: chunker + thread pool (ordered по умолчанию, --unordered опция)
    4. Оптимизация: бенчмарки vs jq, затем SIMD/zero-copy
    5. Форматы: CSV in/out после стабильного v0.1

8. Бенчмарки

    Референс: jq 1.7+, опционально simdjson
    Датасеты: twitter.json (single), ndjson 1M строк (плоские + вложенные)
    Метрики: throughput (MB/s), latency p50/p99 на строку, speedup vs jq, scaling по ядрам (1/2/4/8)
    Запуск: benches/bench.sh собирает отчет в markdown; регресс >5% — красный CI
