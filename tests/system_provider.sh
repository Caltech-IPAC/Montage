#!/bin/sh
# USE_SYSTEM_CFITSIO=1 with CFITSIO_DIR pointing at a realistic install:
# CFITSIO 4.7.0 and a separate zlib 1.3.2 installed into one prefix with
# their own configure and `make install` (tests/upstream_cfitsio.sh), so
# libcfitsio does not contain zlib.
# - shared + static prefix: CFITSIO_DIR links the shared library with an
#   rpath to <prefix>/lib, and the probe's programs run;
# - static-only copy of the prefix: CFITSIO_DIR adds -lz -lm and links;
# - (Linux) the shared library without an rpath does not run, and the probe
#   says how to fix that.
#
# Does not need a completed `make`: the provider is built from lib/src.
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

shared=$tmp/upstream
sh "$root/tests/upstream_cfitsio.sh" "$shared" "$tmp/work"
static=$tmp/upstream-static
mkdir -p "$static/lib"
cp -R "$shared/include" "$static/"
cp "$shared/lib/libcfitsio.a" "$shared/lib/libz.a" "$static/lib/"

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
        *"System CFITSIO 4.7.0: compiles, links and runs"*"$want"*) ;;
        *) echo "FAIL: $name: unexpected probe output: $out" >&2; exit 1 ;;
    esac
    case "$out" in
        *WARNING:*|*ERROR:*) echo "FAIL: $name: unexpected warning: $out" >&2; exit 1 ;;
    esac
    echo "ok   $name"
}

expect_ok "CFITSIO_DIR, shared and static" \
    "CFITSIO_LIBS='-L$shared/lib -Wl,-rpath,$shared/lib -lcfitsio -lm'" CFITSIO_DIR="$shared"
expect_ok "CFITSIO_DIR, static-only" \
    "CFITSIO_LIBS='-L$static/lib -lcfitsio -lz -lm'" CFITSIO_DIR="$static"

# macOS records the install path in the library, so only Linux needs the
# loader search path.
if [ "$(uname -s)" = Linux ]; then
    if out=$(probe CFITSIO_CFLAGS="-I$shared/include" CFITSIO_LIBS="-L$shared/lib -lcfitsio -lm"); then
        echo "FAIL: shared library without rpath ran: $out" >&2
        exit 1
    fi
    case "$out" in
        *"ERROR: a program linked against the system CFITSIO does not run"*"-Wl,-rpath,<libdir>"*LD_LIBRARY_PATH*)
            echo "ok   shared library without rpath (rejected)" ;;
        *) echo "FAIL: shared library without rpath: unexpected output: $out" >&2; exit 1 ;;
    esac
fi

echo "PASS: CFITSIO_DIR with an upstream-style shared and static-only CFITSIO"
