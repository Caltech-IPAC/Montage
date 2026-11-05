#!/bin/sh
# Compare two Montage builds on generated, offline inputs, on one machine.
# Every Montage step must exit successfully and report stat="OK".
# Uses the tests/ convention of BINDIR-independent invocations and OK checks.
set -eu
unset CDPATH
LC_ALL=C
export LC_ALL

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 2
}

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    fail 'usage: output_compare.sh BASE_BINDIR CAND_BINDIR [WORKDIR]'
fi
base_bin=$(cd "$1" && pwd -P) || fail "cannot open base bindir: $1"
cand_bin=$(cd "$2" && pwd -P) || fail "cannot open candidate bindir: $2"

# mHdr is skipped: MontageLib/Hdr/montageHdr.c always requests a header
# from montage.ipac.caltech.edu over HTTP, even for numeric coordinates.
# mProjectQL is optional only when neither build installs it. If just one
# build has it, the missing counterpart is a setup error, not a silent skip.
have_ql=0
if [ -x "$base_bin/mProjectQL" ] || [ -x "$cand_bin/mProjectQL" ]; then
    have_ql=1
fi
programs='mMakeImg mImgtbl mProject mProjectPP mOverlaps mDiffExec mFitExec
mBgModel mBgExec mAdd mShrink mSubimage mExamine mFixNaN mViewer
mHPXHdr mHPXCoord2Tile mConvert mTblSort'
if [ "$have_ql" -eq 1 ]; then
    programs="$programs mProjectQL"
fi
for bindir in "$base_bin" "$cand_bin"; do
    for program in $programs; do
        [ -x "$bindir/$program" ] || fail "missing program: $bindir/$program"
    done
done
for utility in awk cat cmp cp dd diff dirname find gzip gunzip mkdir mktemp pwd rm sed sort; do
    command -v "$utility" >/dev/null 2>&1 || fail "missing utility: $utility"
done
script_dir=$(cd "$(dirname "$0")" && pwd -P) || fail 'cannot open script directory'
[ -r "$script_dir/data/fits_inputs.fits" ] || fail 'missing FITS input fixture'

remove_work=0
if [ "$#" -eq 3 ]; then
    mkdir -p "$3" || fail "cannot create work directory: $3"
    work=$(cd "$3" && pwd -P) || fail "cannot open work directory: $3"
else
    work=$(mktemp -d "${TMPDIR:-/tmp}/montage-output-compare.XXXXXXXX") ||
        fail 'mktemp failed'
    remove_work=1
fi
# Never erase or reuse a caller's output. A WORKDIR may hold other files,
# but these reserved names must not already exist, even as broken symlinks.
for reserved in base candidate compare; do
    [ ! -e "$work/$reserved" ] && [ ! -L "$work/$reserved" ] ||
        fail "work directory already contains $reserved: $work"
done
phase=setup
# shellcheck disable=SC2329 # Invoked by the POSIX exit trap (signal 0).
cleanup() {
    status=$?
    trap - 0
    # An unhandled shell or utility failure during setup/pipeline is error 2.
    if [ "$status" -ne 0 ] && [ "$phase" != compare ]; then
        status=2
    fi
    if [ "$remove_work" -eq 1 ] && [ "${KEEP:-0}" != 1 ]; then
        rm -rf "$work"
    else
        printf 'Work directory: %s\n' "$work" >&2
    fi
    exit "$status"
}
trap 'cleanup' 0
trap 'exit 2' HUP INT PIPE QUIT TERM
mkdir "$work/base" "$work/candidate" "$work/compare"
printf 'Base: %s\nCandidate: %s\n' "$base_bin" "$cand_bin"

# Only status-line elapsed time is normalized. These five programs print
# time() deltas as their final field. No FITS card, pixel, table field,
# filename, error message, or other numeric result is changed.
normalize_stdout() {
    case "$1" in
        mProject|mProjectPP|mProjectQL|mAdd|mShrink)
            sed 's/\(, time=\)[0-9][0-9.]*]/\1<elapsed>]/g' "$2" ;;
        *) cat "$2" ;;
    esac
}

