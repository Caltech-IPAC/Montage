
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

The second addition to Montage is a complete set of modern procedures 
for building Python binary extension Montage wheels for Linux and Mac 
systems (and extensible to some others).  This is very much a moving
target and we are building as many of these as we can and pushing them
to PyPI but if someone want to extend Montage for their own use they
can use the same infrastructure to build custom wheels as well.

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
extension (available via "pip install MontagePy").

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
