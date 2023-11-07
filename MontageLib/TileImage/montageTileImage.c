/* Module: mTileImage.c

Version  Developer        Date     Change

1.0      John Good        26Aug22  Baseline code
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

#include <mTileImage.h>
#include <montage.h>

#define STRLEN 1024

#define PIX 1

struct apPhoto 
{
   double rad;
   double flux;
   double fit;
   double sum;
};


static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mTileImage                                                           */
/*                                                                       */
/*  Generates an NxM set of subimages (tiles) of an input FITS image     */
/*  with optional padding for overlap.                                   */
/*                                                                       */
/*   char  *input_file     FITS file to tile.                            */
/*   char  *output_base    Base file name (including path) for output    */
/*                         tiles                                         */
/*                                                                       */
/*   int    hdu            Optional HDU offset for input file            */
/*                                                                       */
/*   int    nx             Number of tiles in X direction                */
/*   int    ny             Number of tiles in Y direction                */
/*                                                                       */
/*   int    xpad           X border padding                              */
/*   int    ypad           Y border padding                              */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mTileImageReturn *mTileImage(char *input_file, char *output_base, int hdu, 
                                    int nx, int ny, int xpad, int ypad, int debug)
{
   int    i, j, status, nfound;

   char  *header;
   char   output_tile[STRLEN];

   char   tmpstr[32768];

   long   naxis;
   long   naxes[10];
   int    naxis1, naxis2;

   double dsizex, dsizey;
   int    sizex, sizey;
   int    xmin, xmax, ymin, ymax;
   int    xpixsize, ypixsize;
   int    ntile, nfailed;

   fitsfile *fptr;

   struct mSubimageReturn *subimage;

   struct mTileImageReturn *returnStruct;

   returnStruct = (struct mTileImageReturn *)malloc(sizeof(struct mTileImageReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /* Process basic command-line arguments */

   if(debug)
   {
      printf("DEBUG> input_file  = %s\n", input_file);
      printf("DEBUG> output_base = %s\n", output_base);
      printf("DEBUG> hdu         = %d\n", hdu);
      printf("DEBUG> nx          = %d\n", nx);
      printf("DEBUG> ny          = %d\n", ny);
      printf("DEBUG> xpad        = %d\n", xpad);
      printf("DEBUG> ypad        = %d\n", ypad);
      printf("\n");
      fflush(stdout);
   }


   /* Open the FITS file */

   status = 0;
   if(fits_open_file(&fptr, input_file, READONLY, &status))
   {
      sprintf(returnStruct->msg, "Cannot open FITS file %s", input_file);
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
      sprintf(returnStruct->msg, "Cannot find WCS keys in FITS file %s", input_file);
      return returnStruct;
   }

   status = 0;
   if(fits_read_key_lng(fptr, "NAXIS", &naxis, (char *)NULL, &status))
   {
      sprintf(returnStruct->msg, "Cannot find NAXIS keyword in FITS file %s", input_file);
      return returnStruct;
   }

   status = 0;
   if(fits_read_keys_lng(fptr, "NAXIS", 1, naxis, naxes, &nfound, &status))
   {
      sprintf(returnStruct->msg, "Cannot find NAXIS1,2 keywords in FITS file %s", input_file);
      return returnStruct;
   }
   
   naxis1 = (int)naxes[0];
   naxis2 = (int)naxes[1];


   /* Input image sizes */

   if(debug)
   {
      printf("DEBUG> naxis1      = %d\n", naxis1);
      printf("DEBUG> naxis2      = %d\n", naxis2);
      printf("\n");
      fflush(stdout);
   }


   /* Nominal tile size. Individual can differ because of  */
   /* edge effects like lack of padding and round-off.     */
   /* For our purposes, we want the tiles to be as similar */
   /* as possible, so we adjust nx,ny accordingly.         */

   dsizex = (double)naxis1 / (double)nx;
   dsizey = (double)naxis2 / (double)ny;

   sizex = dsizex;
   sizey = dsizey;

   if((double)sizex < dsizex)
      ++sizex;

   if((double)sizey < dsizey)
      ++sizey;

   if(debug)
   {
      printf("DEBUG> dsizex       = %-g\n", dsizex);
      printf("DEBUG> dsizey       = %-g\n", dsizey);
      printf("\n");
      printf("DEBUG> sizex        = %d (%d)\n", sizex, sizex*nx);
      printf("DEBUG> sizey        = %d (%d)\n", sizey, sizey*ny);
      printf("\n");
      fflush(stdout);
   }


   /* Loop over the tiles, running mSubimage. */

   ntile   = 0;
   nfailed = 0;

   for(j=0; j<ny; ++j)
   {
      ymin = j*sizey;
      ymax = ymin + sizey - 1;

      ymin -= ypad;
      if(ymin < 0)
         ymin = 0;

      ymax += ypad;
      if(ymin > naxis2-1)
         ymin = naxis2-1;

      for(i=0; i<nx; ++i)
      {
         xmin = i*sizex;
         xmax = xmin + sizex - 1;

         xmin -= xpad;
         if(xmin < 0)
            xmin = 0;

         xmax += xpad;
         if(xmin > naxis1-1)
            xmin = naxis1-1;
      
         xpixsize = xmax-xmin+1;
         ypixsize = ymax-ymin+1;

         sprintf(output_tile, "%s_%d_%d.fits", output_base, i+1, j+1);

         if(debug)
         {
            printf("DEBUG> [%s] %6d: %6d -> %6d   %6d -> %6d (%dx%d)\n",
                   output_tile, ntile, xmin, xmax, ymin, ymax, xpixsize, ypixsize);
            fflush(stdout);
         }
         
         subimage = mSubimage(input_file, output_tile, xmin, ymin, xpixsize, ypixsize, PIX, hdu, 0, 0);
         
         status = subimage->status;
         
         if(status)
            ++nfailed;

         ++ntile;
      }

      if(debug)
      {
         printf("\n");
         fflush(stdout);
      }
   }
   

   /* Finally, print out parameters */

   strcpy(montage_json,  "{");

   sprintf(tmpstr, " \"sizex\":%d,",  (int)sizex);   strcat(montage_json, tmpstr);
   sprintf(tmpstr, " \"sizey\":%d,",  (int)sizey);   strcat(montage_json, tmpstr);
   sprintf(tmpstr, " \"ntile\":%d",   (int)ntile);   strcat(montage_json, tmpstr);
   sprintf(tmpstr, " \"nfailed\":%d", (int)nfailed); strcat(montage_json, tmpstr);

   strcat(montage_json, "}");
 

   strcpy(montage_msgstr,  "");

   sprintf(tmpstr, "sizex=%d,",   (int)sizex);   strcat(montage_msgstr, tmpstr);
   sprintf(tmpstr, " sizey=%d,",  (int)sizey);   strcat(montage_msgstr, tmpstr);
   sprintf(tmpstr, " ntile=%d",   (int)ntile);   strcat(montage_msgstr, tmpstr);
   sprintf(tmpstr, " nfailed=%d", (int)nfailed); strcat(montage_msgstr, tmpstr);

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   returnStruct->sizex   = sizex;
   returnStruct->sizey   = sizey;
   returnStruct->ntile   = ntile;
   returnStruct->nfailed = nfailed;

   return returnStruct;
}