run() {  # run <unique-step-name> <program> <args...>
    step=$1
    program=$2
    shift 2
    command_status=0
    "$bin/$program" "$@" > "logs/$step.stdout.tmp" 2> "logs/$step.stderr" ||
        command_status=$?
    out=$(cat "logs/$step.stdout.tmp")
    normalize_stdout "$program" "logs/$step.stdout.tmp" > "logs/$step.stdout"
    rm "logs/$step.stdout.tmp"
    if [ "$command_status" -ne 0 ]; then
        cat "logs/$step.stdout" "logs/$step.stderr" >&2
        fail "$label / $step exited $command_status"
    fi
    case "$out" in
        *'stat="ERROR"'*|*'stat="ABORT"'*)
            cat "logs/$step.stdout" "logs/$step.stderr" >&2
            fail "$label / $step reported failure" ;;
        *'stat="OK"'*) ;;
        *)
            cat "logs/$step.stdout" "logs/$step.stderr" >&2
            fail "$label / $step did not report stat=\"OK\"" ;;
    esac
}

expect() {  # expect <key=value> ... as whole fields in the most recent step
    for want in "$@"; do
        case "$out" in
            *" $want,"*|*" $want]"*) ;;
            *) fail "$label / $step: missing field '$want' in: $out" ;;
        esac
    done
}

header() {  # header <path> <nx> <ny> <ra> <dec> <rotation>
    awk -v nx="$2" -v ny="$3" -v ra="$4" -v dec="$5" -v rot="$6" 'BEGIN {
        print "SIMPLE  =                    T"
        print "BITPIX  =                  -64"
        print "NAXIS   =                    2"
        printf "NAXIS1  = %20d\n", nx
        printf "NAXIS2  = %20d\n", ny
        print "CTYPE1  = '\''RA---TAN'\''"
        print "CTYPE2  = '\''DEC--TAN'\''"
        print "EQUINOX =               2000.0"
        printf "CRVAL1  = %20.8f\n", ra
        printf "CRVAL2  = %20.8f\n", dec
        printf "CRPIX1  = %20.8f\n", (nx+1)/2
        printf "CRPIX2  = %20.8f\n", (ny+1)/2
        print "CDELT1  =                -0.01"
        print "CDELT2  =                 0.01"
        printf "CROTA2  = %20.8f\n", rot
        print "END"
    }' > "$1"
}

