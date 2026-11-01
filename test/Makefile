# Golden tests; see run.sh.  Build Montage (make at the top level) first.
#
#   make check             run every case
#   make check UPDATE=1    rewrite expected/ from the current output
#   make fixtures          regenerate fixtures/ (rarely; commit the result)

CC     =	gcc
CFLAGS =	-g -O2 -I../lib/include -std=c99 -Wall
LIBS   =	-L../lib -lcfitsio -lm

check:		fitsdump
		UPDATE="$(UPDATE)" sh run.sh

fitsdump:	fitsdump.c
		$(CC) $(CFLAGS) -o fitsdump fitsdump.c $(LIBS)

mkfixtures:	mkfixtures.c
		$(CC) $(CFLAGS) -o mkfixtures mkfixtures.c $(LIBS)

fixtures:	mkfixtures
		./mkfixtures fixtures

clean:
		rm -rf fitsdump mkfixtures results

.PHONY:		check fixtures clean
