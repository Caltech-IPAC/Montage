#!/bin/sh
# Contract test for the real prototypes in coord.h and svc.h.
#
# 1. Code that includes the public headers and calls the functions with their
#    real arguments must compile as strict C23 (c23_headers_ok.c).
# 2. A call with the wrong number of arguments must be rejected even under
#    gnu17 (c23_headers_bad.c). With the old K&R declarations it compiled
#    silently, which is how mHPXCoord2Tile passed 8 of 9 arguments.
#
# If the compiler cannot apply C23's rule that `f()` takes no arguments
# (GCC <= 12, clang <= 14), check 1 would pass whether or not the headers are
# fixed, so the test reports SKIP (exit 77). REQUIRE_C23=1 makes that a
# failure (for CI). Only the exact value 1 is recognised. A
# compiler that cannot build a trivial file is a FAIL.
#
# CC may hold a command with options (e.g. CC="cc -O0"); it is split into
# words (no quote processing).
#
# Compile-only; does not need a completed `make`.
set -eu

# An exported CDPATH makes `cd` print the directory, corrupting $root.
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd)
cc=${CC:-cc}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT
coord_inc=$root/lib/src/coord
svc_inc=$root/lib/src/svc

cannot_run() {
    if [ "${REQUIRE_C23:-0}" = 1 ]; then
        echo "FAIL: $1 (REQUIRE_C23=1)" >&2
        exit 1
    fi
    echo "SKIP: $1"
    exit 77
}

# A compiler that cannot build a trivial file is broken, not merely old.
printf 'int main(void) { return 0; }\n' > "$tmp/trivial.c"
if ! $cc -c "$tmp/trivial.c" -o "$tmp/trivial.o" 2> "$tmp/trivial.err"; then
    cat "$tmp/trivial.err" >&2
    echo "FAIL: $cc cannot compile a trivial C file" >&2
    exit 1
fi

printf 'int main(void) { return 0; }\n' > "$tmp/probe.c"
std=
for s in c23 c2x; do
    if $cc -std=$s -c "$tmp/probe.c" -o "$tmp/probe.o" 2>/dev/null; then
        std=$s
        break
    fi
done
if [ -z "$std" ]; then
    cannot_run "$cc accepts neither -std=c23 nor -std=c2x"
fi

printf 'void f();\nint main(void) { f(1); return 0; }\n' > "$tmp/kr.c"
if $cc -std=$std -c "$tmp/kr.c" -o "$tmp/kr.o" 2>/dev/null; then
    cannot_run "$cc -std=$std does not apply C23 prototype rules"
fi

if ! $cc -std=$std -Werror=implicit-function-declaration -I"$coord_inc" -I"$svc_inc" \
        -c "$root/tests/c23_headers_ok.c" -o "$tmp/ok.o"; then
    echo "FAIL: coord.h/svc.h cannot be used from -std=$std code" >&2
    exit 1
fi
echo "ok   coord.h and svc.h compile as -std=$std with real arguments"

# LC_ALL=C: the diagnostic checked below must be in English.
if LC_ALL=C $cc -std=gnu17 -I"$coord_inc" -I"$svc_inc" -c "$root/tests/c23_headers_bad.c" \
        -o "$tmp/bad.o" 2> "$tmp/bad.log"; then
    echo "FAIL: an 8-argument convertCoordinates call compiled;" \
         "coord.h does not declare its real prototype" >&2
    exit 1
fi
if ! grep -qi 'too few arguments' "$tmp/bad.log"; then
    echo "FAIL: the bad call was rejected for an unexpected reason:" >&2
    cat "$tmp/bad.log" >&2
    exit 1
fi
echo "ok   an 8-argument convertCoordinates call is rejected"
