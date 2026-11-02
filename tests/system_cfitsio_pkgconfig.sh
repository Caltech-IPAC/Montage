#!/bin/sh
# USE_SYSTEM_CFITSIO=1 selection through pkg-config, exercised through the
# check-system-cfitsio probe. Exits 77 (SKIP) when pkg-config is missing.
# - A test cfitsio.pc over the bundled static lib/libcfitsio.a: plain
#   (--libs, -lm added) and CFITSIO_STATIC=1 (--libs --static).
# - The real cfitsio.pc and zlib.pc installed by an upstream-style CFITSIO
#   with a separate zlib (tests/upstream_cfitsio.sh), reduced to a
#   static-only prefix: CFITSIO_STATIC=1 links (zlib from Libs.private);
#   without it, the link fails for want of zlib.
# - A genuinely absent package fails with a message naming the selectors.
#
# Needs a completed `make` (uses lib/libcfitsio.a and lib/include).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)

if [ "$(sed -n 1p "$root/.cfitsio-build-mode" 2>/dev/null)" = 1 ]; then
    echo "SKIP: needs a bundled CFITSIO build (this tree was built with USE_SYSTEM_CFITSIO=1)"
    exit 77
fi

if ! command -v pkg-config > /dev/null 2>&1; then
    echo "SKIP: pkg-config is not installed"
    exit 77
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

if [ ! -f "$root/lib/libcfitsio.a" ] || [ ! -f "$root/lib/include/fitsio.h" ]; then
    echo "FAIL: lib/libcfitsio.a or lib/include/fitsio.h missing; run make first" >&2
    exit 1
fi

# Test package over the bundled library.
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

# Upstream-style install, then a static-only copy with its .pc files moved.
sh "$root/tests/upstream_cfitsio.sh" "$tmp/upstream" "$tmp/work"
static=$tmp/upstream-static
mkdir -p "$static/lib/pkgconfig"
cp -R "$tmp/upstream/include" "$static/"
cp "$tmp/upstream/lib/libcfitsio.a" "$tmp/upstream/lib/libz.a" "$static/lib/"
for pc in cfitsio zlib; do
    sed "s|^prefix=.*|prefix=$static|" "$tmp/upstream/lib/pkgconfig/$pc.pc" \
        > "$static/lib/pkgconfig/$pc.pc"
done

# Run the probe as a standalone `make`, free of any settings inherited from
# an enclosing `make test`. PKG_CONFIG_LIBDIR also replaces pkg-config's
# built-in search path, so a machine-wide cfitsio.pc cannot leak in.
probe() {  # probe <pkg-config dir> <make args...>
    dir=$1
    shift
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
        PKG_CONFIG_PATH="$dir" PKG_CONFIG_LIBDIR="$dir" \
        make -s -C "$root" check-system-cfitsio USE_SYSTEM_CFITSIO=1 "$@" 2>&1
}

expect_ok() {  # expect_ok <case> <required substring> <pkg-config dir> <make args...>
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

expect_error() {  # expect_error <case> <pkg-config dir> <make args...>: must fail; output in $out
    name=$1
    shift
    if out=$(probe "$@"); then
        echo "FAIL: $name: was accepted: $out" >&2
        exit 1
    fi
}
need() {  # need <case> <substrings...>, checked in $out
    name=$1
    shift
    for w in "$@"; do
        case "$out" in
            *"$w"*) ;;
            *) echo "FAIL: $name: missing '$w' in: $out" >&2; exit 1 ;;
        esac
    done
    echo "ok   $name (rejected)"
}

expect_ok "test package" "CFITSIO_LIBS='-L$prefix/lib -lcfitsio -lm'" "$tmp/pc"
expect_ok "test package, CFITSIO_STATIC=1" "static-only -lm" "$tmp/pc" CFITSIO_STATIC=1

expect_ok "upstream cfitsio.pc, static-only, CFITSIO_STATIC=1" "-lz" \
    "$static/lib/pkgconfig" CFITSIO_STATIC=1
expect_error "upstream cfitsio.pc, static-only, without CFITSIO_STATIC" \
    "$static/lib/pkgconfig"
need "upstream cfitsio.pc, static-only, without CFITSIO_STATIC" \
    "ERROR: cannot compile and link against the system CFITSIO"

expect_error "absent package" "$tmp/empty"
need "absent package" "USE_SYSTEM_CFITSIO=1: no system CFITSIO found" "cfitsio" \
    "select one with CFITSIO_DIR=<prefix>, CFITSIO_CFLAGS=... CFITSIO_LIBS=..., or a pkg-config 'cfitsio' package" \
    "USE_SYSTEM_CFITSIO=0"

echo "PASS: pkg-config selection (test package, upstream static install, absent package)"
