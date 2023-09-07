/* Module: mHPXtile2xy.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        24Jun21  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define MAXHIPS 256


void mHPXtile2xy_transform (int level, int tile, int *x, int *y);
void mHPXtile2xy_splitIndex(long index, int level, int *x, int *y);

int debug;


//  base tile:   0  1  2  3  4  5  6  7  8  9 10 11 12->6

int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3, 4};
int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2, 4};


/*************************************************************************/
/*                                                                       */
/*  mHPXtile2xy                                                          */
/*                                                                       */
/*  This program converts a tile ID for a given HPX level to tile X,Y    */
/*  coordinates.                                                         */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int i, tileID, tileLevel, xtile, ytile;

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXtile2xy [-d] level tileID\"]\n");
      exit(1);
   }

   for(i=0; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
         debug = 1;
   }

   if(debug)
   {
      --argc;
      ++argv;
   }
      
   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXtile2xy [-d] level tileID\"]\n");
      exit(1);
   }

   tileLevel = atoi(argv[1]);
   tileID    = atoi(argv[2]);

   mHPXtile2xy_transform(tileLevel, tileID, &xtile, &ytile);

   printf("[struct stat=\"OK\", level=%d, tile=%d, x=%d, y=%d]\n",
      tileLevel, tileID, xtile, ytile);
   fflush(stdout);

   exit(0);
}



/*************************************************************************/
/*                                                                       */
/*  mHPXtile2xy_transform                                                */
/*                                                                       */
/*  Given a HiPS tile level and tile ID within that level, determine     */
/*  the x, y HPX location for the tile.                                  */
/*                                                                       */
/*************************************************************************/

void mHPXtile2xy_transform(int level, int tile, int *xtile, int *ytile)
{
   int  baseTile, nside;
   long index;
   int  i, x, y;


   // Command-line arguments

   if(debug)
   {
      printf("\nDEBUG> tile level   = %d\n", level);
      printf("DEBUG> tile         = %d (input)\n", tile);
      fflush(stdout);
   }


   // Size of a base tile, in both output tile units
   // and output pixel units

   nside = pow(2., (double)level);

   if(debug)
   {
      printf("\nDEBUG> nside(tile)  = %d\n", nside);
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
      printf("\nDEBUG> Base tile    = %d (offset %d %d)\n", 
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

   mHPXtile2xy_splitIndex(index, level, &x, &y);

   x = nside - 1 - x;

   if(debug)
   {
      printf("\nDEBUG> Relative tile: X = %7d, Y = %7d (of %d) [X was originally %d]\n", x, y, nside, nside - 1 - x);
      fflush(stdout);
   }


   // Now add in the base tile offsets

   x = x + xoffset[baseTile]*nside;
   y = y + yoffset[baseTile]*nside;

   if(debug)
   {
      printf("DEBUG> Absolute:      X = %7d, Y = %7d (level %d)\n", x, y, level);
      fflush(stdout);
   }

   *xtile = x;
   *ytile = y;

   return;
}


/***************************************************/
/*                                                 */
/* mHPXtile2xy_splitIndex()                        */
/*                                                 */
/* Cell indices are Z-order binary constructs.     */
/* The x and y pixel offsets are constructed by    */
/* extracting the pattern made by every other bit  */
/* to make new binary numbers.                     */
/*                                                 */
/***************************************************/

void mHPXtile2xy_splitIndex(long index, int level, int *x, int *y)
{
   int  i;
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
