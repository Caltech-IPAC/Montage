#!/bin/sh
# ./configure writes only the install prefix (config.make); the CFITSIO
# selection lives in the tracked cfitsio.make and must survive it:
# after ./configure, CFITSIO_DIR still selects and probes a system CFITSIO,
# and the default build still links -lcfitsio.
#
# Runs in a temporary copy of the top-level build files; the "system"
# CFITSIO is a temporary prefix holding the bundled lib/libcfitsio.a and
# the bundled zlib's libz.a. Exits 77 (SKIP) if there is no ./configure.
#
# Needs a completed `make` (uses lib/libcfitsio.a, lib/include and
# lib/src/zlib-build/libz.a).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)

if [ "$(sed -n 1p "$root/.cfitsio-build-mode" 2>/dev/null)" = 1 ]; then
    echo "SKIP: needs a bundled CFITSIO build (this tree was built with USE_SYSTEM_CFITSIO=1)"
    exit 77
fi

if [ ! -f "$root/configure" ]; then
    echo "SKIP: this tree has no ./configure"
    exit 77
fi
for f in lib/libcfitsio.a lib/include/fitsio.h lib/src/zlib-build/libz.a; do
    [ -f "$root/$f" ] || { echo "FAIL: $f missing; run make first" >&2; exit 1; }
done

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

prefix=$tmp/prefix
mkdir -p "$prefix/include" "$prefix/lib"
cp "$root/lib/include/fitsio.h" "$root/lib/include/longnam.h" "$prefix/include/"
cp "$root/lib/libcfitsio.a" "$root/lib/src/zlib-build/libz.a" "$prefix/lib/"

t=$tmp/tree
mkdir -p "$t/tests"
cp "$root/Makefile" "$root/configure" "$root/cfitsio.make" "$t/"
cp "$root/tests/cfitsio_system_version.c" "$root/tests/cve_vlen_regression.c" "$t/tests/"
printf '%s\n' 'print-%:' '	@echo "$*=$($*)"' > "$tmp/print.mk"

if ! (cd "$t" && ./configure --prefix="$tmp/install") > "$tmp/configure.log" 2>&1; then
    cat "$tmp/configure.log" >&2
    echo "FAIL: ./configure failed" >&2
    exit 1
fi

mk() {
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
        make -s -C "$t" -f Makefile -f "$tmp/print.mk" "$@" 2>&1
}

out=$(mk print-montage_prefix)
[ "$out" = "montage_prefix=$tmp/install" ] || {
    echo "FAIL: ./configure prefix not used: $out" >&2; exit 1; }
echo "ok   ./configure set the install prefix"

out=$(mk print-CFITSIO_LIBS)
[ "$out" = "CFITSIO_LIBS=-lcfitsio" ] || {
    echo "FAIL: default build does not link -lcfitsio: $out" >&2; exit 1; }
echo "ok   default build links -lcfitsio"

sys="USE_SYSTEM_CFITSIO=1 CFITSIO_DIR=$prefix"
out=$(mk print-CFITSIO_CFLAGS print-CFITSIO_LIBS $sys)
want="CFITSIO_CFLAGS=-I$prefix/include
CFITSIO_LIBS=-L$prefix/lib -lcfitsio -lz -lm"
[ "$out" = "$want" ] || {
    echo "FAIL: CFITSIO_DIR not resolved after ./configure: $out" >&2; exit 1; }
echo "ok   CFITSIO_DIR resolves after ./configure"

if ! out=$(mk check-system-cfitsio $sys); then
    echo "FAIL: probe failed after ./configure: $out" >&2
    exit 1
fi
case "$out" in
    *"compiles, links and runs"*) echo "ok   probe passes after ./configure" ;;
    *) echo "FAIL: unexpected probe output: $out" >&2; exit 1 ;;
esac

echo "PASS: ./configure keeps the CFITSIO selection"
