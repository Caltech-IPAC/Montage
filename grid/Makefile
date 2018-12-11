all:
	(cd Pegasus         && ./Configure.sh && $(MAKE) && $(MAKE) install)
	(cd Globus/ExecTG   && ./Configure.sh && $(MAKE) && $(MAKE) install)
	(cd Globus/NotifyTG && ./Configure.sh && $(MAKE) && $(MAKE) install)

clean:
	(cd Pegasus         && test ! -e Makefile || $(MAKE) clean)
	(cd Globus/ExecTG   && test ! -e Makefile || $(MAKE) clean)
	(cd Globus/NotifyTG && test ! -e Makefile || $(MAKE) clean)
