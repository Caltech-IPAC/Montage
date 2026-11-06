# USE_SYSTEM_CFITSIO=1 (set by the top-level Makefile, see ../../cfitsio.make)
# links an installed CFITSIO instead, so the bundled one is not built and any
# bundled archive and headers are removed from ../libcfitsio.a and ../include.
USE_SYSTEM_CFITSIO ?= 0

all:
	(cd bzip2-1.0.6 && $(MAKE) && cp libbz2.a ../.. && cp bzlib.h ../../include)
ifeq ($(USE_SYSTEM_CFITSIO),1)
	rm -rf zlib-build cfitsio-build ../libcfitsio.a
	(cd cfitsio-4.7.0 && for h in *.h; do rm -f ../../include/$$h; done)
else
	# CFITSIO 4.7.0 with the CVE-2026-115504 fix (see cfitsio-4.7.0/MONTAGE_PROVENANCE)
	# and the zlib 1.3.2 it requires. Built out of tree so the vendored sources
	# stay unmodified; static and position-independent because MontagePy links
	# the objects. The two archives are combined into one lib/libcfitsio.a, so
	# consumers keep linking with -lcfitsio alone.
	# zlib-build comes first in CPPFLAGS so CFITSIO sees the zconf.h zlib was
	# configured with. Only the library is built, without Fortran wrappers, so
	# the archive is the same whether or not a Fortran compiler is installed.
	rm -rf zlib-build cfitsio-build && mkdir zlib-build cfitsio-build
	(cd zlib-build && CFLAGS='-g -O2 -fPIC' ../zlib-1.3.2/configure --static && $(MAKE) libz.a)
	(cd cfitsio-build && CFLAGS='-g -O2 -fPIC' \
	    CPPFLAGS='-I$(CURDIR)/zlib-build -I$(CURDIR)/zlib-1.3.2' LDFLAGS='-L$(CURDIR)/zlib-build' \
	    ../cfitsio-4.7.0/configure --disable-curl --disable-shared --enable-static \
	    --without-fortran && $(MAKE) libcfitsio.la)
	cp cfitsio-4.7.0/*.h ../include
	rm -f ../libcfitsio.a && mkdir cfitsio-build/combined
	(cd cfitsio-build/combined && $(AR) x ../.libs/libcfitsio.a && \
	    $(AR) x ../../zlib-build/libz.a && $(AR) crs ../../../libcfitsio.a *.o)
endif
	(cd cmd && $(MAKE) && $(MAKE) install)
	(cd coord && $(MAKE) && $(MAKE) install)
	(cd mtbl && $(MAKE) && $(MAKE) install)
	(cd svc && $(MAKE) && $(MAKE) install)
	(cd json && $(MAKE) && $(MAKE) install)
	(cd boundaries && $(MAKE) && $(MAKE) install)
	(cd pixbounds && $(MAKE) && $(MAKE) install)
	(cd www && $(MAKE) && $(MAKE) install)
	(cd wcssubs3.9.0_montage && $(MAKE) && cp libwcs.a ../.. && cp *.h ../../include)
	(cd two_plane_v1.1 && $(MAKE) && $(MAKE) install)
	(cd lodepng_20140823 && $(MAKE) && $(MAKE) install)
	(cd jpeg-8b && ./configure --with-pic && $(MAKE) && cp .libs/libjpeg.a ../.. && cp jpeglib.h jconfig.h jmorecfg.h ../../include)
	(cd freetype-2.13.3 && ./configure --prefix=`pwd`/../../freetype --without-png --without-zlib --without-bzip2 --without-harfbuzz --without-brotli && $(MAKE) && $(MAKE) install)
	(cd .. && ranlib *.a)
	chmod 644 ../include/*.h

clean:
	rm -rf ../include/* ../freetype ../*.a
	(cd bzip2-1.0.6 && $(MAKE) clean)
	rm -rf zlib-build cfitsio-build
	(cd cmd && $(MAKE) clean)
	(cd coord && $(MAKE) clean)
	(cd mtbl && $(MAKE) clean)
	(cd svc && $(MAKE) clean)
	(cd json && $(MAKE) clean)
	(cd boundaries && $(MAKE) clean)
	(cd www && $(MAKE) clean)
	(cd pixbounds && $(MAKE) clean)
	(cd www && $(MAKE) clean)
	(cd wcssubs3.9.0_montage && $(MAKE) clean)
	(cd two_plane_v1.1 && $(MAKE) clean)
	(cd lodepng_20140823 && $(MAKE) clean)
	test ! -e jpeg-8b/Makefile || (cd jpeg-8b && $(MAKE) clean)
	(cd freetype-2.13.3 && $(MAKE) distclean && rm -rf objs/libfreetype.la  objs/.libs)
