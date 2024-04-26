#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define STRLEN 1024

#define CW          0
#define CCW         1

#define NONE        0
#define NPOLEG1     1
#define NPOLEG2     2
#define NPOLEG3     3
#define NPOLEG4     4
#define NPOLEG5     5
#define NPOLEG6     6
#define NPOLEG7     7
#define NPOLEG8     8
#define SPOLEG1     9
#define SPOLEG2    10
#define SPOLEG3    11
#define SPOLEG4    12
#define SPOLEG5    13
#define SPOLEG6    14
#define SPOLEG7    15
#define SPOLEG8    16
#define WRAP1      17
#define WRAP2      18
#define WRAP3      19
#define WRAP4      20
#define NCPOLE1    21
#define NCPOLE2    22
#define NCPOLE3    23
#define NCPOLE4    24
#define SCPOLE1    25
#define SCPOLE2    26
#define SCPOLE3    27
#define SCPOLE4    28
#define WCORNER1   29
#define WCORNER2   30

const char * gapnames[] = {
   "NONE",
   "NPOLEG1",
   "NPOLEG2",
   "NPOLEG3",
   "NPOLEG4",
   "NPOLEG5",
   "NPOLEG6",
   "NPOLEG7",
   "NPOLEG8",
   "SPOLEG1",
   "SPOLEG2",
   "SPOLEG3",
   "SPOLEG4",
   "SPOLEG5",
   "SPOLEG6",
   "SPOLEG7",
   "SPOLEG8",
   "WRAP1",
   "WRAP2",
   "WRAP3",
   "WRAP4",
   "NCPOLE1",
   "NCPOLE2",
   "NCPOLE3",
   "NCPOLE4",
   "SCPOLE1",
   "SCPOLE2",
   "SCPOLE3",
   "SCPOLE4",
   "WCORNER1",
   "WCORNER2",
};

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

void pivot     (double x, double y, double xpivot, double ypivot, 
                double *xtrans, double *ytrans, int dir);
int  xy2id     (int order, int i, int j);
void xy2tile   (int order, int tile, int *xtile, int *ytile);
void splitIndex(long index, long level, int *x, int *y);

//  base tile:   0  1  2  3  4  5  6  7  8  9 10 11

int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3};
int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2};


