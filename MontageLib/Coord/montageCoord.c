/* Module: mCoord.c

Version  Developer        Date     Change

2.1      John Good        08Sep15  fits_read_pix() incorrect null value
2.0      John Good        15Apr15  Complete revamp, with more image info 
                                   and region statistics.
1.0      John Good        13Feb08  Baseline code
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <fitsio.h>
#include <wcs.h>
#include <coord.h>

#include <mCoord.h>
#include <montage.h>

static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mCoord                                                               */
/*                                                                       */
/*  Sometimes, in higher level applications, we need to convert a sky    */
/*  coordinate to pixels or pixel coordinates to sky. This utility does  */
/*  that.  Not to be used when looping over a large number of pixels.    */
/*                                                                       */
/*   char  *imgfile        FITS file reference                           */
/*                                                                       */
/*   int    hdu            Optional HDU offset for input file            */
/*                                                                       */
/*   int    plane3         If datacube, the plane index for dimension 3  */
/*   int    plane4         If datacube, the plane index for dimension 4  */
/*                                                                       */
/*   double ra / x         Point RA or pixel X location                  */
/*   double dec / y        Point Dec or pixel Y location                 */
/*                                                                       */
/*   int    locinpix       The coordinates are in pixels (default 0)     */
/*                                                                       */
/*   int    debug          Turn debugging output on                      */
/*                                                                       */
/*************************************************************************/

