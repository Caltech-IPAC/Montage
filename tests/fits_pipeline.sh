#!/bin/sh
# End-to-end FITS I/O through CFITSIO and its zlib:
# mMakeImg writes a constant-background image, mImgtbl catalogues it,
# mProject reprojects it onto a rotated header, and mAdd mosaics the result.
# Every step must report stat="OK", the catalogues must see valid FITS and
# WCS, and the mosaic's interior must keep the input value (reprojection
# preserves surface brightness, and the pixel scale is unchanged).
# Full-width COMMENT and HISTORY template cards must keep column 80 in
# mMakeImg and mProject output. mMakeImg's .fits.gz output must decompress
# to the same bytes as its uncompressed output.
#
# The same image, gzip-compressed, must also be reprojected and catalogued,
# giving output byte-identical to the uncompressed run. mProject reads the
# .fits.gz through CFITSIO, which decompresses it with CFITSIO's zlib
# (mImgtbl instead runs the system gunzip).
#
# Needs a completed `make`. Uses bin/ under the repository root unless
# BINDIR is set.
set -eu

# An exported CDPATH makes `cd` print the directory, corrupting $root.
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd)
bin=$(cd "${BINDIR:-$root/bin}" && pwd)
# CFITSIO takes ".Z" anywhere in a path as a Unix compress file, and builds
# with bzip2 do the same for ".bz2", so the .fits.gz step below cannot open a
# file in such a path. A plain mktemp name is tmp.XXXXXXXXXX, and when the
# random part began with Z the step failed, about 1 run in 62. This name has
# no dot before the random part.
tmp=$(mktemp -d "${TMPDIR:-/tmp}/montage-fits-pipeline-XXXXXXXX")
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT
case $tmp in
    *.Z*|*.bz2*)
        echo "SKIP: the temp path $tmp contains .Z or .bz2, which CFITSIO reads as a compression suffix"
        exit 77
        ;;
esac
mkdir "$tmp/raw" "$tmp/proj" "$tmp/gz" "$tmp/projgz" "$tmp/writegz"

VALUE=3.25

run() {  # run <program> <args...>: its output, or FAIL if it exits nonzero
    p=$1
    shift
    status=0
    run_out=$("$bin/$p" "$@" 2>&1) || status=$?
    printf '%s\n' "$run_out"
    if [ "$status" -ne 0 ]; then
        echo "FAIL: $p exited $status: $run_out" >&2
        exit 1
    fi
}

expect() {  # expect <step> <output> <key=value...> as whole status fields
    step=$1 out=$2
    shift 2
    for want in "$@"; do
        case "$out" in
            *" $want,"*|*" $want]"*) ;;
            *) echo "FAIL: $step: missing field '$want' in: $out" >&2; exit 1 ;;
        esac
    done
    echo "ok   $step"
}

check_template_cards() {  # check_template_cards <fits>: both cards intact
    # The header is 80-column cards up to END. Stop there, before pixel data.
    header=$(fold -w 80 "$1" | sed '/^END /q')
    for card in COMMENT HISTORY; do
        expected=$(sed -n "/^$card /p" "$tmp/in.hdr")
        if ! printf '%s\n' "$header" | grep -F -x "$expected" >/dev/null; then
            echo "FAIL: $1: $card template card did not keep column 80" >&2
            exit 1
        fi
    done
}

cat > "$tmp/in.hdr" <<'EOF'
SIMPLE  =                    T
BITPIX  =                  -64
NAXIS   =                    2
NAXIS1  =                   50
NAXIS2  =                   50
CTYPE1  = 'RA---TAN'
CTYPE2  = 'DEC--TAN'
EQUINOX =               2000.0
CRVAL1  =                 10.0
CRVAL2  =                 20.0
CRPIX1  =                 25.5
CRPIX2  =                 25.5
CDELT1  =               -0.001
CDELT2  =                0.001
COMMENT CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCZ
HISTORY HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHY
END
EOF

