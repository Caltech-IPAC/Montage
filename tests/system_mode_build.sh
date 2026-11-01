#!/bin/sh
# End to end: a full `make USE_SYSTEM_CFITSIO=1 CFITSIO_DIR=<prefix>` in a
# clean copy of the source tree builds all 123 programs without building the
# bundled CFITSIO (no lib/libcfitsio.a, no CFITSIO headers in lib/include),
# and the programs read and write FITS (tests/fits_pipeline.sh: mMakeImg,
# mImgtbl, mProject, mAdd).
#
# The "system" CFITSIO is a temporary prefix holding the bundled static
# lib/libcfitsio.a and its headers. Slow: a full build.
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

# Copy source files only (as in c23_default_build.sh).
mkdir "$tmp/src"
if [ -n "${SRC_TREE:-}" ]; then
    cp -R "$SRC_TREE/." "$tmp/src"
elif git -C "$root" rev-parse --git-dir > /dev/null 2>&1; then
    (cd "$root" && git ls-files -z --cached --others --exclude-standard) \
        | (cd "$root" && tar --null -T - -cf -) | tar -xf - -C "$tmp/src"
else
    cp -R "$root/." "$tmp/src"
    make -C "$tmp/src" clean > /dev/null 2>&1
fi

if ! env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL \
        make -C "$tmp/src" USE_SYSTEM_CFITSIO=1 CFITSIO_DIR="$prefix" \
        > "$tmp/build.log" 2>&1; then
    grep -E ': error: |Error [0-9]|ERROR' "$tmp/build.log" | head -n 20 >&2
    echo "FAIL: system-mode build failed" >&2
    exit 1
fi
grep -q 'compiles, links and runs' "$tmp/build.log" || {
    echo "FAIL: the system CFITSIO probe did not run" >&2; exit 1; }

n=$(ls "$tmp/src/bin" | wc -l | tr -d ' ')
[ "$n" -eq 123 ] || { echo "FAIL: expected 123 programs, got $n" >&2; exit 1; }
echo "ok   $n programs"
for f in lib/libcfitsio.a lib/include/fitsio.h lib/include/longnam.h; do
    [ ! -e "$tmp/src/$f" ] || { echo "FAIL: bundled $f exists in system mode" >&2; exit 1; }
done
echo "ok   no bundled CFITSIO in lib/"
[ "$(cat "$tmp/src/.cfitsio-build-mode")" = 1 ] || {
    echo "FAIL: build-mode stamp is not 1" >&2; exit 1; }
echo "ok   build-mode stamp is 1"

BINDIR=$tmp/src/bin sh "$root/tests/fits_pipeline.sh"
echo "PASS: full system-mode build ($n programs) reads and writes FITS"
