#include <stdio.h>

#include "fitsio.h"

#define SIDE 32
#define PIXELS (SIDE * SIDE)

static void write_wcs(fitsfile *file, int *status)
{
    double ra = 10.0;
    double dec = 20.05;
    double crpix = (SIDE + 1) / 2.0;
    double cdelt1 = -0.01;
    double cdelt2 = 0.01;
    double rotation = 0.0;
    double equinox = 2000.0;

    fits_update_key(file, TSTRING, "CTYPE1", "RA---TAN", NULL, status);
    fits_update_key(file, TSTRING, "CTYPE2", "DEC--TAN", NULL, status);
    fits_update_key(file, TDOUBLE, "CRVAL1", &ra, NULL, status);
    fits_update_key(file, TDOUBLE, "CRVAL2", &dec, NULL, status);
    fits_update_key(file, TDOUBLE, "CRPIX1", &crpix, NULL, status);
    fits_update_key(file, TDOUBLE, "CRPIX2", &crpix, NULL, status);
    fits_update_key(file, TDOUBLE, "CDELT1", &cdelt1, NULL, status);
    fits_update_key(file, TDOUBLE, "CDELT2", &cdelt2, NULL, status);
    fits_update_key(file, TDOUBLE, "CROTA2", &rotation, NULL, status);
    fits_update_key(file, TDOUBLE, "EQUINOX", &equinox, NULL, status);
}

int main(int argc, char **argv)
{
    fitsfile *file = NULL;
    int status = 0;
    int close_status = 0;
    long axes[2] = {SIDE, SIDE};
    long tiles[2] = {16, 16};
    float floats[PIXELS];
    float float_null = -9999.0f;
    unsigned short unsigned_pixels[PIXELS];
    short scaled_pixels[PIXELS];
    short blank = -32768;
    unsigned short unsigned_null = 0;
    double scale = 0.5;
    double zero = 100.0;
    const int blanks[] = {5 * SIDE + 5, 12 * SIDE + 18, 26 * SIDE + 24};
    int x;
    int y;
    unsigned int i;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s output.fits\n", argv[0]);
        return 1;
    }

    for (y = 0; y < SIDE; ++y)
    {
        for (x = 0; x < SIDE; ++x)
        {
            int pixel = y * SIDE + x;
            floats[pixel] = 20.0f + x / 8.0f + y / 16.0f + x * y / 128.0f;
            unsigned_pixels[pixel] = (unsigned short)(1024 + 1536 * x + 32 * y);
            scaled_pixels[pixel] = (short)(20 * x - 12 * y - 128);
        }
    }
    for (i = 0; i < sizeof(blanks) / sizeof(blanks[0]); ++i)
    {
        floats[blanks[i]] = float_null;
        unsigned_pixels[blanks[i]] = unsigned_null;
        scaled_pixels[blanks[i]] = blank;
    }

    if (fits_create_file(&file, argv[1], &status))
    {
        fits_report_error(stderr, status);
        return 1;
    }
    fits_create_img(file, SHORT_IMG, 0, NULL, &status);

    /* An absolute quantization step also works for a smooth, noiseless image. */
    fits_set_compression_type(file, RICE_1, &status);
    fits_set_tile_dim(file, 2, tiles, &status);
    fits_set_quantize_method(file, SUBTRACTIVE_DITHER_1, &status);
    fits_set_quantize_level(file, -0.125f, &status);
    fits_set_dither_seed(file, 1234, &status);
    fits_create_img(file, FLOAT_IMG, 2, axes, &status);
    write_wcs(file, &status);
    /* CFITSIO encodes these undefined floating point pixels as NaNs. */
    fits_write_imgnull(file, TFLOAT, 1, PIXELS, floats, &float_null, &status);

    fits_create_img(file, USHORT_IMG, 2, axes, &status);
    write_wcs(file, &status);
    /* BLANK is the stored signed value, before the unsigned BZERO offset. */
    fits_update_key(file, TSHORT, "BLANK", &blank, NULL, &status);
    fits_write_imgnull(file, TUSHORT, 1, PIXELS, unsigned_pixels,
                       &unsigned_null, &status);

    fits_set_compression_type(file, 0, &status);
    fits_create_img(file, SHORT_IMG, 2, axes, &status);
    write_wcs(file, &status);
    fits_update_key(file, TDOUBLE, "BSCALE", &scale, NULL, &status);
    fits_update_key(file, TDOUBLE, "BZERO", &zero, NULL, &status);
    fits_update_key(file, TSHORT, "BLANK", &blank, NULL, &status);
    /* Write raw integers without applying the header scaling a second time. */
    fits_set_bscale(file, 1.0, 0.0, &status);
    fits_write_img(file, TSHORT, 1, PIXELS, scaled_pixels, &status);

    fits_close_file(file, &close_status);
    if (status == 0)
        status = close_status;
    if (status != 0)
    {
        fits_report_error(stderr, status);
        return 1;
    }
    return 0;
}