pipeline() {  # Runs in a subshell whose cwd is one empty run directory.
    bin=$1
    label=$2
    mkdir logs templates raw projected pp quick compressed projected-gz diffs corrected hpx products fixture
    cp "$script_dir/data/fits_inputs.fits" fixture/
    header templates/a.hdr 64 64 9.85 19.95 0
    header templates/b.hdr 64 64 10.10 20.05 9
    header templates/c.hdr 64 64 10.00 20.20 -7
    header templates/mosaic.hdr 112 112 10.00 20.05 17

    # Fixed-width IPAC tables. Explicit file lists remove readdir ordering
    # from mImgtbl, without sorting or otherwise rewriting any output table.
    {
        printf '|%16s|\n' fname
        printf '|%16s|\n' char
        printf ' %16s \n' a.fits b.fits c.fits
    } > inputs.tbl
    {
        printf '|%16s|\n' fname
        printf '|%16s|\n' char
        printf ' %16s \n' a.fits.gz
    } > gzip-input.tbl
    {
        printf '|%16s|\n' fname
        printf '|%16s|\n' char
        printf ' %16s \n' fits_inputs.fits
    } > fixture-input.tbl
    {
        printf '|%12s|%12s|%12s|\n' ra dec flux
        printf '|%12s|%12s|%12s|\n' double double double
        printf ' %12.6f %12.6f %12.3f \n' 9.90 20.00 35 10.08 20.12 50 10.18 19.94 22
    } > sources.tbl

    # mMakeImg uses rand() without srand(). Each invocation starts from the
    # libc default sequence. Noise is reproducible on the same machine.
    run make-a mMakeImg -n 0.03 -b 10 10 10 10 \
        -t sources.tbl flux 1.8 equatorial 2000 1 flux gaussian templates/a.hdr raw/a.fits
    run make-b mMakeImg -n 0.05 -b 11 12 13 12 \
        -t sources.tbl flux 2.4 equatorial 2000 1 flux gaussian templates/b.hdr raw/b.fits
    run make-c mMakeImg -n 0.07 -b 9 10.5 11.5 10 \
        -t sources.tbl flux 0 equatorial 2000 1 flux flat templates/c.hdr raw/c.fits
    run images-raw mImgtbl -c -t inputs.tbl raw raw.tbl
    expect 'count=3' 'badfits=0' 'badwcs=0'
    run sort-ascending mTblSort raw.tbl crval1 raw-sorted.tbl
    expect 'count=3'
    run sort-descending mTblSort -r raw.tbl crval1 raw-reverse.tbl
    expect 'count=3'

    for image in a b c; do
        run "project-$image" mProject "raw/$image.fits" "projected/$image.fits" templates/mosaic.hdr
    done
    run project-pp mProjectPP raw/b.fits pp/b.fits templates/mosaic.hdr
    if [ "$have_ql" -eq 1 ]; then
        # Use the default nearest neighbor mode. The -L mode gave different
        # pixels between two runs of the same build, so it is not used here.
        run project-quick mProjectQL raw/c.fits quick/c.fits templates/mosaic.hdr
    else
        printf '%s\n' 'SKIP mProjectQL: neither build installs this optional program.' > logs/skipped.txt
    fi

    # -n omits gzip's original filename and modification time. Its payload
    # is otherwise the unmodified generated FITS file.
    gzip -n -c raw/a.fits > compressed/a.fits.gz 2> logs/gzip.stderr
    run project-gzip mProject compressed/a.fits.gz projected-gz/a.fits templates/mosaic.hdr
    run images-gzip mImgtbl -c -t gzip-input.tbl compressed compressed.tbl
    expect 'count=1' 'badfits=0' 'badwcs=0'
    run images-projected mImgtbl -c -t inputs.tbl projected projected.tbl
    expect 'count=3' 'badfits=0' 'badwcs=0'
    run overlaps mOverlaps projected.tbl overlaps.tbl
    expect 'count=3'
    run differences mDiffExec -p projected overlaps.tbl templates/mosaic.hdr diffs
    expect 'count=3' 'failed=0'
    run fits mFitExec overlaps.tbl fits.tbl diffs
    expect 'count=3' 'failed=0' 'warning=0' 'missing=0'
    run background-model mBgModel -i 200 projected.tbl fits.tbl corrections.tbl
    run background-exec mBgExec -p projected projected.tbl corrections.tbl corrected
    expect 'count=3' 'nocorrection=0' 'failed=0'
    run images-corrected mImgtbl -c -t inputs.tbl corrected corrected.tbl
    expect 'count=3' 'badfits=0' 'badwcs=0'
    run add-default mAdd -p corrected corrected.tbl templates/mosaic.hdr products/mean.fits
    for mode in median count sum; do
        run "add-$mode" mAdd -a "$mode" -p corrected corrected.tbl templates/mosaic.hdr "products/$mode.fits"
    done
    run shrink mShrink products/mean.fits products/shrunk.fits 2
    run subimage-sky mSubimage products/mean.fits products/sub-sky.fits 10 20.05 0.30 0.25
    run subimage-pixel mSubimage -p products/mean.fits products/sub-pixel.fits 20 20 24 28
    run examine-all mExamine products/mean.fits
    run examine-region mExamine -p 10 20.05 0.10 products/mean.fits
    run fixnan-blank mFixNaN products/mean.fits products/blanked.fits 9 10
    run fixnan-fill mFixNaN -v -999 products/blanked.fits products/filled.fits
    run convert-int16 mConvert -b 16 -min -1000 -max 1000 products/filled.fits products/int16.fits
    run convert-float32 mConvert -b -32 products/mean.fits products/float32.fits
    for image in int16 float32; do
        {
            printf '|%16s|\n' fname
            printf '|%16s|\n' char
            printf ' %16s \n' "$image.fits"
        } > "$image-input.tbl"
        run "images-$image" mImgtbl -c -t "$image-input.tbl" products "$image.tbl"
        expect 'count=1' 'badfits=0' 'badwcs=0'
        run "project-$image" mProject "products/$image.fits" "products/project-$image.fits" templates/mosaic.hdr
        run "subimage-$image" mSubimage -p "products/$image.fits" "products/sub-$image.fits" 20 20 24 28
        run "examine-$image" mExamine "products/$image.fits"
    done

    run images-fixture mImgtbl -c -t fixture-input.tbl fixture fixture.tbl
    # The empty primary HDU has no WCS. Only its three image extensions count.
    expect 'count=3' 'badfits=0' 'badwcs=1'
    # mProjectPP decompresses and scales every fixture HDU. mProject, mSubimage,
    # mShrink and mProjectQL fail on some HDUs. See output_compare.md.
    fixture=fixture/fits_inputs.fits
    for hdu in 1 2 3; do
        run "projectpp-fixture-$hdu" mProjectPP -h "$hdu" "$fixture" \
            "products/projectpp-fixture-$hdu.fits" templates/mosaic.hdr
    done
    run shrink-fixture-3 mShrink -h 3 "$fixture" products/shrink-fixture-3.fits 2
    if [ "$have_ql" -eq 1 ]; then
        run projectql-fixture-3 mProjectQL -h 3 "$fixture" \
            products/projectql-fixture-3.fits templates/mosaic.hdr
    fi

    # Plain grayscale needs no fonts, overlays, catalogues, or network.
    run viewer mViewer -ct 0 -gray products/mean.fits 0 65 lin -out products/mean.png

    run healpix-header mHPXHdr 0 templates/healpix.hdr
    expect 'pixlevel=9' 'naxis=2560'
    run healpix-project mProject raw/a.fits hpx/a.fits templates/healpix.hdr
    for order in 5 12; do
        run "tile-equinox-$order" mHPXCoord2Tile "$order" 0 0
        run "tile-m42-$order" mHPXCoord2Tile "$order" 83.8221 -5.3911
        run "tile-galcen-$order" mHPXCoord2Tile "$order" 266.4168 -29.0078
    done
    # mHdr is deliberately never invoked. See the network-only skip above.
}

