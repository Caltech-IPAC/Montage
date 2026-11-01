# FITS input fixture

`fits_inputs.fits` was made with `make_fits_inputs.c` and CFITSIO 3.49 bundled
with Montage main at `3d1f17f`. It is 23040 bytes and is committed rather than
generated during output comparison.

HDU 0 is an empty primary image with NAXIS 0.

HDU 1 is a 32 by 32 float image with a smooth pixel pattern and three NaNs.
It uses RICE_1 tile compression, SUBTRACTIVE_DITHER_1, dither seed 1234 and an
absolute quantization step of 0.125. The tiles are 16 by 16 pixels.

HDU 2 is a 32 by 32 unsigned 16 bit image with lossless RICE_1 compression,
BZERO 32768 and BSCALE 1. BLANK is the minimum signed 16 bit value and marks
three pixels. The tiles are 16 by 16 pixels.

HDU 3 is a 32 by 32 uncompressed BITPIX 16 image with BSCALE 0.5 and BZERO 100.
BLANK is the minimum signed 16 bit value and marks three pixels.

Each image has a TAN WCS centered at RA 10 and Dec 20.05 degrees with a pixel
scale of 0.01 degrees and equinox 2000. All pixel values use fixed formulas.
There are no DATE or CHECKSUM keywords.

Real archives deliver scaled integer data, BLANK values and tile compressed
images. This fixture exercises those read paths without a network request.

Run these commands from the repository root after `make` has built the bundled
CFITSIO. CFITSIO 4.7.0 produces the same 23040 bytes. The leading `!` replaces
the existing fixture.

```sh
cc -std=c99 -Wall -Wextra -Ilib/include \
    tests/data/make_fits_inputs.c lib/libcfitsio.a \
    -lm -o make_fits_inputs
./make_fits_inputs '!tests/data/fits_inputs.fits'
```

The SHA256 is:

```text
7b469ac56fc9e0599ff06be3733d3451e68e5ee849629377bf0e5b6f441b9efa  fits_inputs.fits
```
