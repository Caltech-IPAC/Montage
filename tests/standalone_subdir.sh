#!/bin/sh
# A program directory must still build on its own (`make` inside it, with
# no settings from the top-level Makefile) and link the bundled CFITSIO.
#
# Copies the built libraries and headers, MontageLib's shared objects and
# headers, and the MontageLib/Add sources into a temporary tree, runs
# `make clean` and then `make` there with no CFITSIO_* variables set, and
# runs the resulting mAdd.
#
# Needs a completed `make` (uses lib/ and MontageLib/util/*.o).
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

if [ ! -f "$root/lib/libcfitsio.a" ] || [ ! -f "$root/MontageLib/util/checkHdr.o" ]; then
    echo "FAIL: lib/libcfitsio.a or MontageLib/util/*.o missing; run make first" >&2
    exit 1
fi

t=$tmp/tree
mkdir -p "$t/lib" "$t/MontageLib/util" "$t/MontageLib/Add"
cp -R "$root/lib/include" "$t/lib/"
cp "$root"/lib/*.a "$t/lib/"
cp "$root"/MontageLib/*.h "$t/MontageLib/"
cp "$root"/MontageLib/util/*.o "$t/MontageLib/util/"
cp "$root"/MontageLib/Add/Makefile "$root"/MontageLib/Add/*.c "$root"/MontageLib/Add/*.h "$t/MontageLib/Add/"

mk() {
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
        make -C "$t/MontageLib/Add" "$@"
}

mk clean > /dev/null
if ! mk > "$tmp/build.log" 2>&1; then
    cat "$tmp/build.log" >&2
    echo "FAIL: standalone make in MontageLib/Add failed" >&2
    exit 1
fi
grep -q -- '-lcfitsio' "$tmp/build.log" || {
    cat "$tmp/build.log" >&2
    echo "FAIL: standalone link did not use -lcfitsio" >&2
    exit 1
}
out=$("$t/MontageLib/Add/mAdd" 2>&1 || :)
case "$out" in
    *'stat="ERROR"'*Usage*) ;;
    *) echo "FAIL: mAdd did not run: $out" >&2; exit 1 ;;
esac
echo "PASS: standalone make in MontageLib/Add links the bundled CFITSIO"
