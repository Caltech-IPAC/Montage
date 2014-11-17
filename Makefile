all:
	mkdir -p bin
	mkdir lib/include
	(cd lib/src; make)
	(cd Montage; ./Configure.sh; make; make install)
	if test -d util; then (cd util; make); fi
	if test -d grid; then (cd grid; make); fi

clean:
	mkdir -p bin
	mkdir lib/include
	rm -f bin/*
	(cd lib/src; make clean)
	(cd Montage; make clean)
	if test -d util; then (cd util; make clean); fi
	if test -d grid; then (cd grid; make clean); fi