struct mCoordReturn * mCoord(char *imgfile, int hdu, int plane3, int plane4, 
                                 double xlon, double ylat, int locinpix, int debug)
{
   int    status, csys, offscl;

   char  *header;
   char   tmpstr[32768];

   double ra, dec, xpix, ypix, im_lon, im_lat;
   double equinox;

   char   ctype1[256];

   struct WorldCoor *wcs;

   fitsfile *fptr;

   struct mCoordReturn *returnStruct;

   returnStruct = (struct mCoordReturn *)malloc(sizeof(struct mCoordReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /* Process basic command-line arguments */

   if(debug)
   {
      printf("DEBUG> imgfile  = %s \n", imgfile);
      printf("DEBUG> xlon     = %-g\n", xlon);
      printf("DEBUG> ylat     = %-g\n", ylat);
      fflush(stdout);
   }


   /* Open the FITS file and initialize the WCS transform */

   status = 0;
   if(fits_open_file(&fptr, imgfile, READONLY, &status))
   {
      sprintf(returnStruct->msg, "Cannot open FITS file %s", imgfile);
      return returnStruct;
   }

   if(hdu > 0)
   {
      status = 0;
      if(fits_movabs_hdu(fptr, hdu+1, NULL, &status))
      {
         sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
         return returnStruct;
      }
   }

   status = 0;
   if(fits_get_image_wcs_keys(fptr, &header, &status))
   {
      sprintf(returnStruct->msg, "Cannot find WCS keys in FITS file %s", imgfile);
      return returnStruct;
   }

   wcs = wcsinit(header);

   if(wcs == (struct WorldCoor *)NULL)
   {
      sprintf(returnStruct->msg, "WCS initialization failed.");
      return returnStruct;
   }



   /* We get the Equinox from the WCS.  If not    */
   /* there we take the command-line value. We    */
   /* then infer Julian/Besselian as best we can. */

   strcpy(ctype1, wcs->ctype[0]);

   equinox = wcs->equinox;

   csys = EQUJ;

   if(strncmp(ctype1, "RA--", 4) == 0)
   {
      csys = EQUJ;

      if(equinox < 1975.)
         csys = EQUB;
   }

   if(strncmp(ctype1, "LON-", 4) == 0)
      csys = GAL;

   if(strncmp(ctype1, "GLON", 4) == 0)
      csys = GAL;

   if(strncmp(ctype1, "ELON", 4) == 0)
   {
      csys = ECLJ;

      if(equinox < 1975.)
         csys = ECLB;
   }

   if(debug)
   {
      printf("\nDEBUG> Image:\n");
      printf("DEBUG> ctype1    = [%s]\n", ctype1);
      printf("DEBUG> csys      = %d\n",   csys);
      printf("DEBUG> equinox   = %-g\n",  equinox);
      printf("\n");
      fflush(stdout);
   }


   /* If the input is RA,Dec we convert to the image system and project to pixel coordinates */
   /* or if it is X,Y we deproject to image lon,lat and convert to RA,Dec                    */

   if(locinpix == 0) /* RA,Dec coordinates */
   {
      ra  = xlon;
      dec = ylat;

      if(debug)
      {
         printf("\nDEBUG> sky -> pix\n");
         printf("DEBUG> ra     = %-g\n", ra);
         printf("DEBUG> dec    = %-g\n", dec);
         fflush(stdout);
      }

      convertCoordinates (EQUJ, 2000., xlon, ylat, 
                          csys, equinox, &im_lon, &im_lat, 0.);

      if(debug)
      {
         printf("DEBUG> im_lon = %-g\n", im_lon);
         printf("DEBUG> im_lat = %-g\n", im_lat);
         fflush(stdout);
      }

      wcs2pix(wcs, im_lon, im_lat, &xpix, &ypix, &offscl);
      
      if(debug)
      {
         printf("DEBUG> xpix   = %-g\n", xpix);
         printf("DEBUG> ypix   = %-g\n", ypix);
         printf("DEBUG> offscl = %d\n",  offscl);
         fflush(stdout);
      }

      if(offscl)
      {
         sprintf(returnStruct->msg, "Location off the image.");
         return returnStruct;
      }
   }

   else  /* X,Y pixel coordinates */
   {
      xpix = xlon;
      ypix = ylat;

      if(debug)
      {
         printf("\nDEBUG> pix -> sky\n");
         printf("DEBUG> xpix   = %-g\n", xpix);
         printf("DEBUG> xpix   = %-g\n", ypix);
         fflush(stdout);
      }

      pix2wcs(wcs, xlon, ylat,  &im_lon, &im_lat);

      if(debug)
      {
         printf("DEBUG> im_lon = %-g\n", im_lon);
         printf("DEBUG> im_lat = %-g\n", im_lat);
         fflush(stdout);
      }

      convertCoordinates (csys, equinox, im_lon, im_lat, 
                          EQUJ, 2000., &ra, &dec, 0.);

      if(debug)
      {
         printf("DEBUG> ra     = %-g\n", ra);
         printf("DEBUG> dec    = %-g\n", dec);
         fflush(stdout);
      }
   }



   /* Finally, print out parameters */

   strcpy(montage_msgstr,  "");

   sprintf(tmpstr, " ra=%-g,",   ra  ); strcat(montage_msgstr, tmpstr);
   sprintf(tmpstr, " dec=%-g,",  dec ); strcat(montage_msgstr, tmpstr);
   sprintf(tmpstr, " xpix=%-g,", xpix); strcat(montage_msgstr, tmpstr);
   sprintf(tmpstr, " ypix=%-g",  ypix); strcat(montage_msgstr, tmpstr);


   strcpy(montage_json,  "{");

   sprintf(tmpstr, " \"ra\":\"%-g\",",   ra);   strcat(montage_json, tmpstr);
   sprintf(tmpstr, " \"dec\":\"%-g\",",  dec);  strcat(montage_json, tmpstr);
   sprintf(tmpstr, " \"xpix\":\"%-g\",", xpix); strcat(montage_json, tmpstr);
   sprintf(tmpstr, " \"ypix\":\"%-g\",", ypix); strcat(montage_json, tmpstr);

   strcat(montage_json, "}");


   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   returnStruct->ra   = ra;
   returnStruct->dec  = dec;
   returnStruct->xpix = xpix;
   returnStruct->ypix = ypix;

   return returnStruct;
}


/**************************************************/
/*                                                */
/*  Parse the HDU / plane info from the file name */
/*                                                */
/**************************************************/

int mCoord_getPlanes(char *file, int *planes)
{
   int   count, len;
   char *ptr, *ptr1;

   count = 0;

   ptr = file;

   len = strlen(file);

   while(ptr < file+len && *ptr != '[')
      ++ptr;

   while(1)
   {
      if(ptr >= file+len)
         return count;

      if(*ptr != '[')
         return count;

      *ptr = '\0';
      ++ptr;

      ptr1 = ptr;

      while(ptr1 < file+len && *ptr1 != ']')
         ++ptr1;

      if(ptr1 >= file+len)
         return count;

      if(*ptr1 != ']')
         return count;

      *ptr1 = '\0';

      planes[count] = atoi(ptr);
      ++count;

      ptr = ptr1;
      ++ptr;
   }
}
