#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


//  base tile:   0  1  2  3  4  5  6  7  8  9 10 11 

int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3};
int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2};


/*************************************************************************/
/*                                                                       */
/*  mHiPSID                                                              */
/*                                                                       */
/*  Given HPX projection coordinates (and level), determine the tile     */
/*  ID, possibly at a different level.                                   */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   long long pixlev, outlevel, baseTile;
   long long nside, nsideout, index, id;
   long long xi, yi, ibase, jbase, base, fullscale;

   double x, y;
   double xoriginal, yoriginal;
   double xreverse,  yreverse;
   double xabsolute, yabsolute;
   double xintile,   yintile;

   int i, ix, iy;

   int debug = 0;


   // Command-line arguments

   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSID [-d] level x y [outlevel]\"]\n");
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
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSID [-d] level x y [outlevel]\"]\n");
      fflush(stdout);
      exit(0);
   }

   pixlev = atoll(argv[1]);

   xoriginal = atof(argv[2]);
   yoriginal = atof(argv[3]);

   outlevel = pixlev;

   if(argc > 4)
      outlevel = atoll(argv[4]);


   // Size of a base tile at the input level

   nside    = (long long) pow(2., (double)pixlev);
   nsideout = (long long) pow(2., (double)outlevel);

   if(debug)
   {
      printf("\nDEBUG> nside     = %lld\n", nside);
      printf("DEBUG> nsideout  = %lld\n\n", nsideout);
      fflush(stdout);
   }

   if(debug)
   {
      printf("DEBUG> Original XY:   X = %9.1f, Y = %9.1f (level %ld)\n", xoriginal, yoriginal, pixlev);
      fflush(stdout);
   }


   // The X and Y values we have (like the CRPIX for a plate) are
   // relative to the projection center.  What we need are absolute
   // coordinates in a full scale sense.  The projection center is
   // in the middle of the fullscale image.

   // When we use the program, if we are trying to find the center
   // of a tile, for instance, we would give an X value equal to 
   // the tile CRPIX1 plus half the tile size (i.e., 512/2)

   fullscale = nside * 5;

   if(debug)
   {
      printf("\nDEBUG> Fullscale:   %lld\n\n", fullscale);
      fflush(stdout);
   }

   xabsolute = xoriginal + fullscale / 2.;
   yabsolute = yoriginal + fullscale / 2.;

   if(debug)
   {
      printf("DEBUG> Absolute:      X = %9.1f, Y = %9.1f (level %ld)\n", xabsolute, yabsolute, pixlev); 
      fflush(stdout);
   }

   if(xabsolute < 0 || xabsolute > fullscale
   || yabsolute < 0 || yabsolute > fullscale)
   {
      printf("[struct stat=\"ERROR\", msg=\"Location invalid.\"]\n");
      fflush(stdout);
      exit(0);
   }

   xreverse = fullscale - 1. - xabsolute;
   yreverse = yabsolute;

   if(debug)
   {
      printf("DEBUG> Reverse:       X = %9.1f, Y = %9.1f (level %ld)\n", xreverse, yreverse, pixlev); 
      fflush(stdout);
   }



   // Next we need to determine the base tile.
   // We start by dividing the coordinates by the face size.
   // Then we can just look it up using the two offset
   // lists. 

   ibase = xreverse / nside;
   jbase = yreverse / nside;

   if(debug)
   {
      printf("\nDEBUG> ibase     = %lld\n", ibase);
      printf("DEBUG> jbase     = %lld\n", jbase);
      fflush(stdout);
   }

   baseTile = -1;

   for(i=0; i<11; ++i)
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
      printf("DEBUG> Base tile = %d\n\n", baseTile);
      fflush(stdout);
   }

   if(baseTile < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Location invalid.\"]\n");
      fflush(stdout);
      exit(0);
   }
 

   // We need the index offset within the base tile

   xintile = xreverse - ibase * nside;
   yintile = yreverse - jbase * nside;

   if(debug)
   {
      printf("DEBUG> Relative tile: X = %9.5f, Y = %9.5f (level %ld)\n", xintile, yintile, pixlev); 
      fflush(stdout);
   }


   // But make that in output tile units

   xintile = xintile * nsideout / nside;
   yintile = yintile * nsideout / nside;

   if(debug)
   {
      printf("DEBUG> Relative tile: X = %9.5f, Y = %9.5f (level %ld)\n", xintile, yintile, outlevel); 
      fflush(stdout);
   }


   // The starting point for the index is the 
   // lowest index for the base tile

   index = baseTile * nsideout * nsideout;

   if(debug)
   {
      printf("\nDEBUG> Tile starting index = %lld (level %ld)\n\n", index, outlevel);
      fflush(stdout);
   }


   // The Z-ordering scheme used to arrange the 
   // smaller tiles inside the base tile relates the
   // coordinates inside the tile to the index offset
   // by and interleaving of bits.

   // X is measured from the right, so we need to
   // complement it.

   xintile = nsideout - xintile;

   if(debug)
   {
      printf("DEBUG> Relative tile: X = %9.5f, Y = %9.5f (of %d)\n", xintile, yintile, nsideout);
      fflush(stdout);
   }

   id = 0;

   xi = xintile;
   yi = yintile;

   if(debug)
   {
      printf("DEBUG> Integer tile:  X = %9d, Y = %9d (of %d)\n", xi, yi, nsideout);
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
      printf("\nDEBUG> Tile index   = %ld (i.e., index inside base tile)\n", id);
      printf("DEBUG> Tile index   = %o (octal)\n\n", id);
      fflush(stdout);
   }

   index += id;
      
   printf("[struct stat=\"OK\", module=\"mHiPSID\", pixOrder=%lld, x=%-g, y=%-g, xabs=%-g, yabs=%-g, baseTile=%lld, id=%lld]\n",
      pixlev, xoriginal, yoriginal, xabsolute, yabsolute, baseTile, index);
   fflush(stdout);
} 
