#!/bin/sh
# ./configure writes only the install prefix (config.make); the CFITSIO
# selection lives in the tracked cfitsio.make and must survive it:
# after ./configure, CFITSIO_DIR still selects and probes a system CFITSIO,
# and the default build still links -lcfitsio.
#
# Runs in a temporary copy of the top-level build files; the "system"
# CFITSIO is a temporary prefix holding the bundled lib/libcfitsio.a.
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
mkdir -p "$prefix/include" "$prefix/lib"
cp "$root/lib/include/fitsio.h" "$root/lib/include/longnam.h" "$prefix/include/"
cp "$root/lib/libcfitsio.a" "$prefix/lib/"

t=$tmp/tree
mkdir -p "$t/tests"
for f in Makefile configure config.make cfitsio.make; do
    [ ! -f "$root/$f" ] || cp "$root/$f" "$t/"
done
cp "$root/tests/cve_vlen_regression.c" "$t/tests/"
printf '%s\n' 'print-%:' '	@echo "$*=$($*)"' > "$tmp/print.mk"

if ! (cd "$t" && ./configure --prefix="$tmp/install") > "$tmp/configure.log" 2>&1; then
    cat "$tmp/configure.log" >&2
    echo "FAIL: ./configure failed" >&2
    exit 1
fi

mk() {
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL \
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
CFITSIO_LIBS=-L$prefix/lib -lcfitsio -lm"
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
