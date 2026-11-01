/* mkfixtures: write the input images used by the golden tests.
 *
 * Usage: mkfixtures outdir
 *
 * Run this rarely (make fixtures) and commit what it writes, so the test
 * inputs never change when Montage code changes.  Pixel values come from
 * a fixed formula; there are no random numbers.
 *
 * tan32.fits  32x32 double image, gnomonic (TAN) projection at
 *             RA 150, Dec 2, 3.6 arcsec pixels.  A linear gradient plus
 *             a Gaussian bump, with one NaN pixel at (6,6).
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fitsio.h>

#define NX 32
#define NY 32

static int write_tan32(const char *outdir)
{
   fitsfile *fptr;
   int       status = 0;
   long      naxes[2] = {NX, NY};
   long      fpixel[2] = {1, 1};
   double    data[NY][NX];
   double    dx, dy;
   char      path[1024];
   int       x, y;

   for(y=0; y<NY; ++y)
   {
      for(x=0; x<NX; ++x)
      {
         dx = x - 12.0;
         dy = y - 20.0;

         data[y][x] = 100.0 + 2.0*x + 3.0*y + 50.0*exp(-(dx*dx + dy*dy) / 18.0);
      }
   }

   data[5][5] = NAN;

   /* A leading "!" tells cfitsio to overwrite an existing file */

   snprintf(path, sizeof(path), "!%s/tan32.fits", outdir);

   fits_create_file(&fptr, path, &status);
   fits_create_img (fptr, DOUBLE_IMG, 2, naxes, &status);

   fits_write_key_str(fptr, "CTYPE1",  "RA---TAN", NULL, &status);
   fits_write_key_str(fptr, "CTYPE2",  "DEC--TAN", NULL, &status);
   fits_write_key_dbl(fptr, "CRVAL1",  150.0,  -14, NULL, &status);
   fits_write_key_dbl(fptr, "CRVAL2",    2.0,  -14, NULL, &status);
   fits_write_key_dbl(fptr, "CRPIX1",   16.5,  -14, NULL, &status);
   fits_write_key_dbl(fptr, "CRPIX2",   16.5,  -14, NULL, &status);
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

   return write_tan32(argv[1]);
}
