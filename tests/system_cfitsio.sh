#!/bin/sh
# USE_SYSTEM_CFITSIO=1 selectors, exercised through the check-system-cfitsio
# probe (compile, link, run, CVE-2026-115504 check):
# - explicit CFITSIO_CFLAGS/CFITSIO_LIBS;
# - CFITSIO_DIR=<prefix>;
# - pkg-config, with and without CFITSIO_STATIC=1;
# - a missing CFITSIO_DIR and a genuinely absent pkg-config package must
#   fail with a message naming the ways to select a CFITSIO.
#
# The "system" CFITSIO is a temporary prefix holding the bundled static
# lib/libcfitsio.a and its headers. A static archive links only with -lm
# added (Linux), so CFITSIO_DIR and pkg-config must supply it.
#
# Needs a completed `make` (uses lib/libcfitsio.a and lib/include).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

if [ ! -f "$root/lib/libcfitsio.a" ] || [ ! -f "$root/lib/include/fitsio.h" ]; then
    echo "FAIL: lib/libcfitsio.a or lib/include/fitsio.h missing; run make first" >&2
    exit 1
fi

prefix=$tmp/prefix
mkdir -p "$prefix/include" "$prefix/lib" "$prefix/static-only" "$tmp/pc" "$tmp/empty"
cp "$root/lib/include/fitsio.h" "$root/lib/include/longnam.h" "$prefix/include/"
cp "$root/lib/libcfitsio.a" "$prefix/lib/"
cat > "$tmp/pc/cfitsio.pc" <<EOF
prefix=$prefix
Name: cfitsio
Description: Montage test copy of CFITSIO
Version: 4.7.0
Libs: -L\${prefix}/lib -lcfitsio
Libs.private: -L\${prefix}/static-only -lm
Cflags: -I\${prefix}/include
EOF

# Run the probe as a standalone `make`, free of any settings inherited from
# an enclosing `make test`.
probe() {
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL \
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
        *WARNING*) echo "FAIL: $name: patched CFITSIO reported a warning: $out" >&2; exit 1 ;;
    esac
    echo "ok   $name"
}

expect_error() {  # expect_error <case> <make args...>; message in $want1..3
    name=$1
    shift
    if out=$(probe "$@"); then
        echo "FAIL: $name: was accepted: $out" >&2
        exit 1
    fi
    for w in "$want1" "$want2" "$want3" CFITSIO_DIR= CFITSIO_LIBS= pkg-config; do
        case "$out" in
            *"$w"*) ;;
            *) echo "FAIL: $name: missing '$w' in: $out" >&2; exit 1 ;;
        esac
    done
    echo "ok   $name (rejected)"
}

expect_ok "explicit flags" "-lcfitsio -lm" \
    CFITSIO_CFLAGS="-I$prefix/include" CFITSIO_LIBS="-L$prefix/lib -lcfitsio -lm"
expect_ok "CFITSIO_DIR" "-L$prefix/lib -lcfitsio -lm" CFITSIO_DIR="$prefix"

want1="USE_SYSTEM_CFITSIO=1: CFITSIO_DIR=$tmp/missing has no include/fitsio.h"
want2=$want1 want3=$want1
expect_error "missing CFITSIO_DIR" CFITSIO_DIR="$tmp/missing"

want1="USE_SYSTEM_CFITSIO must be 0 or 1, not 'yes'"
want2=$want1 want3=$want1
if out=$(probe USE_SYSTEM_CFITSIO=yes); then
    echo "FAIL: USE_SYSTEM_CFITSIO=yes was accepted: $out" >&2
    exit 1
fi
case "$out" in
    *"$want1"*) echo "ok   USE_SYSTEM_CFITSIO=yes (rejected)" ;;
    *) echo "FAIL: USE_SYSTEM_CFITSIO=yes: missing '$want1' in: $out" >&2; exit 1 ;;
esac

if ! command -v pkg-config > /dev/null 2>&1; then
    echo "NOTICE: pkg-config is not installed; skipping the pkg-config cases"
    echo "PASS: explicit flags and CFITSIO_DIR selectors and errors"
    exit 0
fi

# PKG_CONFIG_LIBDIR also replaces pkg-config's built-in search path, so a
# machine-wide cfitsio.pc cannot leak in.
PKG_CONFIG_PATH=$tmp/pc PKG_CONFIG_LIBDIR=$tmp/pc
export PKG_CONFIG_PATH PKG_CONFIG_LIBDIR
expect_ok "pkg-config" "CFITSIO_LIBS='-L$prefix/lib -lcfitsio -lm'"
expect_ok "pkg-config, CFITSIO_STATIC=1" "static-only -lm" CFITSIO_STATIC=1

PKG_CONFIG_PATH=$tmp/empty PKG_CONFIG_LIBDIR=$tmp/empty
want1="USE_SYSTEM_CFITSIO=1: no system CFITSIO found"
want2="cfitsio" want3="USE_SYSTEM_CFITSIO=0"
expect_error "absent pkg-config package"

echo "PASS: explicit flags, CFITSIO_DIR and pkg-config selectors and errors"
