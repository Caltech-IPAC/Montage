#!/bin/sh

osname=`uname| cut -b 1-6`

echo OS: $osname

  if [ $osname = 'SunOS'  ] ; then cp Makefile.SunOS  Makefile ;
elif [ $osname = 'HPUX'   ] ; then cp Makefile.LINUX  Makefile ;
elif [ $osname = 'AIX'    ] ; then cp Makefile.LINUX  Makefile ;
elif [ $osname = 'LINUX'  ] ; then cp Makefile.LINUX  Makefile ;
elif [ $osname = 'Darwin' ] ; then cp Makefile.Darwin Makefile ;
elif [ $osname = 'CYGWIN' ] ; then cp Makefile.Darwin Makefile ;
else                               cp Makefile.LINUX  Makefile ;  fi
