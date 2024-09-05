
Building a Montage Python Binary Extension
------------------------------------------

If you want to build a complete set of Montage Python binary extension wheels
for all current versions of Python, Linux and Mac OSX, things get complicated.
This is covered for Linux and MacOS in two subdirectories (linux_wheels
and macos_wheels).  However building a wheel just for the local machine and
your current Python is quite straightforward and that is what is covered here.

For Linux, all you need to do is install a few packages in Python and run the
"make_local.sh" script included here.  The result is a wheel (zip) file in
the "dist" subdirectory which you can then "pip install" into Python (if
you already have Montage install using the same version number, you might
have to run "pip uninstall MontagePy" first).

The packages needed are "jinja2", "importlib-resources" and "build".

For MacOSX, there are a couple of additional things that have to be done.
The most important has to do with the way OSX builds code that needs to
run on multiple OS versions.  Mac executables are generally forward version
compatible.  That is, if you build it for an older version it will also run
on anything newer.  Their compiler can be told to build for a previous
OS version (the "deployment target").  So people building distributable code
usually target a version a few years old.

It gets more complicated when you need to link with libraries built by
someone else (as we have to do with Python).  Then your choice of target
version has to match theirs.  Right now, the current MacOSX version is 14.4
but the Python version we can install are all built with a target of 11.1
(for Anaconda; the version of the build from python.org is 10.9).

Therefore we have to make sure all our code is compiled to the same target.
This is done with an environment variable:

   MACOSX_DEPLOYMENT_TARGET='11.1'

Also, the Mac comes in two hardware flavors, x86_64 chips and ARM64.
Their recommended approach is to construct "universal2" object code, where
both versions are generated and kept in a single file (executable, .o
 or .so).  Since here we are just building for the local hardware, we
can let the compiler default that.

After that the process described in the second paragraph above is the same.
