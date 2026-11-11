
Montage: Astronomical Image Mosaics, Examination, and Visualization
===================================================================

[![Build Status](http://vmmontage.ipac.caltech.edu:8080/buildStatus/icon?job=MontageDev)](http://vmmontage.ipac.caltech.edu:8080/job/MontageDev/)
![Jenkins tests](https://img.shields.io/jenkins/tests?compact_message&jobUrl=http%3A%2F%2Fvmmontage.ipac.caltech.edu%3A8080%2Fjob%2FMontageDev&passed_label=passed&skipped_label=skipped&failed_label=failed)
![GitHub Repo Size](https://img.shields.io/github/repo-size/Caltech-IPAC/Montage)
![Lines of code](https://img.shields.io/tokei/lines/github/Caltech-IPAC/Montage)

Montage 7.0 adds two major capabilities.  The first is a set of tools
for building HiPS map.  HiPS (Hierarchical Progressive Surveys) is a
hierarchical tiling mechanism which allows one to access, visualize
and browse seamlessly image data and in particular large-scale, 
high-resolution surveys.

HiPS construction through Montage consists of building large-scale
mosaics using pre-existing Montage modules (reprojection, background
matching, and coaddition) with a HiPS-specific projection (HPX).
The image hierarchy just requires repetative shrinking of higher
resolution images by factors of two.  Finally, the tiles to be 
served are simply 512x512 cutouts from these mosaics.

For high-resolution data, this process can benefit greatly from 
massive parallelization and this can be achieved in a number of
ways. In particular, tools have been developed to streamline this
on cloud platforms like AWS.

So at arcminute scale all-sky processing can adequately be done on 
a single desktop machine and at arcsecond scale the same tools can
create a set of jobs that can be submitted to run on a cloud in
a few days (or a few tens of processors if one has that in-house).

The repository also includes procedures for building Python binary
extension Montage wheels for Linux and macOS. The release workflow
publishes wheels only to GitHub Releases. The same infrastructure can
be used to build custom wheels locally. See the
[MontagePy README](python/MontagePy/README.md) for installation
instructions.

The Montage 6.0 release can be accessed via git tag "v6.1".

--------------

Montage (http://montage.ipac.caltech.edu) is an Open Source toolkit,
distributed with a BSD 3-clause license, for assembling Flexible 
Image Transport System (FITS) images into mosaics according to 
the user's custom specifications of coordinates, projection,
spatial sampling, rotation and background matching.

The toolkit contains utilities for reprojecting and background 
matching images, assembling them into mosaics, visualizing the
results, and discovering, analyzing and understanding image metadata
from archives or the user's images.

Montage is written in ANSI-C and is portable across all common
Unix-like platforms, including Linux, Solaris, Mac OSX and Cygwin on
Windows.  The package provides both stand-alone executables and
the same functionality in library form.  It has been cross-compiled
to provide native Windows executables and packaged as a binary Python
extension (see the MontagePy README for installation).

The distribution contains all libraries needed to build the toolkit 
from a single simple "make" command, including CFITSIO and the WCS
library (which has been extended to support HEALPix and World-Wide
Telescope TOAST projections. The toolkit is in wide use in astronomy
to support research projects, and to support pipeline development,
product generation and image visualization for major projects and
missions; e.g. Spitzer Space Telescope, Herschel, Kepler, AKARI and
others. Montage is used as an exemplar application by the computer
science community in developing next-generation cyberinfrastructure,
especially workflow frameworks on distributed platforms, including
multiple clouds.

Building against a system CFITSIO
---------------------------------

By default "make" builds and links the bundled CFITSIO (lib/src). To use
an installed CFITSIO instead, run make from the repository root with
USE_SYSTEM_CFITSIO=1 and one of (checked in this order):

    make USE_SYSTEM_CFITSIO=1 CFITSIO_CFLAGS=-I/opt/include CFITSIO_LIBS="-L/opt/lib -lcfitsio -lm"
    make USE_SYSTEM_CFITSIO=1 CFITSIO_DIR=/opt/cfitsio   # <dir>/include, <dir>/lib
    make USE_SYSTEM_CFITSIO=1                            # pkg-config cfitsio

CFITSIO_DIR links a shared libcfitsio with an rpath to <dir>/lib, and a
static-only one with -lz -lm (zlib from <dir>/lib or the default library
path). For other layouts (lib64, or a CFITSIO built with extra libraries
such as curl or bzip2) use pkg-config, with CFITSIO_STATIC=1 ("pkg-config
--libs --static") for a static libcfitsio.a, or explicit flags. A shared
library outside the loader's search path given by explicit flags needs
-Wl,-rpath,<libdir> in CFITSIO_LIBS or LD_LIBRARY_PATH. The selection
lives in cfitsio.make; ./configure only sets the install prefix. If
CFITSIO_LIBS is set (also from the environment), CFITSIO_DIR is ignored
and make prints a warning. CFITSIO_STATIC applies only to pkg-config; it
has no effect with CFITSIO_DIR or explicit flags.

Only the root Makefile applies the selection and its checks: "make" run
inside a subdirectory uses the bundled default (plain -lcfitsio, which in
a system-mode tree links whatever libcfitsio the linker finds), so run
system-mode builds from the repository root.

Before building, "make check-system-cfitsio" (run automatically) compiles,
links and runs test programs against the selected library: its headers and
library must be the same version, and the CVE-2026-115504 regression test
(tests/cve_vlen_regression.c) must pass. A CFITSIO without that fix is
rejected; CFITSIO_ALLOW_UNPATCHED=1 builds anyway with a warning. It does
not override other probe failures. A distribution CFITSIO with the
upstream fix applied passes the check without it. Once a CFITSIO release
includes this fix, the check can become a minimum version. Because the
check runs the programs it builds, system mode does not support cross
compiling.

A tree built against one CFITSIO must be cleaned before building against
another: switching between bundled and system mode, or to a system CFITSIO
with different flags, without "make clean" stops with an error. The
selection is recorded in .cfitsio-build-mode when a build starts, so a
build that fails part way also counts.

The MontagePy wheel objects (MontageLib "pythonlibs") embed the bundled
CFITSIO, so a system-CFITSIO build skips them; wheels are bundled-only.
In a tree built with USE_SYSTEM_CFITSIO=1, "make test" reports the tests
that need the bundled CFITSIO build as SKIP. Run plain "make test" there:
adding USE_SYSTEM_CFITSIO=1 to "make test" or "make install" repeats the
CFITSIO selection, which needs the provider settings again (only "make
clean" is exempt).

Montage provides multiple reprojection algorithms optimized for 
different needs, maximizing alternately flux conservation, range of
projections, and speed.

The visualization module supports full (three-color) display of FITS
images and publication quality overlays of catalogs (scaled symbols),
image metadata, and coordinate grids.  It fits in equally well in
pipelines or as the basis for interactive image exploration and there
is Python support for the latter (It has also been used in web/Javascript
applications).

We are in the process of adding automated regression testing using Jenkins.
At the moment, this only includes a couple of dummy tests on a Jenkins server 
that we maintain specifically for the Montage project).

Montage was funded from 2002 to 2005 by the National Aeronautics and
Space Administration's Earth Science Technology Office, Computation
Technologies Project, under Cooperative Agreement Number NCC5-626
between NASA and the California Institute of Technology. The Montage
distribution includes an adaptation of the MOPEX algorithm developed
at the Spitzer Science Center. Montage has also been funded by the National
Science Foundation under Award Number NSF ACI-1440620.