phase=pipeline
(cd "$work/base" && pipeline "$base_bin" base)
(cd "$work/candidate" && pipeline "$cand_bin" candidate)

# Decode the primary FITS header in 80-byte cards, including END, without
# searching binary data for the word END. The returned block count places
# the data at the next 2880-byte boundary. Generated FITS have one image HDU.
header_cards() {
    card_number=0
    while :; do
        card=$(dd if="$1" bs=80 skip="$card_number" count=1 2>/dev/null) || return 1
        [ "${#card}" -eq 80 ] || return 1
        printf '%06d %s\n' "$((card_number + 1))" "$card"
        card_number=$((card_number + 1))
        case "$card" in
            'END     '*)
                header_blocks=$(((card_number + 35) / 36))
                return 0 ;;
        esac
    done
}

# Python and numpy are used only after cmp has already decided DIFF.
# They never participate in the pass/fail result. No astropy is required.
pixel_report() {
    if command -v python3 >/dev/null 2>&1 &&
            python3 -c 'import numpy' >/dev/null 2>&1; then
        if python3 - "$1" "$2" <<'PY'
import math
import sys
import numpy as np


def image(path):
    header = {}
    with open(path, "rb") as stream:
        ended = False
        while not ended:
            block = stream.read(2880)
            if len(block) != 2880:
                raise ValueError("truncated FITS header")
            for offset in range(0, 2880, 80):
                card = block[offset:offset + 80].decode("ascii")
                key = card[:8].strip()
                if key == "END":
                    ended = True
                    break
                if card[8:10] == "= ":
                    header[key] = card[10:].split("/", 1)[0].strip()
        bitpix = int(header["BITPIX"])
        dtype = np.dtype({8: "u1", 16: ">i2", 32: ">i4", 64: ">i8",
                          -32: ">f4", -64: ">f8"}[bitpix])
        shape = tuple(int(header["NAXIS" + str(i)])
                      for i in range(1, int(header["NAXIS"]) + 1))
        count = math.prod(shape)
        payload = stream.read(count * dtype.itemsize)
        if len(payload) != count * dtype.itemsize:
            raise ValueError("truncated FITS pixels")
    raw = np.frombuffer(payload, dtype=dtype)
    values = raw.astype(np.longdouble)
    scale = np.longdouble(header.get("BSCALE", "1").replace("D", "E"))
    zero = np.longdouble(header.get("BZERO", "0").replace("D", "E"))
    if bitpix > 0 and "BLANK" in header:
        values[raw == int(header["BLANK"])] = np.nan
    values = values * scale + zero
    return shape, dtype, payload, values


try:
    sa, da, pa, a = image(sys.argv[1])
    sb, db, pb, b = image(sys.argv[2])
    if sa != sb:
        print("    pixel shapes differ: %s vs %s, max absolute difference unavailable" % (sa, sb))
    else:
        equal = (a == b) | (np.isnan(a) & np.isnan(b))
        changed = ~equal
        if da == db:
            # Include signed zero and NaN payload changes in the count.
            ra = np.frombuffer(pa, dtype=np.uint8).reshape(-1, da.itemsize)
            rb = np.frombuffer(pb, dtype=np.uint8).reshape(-1, db.itemsize)
            changed |= np.any(ra != rb, axis=1)
        count = int(np.count_nonzero(changed))
        finite = changed & np.isfinite(a) & np.isfinite(b)
        maximum = np.max(np.abs(a[finite] - b[finite])) if np.any(finite) else 0
        if np.any(changed & ~equal & ~(np.isfinite(a) & np.isfinite(b))):
            maximum = np.inf
        print("    differing pixels: %d / %d, max absolute difference: %s" %
              (count, a.size, maximum))
except (OSError, ValueError, KeyError, OverflowError) as error:
    print("    pixel data differs, optional detail unavailable: %s" % error)
PY
        then
            return
        fi
    fi
    if [ "$3" -eq 1 ]; then
        printf '%s\n' '    pixel data bytes identical (difference is in the header or header padding)'
    else
        printf '%s\n' '    pixel data differs (python3 with numpy is unavailable or could not report details)'
    fi
}

