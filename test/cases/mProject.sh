# Reproject the 32x32 TAN image onto a template rotated by 30 degrees
# with slightly larger pixels.  Captures the status line, the output
# image, and the area (coverage) image.

mProject "$FIXTURES/tan32.fits" out.fits "$FIXTURES/rot30.hdr"
fitsdump out.fits
fitsdump out_area.fits
