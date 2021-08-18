/* mHiPSPNGScripts.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        08May21  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <svc.h>
#include <errno.h>

#define MAXSTR 4096

int  nimg[10] = {12, 48, 192, 768, 3072, 12288, 49152, 196608, 786432, 3145728};

int  debug;


/*************************************************************************/
/*                                                                       */
/*  mHiPSTileCheck                                                       */
/*                                                                       */
/*  This problem checks to see if the appropriate HiPS tile FITS and     */
/*  PNG files exist and creates a script to make them if they don't.     */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int i, order, count;
   int nfits, npng, dirno, olddir, maxorder, status;
   
   char filename[MAXSTR];

   struct stat type;

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSTileCheck maxorder\"]\n");
      exit(1);
   }

   maxorder = atoi(argv[1]);


   /******************************************************/
   /* Process the directories, looking for missing files */
   /******************************************************/

   nfits = 0;
   npng  = 0;

   for(order=0; order<=maxorder; ++order)
   {
      fprintf(stderr, "Order %d:\n", order);
      fflush(stderr);


      olddir = -1;


      for(i=0; i<nimg[order]; ++i)
      {
         dirno = (i / 10000) * 10000;

         if(dirno != olddir)
         {
            fprintf(stderr, "Dir%d:\n", dirno);
            fflush(stderr);

            olddir = dirno;
         }


         /*
         sprintf(filename, "Norder%d/Dir%d/Npix%d.fits", order, dirno, i);

         status = stat(filename, &type);

         if (status < 0)
         {
            printf("%s\n", filename);
            fflush(stdout);

            ++nfits;
         }
         */


         sprintf(filename, "Norder%d/Dir%d/Npix%d.png", order, dirno, i);

         status = stat(filename, &type);

         if (status < 0) 
         {
            printf("%s\n", filename);
            fflush(stdout);

            ++npng;
         }
      }
   }

   printf("[struct stat=\"OK\", module=\"mHiPSPNGScripts\", nfits=%d, npng=%d]\n", nfits, npng);
   fflush(stdout);
   exit(0);
}
