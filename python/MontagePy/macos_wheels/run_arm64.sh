#!/bin/sh

# The cibuildwheel utility can handle both linux and macos builds but in different
# ways.  For Linux it uses specially constructed Docker linux containers that have all
# the needed Python versions installed (Docker itself must already be configured installed
# on the system) and for MacOS it uses the local Mac compiler (which supports explicit
# deployment targets) but the versions of Python have to be manually installed.
#
# The set of Pythons varies with time as versions appear and others get sunsetted.
# It is best to install the copies from python.org (not, for instance, from Anaconda)
# using the .pkg files the have there.  The cibuildwheel utility will find and use
# each of these in turn (based on the CIBW_BUILD environment variable below).

# These instances of python get installed in /Library/Frameworks/Python.framework/Versions
# and don't interfere with any other copy you might have so you can delete them when you
# are sure you are done using them.

# We might be able to use the local copy of our Montage (outside the container) as the
# basis of the build but we instead pull a new copy from GitHub compile it locally.
# If we used the local copy, we would need to compile it with the right OSX deployment
# target and that could get overly confusing.

# The cibuildwheel utility uses environment variables to control what it actually 
# builds.  In addition, on MacOS, the compiler (clang) also checks an environment
# variable to determine the OS version target for the compilation (a necessary concern
# for MacOS builds and OS versions must match if you are linking code together).


export OS='macos'
export CIBW_BUILD='cp39-* cp310-* cp311-* cp312-* cp313-*'
export CIBW_ARCHS='arm64'
export CIBW_BEFORE_ALL='sh make.sh'
export CIBW_BUILD_FRONTEND='build'
export MACOSX_DEPLOYMENT_TARGET='11.1'

echo "OS>                      " "$OS"
echo "CIBW_BUILD>              " "$CIBW_BUILD"
echo "CIBW_ARCHS>              " "$CIBW_ARCHS"
echo "CIBW_BEFORE_ALL>         " "$CIBW_BEFORE_ALL"
echo "CIBW_BUILD_FRONTEND>     " "$CIBW_BUILD_FRONTEND"
echo "MACOSX_DEPLOYMENT_TARGET>" "$MACOSX_DEPLOYMENT_TARGET"


# Build all the MacOS-related wheels

pip install pipx

pipx run cibuildwheel --platform $OS

