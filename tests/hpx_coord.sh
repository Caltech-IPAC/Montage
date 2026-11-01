#!/bin/sh
# Output check for mHPXCoord2Tile's missing obstime argument: tile IDs at
# HEALPix orders 5 and 12 for three sky positions must match known values.
#
# convertCoordinates() takes nine arguments; mHPXCoord2Tile passed eight, so
# obstime was whatever was left in the ninth-argument register or stack slot.
# obstime matters on this path: EQUJ -> GAL goes through the Besselian
# (FK4) conversion, where obstime drives the equinox and FK4/FK5
# corrections, so a stray value shifts positions by ~10 arcsec or more.
# Order-5 tiles (~1.8 deg) are far larger than that shift and always match;
# order-12 tiles (~51 arcsec) are small enough that the shifted position
# can land in a neighbouring tile, so those IDs detect a wrong obstime.
#
# Expected IDs were computed with obstime = 0.0 (the default, 1950), the
# value every other caller passes; GCC 13.3, GCC 15.2 and Apple clang 21
# agree. Whether the old code fails depends on the compiler: on Apple clang
# 21 / arm64 it gave wrong order-12 IDs (159541179, 76895557; PR gate,
# 2026-11-01), while on Linux x86-64 with GCC the stray value happened to
# act like the default and the old code passed.
#
# Needs a completed `make`. Uses bin/mHPXCoord2Tile under the repository
# root unless BINDIR is set.
set -u

root=$(cd "$(dirname "$0")/.." && pwd)
bin=${BINDIR:-$root/bin}/mHPXCoord2Tile
if [ ! -x "$bin" ]; then
    echo "FAIL: $bin not found; run make first" >&2
    exit 1
fi

fail=0
check() {
    order=$1 ra=$2 dec=$3 expected=$4 label=$5
    out=$("$bin" "$order" "$ra" "$dec")
    case "$out" in
        *'stat="OK"'*) ;;
        *) echo "FAIL [$label order $order]: $out"; fail=1; return ;;
    esac
    id=$(printf '%s\n' "$out" | sed 's/.*id=\([0-9]*\).*/\1/')
    if [ "$id" = "$expected" ]; then
        echo "ok   [$label order $order] id=$id"
    else
        echo "FAIL [$label order $order] id=$id, expected $expected"
        fail=1
    fi
}

#     order RA        Dec       expected id  label
check 5     0.0       0.0       9737         equinox
check 5     83.8221   -5.3911   11174        m42
check 5     266.4168  -29.0078  4693         galcen
check 12    0.0       0.0       159542545    equinox
check 12    83.8221   -5.3911   183084766    m42
check 12    266.4168  -29.0078  76895556     galcen

exit $fail