fits_details() {
    detail_a=$1
    detail_b=$2
    case "$3" in
        *.gz)
            if [ -f "$detail_a" ]; then
                gzip -d -c "$detail_a" > "$work/compare/a.fits" || return
                detail_a=$work/compare/a.fits
            fi
            if [ -f "$detail_b" ]; then
                gzip -d -c "$detail_b" > "$work/compare/b.fits" || return
                detail_b=$work/compare/b.fits
            fi ;;
    esac
    : > "$work/compare/a.header"
    : > "$work/compare/b.header"
    blocks_a=0
    blocks_b=0
    if [ -f "$detail_a" ]; then
        if header_cards "$detail_a" > "$work/compare/a.header"; then
            blocks_a=$header_blocks
        else
            printf '%s\n' '    base FITS header is incomplete or invalid'
        fi
    fi
    if [ -f "$detail_b" ]; then
        if header_cards "$detail_b" > "$work/compare/b.header"; then
            blocks_b=$header_blocks
        else
            printf '%s\n' '    candidate FITS header is incomplete or invalid'
        fi
    fi
    if cmp -s "$work/compare/a.header" "$work/compare/b.header"; then
        printf '%s\n' '    FITS header cards identical'
    else
        printf '%s\n' '    differing FITS header cards (- base, + candidate):'
        # diff exit 1 is the expected result here and cannot change verdict.
        diff -u "$work/compare/a.header" "$work/compare/b.header" || :
    fi
    if [ "$blocks_a" -eq 0 ] || [ "$blocks_b" -eq 0 ]; then
        printf '%s\n' '    pixel data differs or is missing, valid counterparts unavailable'
        return
    fi
    dd if="$detail_a" bs=2880 skip="$blocks_a" of="$work/compare/a.data" 2>/dev/null || return
    dd if="$detail_b" bs=2880 skip="$blocks_b" of="$work/compare/b.data" 2>/dev/null || return
    data_same=0
    if cmp -s "$work/compare/a.data" "$work/compare/b.data"; then
        data_same=1
    fi
    pixel_report "$detail_a" "$detail_b" "$data_same"
}

