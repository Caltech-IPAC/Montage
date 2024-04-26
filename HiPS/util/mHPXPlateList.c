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

//  Order:          0     1     2     3     4      5      6      7      8      9
//                 -------------------------------------------------------------
int nplate[10] = {  5,    5,    5,    5,    5,     5,    10,    20,    40,    80};
int naxis [10] = {512, 1024, 2048, 4096, 8192, 16384, 16384, 16384, 16384, 16384};


/****************************************************************************/
/*                                                                          */
/*  This program makes a list of plates for the sky, taking into account    */
/*  the regions of the HPX projection that are undefined.  The regions that */
/*  have data represent a diagonal set of 13 of a 5x5 cell grid.            */
/*                                                                          */
/*  We will choose a plate count based on the maximum HiPS order.           */
/*                                                                          */
/*  The size (naxis x naxis) we show here is a minimum; in general we pad   */
/*  the plate so there is some overlap we can use to match backgrounds      */
/*  globally, at least for the highest order.                               */
/*                                                                          */
/****************************************************************************/

int main(int argc, char **argv)
{
   int ch, id, inplate, inaxis;
   int order, nside, ntotal, dx;
   int i, imin, imax, xmin, xmax;
   int j, jmin, jmax, ymin, ymax;

   char outtbl   [1024];
   char platename[1024];

   FILE *ftbl;


   // Command-line arguments
   
   inplate = 0;

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXPlateList [-n nplate] order platelist.tbl\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(strcmp(argv[1], "-n") == 0)
   {
      inplate = atoi(argv[2]);

      argc -= 2;
      argv += 2;
   }

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXPlateList [-n nplate] order platelist.tbl\"]\n");
      fflush(stdout);
      exit(0);
   }

   order = atoi(argv[1]);

   if(order < 0 || order > 9)
   {
      printf("[struct stat=\"ERROR\", msg=\"Order must be 0-9.]\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(outtbl, argv[2]);


   // Basic parameters for this order

   ntotal = naxis[order] * nplate[order];

   if(inplate <= 0)
   {
      inplate = nplate[order];

      inaxis = naxis[order];
   }
   else
      inaxis = ntotal / inplate;

   dx = ntotal/5;

   
   // Generate the plate list.
   
   ftbl = fopen(outtbl, "w+");

   if(ftbl == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output table file.]\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(ftbl, "\\order  = %d\n", order);
   fprintf(ftbl, "\\nplate = %d\n", inplate);
   fprintf(ftbl, "\\naxis  = %d\n", inaxis );
   fprintf(ftbl, "\\total  = %d\n", ntotal);
   fprintf(ftbl, "\\\n");

   fprintf(ftbl, "| id  |   i   |   j   |     plate    |\n");
   fprintf(ftbl, "| int |  int  |  int  |     char     |\n");

   fflush(ftbl);

   id = 0;

   for(i=0; i<inplate; ++i)
   {
      imin = i*inaxis;
      imax = (i+1)*inaxis - 1;

      for(j=0; j<inplate; ++j)
      {
         jmin = j*inaxis;
         jmax = (j+1)*inaxis - 1;

         if(sky[imin/dx][jmin/dx]
         || sky[imin/dx][jmax/dx]
         || sky[imax/dx][jmin/dx]
         || sky[imax/dx][jmax/dx])
         {
            sprintf(platename, "plate_%02d_%02d", i, j);

            fprintf(ftbl, " %5d  %6d  %6d  %13s \n",
               id, i, j, platename); 

            ++id;
         }
      }
   }

   fflush(ftbl);
   fclose(ftbl);


   printf("[struct stat=\"OK\", module=\"mHPXPlateList\", nplates=%d]\n", id);
   fflush(stdout);
   exit(0);
} 
