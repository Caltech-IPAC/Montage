#!/bin/sh
# The release tarball made by build/mktar.sh must carry the build interface:
# the top-level Makefile, cfitsio.make, configure, config.make and every
# file in tests/ (so each script listed in tests/run_tests.sh exists). In
# the extracted tarball, ./configure --prefix=<dir> must work and the root
# Makefile must use that prefix (`make -n install`).
#
# mktar.sh runs in a temporary parent directory holding a source-only copy
# of the tree as Montage/. tar's exit status is not checked: the archive list
# names entries that a source checkout does not have (Montage/bin before a
# build, Montage/Python_ManyLinux), and tar reports those but still writes
# the archive; the files that matter are checked instead.
#
# Exits 77 (SKIP) if this machine's tar cannot archive a file given as
# dir/file: under x86-64 emulation on arm64 Docker, this emulated image's
# tar fails every such path with "Cannot stat: Function not implemented"
# (not GNU tar in general: other images with the same tar version work).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

mkdir "$tmp/probe"
echo probe > "$tmp/probe/file"
if ! err=$(cd "$tmp" && tar -cf probe.tar probe/file 2>&1); then
    echo "SKIP: tar cannot archive probe/file here: $(printf '%s' "$err" | tr '\n' ' ')"
    exit 77
fi

# Source files only (as in c23_default_build.sh).
m=$tmp/parent/Montage
mkdir -p "$m" "$tmp/x"
if [ -n "${SRC_TREE:-}" ]; then
    cp -R "$SRC_TREE/." "$m"
elif git -C "$root" rev-parse --git-dir > /dev/null 2>&1; then
    (cd "$root" && git ls-files -z --cached --others --exclude-standard) \
        > "$tmp/files" || { echo "FAIL: git ls-files failed in $root" >&2; exit 1; }
    (cd "$root" && tar --null -T "$tmp/files" -cf "$tmp/src.tar") ||
        { echo "FAIL: could not archive the source files" >&2; exit 1; }
    tar -xf "$tmp/src.tar" -C "$m" ||
        { echo "FAIL: could not extract the source copy" >&2; exit 1; }
    rm -f "$tmp/files" "$tmp/src.tar"
else
    cp -R "$root/." "$m"
fi
# cp -R keeps read-only modes; mktar.sh and the cleanup must write here.
chmod -R u+w "$m"

(cd "$tmp/parent" && sh Montage/build/mktar.sh) > "$tmp/mktar.log" 2>&1 || :
set -- "$tmp/parent"/Montage_*.tar.gz
if [ ! -f "$1" ]; then
    tail -n 20 "$tmp/mktar.log" >&2
    echo "FAIL: build/mktar.sh made no tarball" >&2
    exit 1
fi
tar -xzf "$1" -C "$tmp/x"
x=$tmp/x/Montage

missing=
for f in Makefile cfitsio.make configure config.make; do
    [ -f "$x/$f" ] || missing="$missing $f"
done
for f in $(cd "$tmp/parent/Montage" && find tests -type f); do
    [ -f "$x/$f" ] || missing="$missing $f"
done
for t in $(sed -n 's/^ *\([A-Za-z0-9_]*\.sh\) *\\$/\1/p' "$x/tests/run_tests.sh"); do
    [ -f "$x/tests/$t" ] || missing="$missing tests/$t"
done
if [ -n "$missing" ]; then
    echo "FAIL: release tarball lacks:$missing" >&2
    exit 1
fi
echo "ok   tarball has the build files and every test"

if ! (cd "$x" && ./configure --prefix="$tmp/install") > "$tmp/configure.log" 2>&1; then
    cat "$tmp/configure.log" >&2
    echo "FAIL: ./configure failed in the extracted tarball" >&2
    exit 1
fi
if ! out=$(env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS make -n -C "$x" install 2>&1); then
    echo "FAIL: make -n install failed in the extracted tarball: $out" >&2
    exit 1
fi
case "$out" in
    *"cp bin/* $tmp/install/bin"*) echo "ok   ./configure prefix used by make install" ;;
    *) echo "FAIL: make -n install does not use the configured prefix: $out" >&2; exit 1 ;;
esac

echo "PASS: release tarball carries configure, config.make, cfitsio.make and the tests"
