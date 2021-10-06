/* Module: mMPXSplit.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <fitsio.h>
#include <mtbl.h>

#include <montage.h>

#define MAXSTR 1024
#define MAXCNT  256


/* This structure contains the basic geometry alignment          */
/* information for images.  Since all images are aligned to the  */
/* same location on the sky (and the same projection, etc.,      */
/* aligning them in cartesian pixel space only requires knowing  */
/* their size and reference pixel coordinates.  In this context  */
/* the main thing we want is the reference from ID back to plate */
/* directory and plate-local image ID.                           */

typedef struct
{
   int  cntr;
   char plate[MAXSTR];
   int  plate_cntr;
}
ImgInfo;

ImgInfo *imgs;

static int nimages, maximages;

int debug = 0;



/*-***********************************************************************/
/*                                                                       */
/*  mHPXSplit                                                            */
/*                                                                       */
/*  For large, high-resolution HiPS maps, we do the background           */
/*  difference fitting subsetted by plates.  Before running the          */
/*  background modelling we need to combine the results of that          */
/*  processing into a single reprojected image list and a a single       */
/*  difference FITS table.                                               */
/*                                                                       */
/*  Running these two tables through mBgModel, we get corrections for    */
/*  all the reprojected images.  Where image split across plates, there  */
/*  will be corrections for each fragment, but hopefully such the split  */
/*  is not noticeable (as, of course, we hope is true for all adjacent   */
/*  images).                                                             */
/*                                                                       */
/*  We want to go back to working on plates in parallel, so this program */
/*  will redistribute the corrections information to the individual      */
/*  plate directories, redirecting the image references to the local     */
/*  plate-specific file IDs.                                             */
/*                                                                       */
/*  The simplest way is to read in the global image list (which          */
/*  remembers what plate the image came from and its ID there), then     */
/*  read through the corrections, creating a corrections table for each  */
/*  plate directory.                                                     */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int     i, ch, stat, ncols; 

   char    cwd        [MAXSTR];
   char    projectdir [MAXSTR];
   char    images     [MAXSTR];
   char    corrections[MAXSTR];
   char    tmpdir     [MAXSTR];
   char    prev_plate [MAXSTR];
   char    corr_file  [MAXSTR];

   FILE   *fcorr = (FILE *)NULL;

   int     icntr;
   int     iplate;
   int     iplate_cntr;

   int     iid;
   int     ia;
   int     ib;
   int     ic;
   int     id;

   double  a;
   double  b;
   double  c;


   strcpy(prev_plate, "");

   getcwd(cwd, MAXSTR);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   // Command-line arguments

   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXSplit [-d] projectdir images.tbl corrections.tbl\"]\n");
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
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXSplit [-d] projectdir images.tbl corrections.tbl\"]\n");
            fflush(stdout);
            exit(1);
      }
   }

   if(argc-optind < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXSplit [-d] projectdir images.tbl corrections.tbl\"]\n");
      fflush(stdout);
      exit(1);
   }

   strcpy(projectdir,  argv[optind]);
   strcpy(images,      argv[optind + 1]);
   strcpy(corrections, argv[optind + 2]);


   if(projectdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, projectdir);

      strcpy(projectdir, tmpdir);
   }

   if(debug)
   {
      printf("projectdir  = %s\n", projectdir);
      printf("images      = %s\n", images);
      printf("corrections = %s\n", corrections);
      fflush(stdout);
   }


   /**************************/
   /* Read in the image list */
   /**************************/

   maximages = MAXCNT;

   if(debug >= 2)
   {
      printf("Allocating imgs to %d (size %lu) [11]\n", maximages, maximages * sizeof(ImgInfo));
      fflush(stdout);
   }

   imgs = (ImgInfo *)malloc(maximages * sizeof(ImgInfo));

   if(imgs == (ImgInfo *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"malloc() failed (ImgInfo).\"]\n");
      fflush(stdout);
      exit(1);
   }

   ncols = topen(images);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid image metadata file: [%s]\"]\n", images);
      fflush(stdout);
      exit(1);
   }

   icntr       = tcol("cntr");
   iplate      = tcol("plate");
   iplate_cntr = tcol("plate_cntr");

   if(icntr       < 0
   || iplate      < 0
   || iplate_cntr < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns: cntr plate plate_cntr in image info file [%s].\"]\n", images);
      fflush(stdout);
      exit(1);
   }



   /******************************/ 
   /* Read the image information */ 
   /******************************/ 

   nimages = 0;

   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      imgs[nimages].cntr       = atoi(tval(icntr));
      imgs[nimages].plate_cntr = atoi(tval(iplate_cntr));

      strcpy(imgs[nimages].plate, tval(iplate));

      ++nimages;

      if(nimages >= maximages)
      {
         maximages += MAXCNT;

         if(debug >= 2)
         {
            printf("Reallocating imgs to %d (size %lu) [14]\n", maximages, maximages * sizeof(ImgInfo));
            fflush(stdout);
         }

         imgs = (ImgInfo *)realloc(imgs, maximages * sizeof(ImgInfo));

         if(imgs == (ImgInfo *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"realloc() failed (ImgInfo) [1].\"]\n");
            fflush(stdout);
            exit(1);
         }
      }
   }

   tclose();

   if(debug)
   {
      for(i=0; i<10; ++i)
      {
         printf("DEBUG> Image %d: cntr=%d, plate=[%s], plate_cntr=%d\n",
            i, imgs[i].cntr, imgs[i].plate, imgs[i].plate_cntr);
         fflush(stdout);
      }
   }


   /***********************************/ 
   /* Open the corrections table file */
   /***********************************/ 

   ncols = topen(corrections);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid corrections file: [%s]\"]\n", corrections);
      fflush(stdout);
      exit(1);
   }

   iid = tcol("id");
   ia  = tcol("a");
   ib  = tcol("b");
   ic  = tcol("c");

   if(iid < 0
   || ia  < 0
   || ib  < 0
   || ic  < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns: id a b c.\"]\n");
      fflush(stdout);
      exit(1);
   }



   /******************************************************************/ 
   /* Read the corrections and write the per-plate corrections files */ 
   /******************************************************************/ 

   i = 0;

   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      id = atoi(tval(iid));
      a  = atof(tval(ia));
      b  = atof(tval(ib));
      c  = atof(tval(ic));

      if(id != imgs[i].cntr)
      {
         printf("[struct stat=\"ERROR\", msg=\"id:%d not equal to imgs[%d].cntr:%d.\"]\n", id, i, imgs[i].cntr);
         fflush(stdout);
         exit(1);
      }

      if(strcmp(imgs[i].plate, prev_plate) != 0)
      {
         sprintf(corr_file, "%s/%s/corrections.tbl", projectdir, imgs[i].plate);

         if(debug)
         {
            printf("\n\nFile: [%s]\n\n", corr_file);

            printf("|%8s|%14s|%14s|%14s|\n", "id", "a", "b", "c");
            fflush(stdout);
         }

         if(fcorr)
            fclose(fcorr);

         fcorr = fopen(corr_file, "w+");

         fprintf(fcorr, "|%8s|%14s|%14s|%14s|\n", "id", "a", "b", "c");
         fflush(fcorr);
      }

      if(debug)
      {
         printf(" %8d   %12.5f   %12.5f   %12.5f   [%s]\n", imgs[i].plate_cntr, a, b, c, imgs[i].plate);
         fflush(stdout);
      }

      fprintf(fcorr, " %8d   %12.5f   %12.5f   %12.5f \n", imgs[i].plate_cntr, a, b, c);
      fflush(fcorr);

      strcpy(prev_plate, imgs[i].plate);

       ++i;
   }

   fclose(fcorr);
   tclose();

   printf("[struct=\"OK\", nimages=%d]\n", nimages);
   fflush(stdout);
   exit(0);
}
