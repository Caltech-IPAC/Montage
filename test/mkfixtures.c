/* mkfixtures: write the input images used by the golden tests.
 *
 * Usage: mkfixtures outdir
 *
 * Run this rarely (make fixtures) and commit what it writes, so the test
 * inputs never change when Montage code changes.  Pixel values come from
 * a fixed formula; there are no random numbers.
 *
 * Both images are 32x32 doubles in the same gnomonic (TAN) projection:
 * tangent point RA 150, Dec 2, 3.6 arcsec pixels.  They differ only in
 * CRPIX, so pixel (x,y) of tan32b.fits falls exactly on pixel
 * (x+10, y+6) of tan32.fits, and both sample the same sky.
 *
 * tan32.fits   The sky, with one NaN pixel at (6,6).
 * tan32b.fits  Shifted by (10,6) pixels; the sky plus 5, to mimic a
 *              background difference between the two images.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fitsio.h>

#define NX 32
#define NY 32

/* The sky: a linear gradient plus a Gaussian bump, in tan32.fits pixel
   coordinates counted from 0 */

static double sky(double x, double y)
{
   double dx = x - 12.0;
   double dy = y - 20.0;

   return 100.0 + 2.0*x + 3.0*y + 50.0*exp(-(dx*dx + dy*dy) / 18.0);
}


static int write_image(const char *outdir, const char *name,
                       int xshift, int yshift, double offset, int nan_pixel)
{
   fitsfile *fptr;
   int       status = 0;
   long      naxes[2] = {NX, NY};
   long      fpixel[2] = {1, 1};
   double    data[NY][NX];
   char      path[1024];
   int       x, y;

   for(y=0; y<NY; ++y)
      for(x=0; x<NX; ++x)
         data[y][x] = sky(x + xshift, y + yshift) + offset;

   if(nan_pixel)
      data[5][5] = NAN;

   /* A leading "!" tells cfitsio to overwrite an existing file */

   snprintf(path, sizeof(path), "!%s/%s", outdir, name);

   fits_create_file(&fptr, path, &status);
   fits_create_img (fptr, DOUBLE_IMG, 2, naxes, &status);

   fits_write_key_str(fptr, "CTYPE1",  "RA---TAN", NULL, &status);
   fits_write_key_str(fptr, "CTYPE2",  "DEC--TAN", NULL, &status);
   fits_write_key_dbl(fptr, "CRVAL1",  150.0,  -14, NULL, &status);
   fits_write_key_dbl(fptr, "CRVAL2",    2.0,  -14, NULL, &status);
   fits_write_key_dbl(fptr, "CRPIX1",   16.5 - xshift, -14, NULL, &status);
   fits_write_key_dbl(fptr, "CRPIX2",   16.5 - yshift, -14, NULL, &status);
   fits_write_key_dbl(fptr, "CDELT1",   -0.001, -14, NULL, &status);
   fits_write_key_dbl(fptr, "CDELT2",    0.001, -14, NULL, &status);
   fits_write_key_dbl(fptr, "EQUINOX", 2000.0,  -14, NULL, &status);

   fits_write_pix(fptr, TDOUBLE, fpixel, NX*NY, data, &status);

   fits_close_file(fptr, &status);

   if(status)
   {
      fits_report_error(stderr, status);
      return 1;
   }

   return 0;
}


int main(int argc, char **argv)
{
   if(argc != 2)
   {
      fprintf(stderr, "Usage: mkfixtures outdir\n");
      return 1;
   }

   if(write_image(argv[1], "tan32.fits",   0, 0, 0.0, 1))
      return 1;

   if(write_image(argv[1], "tan32b.fits", 10, 6, 5.0, 0))
      return 1;

   return 0;
}
