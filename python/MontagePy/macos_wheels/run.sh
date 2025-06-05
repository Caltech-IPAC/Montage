#!/bin/sh

# Before running this script, there are a few things that need to get
# installed if you don't already have them.

# The most important thing is to install the versions of Python we are trying to
# support.  The varies with time as versions appear and others get sunsetted.
# It is best to install the copies from python.org (not, for instance, from Anaconda)
# using the .pkg files the have there.  The cibuildwheel utility will find and use
# each of these in turn (based on the CIBW_BUILD environment variable below).

# These instances of python get installed in /Library/Frameworks/Python.framework/Versions
# and don't interfer with any other copy you might have so you can delete them when you
# are sure you are done using them.

# We might be able to use the local copy of our Montage (outside the container) as the
# basis of the build but we instead pull a new copy into the container and compile it there. 
# If we used the local copy, we would need to compile it with the right OSX deployment
# target and that could get overly confusing.

# The cibuildwheel utility uses environment variables to control what it actually 
# builds.  In addition, on MacOS, the compiler (clang) also checks an environment
# variable to determine the OS version target for the compilation (a necessary concern
# for MacOS builds and OS versions must match if you are linking code together).

# Different settings are used for Linux, so we set things depending on what 'uname'
# says about the system.

export OS='macos'
export CIBW_BUILD='*'
export CIBW_SKIP='cp36-* cp37-*'
export CIBW_ARCHS='x86_64 universal2 arm64'
export CIBW_BEFORE_ALL='sh make.sh'
export MACOSX_DEPLOYMENT_TARGET='11.1'
export CIBW_BUILD_FRONTEND='build'

echo "OS>                 " "$OS"
echo "CIBW_ARCHS>         " "$CIBW_ARCHS"
echo "CIBW_BUILD>         " "$CIBW_BUILD"
echo "CIBW_BEFORE_ALL>    " "$CIBW_BEFORE_ALL"
echo "CIBW_BUILD_FRONTEND>" "$CIBW_BUILD_FRONTEND"


# Build all the MacOS-related wheels

pip install pipx

pipx run cibuildwheel --platform $OS

