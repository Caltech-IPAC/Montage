Contributing to Montage
=======================
Until June of 2020, Montage was maintained in the Accurev institutional Configuration
Management system at IPAC/Caltech.  Even though there had been for several years a
copy of the released version in GitHub, this made contributing difficult as there 
was an average of two years between major releases and the inevitable divergence of
what current code base and what people could see.

Montage is now entirely maintained through git, with both the formally released version
and a development branch (and occasional long-lived feature branches) available.

While we are open to merging certain bug fixes as patches to the released version,
most contributions should be structured as pull requests to the "develop" branch.


Testing and Continuous Integration
----------------------------------

We will make every effort to keep the develop branch in a state where it is safe
to download and use, even operationally, but beware that it cannot be as fully tested
as the formal releases.  Montage is a fairly big package and can deal with very large
datasets, so full regression testing can take weeks of computer time.  For example,
building a HiPS map at scale from 2MASS image data involves almost 2 TBytes for just
the final image data and certain aspects of the processing can only be tested at 
scale.  Moreover, it requires several CPU-months of processing which only makes
sense on a dedicated cluster or cloud.  Other modules can be tested more simply but
even here the volume of data and processing time for any real testing goes beyond 
what is realistic for any simple Continuous Integration model.

With that caveat in mind we still advocate using the develop branch if you can
for your applications.  Just test your use of it first.


Where Can You Contribute
------------------------

Of course, the majority of contributions are going to be things like bug fixes,
support for new versions of compilers, and so on.  Most of the Montage
modules are structured with the main functionality in a C function with each
main function wrapped with a command-line driver program.  Most people use this
set of programs but the functions are also combined into a shared-object library
and that library wrapped as a binary Python extension.  The library/programs are
also cross-compiled into a set of Windows executables.  Some modules have even 
been cross-"compiled" into asm.js Javascript that will run in browsers directly
at near-native speeds (this is not part of the distribution and was done at SAO 
as part of the SAOImage JS9 project).

All of these areas are fair game for contributions.  There is a very long list of 
things we hope to get time for but would be happy if someone else were interested
it taking them on, from as simple as cleaning up the rest of the modules that 
don't have a library form to formalizing the asm.js functionality to building a
version of the Python binary extension that works in Windows.

Montage has been used quite a bit by the IT community for research in developing
workflow systems and we include some early work done with one of the more advanced
of these:  Pegasus by the Information Systems Institute at USC.  But a lot more 
could be done with this, especially to make use of cloud computing.

And then, of course, there are completely new modules that could be added, either
to do new things or in support of specific projects/instruments.  We would be 
happy to advise or even collaborate with anyone interested in such an effort.

