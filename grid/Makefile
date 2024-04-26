all:
	(cd Pegasus         && $(MAKE) && $(MAKE) install)
	(cd Globus/ExecTG   && $(MAKE) && $(MAKE) install)
	(cd Globus/NotifyTG && $(MAKE) && $(MAKE) install)

clean:
	(cd Pegasus         && test ! -e Makefile || $(MAKE) clean)
	(cd Globus/ExecTG   && test ! -e Makefile || $(MAKE) clean)
	(cd Globus/NotifyTG && test ! -e Makefile || $(MAKE) clean)
