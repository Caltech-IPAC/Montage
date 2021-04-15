#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#define  NPLATES 1024

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int sky[5][5] = {{1,1,0,0,0},
                 {1,1,1,0,0},
                 {0,1,1,1,0},
                 {0,0,1,1,1},
                 {0,0,0,1,1}};

int naxis;


/****************************************************************************/
/*                                                                          */
/*  By carefully constructing the order in which we process plates, we      */
/*  can balance the I/O load for a set of storage units while still keeping */
/*  sets of plates that are contiguous on the sky co-located on disk.       */
/*                                                                          */
/*  This program takes a range of plates (by default the whole sky) and     */
/*  outputs a list of plates to process in optimal order.                   */
/*                                                                          */
/****************************************************************************/

int main(int argc, char **argv)
{
   int ch, order, nside, nstorage, nplate, nx, dx;
   int k, nbin, outid, outshift, id;
   int i, imin, imax, xmin, xmax;
   int j, jmin, jmax, ymin, ymax;

   int  platecount, maxplates;

   char outtbl[1024];

   struct plates
   {
      int  id;
      int  i;
      int  j;
      char platename[64];
      int  bin;
   };

   FILE *ftbl;

   struct plates *platelist;


   int debug = 0;


   // Command-line arguments

   nstorage = 1;

   for(i=0; i<argc; ++i)
       printf("%d: [%s]\n", i, argv[i]);

   opterr = 0;

   while ((ch = getopt(argc, argv, "ds:")) != EOF)
   {
      switch (ch)
      {
         case 'd':
            debug = 1;
            break;

         case 's':
            nstorage = atoi(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXPlateList [-d][-s nstorage] order nplate platelist.tbl [xmin ymin xmax ymax] (1)\"]\n");
            fflush(stdout);
            exit(1);
      }
   }

   argc -= optind;
   argv += optind;

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXPlateList [-d][-s nstorage] order nplate platelist.tbl [xmin ymin xmax ymax] (2)\"]\n");
      fflush(stdout);
      exit(0);
   }

   order    = atoi(argv[0]);
   nplate   = atoi(argv[1]);

   strcpy(outtbl, argv[2]);

   if(argc > 3 && argc < 7)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXPlateList [-d][-s nstorage] order nplate platelist.tbl [xmin ymin xmax ymax] (3)\"]\n");
      fflush(stdout);
      exit(0);
   }

   xmin = 0;
   ymin = 0;
   xmax = nplate-1;
   ymax = nplate-1;

   if(argc > 3)
   {
      xmin = atoi(argv[3]);
      ymin = atoi(argv[4]);
      xmax = atoi(argv[5]);
      ymax = atoi(argv[6]);
   }

   
   // Basic parameters for this order

   nside = pow(2., (double)order);

   naxis = 5 * nside;

   nx = naxis/nplate;
   dx = naxis/5;

   if(nx*nplate < nside)
      ++nx;

   if(debug)
   {
      printf("DEBUG> nside = %d\n", nside);
      printf("DEBUG> naxis = %d\n", naxis);
      printf("DEBUG> nx    = %d\n", nx);
      printf("DEBUG> dx    = %d\n", dx);
      printf("DEBUG> xmin  = %d\n", xmin);
      printf("DEBUG> xmax  = %d\n", xmax);
      printf("DEBUG> ymin  = %d\n", ymin);
      printf("DEBUG> ymax  = %d\n", ymax);
      fflush(stdout);
   }


   // We'll need to do this loop twice since we
   // need the overall plate count early.

   platecount = 0;

   for(i=xmin; i<=xmax; ++i)
   {
      imin = i*nx;
      imax = (i+1)*nx - 1;

      for(j=ymin; j<=ymax; ++j)
      {
         jmin = j*nx;
         jmax = (j+1)*nx - 1;

         if(sky[imin/dx][jmin/dx]
         || sky[imin/dx][jmax/dx]
         || sky[imax/dx][jmin/dx]
         || sky[imax/dx][jmax/dx])
         
            ++platecount;
      }
   }


   // Find out how many plates we should have per storage container

   nbin = (int)((double)platecount / (double)nstorage);

   if(debug)
   {
      printf("DEBUG> nbin  = %d\n", nbin);
      fflush(stdout);
   }

   // Generate the plate list.
   
   platelist = (struct plates *)malloc(platecount * sizeof(struct plates));

   id         = 0;
   outid      = 0;
   outshift   = 0;

   for(i=xmin; i<=xmax; ++i)
   {
      imin = i*nx;
      imax = (i+1)*nx - 1;

      for(j=ymin; j<=ymax; ++j)
      {
         jmin = j*nx;
         jmax = (j+1)*nx - 1;

         if(sky[imin/dx][jmin/dx]
         || sky[imin/dx][jmax/dx]
         || sky[imax/dx][jmin/dx]
         || sky[imax/dx][jmax/dx])
         {
            platelist[outid].id = id;

            platelist[outid].i = i;
            platelist[outid].j = j;

            sprintf(platelist[outid].platename, "plate_%02d_%02d", i, j);

            platelist[outid].bin = id / nbin;

            outid += nstorage;

            if(outid >= platecount)
            {
               ++outshift;

               outid = outshift;
            }

            ++id;
         }
      }
   }


   ftbl = fopen(outtbl, "w+");

   if(ftbl == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output table file.]\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(ftbl, "\\order = %d\n", order);
   fprintf(ftbl, "\\nplate = %d\n\n", nplate);

   fprintf(ftbl, "| id  |    plate    |  i  |  j  | bin |\n");
   fprintf(ftbl, "| int |    char     | int | int | int |\n");

   for(i=0; i<platecount; ++i)
      fprintf(ftbl, " %5d %13s %5d %5d %5d \n",
         platelist[i].id, platelist[i].platename, platelist[i].i, platelist[i].j, platelist[i].bin); 

   fflush(ftbl);
   fclose(ftbl);


   printf("[struct stat=\"OK\", module=\"mHPXPlateList\", order=%d, nplates=%d]\n",
      order, platecount);
   fflush(stdout);
   exit(0);
} 
