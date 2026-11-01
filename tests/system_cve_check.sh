#!/bin/sh
# The check-system-cfitsio probe must reject a CFITSIO without the
# CVE-2026-115504 fix unless CFITSIO_ALLOW_UNPATCHED=1 (then it warns), and
# accept the patched one.
#
# The unpatched library is the vendored CFITSIO 4.7.0 with MONTAGE_PATCH.diff
# reversed, built out of tree in a temporary directory against the vendored
# zlib, exactly as lib/src builds the bundled one. The patched library is the
# bundled lib/libcfitsio.a.
#
# Needs a completed `make` (uses lib/libcfitsio.a and lib/include).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
src=$root/lib/src
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

if [ ! -f "$root/lib/libcfitsio.a" ] || [ ! -f "$root/lib/include/fitsio.h" ]; then
    echo "FAIL: lib/libcfitsio.a or lib/include/fitsio.h missing; run make first" >&2
    exit 1
fi

# Patched prefix: the bundled library.
mkdir -p "$tmp/patched/include" "$tmp/patched/lib"
cp "$root/lib/include/fitsio.h" "$root/lib/include/longnam.h" "$tmp/patched/include/"
cp "$root/lib/libcfitsio.a" "$tmp/patched/lib/"

# Unpatched prefix: 4.7.0 with the CVE fix reversed.
cp -R "$src/cfitsio-4.7.0" "$tmp/cfitsio"
cp -R "$src/zlib-1.3.2" "$tmp/zlib"
# Drop any in-tree configuration so the out-of-tree builds below start clean.
[ ! -f "$tmp/cfitsio/config.status" ] || (cd "$tmp/cfitsio" && make distclean) > /dev/null 2>&1
[ ! -f "$tmp/zlib/configure.log" ] || (cd "$tmp/zlib" && make distclean) > /dev/null 2>&1
if ! (cd "$tmp/cfitsio" && patch -s -R -p1 < MONTAGE_PATCH.diff); then
    echo "FAIL: could not reverse MONTAGE_PATCH.diff" >&2
    exit 1
fi
mkdir "$tmp/zlib-build" "$tmp/cfitsio-build" "$tmp/combined"
if ! { (cd "$tmp/zlib-build" && CFLAGS='-g -O2' ../zlib/configure --static && make libz.a) &&
       (cd "$tmp/cfitsio-build" && CFLAGS='-g -O2' \
            CPPFLAGS="-I$tmp/zlib -I$tmp/zlib-build" LDFLAGS="-L$tmp/zlib-build" \
            ../cfitsio/configure --disable-curl --disable-shared --enable-static && make) &&
       (cd "$tmp/combined" && ar x ../cfitsio-build/.libs/libcfitsio.a &&
            ar x ../zlib-build/libz.a && ar crs libcfitsio.a ./*.o); } > "$tmp/build.log" 2>&1; then
    tail -n 20 "$tmp/build.log" >&2
    echo "FAIL: could not build the unpatched CFITSIO" >&2
    exit 1
fi
mkdir -p "$tmp/unpatched/include" "$tmp/unpatched/lib"
cp "$tmp/cfitsio/fitsio.h" "$tmp/cfitsio/longnam.h" "$tmp/unpatched/include/"
cp "$tmp/combined/libcfitsio.a" "$tmp/unpatched/lib/"

probe() {
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL \
        make -s -C "$root" check-system-cfitsio USE_SYSTEM_CFITSIO=1 "$@" 2>&1
}

need() {  # need <case> <output> <substrings...>
    name=$1 out=$2
    shift 2
    for w in "$@"; do
        case "$out" in
            *"$w"*) ;;
            *) echo "FAIL: $name: missing '$w' in: $out" >&2; exit 1 ;;
        esac
    done
}

if out=$(probe CFITSIO_DIR="$tmp/unpatched"); then
    echo "FAIL: unpatched CFITSIO was accepted: $out" >&2
    exit 1
fi
need "unpatched" "$out" "ERROR: the system CFITSIO lacks the CVE-2026-115504 fix" \
    CFITSIO_ALLOW_UNPATCHED=1
echo "ok   unpatched CFITSIO rejected"

if ! out=$(probe CFITSIO_DIR="$tmp/unpatched" CFITSIO_ALLOW_UNPATCHED=1); then
    echo "FAIL: CFITSIO_ALLOW_UNPATCHED=1 did not override: $out" >&2
    exit 1
fi
need "unpatched, CFITSIO_ALLOW_UNPATCHED=1" "$out" \
    "WARNING: the system CFITSIO lacks the CVE-2026-115504 fix"
echo "ok   unpatched CFITSIO accepted with a warning under CFITSIO_ALLOW_UNPATCHED=1"

if ! out=$(probe CFITSIO_DIR="$tmp/patched"); then
    echo "FAIL: patched CFITSIO was rejected: $out" >&2
    exit 1
fi
need "patched" "$out" "compiles, links and runs" \
    "PASS: oversized variable-length string rejected with READ_ERROR"
case "$out" in
    *"WARNING: the system CFITSIO"*|*ERROR:*)
        echo "FAIL: patched: unexpected warning or error: $out" >&2; exit 1 ;;
esac
echo "ok   patched CFITSIO accepted"

echo "PASS: the probe enforces the CVE-2026-115504 fix"
