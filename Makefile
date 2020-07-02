all:
	mkdir -p bin
	mkdir -p lib/include
	if test -d lib/src;    then (cd lib/src;    make); fi
	if test -d Montage;    then (cd Montage;  ./Configure.sh; make; make install); fi
	if test -d util;       then (cd util;       make); fi
	if test -d grid;       then (cd grid;       make); fi
	if test -d MontageLib; then (cd MontageLib; make); fi
	if test -d ancillary;  then (cd ancillary;  make; make install); fi
	if test -d HiPS;       then (cd HiPS;       make); fi

clean:
	mkdir -p bin
	mkdir -p lib/include
	rm -rf bin/*
	if test -d lib/src;    then (cd lib/src;    make clean); fi
	if test -d Montage;    then (cd Montage;    make clean); fi
	if test -d util;       then (cd util;       make clean); fi
	if test -d grid;       then (cd grid;       make clean); fi
	if test -d MontageLib; then (cd MontageLib; make clean); fi
	if test -d ancillary;  then (cd ancillary;  make clean); fi
	if test -d HiPS;       then (cd HiPS;       make clean); fi
