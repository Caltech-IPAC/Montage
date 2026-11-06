#!/bin/sh
# Regression test for issue #96: Montage must build when the C compiler
# defaults to C23, as GCC 15 does. Under C23 an empty parameter list `f()`
# means `f(void)`, so calling such a function with arguments, or defining it
# with parameters, is an error.
#
# Any installed gcc/clang can reproduce GCC 15's default: a wrapper placed
# first on PATH prepends -std=gnu23 (or gnu2x) to every compile. An explicit
# -std= in a Makefile comes later on the command line and still wins, exactly
# as it does with GCC 15. A clean copy of the source tree is built; the
# working tree is never modified. The wrappers restore the caller's PATH
# before running the real compiler, so a ccache shim (e.g. gcc on PATH is
# /usr/lib/ccache/gcc) does not find the wrapper again and loop.
#
# If gcc or cc is not on PATH, or either cannot apply C23's prototype rules
# (GCC <= 12, clang <= 14), the test cannot prove anything and reports
# SKIP (exit 77), not a failure. A compiler that cannot build a trivial file
# is a FAIL.
#
# Environment:
#   REQUIRE_C23=1  treat "cannot run" as a failure (for CI, so a capable
#                  compiler can never silently skip this test).
#                  Only the exact value 1 is recognised.
#   SRC_TREE=dir   build from this pristine source tree instead of copying
#                  the checkout (e.g. a `git archive` export of a commit)
set -eu

# An exported CDPATH makes `cd` print the directory, corrupting $root.
# -P resolves symlinks, so $root compares equal to git's top level below.
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd -P)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT
# mktemp returns a relative path when TMPDIR is relative; $tmp/bin goes on
# PATH for a build that runs in another directory, so make it absolute.
tmp=$(cd "$tmp" && pwd)

cannot_run() {
    if [ "${REQUIRE_C23:-0}" = 1 ]; then
        echo "FAIL: $1 (REQUIRE_C23=1)" >&2
        exit 1
    fi
    echo "SKIP: $1"
    exit 77
}

real_gcc=$(command -v gcc) || cannot_run "no gcc found on PATH"
real_cc=$(command -v cc) || cannot_run "no cc found on PATH"

# A compiler that cannot build a trivial file is broken, not merely old.
printf 'int main(void) { return 0; }\n' > "$tmp/ok.c"
for c in "$real_gcc" "$real_cc"; do
    if ! "$c" -c "$tmp/ok.c" -o "$tmp/ok.o" 2> "$tmp/ok.err"; then
        cat "$tmp/ok.err" >&2
        echo "FAIL: $c cannot compile a trivial C file" >&2
        exit 1
    fi
done

# Pick a C23 spelling that both gcc and cc accept (cc is wrapped with the
# same -std as gcc).
std=
for s in gnu23 gnu2x; do
    if "$real_gcc" -std=$s -c "$tmp/ok.c" -o "$tmp/ok.o" 2>/dev/null &&
       "$real_cc" -std=$s -c "$tmp/ok.c" -o "$tmp/ok.o" 2>/dev/null; then
        std=$s
        break
    fi
done
if [ -z "$std" ]; then
    cannot_run "$real_gcc and $real_cc do not both accept -std=gnu23 or -std=gnu2x"
fi

# The test is only meaningful if both compilers apply C23's rule that
# `f()` takes no arguments. Refuse to pass vacuously otherwise.
printf 'void f();\nint main(void) { f(1); return 0; }\n' > "$tmp/kr.c"
for c in "$real_gcc" "$real_cc"; do
    if "$c" -std=$std -c "$tmp/kr.c" -o "$tmp/kr.o" 2>/dev/null; then
        cannot_run "$c -std=$std does not apply C23 prototype rules"
    fi
done

# C23_ORIG_PATH is the caller's PATH and C23_REAL_GCC/C23_REAL_CC the real
# compilers, all passed to the build below. Passing the paths as data keeps
# a path containing `$`, a backtick or `"` from being run as shell syntax.
mkdir "$tmp/bin"
printf '#!/bin/sh\nPATH=$C23_ORIG_PATH exec "$C23_REAL_GCC" -std=%s "$@"\n' "$std" > "$tmp/bin/gcc"
printf '#!/bin/sh\nPATH=$C23_ORIG_PATH exec "$C23_REAL_CC" -std=%s "$@"\n' "$std" > "$tmp/bin/cc"
chmod +x "$tmp/bin/gcc" "$tmp/bin/cc"

