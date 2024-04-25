
Building a Montage Python Binary Extension 
------------------------------------------

If you want to build Python binary extension wheels for Montage generally,
things get complicated.  This is covered for Linux and MacOS in two 
subdirectories.  But for most people who want to buid wheel for their own
use and on the machine where they will use it, the process is quite 
straightforward.

In fact for Linux, all you need to do is install a few packages in Python
and run the "make_local.sh" script included here.  There result is a 
wheel (zip) file in the "dist" subdirectory which you can then "pip 
install" into Python (if you already have Montage install using the same
version number, you might have to run "pip uninstall MontagePy" first).

The packages needed are "jinja2" and "build".

For MacOSX, there are few additional things that have to be done.
The most important has to do with the way OSX deals with building code
that needs to run on multiple OS versions.  Mac executable are generally
forward version compatible.  That is, if you build it on an older version
it will also run on anything newer.  Their compiler can also be told to
build for a previous OS version (the "deployment target").  So people 
building distributable code usually target a version a few years old.

It gets more complicated when you need to link with libraries built by
someone else (as we have to do with Python).  Then your choice of target
version has to match theirs.  Right now, the current MacOSX version is 14.4
but the Python version we can install are all built with a target of 11.1
(Anaconda; the version for the build from python.org is 10.9).

Therefore we have to make sure all our code is compiled to the same target.
This is done with an environment variable:

   MACOSX_DEPLOYMENT_TARGET='11.1' 

Next, the Mac comes in two hardware flavors, x86_64 chips and ARM64.
Their recommended approach is to construct "universal2" object code, where
both versions are generated and kept in a single file (executable, .o
 or .so).  Since here we are just building for the local hardware, we
can let the compiler default to that.

Finally, the build includes one "sed" call, which is currently set 
up to use GNU sed (the Linux default).  The Mac doesn't support this
out of the box and we may update this to something that works the same
on both versions in thd future but for now the simplest thing is to install
the GNU sed (using "brew install gnu-sed") and change the "make_local"
script to use "gsed" instead of "sed".  

After that the process describe in the second paragraph above is the
same.
