all:
	mkdir -p bin
	mkdir -p lib/include
	test ! -d lib/src    || (cd lib/src                   && make)
	test ! -d Montage    || (cd Montage && ./Configure.sh && make && make install)
	test ! -d util       || (cd util                      && make)
	test ! -d grid       || (cd grid                      && make)
	test ! -d MontageLib || (cd MontageLib                && make)
	test ! -d ancillary  || (cd ancillary                 && make && make install)

clean:
	mkdir -p bin
	mkdir -p lib/include
	rm -rf bin/*
	test ! -d lib/src          || (cd lib/src &&    make clean)
	test ! -e Montage/Makefile || (cd Montage &&    make clean)
	test ! -d util             || (cd util &&       make clean)
	test ! -d grid             || (cd grid &&       make clean)
	test ! -d MontageLib       || (cd MontageLib && make clean)
	test ! -d ancillary        || (cd ancillary &&  make clean)
