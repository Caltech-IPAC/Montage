#!/bin/sh

mViewer -t 0 \
   -color   909090 \
   -grid    equ j2000 \
   -color   90ff90 \
   -imginfo mipsed.tbl \
   -color   ff9090 \
   -imginfo irspeakup.tbl \
   -color   white \
   -symbol  0.5 circle \
   -catalog fp_2mass.tbl j_m 16.0 mag \
   -blue    sdss_u2.fits -0.50s max gaussian-log \
   -green   sdss_g2.fits -0.20s max gaussian-log \
   -red     sdss_r2.fits -0.25s max gaussian-log \
   -jpeg    m51_color.jpg

cp m51_color.jpg /work/jcg_9021
