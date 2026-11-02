#!/bin/sh
# USE_SYSTEM_CFITSIO=1 selection without pkg-config (see
# system_cfitsio_pkgconfig.sh), exercised through the check-system-cfitsio
# probe (compile, link, run, header/library version agreement,
# CVE-2026-115504 check):
# - explicit CFITSIO_CFLAGS/CFITSIO_LIBS, which win over CFITSIO_DIR with a
#   warning naming where CFITSIO_LIBS came from;
# - CFITSIO_DIR=<prefix> holding a static-only libcfitsio (-lz -lm added);
# - every selection error in cfitsio.make, with its message: CFITSIO_DIR
#   without include/fitsio.h or without a library, CFITSIO_CFLAGS without
#   CFITSIO_LIBS, USE_SYSTEM_CFITSIO other than 0 or 1;
# - headers of another version than the linked library are rejected.
#
# The "system" CFITSIO is a temporary prefix holding the bundled static
# lib/libcfitsio.a, its headers and the bundled zlib's libz.a.
#
# Needs a completed `make` (uses lib/libcfitsio.a, lib/include and
# lib/src/zlib-build/libz.a).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)

if [ "$(sed -n 1p "$root/.cfitsio-build-mode" 2>/dev/null)" = 1 ]; then
    echo "SKIP: needs a bundled CFITSIO build (this tree was built with USE_SYSTEM_CFITSIO=1)"
    exit 77
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

for f in lib/libcfitsio.a lib/include/fitsio.h lib/src/zlib-build/libz.a; do
    [ -f "$root/$f" ] || { echo "FAIL: $f missing; run make first" >&2; exit 1; }
done

prefix=$tmp/prefix
mkdir -p "$prefix/include" "$prefix/lib" "$tmp/nolib/include" "$tmp/nolib/lib"
cp "$root/lib/include/fitsio.h" "$root/lib/include/longnam.h" "$prefix/include/"
cp "$root/lib/libcfitsio.a" "$root/lib/src/zlib-build/libz.a" "$prefix/lib/"
cp "$prefix/include/"*.h "$tmp/nolib/include/"

# Same library, headers claiming another micro version.
cp -R "$prefix" "$tmp/mismatch"
sed 's/^#define CFITSIO_MICRO .*/#define CFITSIO_MICRO 9/' "$prefix/include/fitsio.h" \
    > "$tmp/mismatch/include/fitsio.h"
grep -q '^#define CFITSIO_MICRO 9$' "$tmp/mismatch/include/fitsio.h" || {
    echo "FAIL: could not edit CFITSIO_MICRO" >&2; exit 1; }

# Run the probe as a standalone `make`, free of any settings inherited from
# an enclosing `make test`.
probe() {
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
        make -s -C "$root" check-system-cfitsio USE_SYSTEM_CFITSIO=1 "$@" 2>&1
}

expect_ok() {  # expect_ok <case> <required substring> <make args...>
    name=$1 want=$2
    shift 2
    if ! out=$(probe "$@"); then
        echo "FAIL: $name: probe failed: $out" >&2
        exit 1
    fi
    case "$out" in
        *"compiles, links and runs"*"$want"*) ;;
        *) echo "FAIL: $name: unexpected probe output: $out" >&2; exit 1 ;;
    esac
    case "$out" in
        *WARNING:*|*ERROR:*) echo "FAIL: $name: unexpected warning: $out" >&2; exit 1 ;;
    esac
    echo "ok   $name"
}

expect_warning() {  # expect_warning <case> <warning> <make args...>
    name=$1 want=$2
    shift 2
    if ! out=$(probe "$@"); then
        echo "FAIL: $name: probe failed: $out" >&2
        exit 1
    fi
    case "$out" in
        *"compiles, links and runs"*) ;;
        *) echo "FAIL: $name: unexpected probe output: $out" >&2; exit 1 ;;
    esac
    case "$out" in
        *"$want"*) echo "ok   $name (warned)" ;;
        *) echo "FAIL: $name: missing warning '$want' in: $out" >&2; exit 1 ;;
    esac
}

expect_error() {  # expect_error <case> <message> <make args...>
    name=$1 want=$2
    shift 2
    if out=$(probe "$@"); then
        echo "FAIL: $name: was accepted: $out" >&2
        exit 1
    fi
    case "$out" in
        *"$want"*) echo "ok   $name (rejected)" ;;
        *) echo "FAIL: $name: missing '$want' in: $out" >&2; exit 1 ;;
    esac
}

# Selection errors also name every way to select a CFITSIO.
help="select one with CFITSIO_DIR=<prefix>, CFITSIO_CFLAGS=... CFITSIO_LIBS=..., or a pkg-config 'cfitsio' package"

expect_ok "explicit flags" "CFITSIO_LIBS='-L$prefix/lib -lcfitsio -lm'" \
    CFITSIO_CFLAGS="-I$prefix/include" CFITSIO_LIBS="-L$prefix/lib -lcfitsio -lm"
expect_ok "CFITSIO_DIR, static-only" \
    "CFITSIO_CFLAGS='-I$prefix/include' CFITSIO_LIBS='-L$prefix/lib -lcfitsio -lz -lm'" \
    CFITSIO_DIR="$prefix"
expect_warning "CFITSIO_LIBS and CFITSIO_DIR both set" \
    "USE_SYSTEM_CFITSIO=1: CFITSIO_LIBS is set (origin: command line), so CFITSIO_DIR=$tmp/missing is ignored" \
    CFITSIO_CFLAGS="-I$prefix/include" CFITSIO_LIBS="-L$prefix/lib -lcfitsio -lm" \
    CFITSIO_DIR="$tmp/missing"

expect_error "CFITSIO_DIR without include/fitsio.h" \
    "USE_SYSTEM_CFITSIO=1: CFITSIO_DIR=$tmp/missing has no include/fitsio.h; $help" \
    CFITSIO_DIR="$tmp/missing"
expect_error "CFITSIO_DIR without a library" \
    "USE_SYSTEM_CFITSIO=1: CFITSIO_DIR=$tmp/nolib has no lib/libcfitsio library; $help" \
    CFITSIO_DIR="$tmp/nolib"
expect_error "CFITSIO_CFLAGS without CFITSIO_LIBS" \
    "USE_SYSTEM_CFITSIO=1: CFITSIO_CFLAGS was given without CFITSIO_LIBS; $help" \
    CFITSIO_CFLAGS="-I$prefix/include"
expect_error "USE_SYSTEM_CFITSIO=yes" \
    "USE_SYSTEM_CFITSIO must be 0 or 1, not 'yes'" \
    USE_SYSTEM_CFITSIO=yes
expect_error "headers and library of different versions" \
    "ERROR: the system CFITSIO headers and library are different versions (headers 4.7.9, library 4.7.0;" \
    CFITSIO_DIR="$tmp/mismatch"

echo "PASS: explicit flags and CFITSIO_DIR selectors, selection errors, version check"
