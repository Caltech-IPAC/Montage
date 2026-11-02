# Mosaic the two overlapping images with mAdd's default mean coadd.
#
# The mosaic template has the inputs' pixel scale but is offset by
# (0.3, 0.4) pixels.  Ideally, where the two images overlap, the mosaic
# would be the projected sky plus 2.5: the mean of tan32.fits (sky) and
# tan32b.fits (sky plus 5).
#
# Current behavior, recorded as is: mAdd places tan32b.fits one row too
# low.  montageAdd.c computes an image's first output row by truncating
# output.crpix2 - incrpix2 + 1 to an int (columns are rounded), and here
# 16.9 - 10.9 + 1 is 6.9999999999999982, which truncates to 6, not 7.
#
# The offset is deliberate.  When the template's pixel edges coincide
# exactly with the input's, mProject moves a scattering of pixels into
# their neighbors, chosen by floating-point rounding, which would make
# this test fail for reasons unrelated to mAdd.
#
# The image table is built from an explicit list (mImgtbl -t) so its
# row order does not depend on the filesystem.

mkdir proj
mProject "$FIXTURES/tan32.fits"  proj/a.fits "$FIXTURES/mosaic.hdr"
mProject "$FIXTURES/tan32b.fits" proj/b.fits "$FIXTURES/mosaic.hdr"

cat > list.tbl <<'EOF'
|fname     |
 a.fits
 b.fits
EOF

mImgtbl -t list.tbl proj proj.tbl
mAdd -p proj proj.tbl "$FIXTURES/mosaic.hdr" mosaic.fits
fitsdump mosaic.fits
fitsdump mosaic_area.fits
