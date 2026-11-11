#!/bin/sh
# Regression test: check_macos_wheel.py must accept a wheel whose every
# compiled extension matches the single architecture in its platform tag,
# and reject bad names, unsupported or mixed tags, files that are not zip
# archives, wheels with no extension, extensions that cannot be unpacked
# or that lipo cannot read, and extensions built for the wrong or for
# more than one architecture. Needs macOS with python3, cc and lipo, and
# exits 77 elsewhere.
set -eu
unset CDPATH
root=$(cd "$(dirname "$0")/.." && pwd)
checker="$root/python/MontagePy/check_macos_wheel.py"

[ "$(uname -s)" = Darwin ] || { echo "SKIP: needs macOS lipo"; exit 77; }
command -v python3 > /dev/null 2>&1 || { echo "SKIP: python3 not found"; exit 77; }
command -v cc > /dev/null 2>&1 || { echo "SKIP: cc not found"; exit 77; }
command -v lipo > /dev/null 2>&1 || { echo "SKIP: lipo not found"; exit 77; }

# Pin str hashing so a set in the checker iterates in the same order in
# every run. The mixed tag case below depends on it.
PYTHONHASHSEED=0
export PYTHONHASHSEED

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 131' QUIT

# Build one arm64, one x86_64, and one universal shared object. Either
# single-arch build can fail where the SDK lacks that slice, so skip then.
echo 'int montagepy_probe(void) { return 1; }' > "$tmp/probe.c"
cc -shared -arch arm64 -o "$tmp/arm64.so" "$tmp/probe.c" > /dev/null 2>&1 \
    || { echo "SKIP: cannot build an arm64 shared object"; exit 77; }
cc -shared -arch x86_64 -o "$tmp/x86_64.so" "$tmp/probe.c" > /dev/null 2>&1 \
    || { echo "SKIP: cannot build an x86_64 shared object"; exit 77; }
lipo -create -output "$tmp/fat.so" "$tmp/arm64.so" "$tmp/x86_64.so"
echo 'not a Mach-O file' > "$tmp/text.so"
echo 'print("py")' > "$tmp/module.py"

# make_wheel <wheel name> <member path>=<source file> ...
# Members are deflated, as they are in a built wheel.
make_wheel() {
    name=$1
    shift
    python3 - "$tmp/$name" "$tmp" "$@" <<'EOF'
import sys, zipfile
wheel, tmp, *members = sys.argv[1:]
with zipfile.ZipFile(wheel, "w", zipfile.ZIP_DEFLATED) as archive:
    for member in members:
        name, source = member.split("=", 1)
        archive.write(f"{tmp}/{source}", name)
EOF
}

expect_pass() {  # expect_pass <label> <wheel name> ...
    label=$1
    shift
    for w; do set -- "$@" "$tmp/$w"; shift; done
    python3 "$checker" "$@" > "$tmp/output" 2>&1 \
        || { echo "FAIL: $label should pass" >&2; cat "$tmp/output" >&2; exit 1; }
}

# A rejection exits 1. An uncaught Python exception also exits 1, so a
# traceback on stderr must not pass as a rejection.
expect_fail() {  # expect_fail <label> <wheel name> ...
    label=$1
    shift
    for w; do set -- "$@" "$tmp/$w"; shift; done
    rc=0
    python3 "$checker" "$@" > /dev/null 2> "$tmp/stderr" || rc=$?
    if [ "$rc" -ne 1 ]; then
        echo "FAIL: $label should exit 1, got $rc" >&2
        cat "$tmp/stderr" >&2
        exit 1
    fi
    if grep -q '^Traceback' "$tmp/stderr"; then
        echo "FAIL: $label raised instead of rejecting" >&2
        cat "$tmp/stderr" >&2
        exit 1
    fi
}

arm=montagepy-2.4.0-cp312-cp312-macosx_11_0_arm64.whl
x86=montagepy-2.4.0-cp312-cp312-macosx_11_0_x86_64.whl

# Positive: every extension matches the tag, on both architectures, and a
# pure Python member alongside does not matter.
make_wheel "$arm" MontagePy/a.so=arm64.so MontagePy/b.so=arm64.so MontagePy/m.py=module.py
make_wheel "$x86" MontagePy/a.so=x86_64.so
expect_pass "matching arm64 wheel" "$arm"
expect_pass "matching x86_64 wheel" "$x86"
expect_pass "two good wheels" "$arm" "$x86"

