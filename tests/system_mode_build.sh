#!/bin/sh
# End to end: a full `make USE_SYSTEM_CFITSIO=1 CFITSIO_DIR=<prefix>` in a
# clean copy of the source tree builds all 123 programs without building the
# bundled CFITSIO (no lib/libcfitsio.a, no CFITSIO headers in lib/include),
# and the programs read and write FITS (tests/fits_pipeline.sh: mMakeImg,
# mImgtbl, mProject, mAdd).
#
# The system CFITSIO is a real upstream-style install in a temporary
# prefix, made by tests/upstream_cfitsio.sh from the vendored sources:
# separate shared and static zlib and CFITSIO libraries, so CFITSIO_DIR
# links the shared libcfitsio with an rpath. Slow: two library builds and
# a full build. Works in a bundled or a system-mode tree (it needs only
# the sources).
#
# Environment:
#   SRC_TREE=dir   build from this pristine source tree instead of copying
#                  the checkout (e.g. a `git archive` export of a commit)
set -eu

root=$(cd "$(dirname "$0")/.." && pwd -P)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

prefix=$tmp/prefix
sh "$root/tests/upstream_cfitsio.sh" "$prefix" "$tmp/upstream-work"

# Copy the source tree (as in c23_default_build.sh). SRC_TREE, a pristine
# tree, is used as is. A copy of Montage's own checkout takes tracked and
# untracked files minus ignored output; otherwise (no git, or inside some
# other repository) the whole tree is copied. Copies are then cleaned: a
# copied build-mode stamp or lib/libcfitsio.a would make check-build-mode
# refuse the system-mode build.
mkdir "$tmp/src"
needs_clean=
if [ -n "${SRC_TREE:-}" ]; then
    cp -R "$SRC_TREE/." "$tmp/src"
elif [ "$(git -C "$root" rev-parse --show-toplevel 2>/dev/null)" = "$root" ]; then
    (cd "$root" && git ls-files -z --cached --others --exclude-standard) \
        > "$tmp/files" || { echo "FAIL: git ls-files failed in $root" >&2; exit 1; }
    (cd "$root" && tar --null -T "$tmp/files" -cf "$tmp/src.tar") ||
        { echo "FAIL: could not archive the source files" >&2; exit 1; }
    tar -xf "$tmp/src.tar" -C "$tmp/src" ||
        { echo "FAIL: could not extract the source copy" >&2; exit 1; }
    rm -f "$tmp/files" "$tmp/src.tar"
    needs_clean=1
else
    cp -R "$root/." "$tmp/src"
    needs_clean=1
fi
# cp -R keeps read-only modes; the build and the cleanup must write here.
chmod -R u+w "$tmp/src"
if [ -n "$needs_clean" ]; then
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
        make -C "$tmp/src" clean > /dev/null ||
        { echo "FAIL: make clean failed in the source copy" >&2; exit 1; }
fi

if ! env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
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
[ "$(sed -n 1p "$tmp/src/.cfitsio-build-mode")" = 1 ] || {
    echo "FAIL: build-mode stamp is not 1" >&2; exit 1; }
case $(sed -n 3p "$tmp/src/.cfitsio-build-mode") in
    *"-Wl,-rpath,$prefix/lib -lcfitsio"*) ;;
    *) echo "FAIL: the build-mode stamp does not select the shared upstream libcfitsio in $prefix/lib" >&2; exit 1 ;;
esac
echo "ok   build-mode stamp is 1 and selects the shared upstream libcfitsio"
# The stamp records what was asked for; the binary shows what was linked.
case $(uname -s) in
    Darwin) libs=$(otool -L "$tmp/src/bin/mProject" 2>&1 || :) ;;
    *)      libs=$(ldd "$tmp/src/bin/mProject" 2>&1 || :) ;;
esac
case "$libs" in
    *"$prefix/lib/libcfitsio"*) ;;
    *) echo "FAIL: bin/mProject is not linked against $prefix/lib/libcfitsio:" >&2
       printf '%s\n' "$libs" >&2
       exit 1 ;;
esac
echo "ok   bin/mProject loads the shared upstream libcfitsio"

BINDIR=$tmp/src/bin sh "$root/tests/fits_pipeline.sh"
echo "PASS: full system-mode build ($n programs) reads and writes FITS"