/*************************************************************************/
/*                                                                       */
/*  mHPXHistCombine                                                      */
/*                                                                       */
/*  We have generated histograms for all the tiles of a particular       */
/*  order.  To implement a smoothly varying stretch, we need to          */
/*  average these in NxN block (usually 3x3) and use these as the        */
/*  histogram for the center tile of each set.                           */
/*                                                                       */
/*  Usually, this is just a matter of taking the i,j of the center       */
/*  tile and running, for instance, from i-1 to i+1 and j-1 to j+1.      */
/*  However, near the poles there are gaps in the projection where       */
/*  the tile we want is on the other side of the gap.  There are only    */
/*  eight of these gaps (four in the north and four in the south),       */
/*  so we can deal with them explicitly.                                 */
/*                                                                       */
/*  We also have function for converting between "tile number",          */
/*  which is how the files are named, and the (i,j) location of the      */
/*  tile in HPX projected space.                                         */
/*                                                                       */
/*  So we loop over the tiles in (i,j) space, determining the tile       */
/*  number and therefore name from the coordinates, then                 */
/*  the adjacent tiles converting their (i,j) back to tile number /      */
/*  name.  Whenever an (i,j) goes outside the space into a gap, we       */
/*  transform it across the gap to the correct (i,j) on the other        */
/*  side.                                                                */
/*                                                                       */
/*  Finally, we use the mCombineHist utility to generate the new,        */
/*  "smoothed" histogram for the center tile.                            */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int    c, ntile, order, width, offset, gap;
   int    debug, refid, id, itile, jtile;
   int    i, j, itrans, jtrans;
   double x, y, xtile, ytile, xpivot, ypivot;
   double xtrans, ytrans;

   char   roughdir [STRLEN];
   char   smoothdir[STRLEN];
   char   tmpstr   [STRLEN];
   char   outname  [STRLEN];
   char   tilename [STRLEN];
   char   tilelist [32768];
   char   cmd      [32768];
   char   cwd      [STRLEN];

   char  *end;

   getcwd(cwd, STRLEN);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug     = 0;
   width     = 3;
   order     = 2;

   opterr    = 0;

   strcpy(roughdir,  "");
   strcpy(smoothdir, "");

   while ((c = getopt(argc, argv, "do:w:")) != EOF) 
   {
      switch (c) 
      {
         case 'd':
            debug = 1;
            break;

         case '0':
            order = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -o (%s) cannot be interpreted as an integer\"]\n", 
                  optarg);
               exit(1);
            }

            if(order < 1)
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -o (%s) must be a positive integer (default: 2)\"]\n", 
                  optarg);
               exit(1);
            }

            break;

         case 'w':
            width = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) cannot be interpreted as an integer\"]\n", 
                  optarg);
               exit(1);
            }

            if(width < 3)
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) must be a positive odd integer (>= 3; default: 3)\"]\n", 
                  optarg);
               exit(1);
            }

            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXHistCombine [-d] [-o order] [-w width] roughdir smoothdir\"]\n");
            exit(1);
            break;
      }
   }

   if (argc - optind < 2) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXHistCombine [-d] [-o order] [-w width] roughdir smoothdir\"]\n");
      exit(1);
   }


   strcpy(roughdir,  argv[optind]);
   strcpy(smoothdir, argv[optind+1]);


   if(roughdir[0] != '/')
   {
      strcpy(tmpstr, cwd);

      if(strlen(roughdir) > 0)
      {
         strcat(tmpstr, "/");
         strcat(tmpstr, roughdir);
      }

      strcpy(roughdir, tmpstr);
   }

   if(roughdir[strlen(roughdir)-1] != '/')
      strcat(roughdir, "/");


   if(smoothdir[0] != '/')
   {
      strcpy(tmpstr, cwd);

      if(strlen(smoothdir) > 0)
      {
         strcat(tmpstr, "/");
         strcat(tmpstr, smoothdir);
      }

      strcpy(smoothdir, tmpstr);
   }

   if(smoothdir[strlen(smoothdir)-1] != '/')
      strcat(smoothdir, "/");


   if(width%2 == 0)
      width = width + 1;

   offset = (width - 1)/2;
         
   if(debug)
   {
      printf("DEBUG> roughdir  = [%s]\n", roughdir);
      printf("DEBUG> smoothdir = [%s]\n", smoothdir);
      printf("DEBUG> order     =  %d \n", order);
      printf("DEBUG> width     =  %d \n", width);
      printf("DEBUG> offset    =  %d \n", offset);
      fflush(stdout);
   }


   /* Loop over the tiles */

   ntile = 5 * pow(2., order); 

   for(jtile=0; jtile<ntile; ++jtile)
   {
      for(itile=0; itile<ntile; ++itile)
      {
         strcpy(tilelist, "");

         refid = xy2id(order, itile, jtile);

         if(refid < 0)
            continue;

         if(debug)
         {
            printf("\nDEBUG> Npix%d (%d,%d):\n", refid, itile, jtile);
            fflush(stdout);
         }

         sprintf(outname, "%sNpix%d.hist", smoothdir, refid);


         /* The loop over the region for this tile, */
         /* collecting the neighbor tile names.     */

         xtile = ((double)itile + 0.5) / (double)ntile;
         ytile = ((double)jtile + 0.5) / (double)ntile;

         for(j=jtile-offset; j<=jtile+offset; ++j)
         {
            for(i=itile-offset; i<=itile+offset; ++i)
            {
               // Special check to see if i,j falls in a gap
               
               x = ((double)i + 0.5) / (double)ntile;
               y = ((double)j + 0.5) / (double)ntile;
               

               // The gaps in the HPX projection running up to the North pole.
               // We are just identifying these special cases with integer numbers
               // for later use in a case statement where we calculate the actual
               // tiles to use in place of these locations.
              
               gap = NONE;

               if(x > -0.2 && x< 0.0 && y >  0.2 && y < 0.4 && xtile > 0.0) gap = NPOLEG1;

               if(x >  0.0 && x< 0.2 && y >  0.4 && y < 0.6 && xtile < 0.2) gap = NPOLEG2;
               if(x >  0.0 && x< 0.2 && y >  0.4 && y < 0.6 && xtile > 0.2) gap = NPOLEG3;

               if(x >  0.2 && x< 0.4 && y >  0.6 && y < 0.8 && xtile < 0.4) gap = NPOLEG4;
               if(x >  0.2 && x< 0.4 && y >  0.6 && y < 0.8 && xtile > 0.4) gap = NPOLEG5;

               if(x >  0.4 && x< 0.6 && y >  0.8 && y < 1.0 && xtile < 0.6) gap = NPOLEG6;
               if(x >  0.4 && x< 0.6 && y >  0.8 && y < 1.0 && xtile > 0.6) gap = NPOLEG7;

               if(x >  0.6 && x< 0.8 && y >  1.0 && y < 1.2 && xtile < 0.8) gap = NPOLEG8;


               // The same for the South pole.
               
               if(x >  0.2 && x< 0.4 && y > -0.2 && y < 0.0 && xtile < 0.4) gap = SPOLEG1;

               if(x >  0.4 && x< 0.6 && y >  0.0 && y < 0.2 && xtile < 0.4) gap = SPOLEG2;
               if(x >  0.4 && x< 0.6 && y >  0.0 && y < 0.2 && xtile > 0.4) gap = SPOLEG3;

               if(x >  0.6 && x< 0.8 && y >  0.2 && y < 0.4 && xtile < 0.6) gap = SPOLEG4;
               if(x >  0.6 && x< 0.8 && y >  0.2 && y < 0.4 && xtile > 0.6) gap = SPOLEG5;

               if(x >  0.8 && x< 1.0 && y >  0.4 && y < 0.6 && xtile < 0.8) gap = SPOLEG6;
               if(x >  0.8 && x< 1.0 && y >  0.4 && y < 0.6 && xtile > 0.8) gap = SPOLEG7;

               if(x >  1.0 && x< 1.2 && y >  0.6 && y < 0.8 && xtile < 1.0) gap = SPOLEG8;


               // The two sides of the lower left face that slot into the upper right.
               // In other words, the -180/+180 (sort of) wrap-around.
               
               if(x > -0.2 && x< 0.0 && y >  0.0 && y < 0.2 && xtile < 0.2) gap = WRAP1;
               if(x >  0.0 && x< 0.2 && y > -0.2 && y < 0.0 && ytile < 0.2) gap = WRAP2;
              

               if(x >  0.8 && x< 1.0 && y >  0.4 && y < 0.6 && xtile > 0.8) gap = SPOLEG7;

               if(x >  1.0 && x< 1.2 && y >  0.6 && y < 0.8 && xtile < 1.0) gap = SPOLEG8;


               // The two sides of the lower left face that slot into the upper right.
               // In other words, the -180/+180 (sort of) wrap-around.
               
               if(x > -0.2 && x< 0.0 && y >  0.0 && y < 0.2 && xtile < 0.2) gap = WRAP1;
               if(x >  0.0 && x< 0.2 && y > -0.2 && y < 0.0 && ytile < 0.2) gap = WRAP2;
              

               // And the matching "slot" in the upper right.

               if(x >  0.8 && x< 1.0 && y >  0.8 && y < 1.0 && xtile < 0.8) gap = WRAP3;
               if(x >  0.8 && x< 1.0 && y >  0.8 && y < 1.0 && ytile < 0.8) gap = WRAP4;


               // Now we have a bunch of one-offs, mostly tiles touching the poles
               // needing to see data from the tile diagonally across the pole.
               // Also, the tile just off the lower left (e.g. -1,-1) which doesn't
               // get included in the above wrap-around.

               if(x <  0.0           && y >  0.4                          ) gap = NCPOLE1;
               if(x <  0.2           && y >  0.6                          ) gap = NCPOLE2;
               if(x <  0.4           && y >  0.8                          ) gap = NCPOLE3;
               if(x <  1.6           && y >  1.0                          ) gap = NCPOLE4;

               if(x >  0.4           && y <  0.0                          ) gap = SCPOLE1;
               if(x >  0.6           && y <  0.2                          ) gap = SCPOLE2;
               if(x >  0.8           && y <  0.4                          ) gap = SCPOLE3;
               if(x >  1.0           && y <  0.6                          ) gap = SCPOLE4;

               if(x <  0.0           && y <  0.0                          ) gap = WCORNER1;
               if(x >  0.8           && y >  0.8                          ) gap = WCORNER2;


               if(gap)
               {
                  switch(gap)
                  {
                     case NPOLEG1:
                        xpivot = 0.0;
                        ypivot = 0.2;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CCW);
                        xtrans += 0.8;
                        ytrans += 0.8;
                        break;

                     case NPOLEG2:
                        xpivot = 0.2;
                        ypivot = 0.4;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CW);
                        break;

                     case NPOLEG3:
                        xpivot = 0.2;
                        ypivot = 0.4;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CCW);
                        break;

                     case NPOLEG4:
                        xpivot = 0.4;
                        ypivot = 0.6;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CW);
                        break;

                     case NPOLEG5:
                        xpivot = 0.4;
                        ypivot = 0.6;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CCW);
                        break;

                     case NPOLEG6:
                        xpivot = 0.6;
                        ypivot = 0.8;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CW);
                        break;

                     case NPOLEG7:
                        xpivot = 0.6;
                        ypivot = 0.8;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CCW);
                        break;

                     case NPOLEG8:
                        xpivot = 0.8;
                        ypivot = 1.0;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CW);
                        xtrans -= 0.8;
                        ytrans -= 0.8;
                        break;


                     case SPOLEG1:
                        xpivot = 0.2;
                        ypivot = 0.0;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CW);
                        xtrans += 0.8;
                        ytrans += 0.8;
                        break;

                     case SPOLEG2:
                        xpivot = 0.4;
                        ypivot = 0.2;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CCW);
                        break;

                     case SPOLEG3:
                        xpivot = 0.4;
                        ypivot = 0.2;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CW);
                        break;

                     case SPOLEG4:
                        xpivot = 0.6;
                        ypivot = 0.4;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CCW);
                        break;

                     case SPOLEG5:
                        xpivot = 0.6;
                        ypivot = 0.4;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CW);
                        break;

                     case SPOLEG6:
                        xpivot = 0.8;
                        ypivot = 0.6;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CCW);
                        break;

                     case SPOLEG7:
                        xpivot = 0.8;
                        ypivot = 0.6;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CW);
                        break;

                     case SPOLEG8:
                        xpivot = 1.0;
                        ypivot = 0.8;
                        pivot(x, y, xpivot, ypivot, &xtrans, &ytrans, CCW);
                        xtrans -= 0.8;
                        ytrans -= 0.8;
                        break;


                     case WRAP1:
                     case WRAP2:
                        xtrans = x + 0.8;
                        ytrans = y + 0.8;
                        break;

                     case WRAP3:
                     case WRAP4:
                        xtrans = x - 0.8;
                        ytrans = y - 0.8;
                        break;


                     case NCPOLE1:
                     case NCPOLE2:
                        xtrans = xtile + 0.4;
                        ytrans = ytile + 0.4;
                        break;

                     case NCPOLE3:
                     case NCPOLE4:
                        xtrans = xtile - 0.4;
                        ytrans = ytile - 0.4;
                        break;


                     case SCPOLE1:
                     case SCPOLE2:
                        xtrans = xtile + 0.4;
                        ytrans = ytile + 0.4;
                        break;

                     case SCPOLE3:
                     case SCPOLE4:
                        xtrans = xtile - 0.4;
                        ytrans = ytile - 0.4;
                        break;


                     case WCORNER1:
                        xtrans = x + 0.8;
                        ytrans = y + 0.8;
                        break;

                     case WCORNER2:
                        xtrans = x - 0.8;
                        ytrans = y - 0.8;
                        break;

                     default:
                        break;
                  }
               }


               if(xtrans < 0.0) xtrans += 1.0;
               if(ytrans < 0.0) ytrans += 1.0;

               if(xtrans > 1.0) xtrans -= 1.0;
               if(ytrans > 1.0) ytrans -= 1.0;

               
               if(gap == NONE)
               {
                  id = xy2id(order, i, j);
                  
                  if(id < 0)
                     continue;

                  if(debug)
                  {
                     printf("DEBUG>   %d %d -> Npix%d\n", i, j, id);
                     fflush(stdout);
                  }

                  sprintf(tilename, "%sNpix%d", roughdir, id);
               }

               else
               {
                  itrans = (int)(xtrans * (double)ntile);
                  jtrans = (int)(ytrans * (double)ntile);
                  
                  id = xy2id(order, itrans, jtrans);

                  if(debug)
                  {
                     printf("DEBUG>   %d %d ->  %s (gap %d)  ->  %d %d  -> Npix%d\n",
                            i, j, gapnames[gap], gap, itrans, jtrans, id);
                     fflush(stdout);
                  }

                  sprintf(tilename, "%sNpix%d", roughdir, id);
               }

               if(strlen(tilelist) == 0)
                  strcat(tilelist, " ");

               strcat(tilelist, tilename);
            }
         }

         sprintf(cmd, "mCombineHist min max gaussian-log -out %s %s", outname, tilelist);
         
         if(debug)
         {
            printf("DEBUG> CMD: [%s]\n", cmd);
            fflush(stdout);
         }

         printf("%s\n", cmd);
         fflush(stdout);
      }
   }

   printf("[struct stat=\"OK\", module=\"mHPXCombineHist\"]\n");
   fflush(stdout);
   exit(0);
}



