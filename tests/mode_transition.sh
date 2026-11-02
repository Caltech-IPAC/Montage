#!/bin/sh
# The CFITSIO build-mode guard of the top-level Makefile:
# - switching between the bundled and a system CFITSIO without `make clean`
#   fails with a message saying so, in both directions;
# - so does switching between two system CFITSIOs (provider A -> B);
# - the .cfitsio-build-mode stamp (mode, CFITSIO_CFLAGS, CFITSIO_LIBS) is
#   written when a build starts, after the mode and CFITSIO checks pass, so
#   a build that fails part way still counts: provider A failing mid-build,
#   then provider B without `make clean`, is rejected. `make clean` removes it.
#
# Runs the real top-level Makefile in a temporary tree whose only
# subdirectory is a stub util/Makefile, so a "build" takes no time and can
# be made to fail with STUB_FAIL=1. System providers A and B are the bundled
# lib/libcfitsio.a in place and a copy of it, through explicit
# CFITSIO_CFLAGS/CFITSIO_LIBS.
#
# Needs a completed `make` (uses lib/libcfitsio.a and lib/include).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)

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

if [ ! -f "$root/lib/libcfitsio.a" ] || [ ! -f "$root/lib/include/fitsio.h" ]; then
    echo "FAIL: lib/libcfitsio.a or lib/include/fitsio.h missing; run make first" >&2
    exit 1
fi

t=$tmp/tree
mkdir -p "$t/tests" "$t/util"
cp "$root/Makefile" "$root/cfitsio.make" "$t/"
cp "$root/tests/cfitsio_system_version.c" "$root/tests/cve_vlen_regression.c" "$t/tests/"
printf '%s\n' 'all:' \
    '	@test "$(STUB_FAIL)" != 1 || { echo "stub build failed" >&2; exit 1; }' \
    'clean:' > "$t/util/Makefile"

system="USE_SYSTEM_CFITSIO=1"
sysflags_c="-I$root/lib/include"
sysflags_l="-L$root/lib -lcfitsio -lm"
b=$tmp/provider-b
mkdir -p "$b/include" "$b/lib"
cp "$root/lib/include/fitsio.h" "$root/lib/include/longnam.h" "$b/include/"
cp "$root/lib/libcfitsio.a" "$b/lib/"
stamp=$t/.cfitsio-build-mode

run() {  # run <make args...>: output in $out, status in $rc
    rc=0
    out=$(env -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
            -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
            -u MAKEFLAGS -u MFLAGS -u MAKELEVEL -u GNUMAKEFLAGS \
            make -s -C "$t" "$@" 2>&1) || rc=$?
}
run_system() {
    run "$@" $system CFITSIO_CFLAGS="$sysflags_c" CFITSIO_LIBS="$sysflags_l"
}
must_fail() {  # must_fail <case> <message>
    if [ "$rc" -eq 0 ]; then echo "FAIL: $1: succeeded: $out" >&2; exit 1; fi
    case "$out" in
        *"$2"*) ;;
        *) echo "FAIL: $1: missing '$2' in: $out" >&2; exit 1 ;;
    esac
    echo "ok   $1 (rejected)"
}
must_pass() {  # must_pass <case>
    if [ "$rc" -ne 0 ]; then echo "FAIL: $1: failed: $out" >&2; exit 1; fi
    echo "ok   $1"
}
stamp_is() {  # stamp_is <case> <absent|0|1> [CFITSIO_LIBS]
    if [ "$2" = absent ]; then
        [ ! -e "$stamp" ] || { echo "FAIL: $1: stamp exists ($(cat "$stamp"))" >&2; exit 1; }
    elif [ ! -f "$stamp" ] || [ "$(sed -n 1p "$stamp")" != "$2" ] ||
         { [ $# -gt 2 ] && [ "$(sed -n 3p "$stamp")" != "$3" ]; }; then
        echo "FAIL: $1: stamp is not mode '$2' ${3:+with CFITSIO_LIBS '$3'}" >&2
        exit 1
    fi
}

to_system="ERROR: this tree was built with the bundled CFITSIO (USE_SYSTEM_CFITSIO=0); run 'make clean' before building with USE_SYSTEM_CFITSIO=1."
to_bundled="ERROR: this tree was built with a system CFITSIO (USE_SYSTEM_CFITSIO=1); run 'make clean' before building with USE_SYSTEM_CFITSIO=0."

to_b="ERROR: this tree was built with a different CFITSIO (CFITSIO_CFLAGS='$sysflags_c' CFITSIO_LIBS='$sysflags_l'); run 'make clean' before building with CFITSIO_CFLAGS='-I$b/include' CFITSIO_LIBS='-L$b/lib -lcfitsio -lm'."

# Bundled mode.
run STUB_FAIL=1;          must_fail "failed bundled build" "stub build failed"
stamp_is "failed bundled build" 0
run;                      must_pass "bundled build"
stamp_is "bundled build" 0
run;                      must_pass "bundled rebuild"
run_system check-build-mode; must_fail "bundled -> system (check-build-mode)" "$to_system"
run_system;               must_fail "bundled -> system (make)" "$to_system"
stamp_is "rejected switch to system" 0
run clean;                must_pass "make clean"
stamp_is "make clean" absent

# System mode.
run_system STUB_FAIL=1;   must_fail "failed system build" "stub build failed"
stamp_is "failed system build" 1 "$sysflags_l"
run $system CFITSIO_CFLAGS="-I$b/include" CFITSIO_LIBS="-L$b/lib -lcfitsio -lm"
must_fail "failed system build, then provider B" "$to_b"
stamp_is "rejected provider B after a failed build" 1 "$sysflags_l"
run_system;               must_pass "system build"
stamp_is "system build" 1 "$sysflags_l"
run check-build-mode;     must_fail "system -> bundled (check-build-mode)" "$to_bundled"
run;                      must_fail "system -> bundled (make)" "$to_bundled"
stamp_is "rejected switch to bundled" 1
run $system CFITSIO_CFLAGS="-I$b/include" CFITSIO_LIBS="-L$b/lib -lcfitsio -lm"
must_fail "system provider A -> B" "$to_b"
stamp_is "rejected switch to provider B" 1 "$sysflags_l"
run_system;               must_pass "system rebuild, same provider"
run_system clean;         must_pass "make clean (system mode)"
stamp_is "make clean (system mode)" absent

# Trees without a stamp, e.g. built before the stamp existed.
mkdir -p "$t/lib"
: > "$t/lib/libcfitsio.a"
run_system;               must_fail "unstamped bundled tree -> system" "$to_system"
rm "$t/lib/libcfitsio.a"
: > "$t/lib/libwcs.a"
run;                      must_fail "unstamped system tree -> bundled" "$to_bundled"
stamp_is "unstamped system tree" absent

echo "PASS: CFITSIO build-mode guard and stamp"
