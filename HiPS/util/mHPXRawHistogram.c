/* Module: mMPXRawHistograms.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#include <fitsio.h>
#include <mtbl.h>

#include <montage.h>

#define MAXSTR 1024

int debug = 0;



/*-***********************************************************************/
/*                                                                       */
/*  mHPXRawHistograms                                                    */
/*                                                                       */
/*  We use a histogram of the region to determine how to stretch FITS    */
/*  images when making PNGs.  If we use an all-sky histogram to stretch  */
/*  all the region images, the global dynamic range washes out most of   */
/*  the local structure.  So we have devised a varying histogram         */
/*  technique where a the local region dominates the local histogram     */
/*  (and therefore stretch) but where this is tempered by using the      */
/*  histogram data from the neighboring space.                           */
/*                                                                       */
/*  This routine generates histograms for all local (usually 512x512)    */
/*  regions.  Later, these will be combined to create the stretches we   */
/*  actually use.                                                        */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int  i, ch, nind, ncols, nplates, order, iplate, count;
   int  status, nfound;

   char platelist[MAXSTR];
   char platedir [MAXSTR];
   char histdir  [MAXSTR];

   char cwd      [MAXSTR];
   char plate    [MAXSTR];
   char filename [MAXSTR];
   char histfile [MAXSTR];
   char tmpdir   [MAXSTR];

   long naxis[2];

   fitsfile *ffits;

   getcwd(cwd, MAXSTR);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   // Command-line arguments

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXRawHistograms [-d] platelist.tbl platedir order histdir\"]\n");
      fflush(stdout);
      exit(1);
   }

   debug = 0;

   while ((ch = getopt(argc, argv, "d")) != EOF)
   {
      switch (ch)
      {
         case 'd':
            debug = 1;
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXRawHistograms [-d] platelist.tbl platedir order histdir\"]\n");
            fflush(stdout);
            exit(1);
      }
   }

   if(optind + 3 >= argc)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXRawHistograms [-d] platelist.tbl platedir order histdir\"]\n");
      fflush(stdout);
      exit(1);
   }

   strcpy(platelist, argv[optind]);
   strcpy(platedir,  argv[optind + 1]);

   order = atoi(argv[optind + 2]);

   strcpy(histdir, argv[optind + 3]);


   if(platelist[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platelist);

      strcpy(platelist, tmpdir);
   }

   if(platedir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platedir);

      strcpy(platedir, tmpdir);
   }

   if(histdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, histdir);

      strcpy(histdir, tmpdir);
   }

   if(debug)
   {
      printf("DEBUG> platelist  = %s\n", platelist);
      printf("DEBUG> platedir   = %s\n", platedir);
      printf("DEBUG> order      = %d\n", order);
      printf("DEBUG> histdir    = %s\n", histdir);
      fflush(stdout);
   }


   /***************************/ 
   /* Loop over the platelist */
   /***************************/ 

   ncols = topen(platelist);

   if(ncols < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open plate list file [%s].\"]\n", platelist);
      fflush(stdout);
      exit(1);
   }

   nind = atoi(tfindkey("nplate"));

   nplates = tlen();

   iplate = tcol("plate");

   if(iplate < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need plate column in plate list file [%s].\"]\n", platelist);
      fflush(stdout);
      exit(1);
   }

   count = 0;

   for(i=0; i<nplates; ++i)
   {
      if(tread() < 0)
         break;

      strcpy(plate, tval(iplate));

      if(debug)
      {
         printf("DEBUG> plate = [%s]\n", plate);
         fflush(stdout);
      }

      sprintf(filename, "%s/order%d/%s.fits", platedir, order, plate);

      status = 0;

      if(fits_open_file(&ffits, filename, READONLY, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Failed to open FITS file [%s].\"]\n", filename);
         fflush(stdout);
         exit(1);
      }

      if(fits_read_keys_lng(ffits, "NAXIS", 1, 2, naxis, &nfound, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Failed to find parameters NAXIS1, NAXIS2 in [%s].\"]\n", filename);
         fflush(stdout);
         exit(1);
      }

      fits_close_file(ffits, &status);

      if(debug)
      {
         printf("DEBUG> naxis = %ldx%ld\n", naxis[0], naxis[1]);
         fflush(stdout);
      }

      ++count;
   }

   tclose();

   if(count < nplates)
   {
      printf("[struct stat=\"ERROR\", msg=\"Should be %d plates; only found %d.\"]\n", nplates, count);
      fflush(stdout);
      exit(1);
   }

   printf("[struct=\"OK\", nplates=%d]\n", count);
   fflush(stdout);
   exit(0);
}
