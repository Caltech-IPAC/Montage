#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <mtbl.h>

#define MAXSTR  4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int debug;


/**********************************************************/
/*                                                        */
/*  mHPXSplitCorrections                                  */
/*                                                        */
/*  Reassign the background corrections to specific plate */
/*  directories in the HPX "project" directory.           */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  i, ch, istat, ncols, offset, nimg, ncorr;

   char cwd       [MAXSTR];
   char corrfile  [MAXSTR];
   char template  [MAXSTR];
   char projectdir[MAXSTR];
   char imglist   [MAXSTR];
   char tmpstr    [MAXSTR];
   char old_plate [MAXSTR];
   char imgfile   [MAXSTR];

   char plates[32768][64];
   int  plate_offset[32768];
   int  nplate;

   int    iid;
   int    inaxis1;
   int    inaxis2;
   int    icrpix1;
   int    icrpix2;
   int    iplate;
   int    ifname;

   int    id;
   int    naxis1;
   int    naxis2;
   double crpix1;
   double crpix2;
   char   plate[MAXSTR];
   char   fname[MAXSTR];

   int    icntr;
   int    ia;
   int    ib;
   int    ic;

   int    cntr;
   double a;
   double b;
   double c;

   FILE *fimg;
   FILE *fcorr;


   getcwd(cwd, MAXSTR);


