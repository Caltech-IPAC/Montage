

Building a Full Set of Montage Python Wheels for Linux
------------------------------------------------------

Summary:  Building a full set of Linux wheels realistically requires using a
"manylinux" Docker container. This will include all the currently-viable Python
version (at this time 3.8 through 3.12) and specialized tools.  With that, you
can just execute the "run.sh" command  The bulk of this write-up are technical
notes to help up remember why the scripts are structured the way they are.

Even though the same tools are used, building Python binary extension wheels
for MacOSX and Linux are actually quite different.  These files and this
description is just for Linux.  See the "macos_wheels" directory for the
Mac write-up.

---

Using the correct tools and versions of system libraries, it is possible
to build code that runs across all versions of Linux.  Called "manylinux",
this configuration has been captured in Docker containers using that name.
At the moment, the Docker image we use is "quay.io/pypa/manylinux2014_x86_64".
This image supports building for Python versions 3.7 through 3.12, though
Python 3.7 actually reached end-of-life last year.

Previously, one had to set up their own processing scenario inside the
container but this has been simplified through the introduction of a new
Python tool, "cibuildwheel", that handles the building of the wheels (using
"pyproject.toml" and Python "build") and running a cleanup utility called
"auditwheel".  It loops over all the available Python versions and builds
wheels for each and for variants of Linux like Musllinux.

---

All of Montage (and support libraries) are written in C for speed.  We use
Cython to create the linkage from Python to these binaries, essentially
just Python functions that form wrappers around the C functions.  We don't
even create this Cython manually.  Instead, we create JSON files for each
C module that documents the calling and return structure syntax and use a
custom built parser the generates the Cython code.

Since all of this has to be done now inside the Docker container, we use
a cibuildwheel facility that allows running code in the container (in our 
case "make.sh") before the actually wheel building.  This directive is
communicated to the container using an environment variable:

   CIBW_BEFORE_ALL='sh make.sh'

The "make.sh" script retrieve Montage from GitHub, builds it (and in particular
the library of object files that need to get included in the wheel), and
constructs the Cython needed to make the linkage between Python and the
C object modules. "cibuildwheel" will take responsibility for actually
processing the Cython and Montage C libraries and creating the actual
shared-object files included in each wheel.

We could almost certainly use the local copy of our Montage (outside the 
container) as the basis of the build but we instead pull a new copy into
the container and compile it there.  This is probably unnecessary but is
done out of an abundance of caution.

---

Finally, with all the compiled C and Cython code in place, "cibuildwheel" uses
a standard pyproject.toml to describe the package and setup.py to control the
binary build process.  Right now, with multiple Python versions and just the
one "universal2" version, that means five wheels.  All of this is captured in
the "run.sh" script and the results are put in the "wheelhouse" subdirectory.
