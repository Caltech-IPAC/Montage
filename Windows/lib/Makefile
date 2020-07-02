all:	libc zlib libpng

libc:
	(cd fakec; make; make install)

zlib:
	(cd zlib-1.2.11; ./configure --prefix=/usr/src/mxe/usr/x86_64-w64-mingw32.static --static; \
         make; make install)

libpng:
	(cd libpng-1.6.29; \
	 ./configure --prefix=/usr/src/mxe/usr/x86_64-w64-mingw32.static --build x86_64-w64-mingw32 --host x86_64-unknown-linux-gnu; \
	 make; make install)
