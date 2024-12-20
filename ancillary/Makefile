CPPFLAGS +=	-I../lib/include
CFLAGS   +=	-std=gnu99
LIBS      =	-L../lib -lcfitsio -lm

HPXcvt:	HPXcvt.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

HPXcvt.o: HPXcvt.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $^

install:
				cp HPXcvt ../bin

clean:
				rm -f HPXcvt *.o
