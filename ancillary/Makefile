CPPFLAGS +=	-I../lib/include
CFLAGS   +=	-std=gnu99

HPXcvt:		HPXcvt.o
				gcc -std=gnu99 -o HPXcvt HPXcvt.o -L../lib -lcfitsio -lm

HPXcvt.o: HPXcvt.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $^

install:
				cp HPXcvt ../bin

clean:
				rm -f HPXcvt *.o
