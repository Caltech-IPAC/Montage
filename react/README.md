
Montage: Building a React Component for Using mViewer as a Plotly Dash Element
==============================================================================

Montage, and in particular the mViewer component, can be used as
the underpinnings of an interactive interface through the Plotly Dash
dashboard framework.  All that is needed is use-case-specific Python
application code and a React component supporting the actual interaction.

Curiously, the React component doesn't actually use any of the rest of 
Montage directly.  It's purpose is provide a framework for displaying a
PNG that Montage has built, with the ability to zoom, pan, and pick locations.
The code that generates the image (mShrink, mSubimage, mViewer, etc.) is
part of the Python application code.  We will eventually include some of
those examples here but for now that code is part of other projects.

All this is written up here:

http://montage.ipac.caltech.edu/docs/mViewer_DASH/

That write-up details the process of setting up a build environment 
for the React component and this directory contains the React component
JS code (and style) used.
