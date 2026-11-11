#!/bin/sh
# Regression test: make_local.sh and stage_package.sh must clean only
# their own package directory, never the caller's working directory.
# The guard (`unset CDPATH` + `cd "$(dirname "$0")"`) must work when
# the scripts are invoked by absolute path, by relative path from a
# parent directory with a decoy CDPATH, must fail before any `rm`
# when `cd` itself fails, must reject symlink invocation, and must
# reject a bare script name unless the script is in the working
# directory, where it must still work.
#
# No `make` or Python is required. The copies fail at `cp __init__.py`
# by design, after the `rm` calls have run.
set -eu
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd)

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 131' QUIT
trap 'exit 143' TERM

pkg="$tmp/pkg"
caller="$tmp/caller"

# ---- helpers ----------------------------------------------------------------

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

expect_removed() {
    [ ! -e "$1" ] || fail "expected $1 to be removed but it still exists"
}

expect_kept() {
    [ -e "$1" ] || fail "expected $1 to survive but it was removed ($2)"
}

# stage_package.sh removes: src  build
# make_local.sh removes:    dist  (then calls stage_package.sh)

setup() {
    rm -rf "$pkg" "$caller"
    mkdir -p "$pkg" "$caller"

    cp "$root/python/MontagePy/make_local.sh"    "$pkg/make_local.sh"
    cp "$root/python/MontagePy/stage_package.sh" "$pkg/stage_package.sh"

    # Markers inside the package copy (what the scripts must clean).
    mkdir -p "$pkg/src" "$pkg/build" "$pkg/dist"
    touch "$pkg/src/marker" "$pkg/build/marker" "$pkg/dist/marker"

    # Markers in the caller directory (must never be touched).
    mkdir -p "$caller/src" "$caller/build" "$caller/dist"
    touch "$caller/src/marker" "$caller/build/marker" "$caller/dist/marker"
}

check_stage_pkg_cleaned() {
    expect_removed "$pkg/src/marker"
    expect_removed "$pkg/build/marker"
}

check_make_local_cleaned() {
    expect_removed "$pkg/dist/marker"
    expect_removed "$pkg/src/marker"
    expect_removed "$pkg/build/marker"
}

check_caller_untouched() {
    expect_kept "$caller/src/marker"   "$1"
    expect_kept "$caller/build/marker" "$1"
    expect_kept "$caller/dist/marker"  "$1"
}

rsh() { (cd "$caller" && sh "$1" > /dev/null 2>&1) || true; }

# ---- case 1: absolute path --------------------------------------------------

setup
rsh "$pkg/stage_package.sh"
check_stage_pkg_cleaned
check_caller_untouched "absolute path, stage_package.sh"
echo "ok   stage_package.sh: absolute path cleans package dir, not caller"

setup
rsh "$pkg/make_local.sh"
check_make_local_cleaned
check_caller_untouched "absolute path, make_local.sh"
echo "ok   make_local.sh: absolute path cleans package dir, not caller"

# ---- case 2: relative path --------------------------------------------------

relpath="../$(basename "$pkg")"

setup
rsh "$relpath/stage_package.sh"
check_stage_pkg_cleaned
check_caller_untouched "relative path, stage_package.sh"
echo "ok   stage_package.sh: relative path cleans package dir, not caller"

setup
rsh "$relpath/make_local.sh"
check_make_local_cleaned
check_caller_untouched "relative path, make_local.sh"
echo "ok   make_local.sh: relative path cleans package dir, not caller"

# ---- case 3: decoy CDPATH ---------------------------------------------------
# Invoke with a path whose directory component is a bare relative name so
# dirname "$0" is relative and CDPATH would redirect "cd pkg" to the decoy
# tree if unset CDPATH were absent. The -f check would then stop the
# script inside the decoy before any rm, so a redirect shows up as pkg
# markers that were not removed. The decoy markers are a backstop.

decoy_base="$tmp/decoy"
decoy_pkg="$decoy_base/pkg"
mkdir -p "$decoy_pkg/src" "$decoy_pkg/build" "$decoy_pkg/dist"
touch "$decoy_pkg/src/marker" "$decoy_pkg/build/marker" "$decoy_pkg/dist/marker"

setup
# Invoke "pkg/stage_package.sh" (relative) from $tmp with CDPATH pointing
# at decoy_base. Without unset CDPATH, "cd pkg" would go to decoy_pkg.
(cd "$tmp" && CDPATH="$decoy_base" sh "pkg/stage_package.sh" \
    > /dev/null 2>&1) || true
check_stage_pkg_cleaned
# Decoy cleanup targets must be untouched.
expect_kept "$decoy_pkg/src/marker"   "decoy src/marker (stage_package.sh)"
expect_kept "$decoy_pkg/build/marker" "decoy build/marker (stage_package.sh)"
check_caller_untouched "decoy CDPATH, stage_package.sh"
echo "ok   stage_package.sh: decoy CDPATH does not redirect rm"

setup
(cd "$tmp" && CDPATH="$decoy_base" sh "pkg/make_local.sh" \
    > /dev/null 2>&1) || true
check_make_local_cleaned
expect_kept "$decoy_pkg/src/marker"   "decoy src/marker (make_local.sh)"
expect_kept "$decoy_pkg/build/marker" "decoy build/marker (make_local.sh)"
expect_kept "$decoy_pkg/dist/marker"  "decoy dist/marker (make_local.sh)"
check_caller_untouched "decoy CDPATH, make_local.sh"
echo "ok   make_local.sh: decoy CDPATH does not redirect rm"

rm -rf "$decoy_base"

