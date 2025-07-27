#!/bin/sh

# The cibuildwheel utility can handle both linux and macos builds but in different
# ways.  For Linux it uses specially constructed Docker linux containers that have all
# the needed Python versions installed (Docker itself must already be configured installed
# on the system) and for MacOS it uses the local Mac compiler (which supports explicit
# deployment targets) but the versions of Python have to be manually installed.
#
# For Linux, we have to be very careful to remember that everything done in the build
# happens inside the Docker container and that no changes we make locally will have to
# be pushed to GitHub before they will appear in the build.
#
# So inside the make.sh script we will install Montage from GitHub into /project inside
# the container, then build it, arrange files from it for cibuildwheel and setup.py,
# and run cibuildwheel all still inside the container once the container exits everything
# except the wheel files themselves (which will have been written to a container-external
# directory call 'wheelhouse') will go away.
#
# The cibuildwheel utility uses environment variables to control what it actually
# builds.  

export OS='linux'
export CIBW_BUILD='cp*-manylinux*'
export CIBW_SKIP='cp38-*'
export CIBW_ARCHS='x86_64'
export CIBW_BEFORE_ALL='sh make.sh'
export CIBW_BUILD_FRONTEND='build'

echo "CIBW_BUILD>         " "$CIBW_BUILD"
echo "CIBW_SKIP>          " "$CIBW_SKIP"
echo "CIBW_ARCHS>         " "$CIBW_ARCHS"
echo "CIBW_BEFORE_ALL>    " "$CIBW_BEFORE_ALL"
echo "CIBW_BUILD_FRONTEND>" "$CIBW_BUILD_FRONTEND"

pip install pipx

pipx run cibuildwheel --platform $OS 
