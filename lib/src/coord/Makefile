CC      =	gcc
# WORKAROUND (issue #96): coord.h declares functions with `()`, which means
# `(void)` under the GCC 15 C23 default, so calls with arguments and
# definitions with parameters fail. Remove once coord.h has real prototypes.
CFLAGS  =	-g -c -fPIC -D_LARGEFILE_SOURCE \
		-D_FILE_OFFSET_BITS=64 -std=gnu17 -I. -I../../include
LIBNAME =	libcoord.a

SRCS    =	ccalc.c                    \
   		parseCoordinateString.c    \
		sexToDecimalDegree.c       \
		decimalDegreeToSex.c       \
		convertCoordinates.c       \
		convertBesselianJulian.c   \
		precessBesselian.c         \
		precessJulian.c            \
		convertEclEqu.c            \
		convertEquGal.c            \
		convertGalSgal.c           \
		computeFKCorrections.c

OBJS    =	${SRCS:.c=.o}


libcoord.a:	${OBJS}
		rm -f ${LIBNAME}
		ar q ${LIBNAME} ${OBJS}

${OBJS}:	coord.h

install:
		cp libcoord.a ../..
		cp coord.h ../../include

clean:
		rm -f libcoord.a *.o *.a *.i a.out core
