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
# working tree is never modified.
#
# If the compiler cannot apply C23's prototype rules (GCC <= 12, clang <= 14)
# the test cannot prove anything and reports SKIP (exit 77), not a failure.
#
# Environment:
#   REQUIRE_C23=1  treat "cannot run" as a failure (for CI, so a capable
#                  compiler can never silently skip this test)
#   SRC_TREE=dir   build from this pristine source tree instead of copying
#                  the checkout (e.g. a `git archive` export of a commit)
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

cannot_run() {
    if [ "${REQUIRE_C23:-0}" = 1 ]; then
        echo "FAIL: $1 (REQUIRE_C23=1)" >&2
        exit 1
    fi
    echo "SKIP: $1"
    exit 77
}

real_gcc=$(command -v gcc)
real_cc=$(command -v cc)

# Pick the C23 spelling this compiler accepts.
printf 'int main(void) { return 0; }\n' > "$tmp/ok.c"
std=
for s in gnu23 gnu2x; do
    if "$real_gcc" -std=$s -c "$tmp/ok.c" -o "$tmp/ok.o" 2>/dev/null; then
        std=$s
        break
    fi
done
if [ -z "$std" ]; then
    cannot_run "$real_gcc accepts neither -std=gnu23 nor -std=gnu2x"
fi

# The test is only meaningful if this compiler applies C23's rule that
# `f()` takes no arguments. Refuse to pass vacuously otherwise.
printf 'void f();\nint main(void) { f(1); return 0; }\n' > "$tmp/kr.c"
if "$real_gcc" -std=$std -c "$tmp/kr.c" -o "$tmp/kr.o" 2>/dev/null; then
    cannot_run "$real_gcc -std=$std does not apply C23 prototype rules"
fi

mkdir "$tmp/bin"
printf '#!/bin/sh\nexec "%s" -std=%s "$@"\n' "$real_gcc" "$std" > "$tmp/bin/gcc"
printf '#!/bin/sh\nexec "%s" -std=%s "$@"\n' "$real_cc" "$std" > "$tmp/bin/cc"
chmod +x "$tmp/bin/gcc" "$tmp/bin/cc"

# Copy source files only, so nothing already compiled is reused:
# - SRC_TREE: an existing pristine source tree (e.g. a `git archive` export);
# - otherwise tracked + untracked files, minus ignored build output;
# - without git, the whole tree followed by `make clean`.
mkdir "$tmp/src"
if [ -n "${SRC_TREE:-}" ]; then
    cp -R "$SRC_TREE/." "$tmp/src"
elif git -C "$root" rev-parse --git-dir > /dev/null 2>&1; then
    (cd "$root" && git ls-files -z --cached --others --exclude-standard) \
        | (cd "$root" && tar --null -T - -cf -) | tar -xf - -C "$tmp/src"
else
    cp -R "$root/." "$tmp/src"
    make -C "$tmp/src" clean > /dev/null 2>&1
fi

# Build with only the wrapper's -std in effect: drop inherited compiler/flag
# and make settings that could mask a missing pin (e.g. CFLAGS=-std=gnu17)
# or ignore errors (e.g. MAKEFLAGS=-i from `make -i test`).
echo "compiler: $("$real_gcc" --version | sed -n 1p); default forced to -std=$std"
if ! env -u CC -u CFLAGS -u CPPFLAGS -u LDFLAGS -u MAKEFLAGS -u MFLAGS \
        -u MAKELEVEL PATH="$tmp/bin:$PATH" \
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
