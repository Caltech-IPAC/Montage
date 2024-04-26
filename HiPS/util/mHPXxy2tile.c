#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


//  base tile:   0  1  2  3  4  5  6  7  8  9 10 11 

int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3};
int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2};


/*************************************************************************/
/*                                                                       */
/*  mHPXxy2tile                                                          */
/*                                                                       */
/*  Given HPX projection coordinates (and level), determine the tile     */
/*  ID.  It probably has other uses but was written to help find         */
/*  neighbors to HiPS tiles.                                             */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   long long level, baseTile;
   long long nside, index, id;
   long long xi, yi, ibase, jbase, base, fullscale;

   double x, y;
   double xintile,  yintile;

   int i, ix, iy;

   int debug = 0;


   // Command-line arguments

   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXxy2tile [-d] level x y\"]\n");
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
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXxy2tile [-d] level x y\"]\n");
      fflush(stdout);
      exit(0);
   }

   level = atoll(argv[1]);

   x = atof(argv[2]);
   y = atof(argv[3]);


   // Size of a base tile at the input level

   nside = (long long) pow(2., (double)level);

   if(debug)
   {
      printf("\nDEBUG> nside     = %lld\n", nside);
      fflush(stdout);
   }

   if(debug)
   {
      printf("DEBUG> Original XY:   X = %9.1f, Y = %9.1f (level %lld)\n", x, y, level);
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

   if(x < 0 || x > fullscale
   || y < 0 || y > fullscale)
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
      printf("DEBUG> Relative tile: X = %9.5f, Y = %9.5f (level %lld)\n", xintile, yintile, level); 
      fflush(stdout);
   }


   // The starting point for the index is the 
   // lowest index for the base tile

   index = baseTile * nside * nside;

   if(debug)
   {
      printf("\nDEBUG> Tile starting index = %lld (level %lld)\n\n", index, level);
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
      printf("DEBUG> Relative tile: X = %9.5f, Y = %9.5f (of %lld)\n", xintile, yintile, nside);
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
      
   printf("[struct stat=\"OK\", module=\"mHPXxy2tile\", index=%lld, x=%-g, y=%-g, id=%lld]\n",
      level, x, y, index);
   fflush(stdout);
} 