# Copy source files only, so nothing already compiled is reused:
# - SRC_TREE: an existing pristine source tree (e.g. a `git archive` export);
# - a git checkout of Montage itself: tracked + untracked files, minus
#   ignored build output;
# - otherwise (no git, or Montage unpacked inside some other repository,
#   whose ignore rules do not cover Montage's build output): the whole tree.
# Both copies of the checkout are then cleaned with `make clean`: a release
# tarball has no .gitignore, so `git init` in one would list its build
# output as untracked files, and reused objects would let make skip the
# compile this test exists to run.
mkdir "$tmp/src"
needs_clean=
if [ -n "${SRC_TREE:-}" ]; then
    cp -R "$SRC_TREE/." "$tmp/src"
elif [ "$(git -C "$root" rev-parse --show-toplevel 2>/dev/null)" = "$root" ]; then
    (cd "$root" && git ls-files -z --cached --others --exclude-standard) \
        > "$tmp/files" || { echo "FAIL: git ls-files failed in $root" >&2; exit 1; }
    (cd "$root" && tar --null -T "$tmp/files" -cf "$tmp/src.tar") ||
        { echo "FAIL: could not archive the source files" >&2; exit 1; }
    tar -xf "$tmp/src.tar" -C "$tmp/src" ||
        { echo "FAIL: could not extract the source copy" >&2; exit 1; }
    rm -f "$tmp/files" "$tmp/src.tar"
    needs_clean=1
else
    cp -R "$root/." "$tmp/src"
    needs_clean=1
fi
# cp -R keeps read-only modes (e.g. a read-only SRC_TREE); the build and the
# cleanup must be able to write here.
chmod -R u+w "$tmp/src"

# Build with only the wrapper's -std in effect: drop inherited compiler/flag
# and make settings that could mask a missing pin (e.g. CFLAGS=-std=gnu17)
# or ignore errors (e.g. MAKEFLAGS=-i from `make -i test`), and the CFITSIO
# selection, so a system-mode `make test` still builds the bundled default.
if [ -n "$needs_clean" ]; then
    env -u CC -u CFLAGS -u CPPFLAGS -u LDFLAGS -u MAKEFLAGS -u MFLAGS \
        -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKELEVEL -u GNUMAKEFLAGS make -C "$tmp/src" clean > /dev/null ||
        { echo "FAIL: make clean failed in the source copy" >&2; exit 1; }
fi
echo "compiler: $("$real_gcc" --version | sed -n 1p); default forced to -std=$std"
if ! env -u CC -u CFLAGS -u CPPFLAGS -u LDFLAGS -u MAKEFLAGS -u MFLAGS \
        -u USE_SYSTEM_CFITSIO -u CFITSIO_CFLAGS -u CFITSIO_LIBS -u CFITSIO_DIR \
        -u CFITSIO_STATIC -u CFITSIO_ALLOW_UNPATCHED \
        -u MAKELEVEL -u GNUMAKEFLAGS C23_ORIG_PATH="$PATH" PATH="$tmp/bin:$PATH" \
        C23_REAL_GCC="$real_gcc" C23_REAL_CC="$real_cc" \
        make -C "$tmp/src" > "$tmp/build.log" 2>&1; then
    grep -E ': error: |Error [0-9]' "$tmp/build.log" | head -n 20 >&2
    echo "FAIL: build with a C23 default failed" >&2
    exit 1
fi

# Programs from every directory that broke under C23 (issue #96).
missing=
for p in mProject mAdd mImgtbl mExec mCatMap mMovingTarget mPix2Coord \
         mSearch mDAG mDiffFit mExecTG mArchiveExec mHiPSPNGScripts \
         mHPXCoord2Tile; do
    [ -x "$tmp/src/bin/$p" ] || missing="$missing $p"
done
if [ -n "$missing" ]; then
    echo "FAIL: build finished but these programs are missing:$missing" >&2
    exit 1
fi
echo "PASS: full tree builds with a C23 default ($(ls "$tmp/src/bin" | wc -l | tr -d ' ') programs)"
