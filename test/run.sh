#!/bin/sh
#
# Golden tests for Montage.
#
# Each test is a shell script in cases/.  It runs in an empty temporary
# directory, with Montage's bin/ and this directory (for fitsdump) on PATH
# and $FIXTURES pointing at the committed input files.  Everything it
# prints, stdout and stderr, is saved to results/NAME.out, cleaned of
# values that change from run to run, and compared with expected/NAME.out.
#
# Usage: sh run.sh [NAME...]           compare (default: every case)
#        UPDATE=1 sh run.sh [NAME...]  overwrite expected/ with the output
#
# Exit status is 0 when every case matches, 1 otherwise.

set -u

testdir=$(cd "$(dirname "$0")" && pwd)
topdir=$(cd "$testdir/.." && pwd)

PATH="$testdir:$topdir/bin:$PATH"
FIXTURES="$testdir/fixtures"
export PATH FIXTURES

mkdir -p "$testdir/results" "$testdir/expected"

if [ $# -eq 0 ]; then
   set -- $(cd "$testdir/cases" && ls *.sh | sed 's/\.sh$//')
fi

npass=0
nfail=0

for name in "$@"; do
   result="$testdir/results/$name.out"
   expected="$testdir/expected/$name.out"
   work=$(mktemp -d "${TMPDIR:-/tmp}/montage-golden.XXXXXX")

   # Paths and elapsed times differ between runs; replace them with
   # fixed placeholders before comparing.

   (cd "$work" && sh "$testdir/cases/$name.sh") 2>&1 |
      sed -e "s|$FIXTURES|<FIXTURES>|g" \
          -e "s|$work|<WORK>|g" \
          -e 's/time=[0-9.]*/time=<T>/g' > "$result"

   rm -rf "$work"

   if [ "${UPDATE:-}" = 1 ]; then
      cp "$result" "$expected"
      echo "UPDATED $name"
   elif diff -u "$expected" "$result"; then
      echo "PASS $name"
      npass=$((npass + 1))
   else
      echo "FAIL $name"
      nfail=$((nfail + 1))
   fi
done

if [ "${UPDATE:-}" != 1 ]; then
   echo "$npass passed, $nfail failed"
fi

[ "$nfail" -eq 0 ]
