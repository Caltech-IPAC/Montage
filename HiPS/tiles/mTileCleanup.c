/* Module: mTileCleanup.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        20May21  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fitsio.h"
#include "wcs.h"
#include "coord.h"

#define MAXHIPS 256


void mHiPSTiles_printFitsError(int status);

int  mHiPSTiles_HiPSID(int pixlev, double x, double y);

void mHiPSTiles_splitIndex(long index, int level, int *x, int *y);

void mHiPSTiles_HiPSHdr(int level, int tile, fitsfile *fitshdr);


struct WorldCoor *wcs;

int debug;


//  base tile:   0  1  2  3  4  5  6  7  8  9 10 11 12->6

int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3, 4};
int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2, 4};


/*************************************************************************/
/*                                                                       */
/*  mTileCleanup                                                         */
/*                                                                       */
/*  Processing, at least on our SLURM cluster, occasionally and for no   */
/*  detectable reason drops individual tiles.  So given a list of such   */
/*  tiles, we need to re-extract them from the original plates.          */
/*                                                                       */
/*  If it turns out this functionality is generally necessary, we will   */
/*  generalize it but for now we are going to assume our standard plate  */
/*  layout, etc.                                                         */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int order, level, index, x, y, nside, tile, baseTile, allsky;

   debug = 1;

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mTileCleanup order tile\"]\n");
      exit(1);
   }

   order = atoi(argv[1]);
   tile  = atoi(argv[2]);


   /***************/
   /* Tile offset */
   /***************/

   nside = pow(2., (double)order);

   allsky = 5 * pow(2., (double)(order + 9));

   baseTile = (int)tile/(nside*nside);

   index = tile - baseTile * nside * nside;

   if(debug)
   {
      printf("\nDEBUG> order    = %d\n", order);
      printf("DEBUG> tile     = %d\n", tile);
      printf("DEBUG> nside    = %d\n", nside);
      printf("DEBUG> allsky   = %d\n", allsky);
      printf("DEBUG> baseTile = %d\n", baseTile);
      printf("DEBUG> index    = %d\n", index);
      fflush(stdout);
   }

   mHiPSTiles_splitIndex(index, order, &x, &y);

   if(debug)
   {
      printf("\nDEBUG> Tile in face: %d -> %d %d\n", index, x, y);
      fflush(stdout);
   }

   x = x + xoffset[baseTile] * nside;
   y = y + yoffset[baseTile] * nside;

   if(debug)
   {
      printf("\nDEBUG> Tile absolute: %d %d\n", x, y);
      fflush(stdout);
   }

   x = x * 512;
   y = y * 512;

   if(debug)
   {
      printf("\nDEBUG> Pixel absolute: %d %d (%.1f%% %.1f%%)\n",
            x, y, (double)x/allsky*100., (double)y/allsky*100.);
      fflush(stdout);
   }


   // x = nside - 1 - x; XXXXXXXXXXXXXXXXXXXXXXXXXX
   //
   
   /* Coordinate range in absolute pixels */


   exit(0);  
}


/***************************************************/
/*                                                 */
/* mHiPSTiles_splitIndex()                         */
/*                                                 */
/* Cell indices are Z-order binary constructs.     */
/* The x and y pixel offsets are constructed by    */
/* extracting the pattern made by every other bit  */
/* to make new binary numbers.                     */
/*                                                 */
/***************************************************/

void mHiPSTiles_splitIndex(long index, int level, int *x, int *y)
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
