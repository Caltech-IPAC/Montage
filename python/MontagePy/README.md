
Montage: Astronomical Image Mosaics, Examination, and Visualization
===================================================================

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
be used to build custom wheels locally.

MontagePy requires Python 3.9 or newer.  Pass ``mode=2`` when giving
``mViewer`` a command string.  The default ``mode=0`` reads a JSON file.

Installing MontagePy from GitHub Release wheels
-----------------------------------------------

The wheel workflow targets CPython 3.9 through 3.15 on Linux x86_64
and on macOS x86_64 and arm64 (minimum macOS 11.0 on both
architectures). When a MontagePy GitHub release has rebuilt wheels for
your system, replace `<version>` with that release version and install
with the release page as a source.

    pip install "MontagePy==<version>" --only-binary MontagePy \
        --find-links "https://github.com/Caltech-IPAC/Montage/releases/expanded_assets/MontagePy-v<version>"

The pin selects one exact release. Without a pin, pip selects the
highest version available across the release page and PyPI.
Keep --only-binary MontagePy. The release page also lists the
repository source archives, and pip would otherwise try to install
MontagePy from them when no wheel matches the system.
Do not use --no-index. MontagePy depends on requests, which is
installed from PyPI.

Maintainer release recipe
-------------------------

Use a new version for rebuilt wheels. The first rebuilt release must
be newer than the historical 2.3.1 wheels so pip upgrades existing
installations. Tagged versions must use numeric `x.y.z` form.

Set the package version in `python/MontagePy/pyproject.toml`.

Merge the changes before pushing the matching `MontagePy-v<version>`
tag. The tag runs the wheel workflow and publishes to GitHub Releases
only after all build legs succeed. If a build leg fails, no release is
created. Rerun failed jobs from Actions. A manual workflow run builds
artifacts without publishing a release. PyPI upload is not part of this
workflow. WHEELS.md in this directory describes how the wheels are
built and tested.

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
extension (see Installing MontagePy from GitHub Release wheels above).

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
