OBJECTS = card.o \
	index.o \
	mfmalloc.o \
	node.o \
	rect.o \
	split_q.o 

all: librtree.a

librtree.a: $(OBJECTS)
	ar rcv librtree.a $(OBJECTS)
	ranlib librtree.a

clean:
	rm -rf ./*.o ./*.a

