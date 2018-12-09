"""
Montage is a toolkit for creating science grade mosaics of astronomical images.
It is widely used in the astronomical and supercomputing communities on a wide
variety of platforms.

The core of Montage involves reprojecting a set of images to a common sky 
projection (all standard and a few custom projections are supported), analyzing
differences in the image backgrounds using overlap areas and modelling for a
set of background correction parameters, and finally mosaicking the resultant
images into a single file.  Astrometry and photometry are preserved throughout.

Montage also includes fairly advanced visualization tools for creating grayscale
and three-color PNG/JPEG renderings of images/mosaics, along with overlays of
various graphics overlays (catalogs, image metadata, coordinate grids, etc.)
The visualizion functionality includes some custom stretch capabilities and tools
for processing sets of images for a common look (e.g. for systems like the
World Wide Telescope).

Finally, there is a suite of support tools to aid in examining and correcting
a number of the artifacts frequently found in this kind of data.
"""
