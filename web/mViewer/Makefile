# ================================================================
# Makefile for product: wsvc/mViewer
#
#
# ================================================================

.SUFFIXES:
SHELL = /bin/sh
INSTALL = install
INSTALL_PROGRAM = $(INSTALL) -m 755 -p
INSTALL_DATA = $(INSTALL) -m 644 -p
INSTALL_DIR = $(INSTALL) -m 755 -d

# The directory for the base product group,
# of which this product is a member
CM_EXO_DIR = $(shell cd ../../..; pwd)

# Standard directory variables
prefix=$(CM_BASE_APP_DIR)
exec_prefix=$(CM_BASE_APP_DIR)

datadir=$(prefix)/share/wsvc/mViewer
webdir=$(prefix)/web
cgidir=$(webdir)/cgi-bin/mViewer
appdir=$(webdir)/applications/mViewer
srcdir=.


# The names of all executables in this product
progs = nph-mViewer nph-mViewerInit nph-mViewerHdr nph-mViewerStats

# The names of all source files in this product
sources = nph-mViewer.c nph-mViewerInit.c nph-mViewerHdr.c nph-mViewerStats.c

# ================================================================
# Setup / sanity checks
# ================================================================

# Determine whether or not compilation can occur
comp_tgts = all install test $(progs) \
            $(sources:.c=.o) 
dep_check = true
ifeq ($(filter-out $(MAKECMDGOALS), $(comp_tgts)),$(strip $(comp_tgts)))
    ifneq ($(MAKECMDGOALS),)
        dep_check = false
    endif
endif

# Make sure required environment variables are defined
ifeq ($(dep_check),true)
    ifeq ($(wildcard $(CM_BASE_DIR)),)
        $(error error: CM_BASE_DIR environment variable invalid)
    endif
    $(warning CM_BASE_DIR:    $(CM_BASE_DIR))
endif

# Function to search PATH for a file
pathsearch = $(firstword $(wildcard $(addsuffix /$(strip $(1)),$(subst :, ,$(PATH)))))

# Standard compiler variables
CC = gcc
ifndef CM_RELEASE_BUILD
    CFLAGS = -g -Wall -D_FILE_OFFSET_BITS=64
else
    CFLAGS = -O -D_FILE_OFFSET_BITS=64
endif
CPPFLAGS = -I$(srcdir) \
           -I$(CM_BASE_DIR)/include \
           -DMY_DATA_DIR='"$(datadir)"'

# When compiling, create/update build environment file
ifeq ($(dep_check),true)
    build_env = $(call pathsearch, $(CC)) \
                $(CM_BASE_DIR) \
		$(CM_EXO_DIR)
    prev_env =  $(shell cat ./.build_env)
    ifneq ($(strip $(build_env)), $(strip $(prev_env)))
        $(warning creating/updating build environment file (.build_env))
        $(shell rm -f ./.build_env)
        $(shell echo "$(build_env)" > ./.build_env)
    endif
endif

# ================================================================
# External library dependencies
# ================================================================
base_libs = $(addprefix $(CM_BASE_DIR)/lib/, \
	libreqlog.a libwww.a libpassword.a libconfig.a libsvc.a libjson.a)

# ================================================================
# Pattern Rules
# ================================================================

vpath %.c       $(srcdir)
vpath %.h       $(srcdir)

# Creates object files from C source files
%.o : %.c
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@


# ================================================================
# Targets
# ================================================================

all :		$(progs) 

.PHONY :	all clean distclean installdirs install installdocs uninstall
.DELETE_ON_ERROR : ;

# Compiles executables
nph-mViewer :	nph-mViewer.o $(base_libs)
		$(CC) $(CFLAGS) -o $@ $^ $(base_libs) -lm

nph-mViewerInit :	nph-mViewerInit.o $(base_libs)
		$(CC) $(CFLAGS) -o $@ $^ $(base_libs) -lm

nph-mViewerHdr :	nph-mViewerHdr.o $(base_libs)
		$(CC) $(CFLAGS) -o $@ $^ $(base_libs) -lm

nph-mViewerStats :	nph-mViewerStats.o $(base_libs)
		$(CC) $(CFLAGS) -o $@ $^ $(base_libs) -lm

# Cleans the build tree
clean :
		rm -f *.o $(progs) ./.build_env core webcontent.tar data.tar

# The same as clean
distclean :	clean

# Creates required installation directories
installdirs :
		$(INSTALL_DIR) $(datadir) $(cgidir) $(appdir)

# Creates a tar-ball for all web content
FORCE:
data.tar : FORCE
		tar -cf $@ -C $(srcdir)/data .
webcontent.tar : FORCE 
		tar -cf $@ -C $(srcdir)/html .


# Installs executables
install :       $(progs) webcontent.tar installdirs 
		tar -cf webcontent.tar -C $(srcdir)/html .
		$(INSTALL_PROGRAM) $(progs) $(cgidir)
		rm -rf $(appdir)/*
		$(INSTALL_DATA) webcontent.tar $(appdir)
		cd $(appdir); tar -xf webcontent.tar; rm webcontent.tar
		cp $(srcdir)/data/template.html $(datadir)

installdata :  data.tar installdirs 
		rm -rf $(datadir)/*
		$(INSTALL_DATA) data.tar $(datadir)
		cd $(datadir); tar -xf data.tar; rm data.tar
		rm -rf $(appdir)/*

# Uninstalls the product
uninstall :
		rm -rf $(cgidir) $(appdir) $(datadir)

# ================================================================

