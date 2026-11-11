#!/bin/sh
# Regression test: the pyproject.toml version field must be a readable
# string, and check_version.py --tag must enforce the MontagePy-v prefix,
# exact version equality, numeric x.y.z format, and the 2.3.1 release
# floor. Requires Python 3.11+ and exits 77 if absent.
set -eu
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd)

python3 -c "import tomllib" 2>/dev/null || {
    echo "SKIP: python3 3.11+ with tomllib not available"
    exit 77
}

# Capture the version (must succeed under set -eu).
v=$(python3 "$root/python/MontagePy/check_version.py")

# Positive: matching tag must pass.
python3 "$root/python/MontagePy/check_version.py" --tag "MontagePy-v$v"

# Negative: bad prefix must fail. Only the prefix check can reject this
# tag, since the rest of it matches the live version.
rc=0
python3 "$root/python/MontagePy/check_version.py" \
    --tag "montagepy-v$v" > /dev/null 2>&1 || rc=$?
[ "$rc" -eq 2 ] || { echo "FAIL: bad-prefix tag should exit 2, got $rc" >&2; exit 1; }

# Negative: version mismatch must fail.
rc=0
python3 "$root/python/MontagePy/check_version.py" \
    --tag "MontagePy-v${v}-bad" > /dev/null 2>&1 || rc=$?
[ "$rc" -eq 2 ] || { echo "FAIL: mismatched tag should exit 2, got $rc" >&2; exit 1; }

# Floor and format checks need a version the live tree never has, so
# copy the script beside a stub pyproject.toml in a temp dir.
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT
cp "$root/python/MontagePy/check_version.py" "$tmp/"
stub_version() {
    printf '[project]\nname = "MontagePy"\nversion = "%s"\n' "$1" \
        > "$tmp/pyproject.toml"
}

# A rejected stub must exit 2, the status parser.error uses. An uncaught
# Python exception exits 1, so a crash in the script does not pass as a
# rejection. The version is given once so the stub and the tag always
# match.
expect_reject() {  # expect_reject <version> <label>
    stub_version "$1"
    rc=0
    python3 "$tmp/check_version.py" --tag "MontagePy-v$1" \
        > /dev/null 2>&1 || rc=$?
    [ "$rc" -eq 2 ] || { echo "FAIL: $2 $1 should exit 2, got $rc" >&2; exit 1; }
}

# Negative: a tag at or below the 2.3.1 floor must fail even when it matches.
expect_reject 2.3.1 "floor version"
expect_reject 2.3.0 "below-floor version"

# Negative: a version that is not numeric x.y.z must fail on the tag path.
# 2.3.2.1 would compare above the floor, so only the format check rejects it.
expect_reject 2.3.2rc1 "non-numeric version"
expect_reject 2.3.2.1 "four-part version"

# Tree problems are reported through parser.error as well, so they must
# exit 2 even without a tag. Each case rewrites the stub metadata.
expect_reject_tree() {  # expect_reject_tree <label>
    rc=0
    python3 "$tmp/check_version.py" > /dev/null 2>&1 || rc=$?
    [ "$rc" -eq 2 ] || { echo "FAIL: $1 should exit 2, got $rc" >&2; exit 1; }
}

# Negative: a missing file is reported, not raised.
rm -f "$tmp/pyproject.toml"
expect_reject_tree "missing pyproject.toml"

# Negative: malformed TOML is reported, not raised.
printf '[project\n' > "$tmp/pyproject.toml"
expect_reject_tree "malformed TOML"

# Negative: a byte that is not UTF-8 is reported, not raised.
printf '[project]\nname = "MontagePy"\nversion = "\377"\n' \
    > "$tmp/pyproject.toml"
expect_reject_tree "non UTF-8 byte"

# Negative: a missing version key is reported, not raised.
printf '[project]\nname = "MontagePy"\n' > "$tmp/pyproject.toml"
expect_reject_tree "missing version key"

# Negative: a project entry that is not a table is reported, not raised.
printf 'project = 1\n' > "$tmp/pyproject.toml"
expect_reject_tree "project is not a table"

# Negative: a version that is not a string is reported, not raised.
printf '[project]\nname = "MontagePy"\nversion = 240\n' > "$tmp/pyproject.toml"
expect_reject_tree "non-string version"

# Negative: an empty string is not a version.
stub_version ""
expect_reject_tree "empty version"

# Positive: the stub above the floor passes, proving the harness itself works.
stub_version 2.3.2
python3 "$tmp/check_version.py" --tag MontagePy-v2.3.2 > /dev/null