allowed_difference() {
    # Deliberate field splitting, but never pathname expansion. Patterns
    # match the whole relative filename. A '*' may match '/'.
    for pattern in ${EXPECT_DIFF:-}; do
        # shellcheck disable=SC2254 # EXPECT_DIFF entries are glob patterns.
        case "$1" in
            $pattern) return 0 ;;
        esac
    done
    return 1
}

# Include all non-directory entries, including hidden files and unexpected
# nonregular outputs. The union catches files present in only one build.
(cd "$work/base" && find . ! -type d -print) > "$work/compare/base.list" || fail 'cannot list base outputs'
(cd "$work/candidate" && find . ! -type d -print) > "$work/compare/candidate.list" || fail 'cannot list candidate outputs'
sort -u "$work/compare/base.list" "$work/compare/candidate.list" > "$work/compare/files.list" ||
    fail 'cannot sort output filenames'
phase=compare
same=0
expected=0
unexpected=0
set -f
while IFS= read -r entry; do
    file=${entry#./}
    a=$work/base/$file
    b=$work/candidate/$file
    if [ -f "$a" ] && [ -f "$b" ]; then
        cmp_status=0
        cmp -s "$a" "$b" || cmp_status=$?
        [ "$cmp_status" -le 1 ] || fail "cannot compare: $file"
    else
        cmp_status=1
    fi
    if [ "$cmp_status" -eq 0 ]; then
        printf 'same %s\n' "$file"
        same=$((same + 1))
    else
        if allowed_difference "$file"; then
            printf 'DIFF (expected) %s\n' "$file"
            expected=$((expected + 1))
        else
            printf 'DIFF %s\n' "$file"
            unexpected=$((unexpected + 1))
        fi
        if [ ! -f "$a" ]; then
            printf '%s\n' '    missing or nonregular in base'
        fi
        if [ ! -f "$b" ]; then
            printf '%s\n' '    missing or nonregular in candidate'
        fi
        case "$file" in
            *.fits|*.fit|*.FITS|*.FIT|*.fits.gz|*.fit.gz|*.FITS.gz|*.FIT.gz)
                fits_details "$a" "$b" "$file" ||
                    printf '%s\n' '    optional FITS detail report could not be completed' ;;
        esac
    fi
done < "$work/compare/files.list"
printf 'Summary: %s same, %s expected differences, %s unexpected differences\n' "$same" "$expected" "$unexpected"
if [ "$unexpected" -ne 0 ]; then
    exit 1
fi
exit 0