# Same field, rotated 30 degrees: mProject must really resample.
sed -e 's/^CRPIX1  =                 25.5/CRPIX1  =                 25.5\
CROTA2  =                 30.0/' "$tmp/in.hdr" > "$tmp/out.hdr"

# -b sets the four corner values; equal corners give a constant image.
out=$(run mMakeImg -b $VALUE $VALUE $VALUE $VALUE "$tmp/in.hdr" "$tmp/raw/img.fits")
expect mMakeImg "$out" 'stat="OK"'
check_template_cards "$tmp/raw/img.fits"

out=$(run mMakeImg -b $VALUE $VALUE $VALUE $VALUE "$tmp/in.hdr" "$tmp/writegz/img.fits.gz")
expect "mMakeImg (.fits.gz)" "$out" 'stat="OK"'
# Do not compare gzip bytes because the header OS byte depends on platform
# and zlib version.
gzip -dc "$tmp/writegz/img.fits.gz" > "$tmp/writegz/img.fits"
if ! cmp -s "$tmp/raw/img.fits" "$tmp/writegz/img.fits"; then
    echo "FAIL: .fits.gz output decompresses to a different image" >&2
    exit 1
fi
echo "ok   .fits.gz output decompresses to the uncompressed image"

out=$(run mImgtbl "$tmp/raw" "$tmp/raw.tbl")
expect "mImgtbl (raw)" "$out" 'stat="OK"' 'count=1' 'badfits=0' 'badwcs=0'

out=$(run mProject "$tmp/raw/img.fits" "$tmp/proj/img.fits" "$tmp/out.hdr")
expect mProject "$out" 'stat="OK"'
check_template_cards "$tmp/proj/img.fits"
echo "ok   full-width COMMENT and HISTORY template cards keep column 80"

out=$(run mImgtbl "$tmp/proj" "$tmp/proj.tbl")
expect "mImgtbl (projected)" "$out" 'stat="OK"' 'count=1' 'badfits=0' 'badwcs=0'

out=$(run mAdd -p "$tmp/proj" "$tmp/proj.tbl" "$tmp/out.hdr" "$tmp/mosaic.fits")
expect mAdd "$out" 'stat="OK"'

# Mean over a 0.005-degree (5-pixel) radius around the field centre.
out=$(run mExamine -p 10.0 20.0 0.005 "$tmp/mosaic.fits")
expect "mExamine (mosaic centre)" "$out" 'stat="OK"'
ave=$(printf '%s\n' "$out" | sed -n 's/.*aveflux=\([^,]*\),.*/\1/p')
if ! awk -v a="$ave" -v v="$VALUE" 'BEGIN { d = a - v; if (d < 0) d = -d; exit !(a != "" && d <= 1e-6 * v) }'; then
    echo "FAIL: mosaic centre mean is '$ave', expected $VALUE: $out" >&2
    exit 1
fi
echo "ok   mosaic centre keeps the input value ($ave)"

# gzip-compressed input. mProject reads it first, through CFITSIO's zlib;
# mImgtbl then catalogues it (mImgtbl uses the system gunzip).
gzip -c "$tmp/raw/img.fits" > "$tmp/gz/img.fits.gz"

out=$(run mProject "$tmp/gz/img.fits.gz" "$tmp/projgz/img.fits" "$tmp/out.hdr")
expect "mProject (.fits.gz)" "$out" 'stat="OK"'

out=$(run mImgtbl "$tmp/gz" "$tmp/gz.tbl")
expect "mImgtbl (.fits.gz)" "$out" 'stat="OK"' 'count=1' 'badfits=0' 'badwcs=0'

for f in img.fits img_area.fits; do
    if ! cmp -s "$tmp/proj/$f" "$tmp/projgz/$f"; then
        echo "FAIL: reprojecting the .fits.gz input gave a different $f" >&2
        exit 1
    fi
done
echo "ok   .fits.gz input reprojects identically to the uncompressed image"
