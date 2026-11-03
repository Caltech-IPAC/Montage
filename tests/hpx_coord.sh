#!/bin/sh
# Output check for mHPXCoord2Tile's missing obstime argument: tile IDs at
# HEALPix orders 5 and 12 for three sky positions must match known values.
#
# convertCoordinates() takes nine arguments; mHPXCoord2Tile passed eight, so
# obstime was whatever was left in the ninth-argument register or stack slot.
# obstime matters on this path: EQUJ -> GAL goes through the Besselian
# (FK4) conversion, where obstime drives the equinox and FK4/FK5
# corrections. Only the exact value 0.0 means 1950; any other value is read
# as a year. A near-zero stray value (as observed on Apple clang) therefore
# acts like year 0 and shifts these points by 6-11 arcsec; a stray value
# equal to a year between 1900 and 2026 shifts them by under 0.5 arcsec,
# which these checks cannot detect.
#
# Order-12 tiles (~51 arcsec) are small enough that a 6-11 arcsec shift can
# move a point into a neighbouring tile, so those IDs detect a wrong obstime.
# Order-5 tiles (~1.8 deg) usually match regardless, but a point near a tile
# edge can flip at any order: galcen is ~4 arcsec from the centre line of
# its HEALPix base tile, which is a tile edge at every order.
#
# Expected IDs were computed with obstime = 0.0 (the default, 1950), the
# value every other caller passes; GCC 13.3, GCC 15.2 and Apple clang 21
# agree. Whether the old code fails depends on the compiler and on what the
# preceding calls leave in the floating-point argument register: on Apple
# clang 21 / arm64 the full build gave wrong order-12 IDs for equinox and
# galcen (159541179, 76895557; seen 2026-11-01), while m42 order 12 still
# matched; on Linux x86-64 with GCC the old code gave the same IDs as the fix.
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

#     order RA        Dec       expected id  label   (order 5: coarse, except galcen)
check 5     0.0       0.0       9737         equinox
check 5     83.8221   -5.3911   11174        m42
check 5     266.4168  -29.0078  4693         galcen
check 12    0.0       0.0       159542545    equinox
check 12    83.8221   -5.3911   183084766    m42
check 12    266.4168  -29.0078  76895556     galcen

exit $fail
