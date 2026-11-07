#!/bin/sh
# Tests for the bundled CFITSIO upgrade:
# - cfitsio_version.c: the installed headers and the linked lib/libcfitsio.a
#   are both CFITSIO 4.7.0;
# - cve_vlen_regression.c: reading a variable-length string longer than
#   CFITSIO's internal buffer fails with exactly READ_ERROR (the
#   CVE-2026-115504 fix) instead of overflowing.
# - cfitsio_dotz_path.c: CFITSIO cannot open a gzip file in a path that
#   contains ".Z", because it reads that as a Unix compress file.
#
# Needs a completed `make` (uses lib/include and lib/libcfitsio.a).
set -eu

# An exported CDPATH makes `cd` print the directory, corrupting $root.
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd)

if [ "$(sed -n 1p "$root/.cfitsio-build-mode" 2>/dev/null)" = 1 ]; then
    echo "SKIP: needs a bundled CFITSIO build (this tree was built with USE_SYSTEM_CFITSIO=1)"
    exit 77
fi

cc=${CC:-cc}
# The name has no dot before the random part, because cfitsio_dotz_path
# needs a path without ".Z".
tmp=$(mktemp -d "${TMPDIR:-/tmp}/montage-cfitsio-bundled-XXXXXXXX")
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

if [ ! -f "$root/lib/libcfitsio.a" ] || [ ! -f "$root/lib/include/fitsio.h" ]; then
    echo "FAIL: lib/libcfitsio.a or lib/include/fitsio.h missing; run make first" >&2
    exit 1
fi

fail=0
for t in cfitsio_version cve_vlen_regression cfitsio_dotz_path; do
    case $t:$tmp in
        cfitsio_dotz_path:*.Z*|cfitsio_dotz_path:*.bz2*)
            echo "SKIP: $t needs a temp path without .Z or .bz2, not $tmp"
            continue
            ;;
    esac
    if ! $cc -I"$root/lib/include" -o "$tmp/$t" "$root/tests/$t.c" \
            "$root/lib/libcfitsio.a" -lm; then
        echo "FAIL: could not build $t" >&2
        fail=1
        continue
    fi
    # Run in the temporary directory: the CVE test writes a scratch FITS file.
    if (cd "$tmp" && "./$t"); then :; else fail=1; fi
done
exit $fail