# ---- case 4: cd failure exits before rm -------------------------------------
# Pass a fixture-owned nonexistent $0 so dirname yields a directory that is
# guaranteed not to exist. Wrap in (cd "$caller" && ...) so a guard regression
# deletes $caller markers rather than the checkout. The nonzero-exit assertion
# catches guards that exit 0 on failure. chmod is not used because the gcc15
# container runs as root.

setup
rc=0
(cd "$caller" && sh -c "$(cat "$pkg/stage_package.sh")" \
    "$tmp/missing/stage_package.sh" > /dev/null 2>&1) || rc=$?
[ "$rc" -ne 0 ] \
    || fail "expected nonzero exit from cd failure (stage_package.sh)"
check_caller_untouched "cd failure, stage_package.sh"
echo "ok   stage_package.sh: cd failure exits before rm"

setup
rc=0
(cd "$caller" && sh -c "$(cat "$pkg/make_local.sh")" \
    "$tmp/missing/make_local.sh" > /dev/null 2>&1) || rc=$?
[ "$rc" -ne 0 ] \
    || fail "expected nonzero exit from cd failure (make_local.sh)"
check_caller_untouched "cd failure, make_local.sh"
echo "ok   make_local.sh: cd failure exits before rm"

# ---- case 5: symlink invocation is rejected before any rm ----------------
# Create symlinks to the scripts in a foreign directory (linkdir) and run
# them from $caller, so a guard regression deletes fixture markers rather
# than the checkout. Assert nonzero exit and that no marker is touched.
# The linkdir markers catch a deleted -L guard, the caller markers a script
# that stays in the working directory, and the pkg markers a script that
# follows the link instead of refusing it.

linkdir="$tmp/linkdir"
mkdir -p "$linkdir/src" "$linkdir/build" "$linkdir/dist"
touch "$linkdir/src/marker" "$linkdir/build/marker" "$linkdir/dist/marker"

setup
rc=0
ln -s "$pkg/stage_package.sh" "$linkdir/stage_package.sh"
(cd "$caller" && sh "$linkdir/stage_package.sh" > /dev/null 2>&1) || rc=$?
[ "$rc" -ne 0 ] \
    || fail "expected nonzero exit from symlink invocation (stage_package.sh)"
expect_kept "$pkg/src/marker"   "symlink rejection must not reach rm (stage_package.sh)"
expect_kept "$pkg/build/marker" "symlink rejection must not reach rm (stage_package.sh)"
expect_kept "$linkdir/src/marker"   "symlink rejection must not touch linkdir (stage_package.sh)"
expect_kept "$linkdir/build/marker" "symlink rejection must not touch linkdir (stage_package.sh)"
check_caller_untouched "symlink invocation, stage_package.sh"
echo "ok   stage_package.sh: symlink invocation is rejected"

setup
rc=0
ln -s "$pkg/make_local.sh" "$linkdir/make_local.sh"
(cd "$caller" && sh "$linkdir/make_local.sh" > /dev/null 2>&1) || rc=$?
[ "$rc" -ne 0 ] \
    || fail "expected nonzero exit from symlink invocation (make_local.sh)"
expect_kept "$pkg/dist/marker"     "symlink rejection must not reach rm (make_local.sh)"
expect_kept "$linkdir/dist/marker" "symlink rejection must not touch linkdir (make_local.sh)"
check_caller_untouched "symlink invocation, make_local.sh"
echo "ok   make_local.sh: symlink invocation is rejected"

# ---- case 6: bare name outside the package dir is rejected before any rm ----
# sh may find a script through PATH when it is named without a directory.
# $0 is then the bare name and the cd stays in the caller's directory.
# Passing the bare name as $0 reaches the guard on every shell. The real
# PATH lookup is run as well, though dash refuses it on its own.

setup
rc=0
(cd "$caller" && sh -c "$(cat "$pkg/stage_package.sh")" stage_package.sh \
    > /dev/null 2>&1) || rc=$?
[ "$rc" -ne 0 ] \
    || fail "expected nonzero exit from bare name (stage_package.sh)"
check_caller_untouched "bare name, stage_package.sh"
rc=0
(cd "$caller" && PATH="$pkg:$PATH" sh stage_package.sh \
    > /dev/null 2>&1) || rc=$?
[ "$rc" -ne 0 ] \
    || fail "expected nonzero exit from PATH lookup (stage_package.sh)"
check_caller_untouched "PATH lookup, stage_package.sh"
echo "ok   stage_package.sh: bare name outside the package dir is rejected"

setup
rc=0
(cd "$caller" && sh -c "$(cat "$pkg/make_local.sh")" make_local.sh \
    > /dev/null 2>&1) || rc=$?
[ "$rc" -ne 0 ] \
    || fail "expected nonzero exit from bare name (make_local.sh)"
check_caller_untouched "bare name, make_local.sh"
rc=0
(cd "$caller" && PATH="$pkg:$PATH" sh make_local.sh \
    > /dev/null 2>&1) || rc=$?
[ "$rc" -ne 0 ] \
    || fail "expected nonzero exit from PATH lookup (make_local.sh)"
check_caller_untouched "PATH lookup, make_local.sh"
echo "ok   make_local.sh: bare name outside the package dir is rejected"

# ---- case 7: bare name inside the package dir still works -------------------
# ci.yml runs `sh make_local.sh` from the package directory, and
# make_local.sh runs `sh stage_package.sh` the same way.

setup
(cd "$pkg" && sh stage_package.sh > /dev/null 2>&1) || true
check_stage_pkg_cleaned
check_caller_untouched "bare name in the package dir, stage_package.sh"
echo "ok   stage_package.sh: bare name in the package dir cleans it"

setup
(cd "$pkg" && sh make_local.sh > /dev/null 2>&1) || true
check_make_local_cleaned
check_caller_untouched "bare name in the package dir, make_local.sh"
echo "ok   make_local.sh: bare name in the package dir cleans it"
