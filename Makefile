top_srcdir=.
CONFIG_MAKE=$(top_srcdir)/config.make

-include $(top_srcdir)/config.make
include $(top_srcdir)/cfitsio.make

CFITSIO_ALLOW_UNPATCHED ?= 0

# Records the CFITSIO mode (0 bundled, 1 system) of the last successful build.
MODE_STAMP = .cfitsio-build-mode

.PHONY: all check-build-mode check-system-cfitsio install clean test

all: check-build-mode check-system-cfitsio
	mkdir -p bin
	mkdir -p lib/include
	test ! -d lib/src    || (cd lib/src    && $(MAKE))
	test ! -d Montage    || (cd Montage    && $(MAKE) && $(MAKE) install)
	test ! -d util       || (cd util       && $(MAKE))
	test ! -d grid       || (cd grid       && $(MAKE))
	test ! -d MontageLib || (cd MontageLib && $(MAKE))
	test ! -d ancillary  || (cd ancillary  && $(MAKE) && $(MAKE) install)
	test ! -d HiPS       || (cd HiPS       && $(MAKE))
	echo $(USE_SYSTEM_CFITSIO) > $(MODE_STAMP)

# Objects and programs from one CFITSIO mode must not be mixed with the other.
# Without a stamp, a leftover lib/libcfitsio.a means a bundled build, and
# lib/libwcs.a without lib/libcfitsio.a means an (unfinished) system build.
check-build-mode:
	@want=$(USE_SYSTEM_CFITSIO); \
	if test -f $(MODE_STAMP); then have=`cat $(MODE_STAMP)`; \
	elif test -f lib/libcfitsio.a; then have=0; \
	elif test -f lib/libwcs.a; then have=1; \
	else have=$$want; fi; \
	if test "$$have" != "$$want"; then \
	    if test "$$have" = 1; then old="a system CFITSIO (USE_SYSTEM_CFITSIO=1)"; \
	    else old="the bundled CFITSIO (USE_SYSTEM_CFITSIO=0)"; fi; \
	    echo "ERROR: this tree was built with $$old; run 'make clean' before building with USE_SYSTEM_CFITSIO=$$want." >&2; \
	    exit 1; \
	fi

# In system mode, compile, link and run test programs against the selected
# CFITSIO, including the CVE-2026-115504 regression test, which requires
# READ_ERROR from an oversized variable-length string read.
check-system-cfitsio:
ifeq ($(USE_SYSTEM_CFITSIO),1)
	@tmp=`mktemp -d` && trap 'rm -rf "$$tmp"' EXIT && \
	sel="CFITSIO_CFLAGS='$(CFITSIO_CFLAGS)' CFITSIO_LIBS='$(CFITSIO_LIBS)'" && \
	printf '%s\n' '#include <stdio.h>' '#include "fitsio.h"' \
	    'int main(void) { float v = 0; int x; fits_get_version(&v); x = (int)(v * 10000 + 0.5);' \
	    '  printf("%d.%d.%d\n", x / 10000, x / 100 % 100, x % 100); return !(v > 0); }' \
	    > "$$tmp/version.c" && \
	if ! $(CC) $(CFITSIO_CFLAGS) -o "$$tmp/version" "$$tmp/version.c" $(CFITSIO_LIBS) || \
	   ! $(CC) $(CFITSIO_CFLAGS) -o "$$tmp/cve" "$(CURDIR)/tests/cve_vlen_regression.c" $(CFITSIO_LIBS); then \
	    echo "ERROR: cannot compile and link against the system CFITSIO ($$sel)." >&2; exit 1; \
	fi; \
	if ! version=`cd "$$tmp" && ./version`; then \
	    echo "ERROR: a program linked against the system CFITSIO does not run ($$sel)." >&2; exit 1; \
	fi; \
	echo "System CFITSIO $$version: compiles, links and runs ($$sel)."; \
	if (cd "$$tmp" && ./cve); then :; \
	elif test "$(CFITSIO_ALLOW_UNPATCHED)" = 1; then \
	    echo "WARNING: the system CFITSIO lacks the CVE-2026-115504 fix; building anyway because CFITSIO_ALLOW_UNPATCHED=1." >&2; \
	else \
	    echo "ERROR: the system CFITSIO lacks the CVE-2026-115504 fix (an oversized variable-length string read did not fail with READ_ERROR). Use a fixed CFITSIO or the bundled one (USE_SYSTEM_CFITSIO=0), or set CFITSIO_ALLOW_UNPATCHED=1 to build anyway." >&2; \
	    exit 1; \
	fi
endif

install:
	cp bin/* $(montage_prefix)/bin

clean:
	rm -f $(MODE_STAMP)
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

test:
	sh tests/run_tests.sh
