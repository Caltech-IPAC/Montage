Montage
=======

Montage: The Astronomer's Toolkit for Building Image Mosaics and 
Analyzing Image Metadata

Montage (http://montage.ipac.caltech.edu) is an Open Source toolkit,
distributed with a BSD 3-clause license, for assembling Flexible 
Image Transport System (FITS) images into mosaics, according to 
the user's custom specifications of coordinates, WCS projection,
spatial sampling and rotation. The toolkit contains utilities for
reprojecting and background matching images, assembling them into
mosaics, visualizing the results, and discovering, analyzing and
understanding image metadata from archives or the user's images.
Montage is written in ANSI-C and is portable across all common
*nix platforms, including Linux, Solaris, Mac OSX and Cygwin on
Windows. The distribution contains all libraries needed to build the
toolkit from a single simple "make" command, including the CFITSIO
library and WCS tools. The toolkit is in wide use in astronomy to
support research projects, and to support pipeline development,
product generation and image visualization for major projects and
missions; e.g. Spitzer Space Telescope, Herschel, Kepler, AKARI and
others. Montage is used as an exemplar application by the computer
science community in developing next-generation cyberinfrastructure,
especially workflow framework on distributed platforms.

Version 5.0 offers a new fast reprojection module mProjectQL,
intended primarily for creating images for visualization rather than
for science analysis. It offers a speed-up of approximately x20 over
mProject; support for HEALPix; modules for creating image files for
consumption by the World Wide Telescope. Version 5 also allows the
most commonly used modules to be built and used as a statically or
dynamically linked library.. 

Montage was funded from 2002 to 2005 by the National Aeronautics and
Space Administration's Earth Science Technology Office, Computation
Technologies Project, under Cooperative Agreement Number NCC5-626
between NASA and the California Institute of Technology. The Montage
distribution includes an adaptation of the MOPEX algorithm developed
at the Spitzer Science Center. Montage is now funded by the National
Science Foundation under Award Number NSF ACI-1440620.
