#!/bin/sh
# Montage regression tests. Run with `make test` from the repository root.
# Each entry is a script in tests/. Exit status 0 is PASS; 77 means the test
# cannot run on this machine (e.g. the compiler lacks a needed feature) and
# is reported as SKIP, not as a failure; anything else is FAIL. Tests that
# run programs installed by `make` expect it to have completed.
set -u
cd "$(dirname "$0")/.." || exit 1
logs=$(mktemp -d)
trap 'rm -rf "$logs"' EXIT HUP INT TERM

pass=0
fail=0
skip=0
for t in \
    c23_default_build.sh \
    ; do
    printf '%-32s ' "$t"
    sh "tests/$t" > "$logs/$t.log" 2>&1
    status=$?
    if [ "$status" -eq 0 ]; then
        echo PASS
        pass=$((pass + 1))
    elif [ "$status" -eq 77 ]; then
        echo "SKIP  ($(tail -n 1 "$logs/$t.log"))"
        skip=$((skip + 1))
    else
        echo FAIL
        fail=$((fail + 1))
        tail -n 25 "$logs/$t.log" | sed 's/^/    /'
    fi
done
echo "Results: $pass passed, $fail failed, $skip skipped"
[ "$fail" -eq 0 ]
