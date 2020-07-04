#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void splitIndex(long index, int level, int *x, int *y);


//  base tile:   0  1  2  3  4  5  6  7  8  9 10 11 

int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3};
int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2};


/*************************************************************************/
/*                                                                       */
/*  mHiPSHdr                                                             */
/*                                                                       */
/*  Given a HiPS tile level and tile ID within that level, create a      */
/*  proper HPX FITS header, to be used for cutouts or mosaicking.        */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   long level, tile, pixlev, baseTile;
   long nside, nsidePix, fullscale, index;
   int  i, x, y;
   int  havepad, pad;

   double cdelt, cos45;

   char hdrfile[1024];

   FILE *fhdr;


   int debug = 0;


   // Command-line arguments

   pad     = 0;
   havepad = 0;

   for(i=1; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
            debug = 1;

      else if(strcmp(argv[i], "-p") == 0)
      {
         if(argc < i)
         {
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSHdr [-d][-p pad] tilelev tileid hdrfile\"]\n");
            fflush(stdout);
            exit(0);
         }

         pad = atoi(argv[i+1]);

         havepad = 1;
      }
   }


   if(debug)
   {
      ++argv;
      --argc;
   }

   if(havepad)
   {
      argv += 2;
      argc -= 2;
   }


   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSHdr [-d][-p pad] tilelev tileid hdrfile\"]\n");
      fflush(stdout);
      exit(0);
   }

   level  = atol(argv[1]);
   tile   = atol(argv[2]);

   strcpy(hdrfile, argv[3]);

   if(debug)
   {
      printf("\nDEBUG> level        = %ld\n", level);
      printf("DEBUG> tile         = %ld\n", tile);
      printf("DEBUG> hdrfile      = [%s]\n", hdrfile);
      fflush(stdout);
   }

   fhdr = fopen(hdrfile, "w+");

   pixlev = level + 9;


   // Size of a base tile, in both output tile units
   // and output pixel units

   nside    = pow(2., (double)level);
   nsidePix = pow(2., (double)pixlev);

   if(debug)
   {
      printf("\nDEBUG> nside(tile)  = %ld\n", nside);
      printf("DEBUG> nside(pixel) = %ld\n", nsidePix);
      fflush(stdout);
   }


   // The simplest way to identify the base tile
   // or output tile is in is by seeing how many
   // nside*nside sets it contains.  Since all our
   // numbering starts at zero, this simple division
   // will show us what base tile we are in.

   baseTile = (int)tile/(nside*nside);

   if(debug)
   {
      printf("\nDEBUG> Base tile    = %ld (offset %d %d)\n", 
         baseTile, xoffset[baseTile], yoffset[baseTile]);
      fflush(stdout);
   }


   // Subtracting the integer number of preceding
   // base tiles shows us the tile index we are at
   // inside the current base tile.

   index = tile - baseTile * nside * nside;

   if(debug)
   {
      printf("DEBUG> Tile index   = %ld (i.e., index inside base tile)\n", index);
      fflush(stdout);
   }


   // The Z-ordering scheme used to arrange the 
   // smaller tiles inside the base tile provides
   // a way to turn a relative index into X and Y
   // tile offsets.  We have a function that can
   // do this.

   // X is measured from the right, so we need to
   // complement it.

   splitIndex(index, level, &x, &y);

   x = nside - 1 - x;

   if(debug)
   {
      printf("\nDEBUG> Relative tile: X = %7d, Y = %7d (of %ld) [X was originally %ld]\n", x, y, nside, nside - 1 - x);
      fflush(stdout);
   }


   // Now add in the base tile offsets

   x = x + xoffset[baseTile]*nside;
   y = y + yoffset[baseTile]*nside;

   if(debug)
   {
      printf("DEBUG> Absolute:      X = %7d, Y = %7d (level %ld)\n", x, y, level);
      fflush(stdout);
   }


   // And scale to pixels

   x = x * nsidePix/nside;
   y = y * nsidePix/nside; 

   if(debug)
   {
      printf("DEBUG> Pixels:        X = %7d, Y = %7d (level %ld)\n", x, y, pixlev);
      fflush(stdout);
   }


   // The whole region is a 5x5 grid of nsidePix boxes, so

   fullscale = 5 * nsidePix;


   if(debug)
   {
      printf("\nDEBUG> Full scale       = %ld pixels\n", fullscale);
      printf("\nDEBUG> Scaled coords  X = %.2f%%, Y = %.2f%%\n", 100.*x/fullscale, 100.*y/fullscale);
      printf("\nDEBUG> CRPIX            = %.1f, Y = %.1f\n\n", (fullscale + 1.)/2. - x, (fullscale + 1.)/2. - y);
      fflush(stdout);
   }

   cos45 = sqrt(2.0)/2.0;

   cdelt = -90.0 / nsidePix / sqrt(2.0);

   fprintf(fhdr, "SIMPLE  = T\n");
   fprintf(fhdr, "BITPIX  = -64\n");
   fprintf(fhdr, "NAXIS   = 2\n");
   fprintf(fhdr, "NAXIS1  = %d\n", 512+2*pad);
   fprintf(fhdr, "NAXIS2  = %d\n", 512+2*pad);
   fprintf(fhdr, "CRPIX1  = %.1f\n", (fullscale + 1.)/2. - x + pad);
   fprintf(fhdr, "CRPIX2  = %.1f\n", (fullscale + 1.)/2. - y + pad);
   fprintf(fhdr, "PC1_1   = %.8f\n",  cos45);
   fprintf(fhdr, "PC1_2   = %.8f\n",  cos45);
   fprintf(fhdr, "PC2_1   = %.8f\n", -cos45);
   fprintf(fhdr, "PC2_2   = %.8f\n",  cos45);
   fprintf(fhdr, "CDELT1  = %.8f\n",  cdelt);
   fprintf(fhdr, "CDELT2  = %.8f\n", -cdelt);
   fprintf(fhdr, "CTYPE1  = 'GLON-HPX'\n");
   fprintf(fhdr, "CTYPE2  = 'GLAT-HPX'\n");
   fprintf(fhdr, "CRVAL1  = 0.0\n");
   fprintf(fhdr, "CRVAL2  = 0.0\n");
   fprintf(fhdr, "LONPOLE = 180.0\n");
   fprintf(fhdr, "PV2_1   = 4\n");
   fprintf(fhdr, "PV2_2   = 3\n");
   fprintf(fhdr, "END\n");

   fclose(fhdr);

   printf("[struct stat=\"OK\", module=\"mHiPSHdr\" naxis1=%d, naxis2=%d, crpix1=%.1f, crpix2=%.1f, fullscale=%ld, pixscale=%.18f]\n",
      512+2*pad, 512+2*pad, (fullscale + 1.)/2. - x + pad, (fullscale + 1.)/2. - y + pad, fullscale, -cdelt);
   fflush(stdout);
} 



/***************************************************/
/*                                                 */
/* splitIndex()                                    */
/*                                                 */
/* Cell indices are Z-order binary constructs.     */
/* The x and y pixel offsets are constructed by    */
/* extracting the pattern made by every other bit  */
/* to make new binary numbers.                     */
/*                                                 */
/***************************************************/

void splitIndex(long index, int level, int *x, int *y)
{
   int i;
   long val;

   val = index;

   *x = 0;
   *y = 0;

   for(i=0; i<level; ++i)
   {
      *x = *x + (((val >> (2*i))   & 1) << i);
      *y = *y + (((val >> (2*i+1)) & 1) << i);
   }

   return;
}
