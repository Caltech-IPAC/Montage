#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <wcs.h>
#include <coord.h>


//  base tile:   0  1  2  3  4  5  6  7  8  9 10 11 

int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3};
int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2};

int stradd(char *header, char *card);


/*************************************************************************/
/*                                                                       */
/*  mHPXCoord2Tile                                                       */
/*                                                                       */
/*  Given HPX projection coordinates (and order), determine the tile     */
/*  ID.  It probably has other uses but was written to help find         */
/*  neighbors to HiPS tiles.                                             */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   long long order, baseTile;
   long long nside, index, id;
   long long xi, yi, ibase, jbase, base, fullscale;

   double ra, dec;
   double glon, glat, dtr;
   double x, y;
   double xintile,  yintile;
   double cos45, cdelt;

   int i, ix, iy, offscl, naxis;

   char temp     [80];
   char im_header[3200];

   static struct WorldCoor *wcs;

   dtr = atan(1.)/45.;


   int debug = 0;


   // Command-line arguments

   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXCoord2Tile [-d] order ra dec\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(strcmp(argv[1], "-d") == 0)
   {
      debug = 1;
      ++argv;
      --argc;
   }

   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXCoord2Tile [-d] order ra dec\"]\n");
      fflush(stdout);
      exit(0);
   }

   order = atoll(argv[1]);

   ra  = atof(argv[2]);
   dec = atof(argv[3]);


   // Size of a base tile at the input order

   nside = (long long) pow(2., (double)order);

   if(debug)
   {
      printf("\nDEBUG> nside     = %lld\n", nside);
      fflush(stdout);
   }


   // Use the order to generate an HPX header
   
   strcpy(im_header, "");

   naxis = 5 * nside * 512;

   cos45 = sqrt(2.0)/2.0;
   
   cdelt = -90.0 / nside / 512. / sqrt(2.0);

   if(debug)
   {
      printf("\nHeader:\n\n");
     
      printf("   SIMPLE  = T\n"                         ); 
      printf("   BITPIX  = -64\n"                       ); 
      printf("   NAXIS   = 2\n"                         ); 
      printf("   NAXIS1  = %d\n",         naxis         ); 
      printf("   NAXIS2  = %d\n",         naxis         ); 
      printf("   CRPIX1  = %.1f\n",      (naxis + 1.)/2.); 
      printf("   CRPIX2  = %.1f\n",      (naxis + 1.)/2.); 
      printf("   PC1_1   = %.8f\n",       cos45         ); 
      printf("   PC1_2   = %.8f\n",       cos45         ); 
      printf("   PC2_1   = %.8f\n",      -cos45         ); 
      printf("   PC2_2   = %.8f\n",       cos45         ); 
      printf("   CDELT1  = %.8f\n",       cdelt         ); 
      printf("   CDELT2  = %.8f\n",      -cdelt         ); 
      printf("   CTYPE1  = 'GLON-HPX'\n"                ); 
      printf("   CTYPE2  = 'GLAT-HPX'\n"                ); 
      printf("   CRVAL1  = 0.0\n"                       ); 
      printf("   CRVAL2  = 0.0\n"                       ); 
      printf("   LONPOLE = 180.0\n"                     ); 
      printf("   PV2_1   = 4\n"                         ); 
      printf("   PV2_2   = 3\n"                         ); 
      printf("   END\n\n"                               ); 
   }


   sprintf(temp, "SIMPLE  = T"                         ); stradd(im_header, temp);
   sprintf(temp, "BITPIX  = -64"                       ); stradd(im_header, temp);
   sprintf(temp, "NAXIS   = 2"                         ); stradd(im_header, temp);
   sprintf(temp, "NAXIS1  = %d",         naxis         ); stradd(im_header, temp);
   sprintf(temp, "NAXIS2  = %d",         naxis         ); stradd(im_header, temp);
   sprintf(temp, "CRPIX1  = %.1f",      (naxis + 1.)/2.); stradd(im_header, temp);
   sprintf(temp, "CRPIX2  = %.1f",      (naxis + 1.)/2.); stradd(im_header, temp);
   sprintf(temp, "PC1_1   = %.8f",       cos45         ); stradd(im_header, temp);
   sprintf(temp, "PC1_2   = %.8f",       cos45         ); stradd(im_header, temp);
   sprintf(temp, "PC2_1   = %.8f",      -cos45         ); stradd(im_header, temp);
   sprintf(temp, "PC2_2   = %.8f",       cos45         ); stradd(im_header, temp);
   sprintf(temp, "CDELT1  = %.8f",       cdelt         ); stradd(im_header, temp);
   sprintf(temp, "CDELT2  = %.8f",      -cdelt         ); stradd(im_header, temp);
   sprintf(temp, "CTYPE1  = 'GLON-HPX'"                ); stradd(im_header, temp);
   sprintf(temp, "CTYPE2  = 'GLAT-HPX'"                ); stradd(im_header, temp);
   sprintf(temp, "CRVAL1  = 0.0"                       ); stradd(im_header, temp);
   sprintf(temp, "CRVAL2  = 0.0"                       ); stradd(im_header, temp);
   sprintf(temp, "LONPOLE = 180.0"                     ); stradd(im_header, temp);
   sprintf(temp, "PV2_1   = 4"                         ); stradd(im_header, temp);
   sprintf(temp, "PV2_2   = 3"                         ); stradd(im_header, temp);
   sprintf(temp, "END"                                 ); stradd(im_header, temp);

   wcs = wcsinit(im_header);

   if(wcs == (struct WorldCoor *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"WCS initialization failed.\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(debug)
   {
      printf("DEBUG> wcs initialized\n");
      fflush(stdout);
   }


   // Find the pixel coordinates for ra, dec (Note: we are using Galactic HPX for all our maps)

   convertCoordinates(EQUJ, 2000.,  ra,    dec,
                      GAL,  2000., &glon, &glat);

   if(glon >  180.) glon -= 360.;
   if(glon < -180.) glon += 360.;

   wcs2pix(wcs, glon, glat, &x, &y, &offscl);

   if(debug)
   {
      printf("DEBUG> ra,dec: %-g %-g  ->  glon,glat: %-g %-g  ->  x,y: %-g %-g\n",
         ra, dec, glon, glat, x, y);
      fflush(stdout);
   }


   // x, y are at this point in pixel units; we need them in 'tile' units.

   x = (x-0.5) / 512.;
   y = (y-0.5) / 512.;

   x = (int)x;
   y = (int)y;
   

   if(debug)
   {
      printf("DEBUG> Original XY:   X = %9.1f, Y = %9.1f (order %lld)\n", x, y, order);
      fflush(stdout);
   }


   // The X and Y values we have here are relative to the bottom 
   // left in our all-sky HPX projection.

   fullscale = nside * 5;

   if(debug)
   {
      printf("\nDEBUG> Fullscale:   %lld\n\n", fullscale);
      fflush(stdout);
   }

   if(x < 0 || x >= fullscale
   || y < 0 || y >= fullscale)
   {
      printf("[struct stat=\"ERROR\", msg=\"Location invalid.\"]\n");
      fflush(stdout);
      exit(0);
   }


   // Next we need to determine the base tile.
   // We start by dividing the coordinates by the face size.
   // Then we can just look it up using the two offset
   // lists. 

   ibase = x / nside;
   jbase = y / nside;

   if(debug)
   {
      printf("\nDEBUG> ibase     = %lld\n", ibase);
      printf("DEBUG> jbase     = %lld\n", jbase);
      fflush(stdout);
   }

   if(ibase == 4 && jbase == 4)  // the unused thirteenth tile
      baseTile = 6;
   else
   {
      baseTile = -1;

      for(i=0; i<12; ++i)
      {
         if(ibase == xoffset[i])
         {
            if(jbase == yoffset[i])
            {
               baseTile = i;
               break;
            }
         }
      }
   }

   if(debug)
   {
      printf("DEBUG> Base tile = %lld\n\n", baseTile);
      fflush(stdout);
   }

   if(baseTile < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Location invalid.\"]\n");
      fflush(stdout);
      exit(0);
   }
 

   // We need the index offset within the base tile

   xintile = x - ibase * nside;
   yintile = y - jbase * nside;

   if(debug)
   {
      printf("DEBUG> Relative tile: X = %9.5f, Y = %9.5f (order %lld)\n", xintile, yintile, order); 
      fflush(stdout);
   }


   // The starting point for the index is the 
   // lowest index for the base tile

   index = baseTile * nside * nside;

   if(debug)
   {
      printf("\nDEBUG> Tile starting index = %lld (order %lld)\n\n", index, order);
      fflush(stdout);
   }


   // The Z-ordering scheme used to arrange the 
   // smaller tiles inside the base tile relates the
   // coordinates inside the tile to the index offset
   // by and interleaving of bits.

   // X is measured from the right, so we need to
   // complement it.

   xintile = nside - 1 - xintile;

   if(debug)
   {
      printf("DEBUG> Relative tile: X = %9.5f, Y = %9.5f (of %lld) [X reversed]\n", xintile, yintile, nside);
      fflush(stdout);
   }

   id = 0;

   xi = xintile;
   yi = yintile;

   if(debug)
   {
      printf("DEBUG> Integer tile:  X = %9lld, Y = %9lld (of %lld)\n", xi, yi, nside);
      fflush(stdout);
   }

   for(i=63; i>=0; --i)
   {
      iy = (yi >> i) & 1;
      ix = (xi >> i) & 1;

      id = 4*id + 2*iy + ix;
   }

   if(debug)
   {
      printf("\nDEBUG> Tile index   = %lld (i.e., index inside base tile)\n", id);
      printf("DEBUG> Tile index   = %llo (octal)\n\n", id);
      fflush(stdout);
   }

   index += id;
      
   printf("[struct stat=\"OK\", module=\"mHPXCoord2Tile\", index=%lld, x=%-g, y=%-g, id=%lld]\n",
      order, x, y, index);
   fflush(stdout);
} 


int stradd(char *header, char *card)
{
   int i, hlen, clen;

   hlen = strlen(header);
   clen = strlen(card);

   for(i=0; i<clen; ++i)
      header[hlen+i] = card[i];

   if(clen < 80)
      for(i=clen; i<80; ++i)
         header[hlen+i] = ' ';

   header[hlen+80] = '\0';

   return(strlen(header));
}
