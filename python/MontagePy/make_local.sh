#!/bin/sh
set -eu
unset CDPATH
if [ -L "$0" ]; then
    echo "$0: refusing symlink invocation; run the real script path" >&2
    exit 1
fi
cd "$(dirname "$0")"
# A script that sh found through PATH has no directory in $0, so the cd
# above stays where the caller is. Stop unless this script is here.
if [ ! -f ./make_local.sh ]; then
    echo "$0: cannot find this script's directory, run it by its path" >&2
    exit 1
fi

# This script is for our local use but should work on most
# systems and is easily modified. It needs the Montage build
# done first, which fills lib/ with the wheel objects.

rm -rf dist

sh stage_package.sh

python -m build --wheel
