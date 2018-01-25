
=========
MontagePy
=========

Montage is a toolkit for mosaicking and visualizing astronomical images.
It contains dozens of routines for reprojecting FITS images and datacubes,
matching backgrounds for a collection of reprojected images, coadding with
proper attention to weighting, and visualizing the results with a variety
of overlays (source catalogs, image set metadata, coordinate grids).

All standard projections are available, plus a couple of specialized ones
(HEALPix and WWT TOAST).  Focal plane distortion models are also supported
using the SAO WCS library.


Reprojecting
============
Different use cases are best served with customized approached to 
image reprojection and Montage has four:

* mProject, which handles all projections and is reliably flux conserving.
  While the most flexible, it is also the slowest.

* mProjectPP, which is also flux conserving and much faster but only
  supports a few (tangent plane) projections.  However, since TAN is 
  by far the most commonly-used projection, it is commonly used.

* mProjectQL is not 100% flux conserving but the fastest of the three.
  It supports all projections and the algorithm is similar the that
  used by the SWARP package.  While not flux conserving in theory, 
  all tests so far have found it's output to be indistinguishable from
  the above routines.

* mProjectCube is a variant of mProject extended and optimized for 
  image cubes (images with a third/fourth dimension).


Background Matching
===================
Montage relies on image data having been taken with overlaps between
the individual images for matching backgrounds.  The image-image 
differences are individually computed and fit (to get offset levels
and optionally slopes), then a global relaxation technique is used to
determine the best individual image offsets to apply to minimize the
overall differences.

Various instrumental and observing anomolies (like persistence issues
and transient airglow) in the individual images can compromise this
process but it will still produce the best model available without
those artifacts being removed beforehand.


Coaddition into Final Mosaic
============================
All through the reprojection and correction process, individual 
pixel weights are maintained.  This incudes any input weighting that
may have been given (the reprojection algorithms support this) and 
keeping track of fractional pixel effects around the image edges and
any "holes" in the images.

The final coaddition takes this weighting into account when coadding
and the coadding process can take different forms (sum, average,
mid-average or even just count), though the default is a simple 
averaging in the normal case where the image data represents flux 
density.


Visualization
=============
The main Montage visualization routine (mViewer) can produce PNG or
JPEG images of either a single image (grayscale or psuedo-color) or
three image (red, green, blue) plus any number of overlays.

Some ancillary Montage tools often used with mViewer include:

* mSubimage, to cut out regions of a FITS image, either based on
  sky location or pixel range.

* mShrink, to shrink (or expand) a FITS image through (fractional)
  pixel replication.

* mHistogram, which can pre-generate a histogram used by mViewer.
  mViewer can generate the same histogram on the fly for a single
  image but with mHistogram the same stretch can be applied to a
  set of images (e.g. tiles for display).


Ancillary Tools
===============
There are a number of other support tools, mainly reflecting issues
that arose in the course of working with image sets:

* mImgtbl, which scans directories/trees for FITS images with 
  WCS in the header.  Most commonly used on a structured collection
  in a single subdirectory as part of the above processing.

* mGetHdr/mPutHdr, for fixing errant FIT headers.  mGetHdr pulls 
  the entire FITS header out into an editable text files, then
  mPutHdr can be used to create a new image from the old using
  the edited text as a replacement header.

* mFixNaN.  There is a lot of data where pixels that should be 
  "blank" (i.e. floating point NaN values) are stored as some 
  other value (frequently zero).  This routine can be used to 
  fix that.

* Executives: Several steps in the mosaicking process involves 
  looping over an image list (reprojection, background analysis
  and background correction).  Montage contains executive processes
  (e.g. mProjExec) to simplify the process.

And there is a growing list of other such routines.

