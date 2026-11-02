top_srcdir=.
CONFIG_MAKE=$(top_srcdir)/config.make

-include $(top_srcdir)/config.make
include $(top_srcdir)/cfitsio.make

CFITSIO_ALLOW_UNPATCHED ?= 0

# Records the CFITSIO selection this tree is built with: the mode (0 bundled,
# 1 system), then the CFITSIO_CFLAGS and CFITSIO_LIBS. `all` writes it once
# check-build-mode and check-system-cfitsio pass, before building anything,
# so a build that fails part way still records what its objects used.
MODE_STAMP = .cfitsio-build-mode
sq = '$(subst ','\'',$(1))'
mode_stamp_text = printf '%s\n' $(call sq,$(USE_SYSTEM_CFITSIO)) \
	$(call sq,$(CFITSIO_CFLAGS)) $(call sq,$(CFITSIO_LIBS))
cfitsio_selection = CFITSIO_CFLAGS='$(CFITSIO_CFLAGS)' CFITSIO_LIBS='$(CFITSIO_LIBS)'

.PHONY: all check-build-mode check-system-cfitsio install clean test

all: check-build-mode check-system-cfitsio
	$(mode_stamp_text) > $(MODE_STAMP).tmp && mv -f $(MODE_STAMP).tmp $(MODE_STAMP)
	mkdir -p bin
	mkdir -p lib/include
	test ! -d lib/src    || (cd lib/src    && $(MAKE))
	test ! -d Montage    || (cd Montage    && $(MAKE) && $(MAKE) install)
	test ! -d util       || (cd util       && $(MAKE))
	test ! -d grid       || (cd grid       && $(MAKE))
	test ! -d MontageLib || (cd MontageLib && $(MAKE))
	test ! -d ancillary  || (cd ancillary  && $(MAKE) && $(MAKE) install)
	test ! -d HiPS       || (cd HiPS       && $(MAKE))

# Objects and programs built against one CFITSIO must not be mixed with
# another: switching mode, or between two system CFITSIOs, needs make clean.
# Without a stamp, a leftover lib/libcfitsio.a means a bundled build, and
# lib/libwcs.a without lib/libcfitsio.a means an (unfinished) system build.
check-build-mode:
	@want=`$(mode_stamp_text)`; wantmode=$(USE_SYSTEM_CFITSIO); have=; \
	if test -f $(MODE_STAMP); then have=`cat $(MODE_STAMP)`; havemode=`sed -n 1p $(MODE_STAMP)`; \
	elif test -f lib/libcfitsio.a; then havemode=0; \
	elif test -f lib/libwcs.a; then havemode=1; \
	else havemode=$$wantmode; fi; \
	if test "$$havemode" != "$$wantmode"; then \
	    if test "$$havemode" = 1; then old="a system CFITSIO (USE_SYSTEM_CFITSIO=1)"; \
	    else old="the bundled CFITSIO (USE_SYSTEM_CFITSIO=0)"; fi; \
	    echo "ERROR: this tree was built with $$old; run 'make clean' before building with USE_SYSTEM_CFITSIO=$$wantmode." >&2; \
	    exit 1; \
	elif test -n "$$have" && test "$$have" != "$$want"; then \
	    old=`sed -n 2,3p $(MODE_STAMP) | { read c; read l; echo "CFITSIO_CFLAGS='$$c' CFITSIO_LIBS='$$l'"; }`; \
	    echo "ERROR: this tree was built with a different CFITSIO ($$old); run 'make clean' before building with $(cfitsio_selection)." >&2; \
	    exit 1; \
	fi

