#!/bin/sh

tar cvf Montage_`date "+%d%b%y"`.tar \
   Montage/LICENSE \
   Montage/README.release \
   Montage/README.md \
   Montage/ChangeHistory \
   Montage/Makefile \
   Montage/lib \
   Montage/data \
   Montage/Montage \
   Montage/util \
   Montage/grid \
   Montage/MontageLib \
   Montage/HiPS \
   Montage/ancillary \
   Montage/python \
   Montage/Python_ManyLinux \
   Montage/Windows \
   Montage/web \
   Montage/bin

gzip -f Montage_`date "+%d%b%y"`.tar
