#!/bin/sh

mViewer \
        -color  yellow  \
        -grid   Equatorial J2000 \
        -symbol 13p triangle  \
        -mark   214.5666 25.2 \
        -gray   issa.fits 0s max gaussian-log \
        -out    issa.png
