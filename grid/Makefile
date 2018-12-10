all:
	(cd Pegasus         && ./Configure.sh && make && make install)
	(cd Globus/ExecTG   && ./Configure.sh && make && make install)
	(cd Globus/NotifyTG && ./Configure.sh && make && make install)

clean:
	(cd Pegasus         && test ! -e Makefile || make clean)
	(cd Globus/ExecTG   && test ! -e Makefile || make clean)
	(cd Globus/NotifyTG && test ! -e Makefile || make clean)
