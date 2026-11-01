# Build an image table from a directory holding one FITS image and two
# files mImgtbl should skip: a header template (not FITS) and an area
# image (skipped by default).
#
# Only one image on purpose: mImgtbl lists the directory with readdir()
# and does not sort, so with several images the row order would depend
# on the filesystem.

mkdir in
cp "$FIXTURES/tan32.fits" in/tan32.fits
cp "$FIXTURES/tan32.fits" in/tan32_area.fits
cp "$FIXTURES/rot30.hdr"  in/rot30.hdr

mImgtbl in images.tbl
cat images.tbl
