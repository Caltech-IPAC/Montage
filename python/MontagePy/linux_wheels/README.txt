

Building a Full Set of Montage Python Wheels for Linux
------------------------------------------------------

Summary:  Building a full set of Linux wheels realistically requires having a "manylinux" Docker container. This will include all the currently-viable Python version (at this time 3.8 through 3.12).  With that, you can just execute the "run.sh" command  The bulk of this write-up are technical notes to help up remember why the scripts are structured the way they are.

---

Even though the same tools are used, building Python binary extension wheels for MacOSX and Linux are actually quite different.  These files and this description is just for Linux.  See the "macos_wheels" directory for the Linux write-up.

Using the correct tools and versions of system libraries, it is possible to build code that runs across all versions of Linux.  Called "manylinux", this configuration has been captured in Docker containers using that name.  At the moment, the Docker image we use is "quay.io/pypa/manylinux2014_x86_64".  This image supports building for Python versions 3.7 through 3.12, though Python 3.7 actually reached end-of-life last year.

Previously, one had to set up your own processing scenario inside the container but this has been simplified through the introduction of a new Python tool, "cibuildwheel", that handle the building of a wheel (using "pyproject.toml" and Python "build") and running a cleanup utility called "auditwheel").  It also loops over all the available Python versions and builds wheels for each and for variants of Linux like Musllinux.

---

Almost all of Montage (and support libraries) are written in C for speed.  We use Cython to create the linkage from Python to these binaries, essentially just Python functions that form wrappers around the C functions.  We don't even create this Cython manually.  Instead, we create JSON files for each C module that documents all the calling the calling and return structure syntax and use a custom built parser the generates the Cython code.

Since all of this has to be done now inside the Docker container, we use a cibuildwheel facility that allows running code (in our case "build.sh") before the actually wheel building.  This directive is communicated to the container using an environment variable:

   CIBW_BEFORE_ALL='sh make.sh'

with the "make.sh" script retrieving Montage from GitHub, building it (and in particular the library of object files that need to get included in the wheel), and construct the Cython needed to make the linkage between Python and the C object modules. "cibuildwheel" will take responsibility for actually processing the Cython and creating the actual shared-object files included in each wheel.  

---

Finally, with all the compiled C and Cython code in place, "cibuildwheel" uses a standard pyproject.toml to describe the package and setup.py to control the binary build process.  Right now, with six Python versions and just the one "universal2" version, that means five wheels.  All of this is captured in the "run.sh" script and the results are put in the "wheelhouse" subdirectory.
