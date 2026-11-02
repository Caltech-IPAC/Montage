#!/bin/sh
# Helper for the system-CFITSIO tests (not a test itself):
#   sh tests/upstream_cfitsio.sh <prefix> <workdir>
# installs the vendored zlib 1.3.2 and CFITSIO 4.7.0 into <prefix> the way an
# upstream user or distribution would: two separate libraries, each built out
# of tree with its own configure and `make install` (shared and static, with
# their cfitsio.pc and zlib.pc). Unlike the bundled lib/libcfitsio.a, this
# libcfitsio does not contain zlib. Build logs go to <workdir>.
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
prefix=$1 work=$2
mkdir -p "$prefix" "$work/zlib-build" "$work/cfitsio-build"

# The nested makes must not inherit the caller's make flags (e.g. -i or -k
# from `make -k test`).
submake() {
    env -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS make "$@"
}

if ! { (cd "$work/zlib-build" &&
            CFLAGS='-g -O2' "$root/lib/src/zlib-1.3.2/configure" --prefix="$prefix" &&
            submake && submake install) &&
       (cd "$work/cfitsio-build" && CFLAGS='-g -O2' \
            CPPFLAGS="-I$prefix/include" LDFLAGS="-L$prefix/lib" \
            "$root/lib/src/cfitsio-4.7.0/configure" --prefix="$prefix" \
                --disable-curl --without-fortran &&
            submake && submake install); } > "$work/upstream-build.log" 2>&1; then
    tail -n 30 "$work/upstream-build.log" >&2
    echo "FAIL: could not build and install the upstream-style CFITSIO and zlib" >&2
    exit 1
fi
