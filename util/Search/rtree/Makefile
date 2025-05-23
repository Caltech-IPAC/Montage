OBJECTS = card.o \
	index.o \
	mfmalloc.o \
	node.o \
	rect.o \
	split_q.o 

all: librtree.a test

librtree.a: $(OBJECTS)
	ar rcv librtree.a $(OBJECTS)
	ranlib librtree.a

test: test.c librtree.a
	$(CC) $(LDFLAGS) -o $@ $^ librtree.a -lm

clean:
	rm -rf ./*.o ./*.a

