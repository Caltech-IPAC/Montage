#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <mCoord.h>
#include <montage.h>

int main(int argc, char **argv) 
{
   int    i, debug, locinpix;
   int    hdu, plane3, plane4;

   int    planeCount;
   int    planes[256];

   double lon, lat;

   char   fitsfile[1024];

   struct mCoordReturn *returnStruct;

   FILE *montage_status;


   /* Process basic command-line arguments */

   lon    = 0.;
   lat    = 0.;
   debug  = 0;

   locinpix = 0;

   montage_status = stdout;

   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mCoord [-d][-s statusfile] image.fits ra dec (or X Y)]\"]\n");
      exit(1);
   }


   for(i=1; i<argc; ++i)
   {
      if(strncmp(argv[i], "-s", 2) == 0)
      {
         if(argc > i+1 && (montage_status = fopen(argv[i+1], "w+")) == (FILE *)NULL)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file.\"]\n");
            exit(1);
         }

         ++i;
      }

      else if(strcmp(argv[i], "-d") == 0)
         debug = 1;

      else
      {
         if(i+2 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"Usage: mCoord [-d][-s statusfile] image.fits ra dec (or X Y pixels; pixel coord have 'p' suffix)]\"]\n");
            exit(1);
         }

         strcpy(fitsfile, argv[i]);
         
         lon = atof(argv[i+1]);
         lat = atof(argv[i+2]);

         if(strstr(argv[i+1], "p") != 0) locinpix = 1;
         if(strstr(argv[i+2], "p") != 0) locinpix = 1;

         break;
      }
   }


   // Parse the HDU from the file name

   hdu    = 0;
   plane3 = 1;
   plane4 = 1;

   planeCount = mCoord_getPlanes(fitsfile, planes);

   if(planeCount > 0)
      hdu = planes[0];

   if(planeCount > 1)
      plane3 = planes[2];

   if(planeCount > 2)
      plane4 = planes[2];

   if(debug)
   {
      printf("DEBUG> fitsfile: [%s]\n",  fitsfile);
      printf("DEBUG> hdu:      [%d]\n",  hdu);
      printf("DEBUG> plane3:   [%d]\n",  plane3);
      printf("DEBUG> plane4:   [%d]\n",  plane4);
      printf("DEBUG> lon:      [%-g]\n", lon);
      printf("DEBUG> lat:      [%-g]\n", lat);
      printf("DEBUG> locinpix: [%d]\n",  locinpix);
      printf("DEBUG> debug:    [%d]\n",  debug);
   }

   returnStruct = mCoord(fitsfile, hdu, plane3, plane4, lon, lat, locinpix, debug);


   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mCoord\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