void pivot(double x, double y, double xpivot, double ypivot, 
           double *xtrans, double *ytrans, int dir)
{
   if(dir == CW)
   {
      *xtrans = xpivot + y - ypivot;
      *ytrans = ypivot - x + xpivot;
   }
   else // dir == CCW
   {
      *xtrans = xpivot - y + ypivot;
      *ytrans = ypivot + x - xpivot;
   }
}



int xy2id(int order, int i, int j)
{
   long long level, baseTile;
   long long nside, index, id;
   long long xi, yi, ibase, jbase, fullscale;

   double x, y;
   double xintile,  yintile;

   int debug, ix, iy, iface, ibit;

   debug = 0;

   level = order;

   x = i;
   y = j;


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
      if(debug)
      {
         printf("DEBUG> ERROR: Location invalid.\n");
         fflush(stdout);
      }

      return -1;
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

   for(iface=0; iface<12; ++iface)
   {
      if(ibase == xoffset[iface])
      {
         if(jbase == yoffset[iface])
         {
            baseTile = iface;
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
      if(debug)
      {
         printf("DEBUG> ERROR: Location invalid.\n");
         fflush(stdout);
      }

      return -1;
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

   for(ibit=63; ibit>=0; --ibit)
   {
      iy = (yi >> ibit) & 1;
      ix = (xi >> ibit) & 1;

      id = 4*id + 2*iy + ix;
   }

   if(debug)
   {
      printf("\nDEBUG> Tile index   = %lld (i.e., index inside base tile)\n", id);
      printf("DEBUG> Tile index   = %llo (octal)\n\n", id);
      fflush(stdout);
   }

   index += id;

   return index;
}


void xy2tile(int level, int tile, int *xtile, int *ytile)
{
   long baseTile, nside;
   long index;
   int  x, y;
   int  debug;

   debug = 0;


   // Size of a base tile, in both output tile units
   // and output pixel units

   nside = pow(2., (double)level);

   if(debug)
   {
      printf("\nDEBUG> nside(tile)  = %ld\n", nside);
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


   splitIndex(index, (long)level, &x, &y);

   x = nside - 1 - x;

   if(debug)
   {
      printf("\nDEBUG> Relative tile: X = %7d, Y = %7d (of %d) [X was originally %d]\n", x, y, (int)nside, (int)nside - 1 - x);
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

void splitIndex(long index, long level, int *x, int *y)
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
