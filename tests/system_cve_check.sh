#!/bin/sh
# The check-system-cfitsio probe must reject a CFITSIO without the
# CVE-2026-115504 fix unless CFITSIO_ALLOW_UNPATCHED=1 (then it warns), and
# accept the patched one. A probe that cannot set up its test file
# (cve_vlen_regression.c exit status 2) is a distinct error that names no
# override, and CFITSIO_ALLOW_UNPATCHED=1 does not bypass it. Exit status 3
# (valid data mishandled) and any unexpected status are distinct errors that
# cannot be overridden either; stub CVE tests produce them.
#
# The unpatched library is the vendored CFITSIO 4.7.0 with MONTAGE_PATCH.diff
# reversed, built out of tree in a temporary directory against the vendored
# zlib; its prefix holds the two static archives separately. The patched
# library is the bundled lib/libcfitsio.a.
#
# Needs a completed `make` (uses lib/libcfitsio.a, lib/include and
# lib/src/zlib-build/libz.a).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
src=$root/lib/src

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

# Nested makes of the vendored sources must not inherit the caller's make
# flags (e.g. -i or -k from `make -k test`).
submake() {
    env -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS make "$@"
}

if [ ! -f "$root/lib/libcfitsio.a" ] || [ ! -f "$root/lib/include/fitsio.h" ]; then
    echo "FAIL: lib/libcfitsio.a or lib/include/fitsio.h missing; run make first" >&2
    exit 1
fi

# Patched prefix: the bundled library.
mkdir -p "$tmp/patched/include" "$tmp/patched/lib"
cp "$root/lib/include/fitsio.h" "$root/lib/include/longnam.h" "$tmp/patched/include/"
cp "$root/lib/libcfitsio.a" "$root/lib/src/zlib-build/libz.a" "$tmp/patched/lib/"

# Unpatched prefix: 4.7.0 with the CVE fix reversed.
cp -R "$src/cfitsio-4.7.0" "$tmp/cfitsio"
cp -R "$src/zlib-1.3.2" "$tmp/zlib"
# Drop any in-tree configuration so the out-of-tree builds below start clean.
[ ! -f "$tmp/cfitsio/config.status" ] || (cd "$tmp/cfitsio" && submake distclean) > /dev/null 2>&1
[ ! -f "$tmp/zlib/configure.log" ] || (cd "$tmp/zlib" && submake distclean) > /dev/null 2>&1
if ! (cd "$tmp/cfitsio" && patch -s -R -p1 < MONTAGE_PATCH.diff); then
    echo "FAIL: could not reverse MONTAGE_PATCH.diff" >&2
    exit 1
fi
mkdir "$tmp/zlib-build" "$tmp/cfitsio-build"
if ! { (cd "$tmp/zlib-build" && CFLAGS='-g -O2' ../zlib/configure --static && submake libz.a) &&
       (cd "$tmp/cfitsio-build" && CFLAGS='-g -O2' \
            CPPFLAGS="-I$tmp/zlib-build -I$tmp/zlib" LDFLAGS="-L$tmp/zlib-build" \
            ../cfitsio/configure --disable-curl --disable-shared --enable-static \
                --without-fortran && submake libcfitsio.la); } > "$tmp/build.log" 2>&1; then
    tail -n 20 "$tmp/build.log" >&2
    echo "FAIL: could not build the unpatched CFITSIO" >&2
    exit 1
fi
mkdir -p "$tmp/unpatched/include" "$tmp/unpatched/lib"
cp "$tmp/cfitsio/fitsio.h" "$tmp/cfitsio/longnam.h" "$tmp/unpatched/include/"
cp "$tmp/cfitsio-build/.libs/libcfitsio.a" "$tmp/zlib-build/libz.a" "$tmp/unpatched/lib/"

probe() {
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
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

# The probe from a temporary copy of the top-level build files, with its own
# tests/cve_vlen_regression.c.
t=$tmp/tree
mkdir -p "$t/tests"
cp "$root/Makefile" "$root/cfitsio.make" "$t/"
cp "$root/tests/cfitsio_system_version.c" "$t/tests/"
tree_probe() {  # tree_probe <CFITSIO_ALLOW_UNPATCHED>
    env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
        make -s -C "$t" check-system-cfitsio USE_SYSTEM_CFITSIO=1 \
        CFITSIO_DIR="$tmp/patched" CFITSIO_ALLOW_UNPATCHED=$1 2>&1
}
no_override() {  # no_override <case> <output>
    case "$2" in
        *CFITSIO_ALLOW_UNPATCHED*|*"lacks the CVE-2026-115504 fix"*)
            echo "FAIL: $1: reported as a missing fix or offers the override: $2" >&2
            exit 1 ;;
    esac
}

# Setup failure: the CVE test writes its scratch file into a missing directory.
sed 's|^#define TEST_FITS_FILE .*|#define TEST_FITS_FILE "no-such-dir/cve_vlen_test.fits"|' \
    "$root/tests/cve_vlen_regression.c" > "$t/tests/cve_vlen_regression.c"
grep -q 'no-such-dir' "$t/tests/cve_vlen_regression.c" || {
    echo "FAIL: could not redirect TEST_FITS_FILE" >&2; exit 1; }
for allow in 0 1; do
    if out=$(tree_probe $allow); then
        echo "FAIL: setup failure accepted (CFITSIO_ALLOW_UNPATCHED=$allow): $out" >&2
        exit 1
    fi
    need "setup failure (CFITSIO_ALLOW_UNPATCHED=$allow)" "$out" \
        "ERROR: the CVE-2026-115504 probe could not run (setup failure"
    no_override "setup failure (CFITSIO_ALLOW_UNPATCHED=$allow)" "$out"
    echo "ok   setup failure rejected without override (CFITSIO_ALLOW_UNPATCHED=$allow)"
done

# Exit 3 (valid data mishandled) and an unexpected status (4), from stubs.
for code in 3 4; do
    printf 'int main(void) { return %s; }\n' "$code" > "$t/tests/cve_vlen_regression.c"
    case $code in
        3) want="ERROR: the system CFITSIO rejects valid strings" ;;
        *) want="ERROR: the CVE-2026-115504 probe failed unexpectedly (exit status $code)" ;;
    esac
    for allow in 0 1; do
        if out=$(tree_probe $allow); then
            echo "FAIL: probe exit $code accepted (CFITSIO_ALLOW_UNPATCHED=$allow): $out" >&2
            exit 1
        fi
        need "probe exit $code (CFITSIO_ALLOW_UNPATCHED=$allow)" "$out" "$want"
        no_override "probe exit $code (CFITSIO_ALLOW_UNPATCHED=$allow)" "$out"
        echo "ok   probe exit $code rejected without override (CFITSIO_ALLOW_UNPATCHED=$allow)"
    done
done

if ! out=$(probe CFITSIO_DIR="$tmp/patched"); then
    echo "FAIL: patched CFITSIO was rejected: $out" >&2
    exit 1
fi
need "patched" "$out" "compiles, links and runs"
case "$out" in
    *"WARNING: the system CFITSIO"*|*ERROR:*)
        echo "FAIL: patched: unexpected warning or error: $out" >&2; exit 1 ;;
esac
echo "ok   patched CFITSIO accepted"

echo "PASS: the probe enforces the CVE-2026-115504 fix"
