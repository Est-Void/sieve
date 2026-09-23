#!/usr/bin/env bash
# CLI smoke tests for sj. Usage: cli.sh <path-to-sj>
set -u
SJ="${1:-./build/sj}"
fail=0

expect_exit() { # name expected_exit -- sj-args...
    local name="$1" expected="$2"; shift 2
    "$SJ" "$@" </dev/null >/tmp/o.txt 2>/tmp/e.txt
    local code=$?
    if [ "$code" -ne "$expected" ]; then
        echo "FAIL $name: exit $code, expected $expected"
        echo "--- stderr ---"; cat /tmp/e.txt
        fail=1
    else
        echo "ok $name (exit $code)"
    fi
}

# BadFilter: exit 2 и пустой stdout
"$SJ" '.foo' </dev/null > /tmp/o.txt 2>/tmp/e.txt
code=$?
[ "$code" -eq 2 ] || { echo "FAIL badfilter-exit: got $code, want 2"; fail=1; }
test ! -s /tmp/o.txt || { echo "FAIL badfilter-stdout: stdout must be empty"; fail=1; }
[ "$fail" -eq 0 ] && echo "ok badfilter"

expect_exit missing-filter 2
expect_exit unknown-flag 2 --bad .
expect_exit conflict 2 -c -p .
expect_exit help 0 --help

# identity через stdin и файл
echo '{"a":1}' | "$SJ" '.' > /tmp/o.txt 2>/tmp/e.txt
[ $? -eq 0 ] && [ "$(cat /tmp/o.txt)" = '{"a":1}' ] && echo "ok stdin" \
    || { echo "FAIL stdin"; fail=1; }

echo '{"a":1}' > /tmp/in.json
"$SJ" '.' /tmp/in.json > /tmp/o.txt 2>/tmp/e.txt
[ $? -eq 0 ] && [ "$(cat /tmp/o.txt)" = '{"a":1}' ] && echo "ok file" \
    || { echo "FAIL file"; fail=1; }

exit "$fail"
