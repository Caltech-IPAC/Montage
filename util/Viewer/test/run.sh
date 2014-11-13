#!/bin/sh

../mViewer  \
   -ct       1 \
   -csys     eq j2000 \
   -color    008080 \
   -grid     gal \
   -color    FF8080 \
   -symbol   30.0 compass \
   -mark      100p  100p \
   -mark      100p -100p \
   -mark     -100p -100p \
   -mark     -100p  100p \
   -gray     iras12.fits -2s max gaussian-log \
   -out      iras12_eq.jpg

cp iras12_eq.jpg /work/jcg_9021
