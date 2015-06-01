Image Mosaics for Astronomers

Montage is a toolkit for assembling Flexible Image Transport System
(FITS) images into custom mosaics.

Montage has been tested by the Montage team on Linux platforms with
mosaics built from the 2-Micron All Sky Survey (2MASS) Atlas
(full-resolution) images. It has been built, tested and the output
products validated by Montage customers on Unix platforms, including
Linux, Solaris, Mac OSX, and IBM AIX. It has been used to generate
mosaics from data released by the Spitzer Space Telescope, the Hubble
Space Telescope, the Infrared Astronomical Satellite (IRAS), the
Midcourse Space Experiment (MSX), the Sloan Digital Sky Survey (SDSS),
and ground-based telescopes such as the National Optical Astronomy
Observatories (NOAO) 4-m telescope and the William Herschel 4-m
telescope.

Montage has also been used to test workflow frameworks on
supercomputers and cloud systems.

Under a new grant, Montage is being extended to deal with astronomical
data cubes and packaged for use on cloud computing resources. The
current fileset has been uploaded here in preparation for that work and
consists of the original release with some update and additions that
have been made over the years but not officially released.

This Python library wraps the Montage mViewer module (plus a few other
Montage support tools) for use in viewing astronomical image data
and overlays.  FITS images or datacube planes can be displayed, 
overlaid with catalogs, image outlines, coordinate grids, etc. 
There is support for interactive zooming and feedback and the result
can be exported to PNG/JPEG files for distribution and publishing.
