#!/bin/sh
# Tests for the bundled CFITSIO upgrade:
# - cfitsio_version.c: the installed headers and the linked lib/libcfitsio.a
#   are both CFITSIO 4.7.0;
# - cve_vlen_regression.c: reading a variable-length string longer than
#   CFITSIO's internal buffer fails with exactly READ_ERROR (the
#   CVE-2026-115504 fix) instead of overflowing.
#
# Needs a completed `make` (uses lib/include and lib/libcfitsio.a).
set -eu

# An exported CDPATH makes `cd` print the directory, corrupting $root.
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd)
cc=${CC:-cc}
tmp=$(mktemp -d)
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
for t in cfitsio_version cve_vlen_regression; do
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