# Negative: one bad wheel fails the whole run, whether it comes after a
# good one or before it.
wrong=wrong-2.4.0-cp312-cp312-macosx_11_0_x86_64.whl
make_wheel "$wrong" MontagePy/a.so=arm64.so
expect_fail "wrong architecture" "$wrong"
expect_fail "good then bad" "$arm" "$wrong"
expect_fail "bad then good" "$wrong" "$arm"

# Negative: a universal extension is not "only" the tagged architecture.
fat=fat-2.4.0-cp312-cp312-macosx_11_0_arm64.whl
make_wheel "$fat" MontagePy/a.so=fat.so
expect_fail "universal extension" "$fat"

# Negative: one bad extension among good ones is enough, in either order.
mixed=mixed-2.4.0-cp312-cp312-macosx_11_0_arm64.whl
make_wheel "$mixed" MontagePy/a.so=arm64.so MontagePy/b.so=x86_64.so
expect_fail "one wrong extension" "$mixed"
mixed_first=mixedfirst-2.4.0-cp312-cp312-macosx_11_0_arm64.whl
make_wheel "$mixed_first" MontagePy/a.so=x86_64.so MontagePy/b.so=arm64.so
expect_fail "wrong extension before a good one" "$mixed_first"

# Negative: lipo cannot read a non Mach-O extension.
text=text-2.4.0-cp312-cp312-macosx_11_0_arm64.whl
make_wheel "$text" MontagePy/a.so=text.so
expect_fail "unreadable extension" "$text"

# Negative: an extension whose compressed data is damaged is reported,
# not raised. Setting both block type bits in the first byte of the
# deflate stream selects the reserved type, which zlib always refuses.
damaged=damaged-2.4.0-cp312-cp312-macosx_11_0_arm64.whl
make_wheel "$damaged" MontagePy/a.so=arm64.so
python3 - "$tmp/$damaged" <<'EOF'
import struct, sys, zipfile
wheel = sys.argv[1]
with zipfile.ZipFile(wheel) as archive:
    offset = archive.infolist()[0].header_offset
with open(wheel, "r+b") as raw:
    raw.seek(offset + 26)
    name_length, extra_length = struct.unpack("<HH", raw.read(4))
    data = offset + 30 + name_length + extra_length
    raw.seek(data)
    first = raw.read(1)[0]
    raw.seek(data)
    raw.write(bytes([first | 0x06]))
EOF
expect_fail "damaged extension data" "$damaged"

# Negative: no compiled extension at all.
pure=pure-2.4.0-cp312-cp312-macosx_11_0_arm64.whl
make_wheel "$pure" MontagePy/m.py=module.py
expect_fail "no extension" "$pure"

# Negative: a tag the checker does not understand is refused.
linux=linux-2.4.0-cp312-cp312-manylinux2014_x86_64.whl
make_wheel "$linux" MontagePy/a.so=x86_64.so
expect_fail "non macOS tag" "$linux"

# Negative: a tag naming two architectures is refused whatever the
# extension holds. Without that check the checker would expect whichever
# architecture set.pop returns, the same one in both runs because the
# hash seed is pinned, so one of these two wheels would pass.
two_arm=twoarm-2.4.0-cp312-cp312-macosx_11_0_arm64.macosx_11_0_x86_64.whl
two_x86=twox86-2.4.0-cp312-cp312-macosx_11_0_arm64.macosx_11_0_x86_64.whl
make_wheel "$two_arm" MontagePy/a.so=arm64.so
make_wheel "$two_x86" MontagePy/a.so=x86_64.so
expect_fail "two architecture tags, arm64 extension" "$two_arm"
expect_fail "two architecture tags, x86_64 extension" "$two_x86"

# Negative: not a wheel name. The first is the good wheel under another
# suffix, so only the suffix check can reject it. The second has no dash
# to split a platform tag from.
cp "$tmp/$arm" "$tmp/${arm%.whl}.zip"
expect_fail "non wheel suffix" "${arm%.whl}.zip"
cp "$tmp/$arm" "$tmp/nodash.whl"
expect_fail "wheel name without a dash" nodash.whl

# Negative: a wheel that is not a zip.
bad=bad-2.4.0-cp312-cp312-macosx_11_0_arm64.whl
echo 'zip?' > "$tmp/$bad"
expect_fail "corrupt archive" "$bad"
