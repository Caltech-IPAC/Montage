#!/bin/sh
# The objects staged for the MontagePy extension (python/MontagePy/lib/*.o,
# produced by `make` via MontageLib's pythonlibs target) must link into one
# shared library with no undefined symbols beyond libc/libm.
#
# This catches basename collisions while staging: with CFITSIO 4.7.0, zlib's
# compress.o overwrote bzip2's compress.o, leaving BZ2_compressBlock
# undefined, so the built extension failed to import.
#
# Needs a completed `make`.
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
objs="$root/python/MontagePy/lib"

if ! ls "$objs"/*.o > /dev/null 2>&1; then
    echo "FAIL: no staged objects in python/MontagePy/lib; run make first" >&2
    exit 1
fi

# Undefined symbols are already an error for macOS dynamic libraries.
case $(uname -s) in
    Darwin) flags="-dynamiclib" ;;
    *)      flags="-shared -Wl,--no-undefined" ;;
esac

if ! $cc $flags -o "$tmp/libstaged.so" "$objs"/*.o -lm 2> "$tmp/link.log"; then
    grep -v 'warning:' "$tmp/link.log" | head -n 20 >&2
    echo "FAIL: staged MontagePy objects do not link on their own" >&2
    exit 1
fi
echo "ok   $(ls "$objs"/*.o | wc -l | tr -d ' ') staged objects link with no undefined symbols"
