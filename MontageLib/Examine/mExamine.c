#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <mExamine.h>
#include <montage.h>

#define STRLEN  1024
#define MAXFLUX 1024

#define NONE    0
#define REGION  1
#define APPHOT  2

#define APRAD  30

int main(int argc, char **argv) 
{
   int    i,debug, areaMode;
   int    locinpix, radinpix;

   int    planeCount;
   int    planes[256];

   int    hdu, plane3, plane4;

   double ra, dec, radius;

   char   infile[1024];

   struct mExamineReturn *returnStruct;

   FILE *montage_status;


   /* Process basic command-line arguments */

   radius = 0.;
   debug  = 0;

   locinpix = 0;
   radinpix = 0;

   areaMode = REGION;

   montage_status = stdout;

   for(i=0; i<argc; ++i)
   {
      if(strncmp(argv[i], "-s", 2) == 0)
      {
         if(argc > i+1 && (montage_status = fopen(argv[i+1], "w+")) == (FILE *)NULL)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file.\"]\n");
            exit(1);
         }
      }

      if(strcmp(argv[i], "-p") == 0 
      || strcmp(argv[i], "-a") == 0)
      {
         areaMode = REGION;

         if(strcmp(argv[i], "-a") == 0)
            areaMode = APPHOT;

         if(strcmp(argv[i], "-p") == 0 && i+4 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"No ra, dec, radius location given or file name missing\"]\n");
            exit(1);
         }

         if(strcmp(argv[i], "-a") == 0 && i+3 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"No ra, dec location given for aperture photometry or file name missing\"]\n");
            exit(1);
         }

         ra  = atof(argv[i+1]);
         dec = atof(argv[i+2]);

         if(strcmp(argv[i], "-p") == 0)
         {
            radius = atof(argv[i+3]);

            if(strstr(argv[i+1], "p") != 0) locinpix = 1;
            if(strstr(argv[i+2], "p") != 0) locinpix = 1;
            if(strstr(argv[i+3], "p") != 0) radinpix = 1;

            i += 3;
         }
         else
         {
            radius = APRAD;

            radinpix = 1;

            i += 2;
         }
      }

      else if(strcmp(argv[i], "-d") == 0)
      {
         if(i+1 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"File name missing\"]\n");
            exit(1);
         }

         debug = 1;
      }
   }

   if(radius > 0.)
   {
      if(areaMode == REGION)
      {
         argv += 4;
         argc -= 4;
      }
      else
      {
         argv += 3;
         argc -= 3;
      }
   }

   if(montage_status != stdout)
   {
      argv += 2;
      argc -= 2;
   }

   if(debug)
   {
      argv += 1;
      argc -= 1;
   }

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mExamine [-d][-p ra dec radius | -a ra dec] image.fits\"]\n");
      exit(1);
   }
   
   strcpy(infile,  argv[1]);


   // Parse the HDU from the file name

   planeCount = mExamine_getPlanes(infile, planes);

   hdu    = 0;
   plane3 = 1;
   plane4 = 1;

   if(planeCount > 0)
      hdu = planes[0];

   if(planeCount > 1)
      plane3 = planes[2];

   if(planeCount > 2)
      plane4 = planes[2];


   returnStruct = mExamine(areaMode, infile, hdu, plane3, plane4, ra, dec, radius, locinpix, radinpix, debug);


   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mExamine\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