/****************************************************************************************************************/
/* At this point in the overall processing, we will have reprojected all the data to HPX projection, processed  */
/* all the image-to-image overlaps and used them to generate corrections for each image.  In doing the above we */
/* created a composite image list and likewise a global corrections list.  Here we want to split these back out */
/* into per plate image and corrections lists in preparation for the actual background correction and mosaic    */
/* coaddition (both of which will be done in parallel).  For sanity's sake we will revert the numbering of the  */
/* images (and therefore the corrections) to the original 0 through N used for each plate.                      */
/****************************************************************************************************************/


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   opterr = 0;

   while ((ch = getopt(argc, argv, "d")) != EOF) 
   {
        switch (ch) 
        {
           case 'd':
                debug = 1;
                break;

           default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] projectdir images.tbl corrections.tbl\"]\n", argv[0]);
                exit(1);
                break;
        }
   }

   if (argc - optind < 3)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] projectdir images.tbl corrections.tbl\"]\n", argv[0]);
        exit(1);
   }

   strcpy(projectdir, argv[optind]);
   strcpy(imglist,    argv[optind + 1]);
   strcpy(corrfile,   argv[optind + 2]);

   if(projectdir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, projectdir);

      strcpy(projectdir, tmpstr);
   }

   if(imglist[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, imglist);

      strcpy(imglist, tmpstr);
   }

   if(corrfile[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, corrfile);

      strcpy(corrfile, tmpstr);
   }

   if(projectdir[strlen(projectdir)-1] != '/')
      strcat(projectdir, "/");


   /***********************************************************************/ 
   /* Read through the global "images.tbl" metadata table and create      */
   /* "pimages_global.tbl" files in the plate subdirectories under        */
   /* projectdir (even though this information should just be a duplicate */
   /* of the "pimages.tbl" file that is already there).                   */
   /***********************************************************************/ 

   ncols = topen(imglist);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid image metadata file: %s\"]\n", imglist);
      fflush(stdout);
      exit(0);
   }

   icntr   = tcol("cntr");
   inaxis1 = tcol("naxis1");
   inaxis2 = tcol("naxis2");
   icrpix1 = tcol("crpix1");
   icrpix2 = tcol("crpix2");
   iplate  = tcol("plate");
   ifname  = tcol("fname");

   if(icntr   < 0
   || inaxis1 < 0
   || inaxis2 < 0
   || icrpix1 < 0
   || icrpix2 < 0
   || iplate  < 0
   || ifname  < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns: cntr, naxis1, naxis2, crpix1, crpix2, plate, fname in image list.\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(old_plate, "");

   offset = 0;
   nplate = 0;
   nimg   = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      cntr = atoi(tval(icntr));

      naxis1 = atoi(tval(inaxis1));
      naxis2 = atoi(tval(inaxis2));
      crpix1 = atof(tval(icrpix1));
      crpix2 = atof(tval(icrpix2));

      strcpy(plate, tval(iplate));
      strcpy(fname, tval(ifname));

      if(strcmp(plate, old_plate) != 0)
      {
         if(strlen(old_plate) > 0)
            fclose(fimg);

         strcpy(old_plate, plate);

         offset = cntr;

         strcpy(plates[nplate], plate);
         
         plate_offset[nplate] = cntr;

         ++nplate;

         sprintf(imgfile, "%s%s/pimages_global.tbl", projectdir, plate);

         fimg = fopen(imgfile, "w+");

         if(fimg == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Can't open image list [%s].\"]\n", imgfile);
            fflush(stdout);
            exit(0);
         }

         if(debug)
         {
            printf("\nDEBUG> Switching to file: %s\n", imgfile);
            fflush(stdout);
         }

         fprintf(fimg, "|  cntr  | naxis1 | naxis2 |     crpix1     |     crpix2     |    plate   |  %46s  |\n", "fname");
         fflush(fimg);
      }

      fprintf(fimg, " %8d %8d %8d %16.5f %16.5f %12s %50s \n",
         (cntr-offset), naxis1, naxis2, crpix1, crpix2, plate, fname);
      fflush(fimg);

      if(debug)
      {
         printf("DEBUG>  image:  %8d %8d %8d %16.5f %16.5f %12s %50s \n",
            (cntr-offset), naxis1, naxis2, crpix1, crpix2, plate, fname);
         fflush(stdout);
      }

      ++nimg;
   }

   fclose(fimg);
   tclose(); 

   if(debug)
   {
      printf("\nplate offsets:\n");

      for(iplate=0; iplate<nplate; ++iplate)
         printf("DEBUG> [%s] (%d)\n", plates[iplate], plate_offset[iplate]);

      printf("\n");
   }

   /******************************************/ 
   /* Open the corrections table file        */
   /******************************************/ 

   ncols = topen(corrfile);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid corrections file: %s\"]\n", corrfile);
      fflush(stdout);
      exit(0);
   }

   iid = tcol( "id");
   ia  = tcol( "a");
   ib  = tcol( "b");
   ic  = tcol( "c");

   if(iid < 0
   || ia  < 0
   || ib  < 0
   || ic  < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns: id,a,b,c in corrections file\"]\n");
      fflush(stdout);
      exit(0);
   }


   /********************************/
   /* And read in the corrections. */
   /********************************/

   ncorr  = 0;
   offset = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      id = atoi(tval(iid));

      a = atof(tval(ia));
      b = atof(tval(ib));
      c = atof(tval(ic));

      for(iplate=0; iplate<nplate; ++iplate)
      {
         if(id == plate_offset[iplate])
         {
            if(ncorr > 0)
               fclose(fcorr);

            strcpy(plate, plates[iplate]);

            offset = id;

            if(debug)
            {
               printf("\nDEBUG> Switching to plate: [%s] (offset: %d)\n", plate, offset);
               fflush(stdout);
            }

            sprintf(corrfile, "%s%s/corrections.tbl", projectdir, plate);

            if(debug)
            {
               printf("DEBUG> Switching to file: %s\n", corrfile);
               fflush(stdout);
            }

            fcorr = fopen(corrfile, "w+");

            if(fcorr == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Can't open corrections list [%s].\"]\n", corrfile);
               fflush(stdout);
               exit(0);
            }

            fprintf(fcorr, "|   id   |      a       |      b       |      c       |\n");
            fflush(fcorr);

            break;
         }
      }

      fprintf(fcorr, " %8d  %13.5e  %13.5e  %13.5e\n", (id-offset), a, b, c);
      fflush(fcorr);

      if(debug)
      {
         printf("DEBUG>  correction:  %8d  %13.5e  %13.5e  %13.5e\n", 
            id, a, b, c);
         fflush(stdout);
      }

      ++ncorr;
   }

   fclose(fcorr);
   tclose();


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", module=\"mHPXSplitCorrections\", nimg=%d, ncorr=%d]\n", 
      nimg, ncorr);
   fflush(stdout);
   exit(0);
}
