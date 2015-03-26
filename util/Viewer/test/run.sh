#!/bin/sh

../mViewer  \
   -ct       1 \
   -csys     eq j2000 \
   -color    808080 \
   -grid     equ J2000 \
   -color    FF8080 \
   -symbol   25p compass \
   -mark       50p  100p \
   -symbol   50p compass \
   -mark       50p -100p \
   -symbol   75p compass \
   -mark      -50p -100p \
   -symbol   100p compass \
   -mark      -50p  100p \
   -gray     iras12.fits -2s max gaussian-log \
   -png      iras12_eq.png

../mViewer  \
   -ct       1 \
   -csys     eq j2000 \
   -color    808080 \
   -grid     equ J2000 \
   -color    FF8080 \
   -symbol   25p compass \
   -mark       50p  100p \
   -symbol   50p compass \
   -mark       50p -100p \
   -symbol   75p compass \
   -mark      -50p -100p \
   -symbol   100p compass \
   -mark      -50p  100p \
   -gray     iras12small.fits -2s max gaussian-log \
   -png      iras12small_eq.png

cp iras12_eq.png /work/jcg_9021
cp iras12small_eq.png /work/jcg_9021
