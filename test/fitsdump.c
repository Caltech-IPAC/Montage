/* fitsdump: print a FITS file as text so the golden tests can diff it.
 *
 * Usage: fitsdump file.fits
 *
 * For every HDU it prints the header cards, then, for image HDUs, one
 * line per pixel: "x y value" in 1-based FITS pixel coordinates (more
 * coordinates for cubes).  Values are printed with %.17g, which
 * round-trips a double exactly, so any numeric change shows up in the
 * diff.  Undefined pixels print as "NaN" regardless of their sign bit.
 * Table HDUs get their header only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fitsio.h>

#define MAXDIM 9

static void dump_header(fitsfile *fptr, int *status)
{
   int  nkeys, i, len;
   char card[FLEN_CARD];

   if(fits_get_hdrspace(fptr, &nkeys, NULL, status))
      return;

   for(i=1; i<=nkeys; ++i)
   {
      if(fits_read_record(fptr, i, card, status))
         return;

      len = strlen(card);

      while(len > 0 && card[len-1] == ' ')
         card[--len] = '\0';

      printf("%s\n", card);
   }

   printf("END\n");
}


static void dump_image(fitsfile *fptr, int *status)
{
   int     naxis, i, anynul;
   long    naxes[MAXDIM], fpixel[MAXDIM];
   double *row;
   double  nulval = NAN;

   if(fits_get_img_dim(fptr, &naxis, status) || naxis == 0)
      return;

   if(naxis > MAXDIM)
   {
      fprintf(stderr, "fitsdump: NAXIS = %d is more than %d\n", naxis, MAXDIM);
      *status = BAD_NAXIS;
      return;
   }

   if(fits_get_img_size(fptr, MAXDIM, naxes, status))
      return;

   for(i=0; i<naxis; ++i)
   {
      if(naxes[i] == 0)
         return;

      fpixel[i] = 1;
   }

   row = malloc(naxes[0] * sizeof(double));

   if(row == NULL)
   {
      fprintf(stderr, "fitsdump: out of memory\n");
      *status = MEMORY_ALLOCATION;
      return;
   }

   /* Read one row at a time; fpixel[1..] counts like an odometer */

   while(1)
   {
      if(fits_read_pix(fptr, TDOUBLE, fpixel, naxes[0], &nulval, row, &anynul, status))
         break;

      for(fpixel[0]=1; fpixel[0]<=naxes[0]; ++fpixel[0])
      {
         for(i=0; i<naxis; ++i)
            printf("%ld ", fpixel[i]);

         if(isnan(row[fpixel[0]-1]))
            printf("NaN\n");
         else
            printf("%.17g\n", row[fpixel[0]-1]);
      }

      fpixel[0] = 1;

      for(i=1; i<naxis; ++i)
      {
         if(++fpixel[i] <= naxes[i])
            break;

         fpixel[i] = 1;
      }

      if(i == naxis)
         break;
   }

   free(row);
}


int main(int argc, char **argv)
{
   fitsfile *fptr;
   int       status = 0, close_status = 0;
   int       nhdus, hdunum, hdutype;

   if(argc != 2)
   {
      fprintf(stderr, "Usage: fitsdump file.fits\n");
      return 1;
   }

   if(fits_open_file(&fptr, argv[1], READONLY, &status))
   {
      fits_report_error(stderr, status);
      return 2;
   }

   printf("== %s\n", argv[1]);

   fits_get_num_hdus(fptr, &nhdus, &status);

   for(hdunum=1; hdunum<=nhdus && !status; ++hdunum)
   {
      if(fits_movabs_hdu(fptr, hdunum, &hdutype, &status))
         break;

      printf("-- HDU %d\n", hdunum);

      dump_header(fptr, &status);

      if(!status && hdutype == IMAGE_HDU)
         dump_image(fptr, &status);
   }

   fits_close_file(fptr, &close_status);

   if(status || close_status)
   {
      fflush(stdout);
      fits_report_error(stderr, status ? status : close_status);
      return 2;
   }

   return 0;
}
