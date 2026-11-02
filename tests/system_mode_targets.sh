#!/bin/sh
# Targets whose behaviour changes with USE_SYSTEM_CFITSIO=1:
# - `make clean` works when no system CFITSIO can be found (while a build
#   target in the same state fails with the selection error);
# - `make -C MontageLib pythonlibs` is refused: the MontagePy wheel objects
#   need the bundled CFITSIO;
# - MontageLib's `all` does not stage the wheel objects in system mode, and
#   does in bundled mode (`make -n`, nothing is built).
#
# The clean checks run the real top-level Makefile in a temporary tree with
# no subdirectories; the MontageLib checks run in the checkout with -n or
# stop before any command. Nothing is written to the checkout.
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

mk() {  # mk <make args...>: output in $out, status in $rc
    rc=0
    out=$(env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
            -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
            -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
            PKG_CONFIG_PATH="$tmp/empty" PKG_CONFIG_LIBDIR="$tmp/empty" \
            make "$@" 2>&1) || rc=$?
}

t=$tmp/tree
mkdir -p "$t" "$tmp/empty"
cp "$root/Makefile" "$root/cfitsio.make" "$t/"
: > "$t/.cfitsio-build-mode"

mk -C "$t" check-build-mode USE_SYSTEM_CFITSIO=1
if [ "$rc" -eq 0 ]; then echo "FAIL: no provider, check-build-mode succeeded: $out" >&2; exit 1; fi
case "$out" in
    *"USE_SYSTEM_CFITSIO=1: no system CFITSIO found"*) echo "ok   no provider: build targets fail" ;;
    *) echo "FAIL: no provider: unexpected error: $out" >&2; exit 1 ;;
esac
mk -C "$t" clean USE_SYSTEM_CFITSIO=1
if [ "$rc" -ne 0 ]; then echo "FAIL: make clean with no provider failed: $out" >&2; exit 1; fi
[ ! -e "$t/.cfitsio-build-mode" ] || { echo "FAIL: make clean left the stamp" >&2; exit 1; }
echo "ok   no provider: make clean works"

mk -C "$root/MontageLib" pythonlibs USE_SYSTEM_CFITSIO=1
if [ "$rc" -eq 0 ]; then echo "FAIL: pythonlibs ran in system mode: $out" >&2; exit 1; fi
case "$out" in
    *"ERROR: pythonlibs (the MontagePy wheel objects) needs the bundled CFITSIO; rebuild with USE_SYSTEM_CFITSIO=0 after 'make clean'."*)
        echo "ok   pythonlibs refused in system mode" ;;
    *) echo "FAIL: pythonlibs: unexpected output: $out" >&2; exit 1 ;;
esac

mk -n -C "$root/MontageLib" all USE_SYSTEM_CFITSIO=1
if [ "$rc" -ne 0 ]; then echo "FAIL: make -n all (system) failed: $out" >&2; exit 1; fi
case "$out" in
    *python/MontagePy*) echo "FAIL: system-mode all stages wheel objects" >&2; exit 1 ;;
esac
mk -n -C "$root/MontageLib" all
if [ "$rc" -ne 0 ]; then echo "FAIL: make -n all (bundled) failed: $out" >&2; exit 1; fi
case "$out" in
    *python/MontagePy*) ;;
    *) echo "FAIL: bundled all does not stage wheel objects" >&2; exit 1 ;;
esac
echo "ok   MontageLib all stages wheel objects only in bundled mode"

echo "PASS: system-mode clean, pythonlibs refusal and MontageLib all"
