#!/bin/sh

# Version  Developer        Date     Change
# -------  ---------------  -------  -----------------------
# 1.1      John Good        31Oct15  Added Cygwin   
# 1.0      John Good        29Jan03  Original script

osname=`uname| cut -b 1-6`

echo OS: $osname

  if [ $osname = 'SunOS'  ] ; then cp Makefile.SunOS  Makefile ;
elif [ $osname = 'HPUX'   ] ; then cp Makefile.LINUX  Makefile ;
elif [ $osname = 'AIX'    ] ; then cp Makefile.LINUX  Makefile ;
elif [ $osname = 'LINUX'  ] ; then cp Makefile.LINUX  Makefile ;
elif [ $osname = 'Darwin' ] ; then cp Makefile.Darwin Makefile ;
elif [ $osname = 'CYGWIN' ] ; then cp Makefile.Darwin Makefile ;
else                               cp Makefile.LINUX  Makefile ;  fi
