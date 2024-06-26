

Building a Full Set of Montage Python Wheels for MacOSX
-------------------------------------------------------

Summary:  Install all the Python versions you want to support (the Mac
has a particular location it wants them to be).  Then execute "run.sh"
in this directory.  The bulk of this write-up consists of technical
notes to help up remember why the scripts are structured the way they
are and what else you need to do.  This includes things you need to do
prior to building.  

---

Even though the same tools are used, building Python binary extension wheels
for MacOSX and Linux are actually quite different.  These files and this
description is just for MacOSX.  See the "linux_wheels" directory for the
Linux write-up.  Both of these directories are, in fact, standalone in that
they don't need the Montage directory structure above them to be built. They
are kept here for convenience.

---

MacOSX compiled code has to be targetted at a specific OSX release.
By default, that is the release under which you are running but the almost
invariably they will also work on newer versions.  So if you are building for
distribution, you are usually advised to target your build a few releases back.

This is compounded by the fact that here we need to link our code together
with a libraries distributed with Python, so we in fact need to match the
version used there.  At the time of this writing, the current OSX release
is 14.1 (Sonoma) and the version used for the Anaconda Python distribution
is 11.1 (Big Sur).  So all our compilation is done with 11.1, using an
environment variable that controls the CLANG compiler:


   MACOSX_DEPLOYMENT_TARGET='11.1'

This may be different if you are using the python.org release.

Note: Even though there is an explicit /usr/bin/gcc (and our Makefiles use
it),  it is actually the same binary as /usr/bin/clang.  You can install
the real gcc but this us usually not recommended.  What all this means is
that we only need to compile our code (and support libraries) once for all
the OS versions we want to support.

But there is yet another wrinkle.  Mac has been transitioning to a different
chipset.  Older Macs use the X86_64 instruction set and the newer ones
the ARM64.  CLANG can build to either one or you can specify "universal2",
which actually builds both and saves them together in a single file.  At the
moment we are doing this but if there are any complaints about the larger
files we can switch to building them separately.

---

Next, we have to decide which Python versions we want to support and install
those on our build Mac.  At the moment the supported versions are 3.8 through
3.12, though both this and the MacOSX versions tend to change every year.
Python versions for MacOSX come as standard .pkg files and install to
/Library/Frameworks/Python.framework/Versions.  There is a Mac program called
"installer" that you can use, but just doing it manually through the GUI is
good enough.

---

All of Montage (and support libraries) are written in C for speed.  We use
Cython to create the linkage from Python to these binaries, essentially
just Python functions that form wrappers around the C functions.  We don't
even create this Cython manually.  Instead, we create JSON files for each
C module that documents the calling and return structure syntax and use a
custom built parser the generates the Cython code.

---

Finally, with all the compiled C and Cython code in place, we use standard
pyproject.toml to describe the package and setup.py to control the binary
build process.  In a case like this, there is standard Python utility called
"cibuildwheel" which loops over all the wheels we have defined and calls
the "build" process for each one.  Right now, with five Python versions and
just the one "universal2" version, that means five wheels.  All of this is
captured in the "run.sh" script and the results are put in the "wheelhouse"
subdirectory.

