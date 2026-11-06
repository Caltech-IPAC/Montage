#!/bin/sh
# mMakeImg and mSubHdr must not depend on newly allocated memory being zero.
#
# Both read the template into a malloc'd buffer with strcat and never
# cleared it first, so the header was appended after whatever the buffer
# held. mMakeImg also read its -a array file name when -a was not given.
# With fresh, zeroed memory this worked, but with reused memory (repeated
# MontagePy calls) the header was corrupted or overran the buffer, and
# mMakeImg could fail with "Image array file [...] not found".
#
# To make those failures repeatable, the programs run with the C library
# filling new allocations with nonzero bytes, using MALLOC_PERTURB_ (glibc)
# and MallocScribble (macOS). Each variable is ignored by the other library.
# mMakeImg with no options must then write a blank (all zero) image, and
# mSubHdr must cut the expected 20x20 header.
#
# Needs a completed `make`. Uses bin/ under the repository root unless
# BINDIR is set.
set -eu

# An exported CDPATH makes `cd` print the directory, corrupting $root.
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd)
bin=$(cd "${BINDIR:-$root/bin}" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

run() {  # run <program> <args...>: its output, or FAIL if it exits nonzero
    p=$1
    shift
    status=0
    run_out=$(MALLOC_PERTURB_=165 MallocScribble=1 "$bin/$p" "$@" 2>&1) || status=$?
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
END
EOF

out=$(run mMakeImg "$tmp/in.hdr" "$tmp/img.fits")
expect "mMakeImg (no options)" "$out" 'stat="OK"'

# Statistics within a 0.005-degree (5-pixel) radius of the field centre.
out=$(run mExamine -p 10.0 20.0 0.005 "$tmp/img.fits")
expect "mExamine (centre)" "$out" 'stat="OK"'
for key in aveflux fluxmin fluxmax nnull; do
    val=$(printf '%s\n' "$out" | sed -n "s/.*, $key=\([^,]*\),.*/\1/p")
    if ! awk -v v="$val" 'BEGIN { exit !(v ~ /^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$/ && v + 0 == 0) }'; then
        echo "FAIL: centre $key is '$val', expected 0: $out" >&2
        exit 1
    fi
done
echo "ok   image is blank (mean, minimum, maximum and null count are 0)"

# 0.02 degrees at 0.001 degrees per pixel is 20 pixels on a side.
out=$(run mSubHdr "$tmp/in.hdr" "$tmp/sub.hdr" 10.0 20.0 0.02)
expect mSubHdr "$out" 'stat="OK"'
for card in 'NAXIS1  = 20' 'NAXIS2  = 20'; do
    if ! grep -F -x "$card" "$tmp/sub.hdr" >/dev/null; then
        echo "FAIL: mSubHdr output lacks '$card':" >&2
        cat "$tmp/sub.hdr" >&2
        exit 1
    fi
done
echo "ok   mSubHdr header is 20x20"
