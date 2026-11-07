#!/bin/sh
# Build the JSON library source with AddressSanitizer and look up keys in
# layouts that are shorter than the key. Needs no earlier make.
set -u
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd) || exit 1

skip() {
    echo "SKIP: $*"
    exit 77
}

cc=${CC:-cc}
# CC may hold a compiler with options, so check only its first word and
# leave $cc unquoted where it is run.
set -- $cc
command -v "$1" >/dev/null 2>&1 || skip "C compiler $1 not found"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/montage-jsonval.XXXXXXXX") || exit 1
trap 'rm -rf "$tmp"' 0
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

printf 'int main(void) { return 0; }\n' > "$tmp/probe.c"
$cc -fsanitize=address "$tmp/probe.c" -o "$tmp/probe" >/dev/null 2>&1 \
    || skip "$cc cannot build with -fsanitize=address"
"$tmp/probe" >/dev/null 2>&1 || skip 'AddressSanitizer programs do not run here'

$cc -g -fsanitize=address -fno-omit-frame-pointer -I"$root/lib/src/json" \
    "$root/tests/json_val_short_layout.c" "$root/lib/src/json/jsonlib.c" \
    -o "$tmp/json_val_short_layout"
status=$?
if [ "$status" -ne 0 ]; then
    echo 'FAIL: cannot compile the json_val test' >&2
    exit 1
fi

cd "$tmp" || exit 1
ASAN_OPTIONS=detect_leaks=0 ./json_val_short_layout
exit $?
