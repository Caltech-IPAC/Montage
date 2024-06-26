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

# OS detection (poor mans autoconf)
os = $(firstword $(shell uname))
ifeq ($(filter SunOS Linux,$(os)),)
    $(error $(os) platform not supported - please compile on Linux or Solaris)
else
    $(info Building for $(os) platform)
endif

# Standard directory variables
prefix=$(CM_BASE_APP_DIR)
exec_prefix=$(CM_BASE_APP_DIR)

datadir=$(prefix)/share/wsvc/mViewer
webdir=$(prefix)/web
cgidir=$(webdir)/cgi-bin/mViewer
appdir=$(webdir)/applications/mViewer
srcdir=.

# The names of all executables in this product
progs = nph-mViewer nph-mViewerHdr

# The names of all source files in this product

srcmviewer = nph-mViewer.c \
	extractViewParam.c \
	getFitshdr.c \
	checkFileExist.c \
        colorLookup.c \
	makeImage.c \
	printRetval.c \
	mviewerUtil.c \
	imZoom.c \
	pick.c \
        pix2sky.c \
	sky2pix.c \
	compute_normal.c \
	computeSkyDist.c \
	constraintFilter.c \
	parseCsysstr.c \
	subsetImage.c \
	constructRetjson.c \
	fileCopy.c \
	extractAvePlane.c \
	generateMedianPlane.c \
	qsort.c


srcmviewerhdr = nph-mViewerHdr.c \
	writeFitshdrHtml.c \
	printRetval.c


# ================================================================
# Setup / sanity checks
# ================================================================

# Determine whether or not compilation can occur
comp_tgts = all install test $(progs) \
            $(sources:.c=.o) $(sources:.c=.d) \
	    $(srcmviewer:.c=.o) $(srcmviewer:.c=.d) \
	    $(srcmviewerhdr:.c=.o) $(srcmviewerhdr:.c=.d)


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
    ifeq ($(wildcard $(CM_TPS_DIR)),)
        $(error error: CM_TPS_DIR environment variable invalid)
    endif
    $(warning CM_TPS_DIR:    $(CM_TPS_DIR))
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
           -I$(CM_TPS_DIR)/include \
           -DCM_BASE_APP_DIR='"$(CM_BASE_APP_DIR)"' \
	   -DMY_DATA_DIR='"$(datadir)"'


# When compiling, create/update build environment file
ifeq ($(dep_check),true)
    build_env = $(call pathsearch, $(CC)) \
                $(CM_TPS_DIR) \
                $(CM_BASE_DIR) \
		$(CM_BASE_APP_DIR)
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
sys_libs = -lexpat -lm
ifeq (SunOS,$(os))
    sys_libs += -lsocket -lnsl
endif
base_libs = $(addprefix $(CM_BASE_DIR)/lib/, \
	liblookup.a libcoord.a libwww.a libpassword.a libencode.a \
	libconfig.a libvarcmd.a libmtbl.a libsvc.a libjson.a)
tps_libs = $(addprefix $(CM_TPS_DIR)/lib/, \
	libcfitsio.a libwcs.a libnedcli.a libsimcli.a)
env_libs =  $(addprefix $(CM_ENV_DIR)/misc/lib/, libscew.a) /usr/lib64/libcrypto.so


# ================================================================
# Pattern Rules
# ================================================================

vpath %.c       $(srcdir)
vpath %.h       $(srcdir)

# Creates object files from C source files
%.o : %.c
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

# Creates dependency files from C source files
%.d : %.c
		@echo ... Creating $@; set -e; rm -f $@; \
		$(CC) -MM -MF $@ $(CPPFLAGS) $<; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ./.build_env ,g' < $@ > $@.$$$$; \
		mv $@.$$$$ $@


# ================================================================
# Targets
# ================================================================

all :		$(progs) 

.PHONY :	all clean distclean installdirs install installdocs uninstall
.SECONDARY :    $(sources:.c=.d) $(srcmviewer:.c=.d) $(srcmviewerhdr:.c=.d)
.DELETE_ON_ERROR : ;

ifeq ($(dep_check),true)
    include $(sources:.c=.d)
    include $(srcmviewer:.c=.d)
    include $(srcmviewerhdr:.c=.d)
endif



# Compiles executables
nph-mViewer :	$(srcmviewer:.c=.o) $(base_libs) $(tps_libs) \
		$(env_libs)
		$(CC) $(CFLAGS) -o $@ $^ $(sys_libs)

nph-mViewerHdr :	$(srcmviewerhdr:.c=.o) $(base_libs)
		$(CC) $(CFLAGS) -o $@ $^ $(sys_libs)


# Cleans the build tree
clean :
		rm -f *.o $(progs) ./.build_env core webcontent.tar

# The same as clean
distclean :	clean

# Creates required installation directories
installdirs :
		$(INSTALL_DIR) $(cgidir) $(appdir)



# Creates a tar-ball for all web content
FORCE:
#data.tar : FORCE
webcontent.tar : FORCE 
		tar -cf $@ -C $(srcdir)/html .


# Installs executables
install :       $(progs) webcontent.tar installdirs 
		tar -cf webcontent.tar -C $(srcdir)/html .
		$(INSTALL_PROGRAM) $(progs) $(cgidir)
		rm -rf $(appdir)/*
		$(INSTALL_DATA) webcontent.tar $(appdir)
		cd $(appdir); tar -xf webcontent.tar; rm webcontent.tar
		mv $(appdir)/mViewer.js $(appdir)/mViewer_v1.0.js


# Uninstalls the product
uninstall :
		rm -rf $(cgidir) $(appdir)

# ================================================================