# In system mode, compile, link and run test programs against the selected
# CFITSIO: its headers and library must be the same version, and the
# CVE-2026-115504 regression test must pass. That test exits 1 if an
# oversized variable-length string read is not rejected (the only failure
# CFITSIO_ALLOW_UNPATCHED=1 overrides), 2 if it cannot set up its test file,
# and 3 if valid data is mishandled. These exit statuses (0 = pass, 1, 2, 3)
# are defined in tests/cve_vlen_regression.c. The test programs are compiled
# with $(CC) (make's default, cc); the component Makefiles use gcc, so pass
# CC=... if cc is not the compiler the build uses.
check-system-cfitsio:
ifeq ($(USE_SYSTEM_CFITSIO),1)
	@tmp=`mktemp -d` && trap 'rm -rf "$$tmp"' EXIT && \
	sel="$(cfitsio_selection)" && \
	if ! $(CC) $(CFITSIO_CFLAGS) -o "$$tmp/version" "$(CURDIR)/tests/cfitsio_system_version.c" $(CFITSIO_LIBS) || \
	   ! $(CC) $(CFITSIO_CFLAGS) -o "$$tmp/cve" "$(CURDIR)/tests/cve_vlen_regression.c" $(CFITSIO_LIBS); then \
	    echo "ERROR: cannot compile and link against the system CFITSIO ($$sel). Set CFITSIO_DIR, or put the -I and -L options in CFITSIO_CFLAGS and CFITSIO_LIBS (CPPFLAGS and LDFLAGS are not used)." >&2; exit 1; \
	fi; \
	status=0; version=`cd "$$tmp" && ./version` || status=$$?; \
	if test $$status = 1; then \
	    echo "ERROR: the system CFITSIO headers and library are different versions ($$version; $$sel)." >&2; exit 1; \
	elif test $$status != 0; then \
	    echo "ERROR: a program linked against the system CFITSIO does not run ($$sel). For a shared library outside the loader's search path, add -Wl,-rpath,<libdir> to CFITSIO_LIBS (CFITSIO_DIR does this) or set LD_LIBRARY_PATH." >&2; exit 1; \
	fi; \
	echo "System CFITSIO $$version: compiles, links and runs ($$sel)."; \
	status=0; (cd "$$tmp" && ./cve) > "$$tmp/cve.log" 2>&1 || status=$$?; \
	case $$status in \
	0) ;; \
	1) if test "$(CFITSIO_ALLOW_UNPATCHED)" = 1; then \
	       echo "WARNING: the system CFITSIO lacks the CVE-2026-115504 fix; building anyway because CFITSIO_ALLOW_UNPATCHED=1." >&2; \
	   else \
	       cat "$$tmp/cve.log" >&2; \
	       echo "ERROR: the system CFITSIO lacks the CVE-2026-115504 fix (an oversized variable-length string read was not rejected). Use a fixed CFITSIO or the bundled one (USE_SYSTEM_CFITSIO=0), or set CFITSIO_ALLOW_UNPATCHED=1 to build anyway." >&2; \
	       exit 1; \
	   fi ;; \
	2) cat "$$tmp/cve.log" >&2; \
	   echo "ERROR: the CVE-2026-115504 probe could not run (setup failure; see the messages above)." >&2; exit 1 ;; \
	3) cat "$$tmp/cve.log" >&2; \
	   echo "ERROR: the system CFITSIO rejects valid strings (a short variable-length string was not read back intact, or a string at the 28800-byte limit was rejected)." >&2; exit 1 ;; \
	*) cat "$$tmp/cve.log" >&2; \
	   echo "ERROR: the CVE-2026-115504 probe failed unexpectedly (exit status $$status)." >&2; exit 1 ;; \
	esac
endif

install:
	cp bin/* $(montage_prefix)/bin

clean:
	mkdir -p bin
	mkdir -p lib/include
	rm -rf bin/*
	test ! -d lib/src          || (cd lib/src    && $(MAKE) clean)
	test ! -e Montage/Makefile || (cd Montage    && $(MAKE) clean)
	test ! -d util             || (cd util       && $(MAKE) clean)
	test ! -d grid             || (cd grid       && $(MAKE) clean)
	test ! -d MontageLib       || (cd MontageLib && $(MAKE) clean)
	test ! -d ancillary        || (cd ancillary  && $(MAKE) clean)
	test ! -d HiPS             || (cd HiPS       && $(MAKE) clean)
	rm -f $(MODE_STAMP) $(MODE_STAMP).tmp

test:
	sh tests/run_tests.sh
