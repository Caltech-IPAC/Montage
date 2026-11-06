# CFITSIO selection, included by the top-level Makefile.
#
# By default (USE_SYSTEM_CFITSIO=0) Montage builds and links the CFITSIO in
# lib/src. With USE_SYSTEM_CFITSIO=1 it uses an installed CFITSIO instead,
# chosen in this order:
#   1. CFITSIO_LIBS (and optionally CFITSIO_CFLAGS) given explicitly;
#   2. CFITSIO_DIR=<prefix>: <prefix>/include and <prefix>/lib. A shared
#      libcfitsio is linked with an rpath to <prefix>/lib; a static-only
#      one with -lz -lm (zlib from <prefix>/lib or the default path). Other
#      layouts (lib64, extra dependencies such as curl or bzip2) need
#      pkg-config or explicit flags;
#   3. pkg-config cfitsio (CFITSIO_STATIC=1 adds --static to --libs).
#
# The result is exported as CFITSIO_CFLAGS and CFITSIO_LIBS; each consumer
# Makefile defaults CFITSIO_LIBS to -lcfitsio when built on its own.
#
# This file is tracked; ./configure only writes config.make.

USE_SYSTEM_CFITSIO ?= 0
CFITSIO_STATIC ?= 0
PKG_CONFIG ?= pkg-config

# `make clean` must work even when no system CFITSIO can be found.
cfitsio_goals := $(filter-out clean,$(if $(MAKECMDGOALS),$(MAKECMDGOALS),all))

cfitsio_select_help := select one with CFITSIO_DIR=<prefix>, \
CFITSIO_CFLAGS=... CFITSIO_LIBS=..., or a pkg-config 'cfitsio' package \
(PKG_CONFIG_PATH); or build the bundled CFITSIO with USE_SYSTEM_CFITSIO=0

# A command-line CFITSIO_LIBS or CFITSIO_CFLAGS overrides the bundled
# default below (make precedence); use USE_SYSTEM_CFITSIO=1 to link an
# installed CFITSIO.
ifeq ($(USE_SYSTEM_CFITSIO),0)
  CFITSIO_CFLAGS :=
  CFITSIO_LIBS := -lcfitsio
else ifeq ($(USE_SYSTEM_CFITSIO),1)
  ifneq ($(cfitsio_goals),)
    ifneq ($(strip $(CFITSIO_LIBS)),)
      # 1. Explicit flags, used as given; they win over CFITSIO_DIR.
      ifneq ($(strip $(CFITSIO_DIR)),)
        $(warning USE_SYSTEM_CFITSIO=1: CFITSIO_LIBS is set (origin: $(origin CFITSIO_LIBS)), so CFITSIO_DIR=$(CFITSIO_DIR) is ignored)
      endif
    else ifneq ($(strip $(CFITSIO_CFLAGS)),)
      $(error USE_SYSTEM_CFITSIO=1: CFITSIO_CFLAGS was given without CFITSIO_LIBS; $(cfitsio_select_help))
    else ifneq ($(strip $(CFITSIO_DIR)),)
      # 2. Installation prefix.
      ifeq ($(wildcard $(CFITSIO_DIR)/include/fitsio.h),)
        $(error USE_SYSTEM_CFITSIO=1: CFITSIO_DIR=$(CFITSIO_DIR) has no include/fitsio.h; $(cfitsio_select_help))
      endif
      ifeq ($(wildcard $(CFITSIO_DIR)/lib/libcfitsio.*),)
        $(error USE_SYSTEM_CFITSIO=1: CFITSIO_DIR=$(CFITSIO_DIR) has no lib/libcfitsio library; $(cfitsio_select_help))
      endif
      CFITSIO_CFLAGS := -I$(CFITSIO_DIR)/include
      ifneq ($(wildcard $(CFITSIO_DIR)/lib/libcfitsio.so $(CFITSIO_DIR)/lib/libcfitsio.dylib),)
        CFITSIO_LIBS := -L$(CFITSIO_DIR)/lib -Wl,-rpath,$(CFITSIO_DIR)/lib -lcfitsio -lm
      else
        CFITSIO_LIBS := -L$(CFITSIO_DIR)/lib -lcfitsio -lz -lm
      endif
    else
      # 3. pkg-config.
      cfitsio_pc_error := $(shell $(PKG_CONFIG) --print-errors --exists cfitsio 2>&1 || echo "(pkg-config --exists cfitsio failed)")
      ifneq ($(strip $(cfitsio_pc_error)),)
        $(error USE_SYSTEM_CFITSIO=1: no system CFITSIO found: $(cfitsio_pc_error); $(cfitsio_select_help))
      endif
      ifeq ($(CFITSIO_STATIC),1)
        cfitsio_pc_libs := --libs --static
      else
        cfitsio_pc_libs := --libs
      endif
      CFITSIO_CFLAGS := $(strip $(shell $(PKG_CONFIG) --cflags cfitsio))
      CFITSIO_LIBS := $(strip $(shell $(PKG_CONFIG) $(cfitsio_pc_libs) cfitsio)) -lm
    endif
  endif
else
  $(error USE_SYSTEM_CFITSIO must be 0 or 1, not '$(USE_SYSTEM_CFITSIO)')
endif

export USE_SYSTEM_CFITSIO CFITSIO_CFLAGS CFITSIO_LIBS
