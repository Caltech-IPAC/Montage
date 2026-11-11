#!/bin/sh
set -eu

# Build the native objects and wrapper sources from this checkout.
unset CDPATH
package=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$package/../.." && pwd)

# The bundled JPEG 8b configure script cannot detect some systems such
# as Linux aarch64. Pass every bundled configure the build type from
# the newer CFITSIO config.guess.
build=$(sh "$root/lib/src/cfitsio-4.7.0/config/config.guess")
ac_cv_build="$build" make -C "$root" USE_SYSTEM_CFITSIO=0

sh "$package/stage_package.sh"
