/* Module: mSubCube.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        15May15  Baseline code, based on mSubimage.c of this date.

*/

/* Module: subCube.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.1      John Good        08Sep15  fits_read_pix() incorrect null value
1.0      John Good        15May15  Baseline code, based on subImage.c of that date.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <wcs.h>
#include <coord.h>

#include <mSubCube.h>
#include <montage.h>

#define MAXSTR 32768


static struct WorldCoor *wcs;

static double xcorrection;
static double ycorrection;

static int mSubCube_debug;
static int isflat;
static int bitpix;

int haveBlank;
static long blank;

static char content[128];


static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mSubimage                                                            */
/*                                                                       */
/*  This program subsets an input image around a location of interest    */
/*  and creates a new output image consisting of just those pixels.      */
/*  The location is defined by the RA,Dec (J2000) of the new center and  */
/*  the XY size in degrees of the area (X and Y) in the direction of     */
/*  the image axes, not Equatorial coordinates.                          */
/*                                                                       */
/*   int    mode           Processing mode. The two main modes are       */
/*                         0 (SKY) and 1 (PIX), corresponding to cutouts */
/*                         are in sky coordinate or pixel space. The two */
/*                         other modes are 3 (HDU) and 4 (SHRINK), where */
/*                         the region parameters are ignored and you get */
/*                         back either a single HDU or an image that has */
/*                         had all the blank border pixels removed.      */
/*                                                                       */
/*   char  *infile         Input FITS file                               */
/*   char  *outfile        Subimage output FITS file                     */
/*                                                                       */
/*   double ra             RA of cutout center (or start X pixel in      */
/*                         PIX mode                                      */
/*   double dec            Dec of cutout center (or start Y pixel in     */
/*                         PIX mode                                      */
/*                                                                       */
/*   double xsize          X size in degrees (SKY mode) or pixels        */
/*                         (PIX mode)                                    */
/*   double ysize          Y size in degrees (SKY mode) or pixels        */
/*                         (PIX mode)                                    */
/*                                                                       */
/*   int    hdu            Optional HDU offset for input file            */
/*   int    nowcs          Indicates that the image has no WCS info      */
/*                         (only makes sense in PIX mode)                */
/*                                                                       */
/*   char  *d3constraint   String describing the datacube third          */
/*                         dimension selection constraints               */
/*                                                                       */
/*   char  *d4constraint   String describing the datacube fourth         */
/*                         dimension selection constraints               */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mSubCubeReturn *mSubCube(int mode, char *infile, char *outfile, double ra, double dec,
                                double xsize, double ysize, int hdu, int nowcs, char *d3constraint, 
                                char *d4constraint, int debugin)
{
   fitsfile *infptr, *outfptr;

   int       i, offscl, pixMode;
   double    cdelt[10];
   int       allPixels, shrinkWrap;
   int       imin, imax, jmin, jmax;

   int       sys;
   double    epoch;
   double    lon, lat;
   double    xpix, ypix;
   double    xoff, yoff;
   double    rotang, dtr;

   double    x, y;
   double    ix, iy;
   double    xpos, ypos;

   char     *checkHdr;

   char     *header[2];

   char      warning[1024];
\
   int       status = 0;

   struct mSubCubeParams params;

   struct mSubCubeReturn *returnStruct;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mSubCubeReturn *)malloc(sizeof(struct mSubCubeReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");



   dtr = atan(1.)/45.;

   mSubCube_debug = debugin;

   pixMode    = 0;
   allPixels  = 0;
   shrinkWrap = 0;


   if(mode == PIX)    pixMode    = 1;
   if(mode == HDU)    allPixels  = 1;
   if(mode == SHRINK) shrinkWrap = 1;

      
   params.ibegin = 1;
   params.iend   = 1;
   params.jbegin = 1;
   params.jend   = 1;
   
   params.nrange[0] = 0;

   params.kbegin = 1;
   params.kend   = 1;

   strcpy(params.dConstraint[0], "");

   params.naxes[2] = 0;

   if(strlen(d3constraint) > 0) 
   {
      strcpy(params.dConstraint[0], d3constraint);

      if(mSubCube_parseSelectList(3, &params))
      {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      params.kbegin = params.range[0][0][0];
      params.kend   = -1;

      for(i=0; i<params.nrange[0]; ++i)
      {
         if(params.range[0][i][0] < params.kbegin)
            params.kbegin = params.range[0][i][0];

         if(params.range[0][i][0] != -1 && params.range[0][i][0] > params.kend)
            params.kend = params.range[0][i][0];
      }

      params.naxes[2] = 0;

      for(i=0; i<params.nrange[0]; ++i)
      {
         if(params.range[0][i][0] < params.kbegin)
            params.kbegin = params.range[0][i][0];

         if(params.range[0][i][0] > params.kend)
            params.kend = params.range[0][i][0];

         if(params.range[0][i][1] != -1 && params.range[0][i][1] > params.kend)
            params.kend = params.range[0][i][1];

         if(params.range[0][i][1] == -1)
            ++params.naxes[2];
         else
            params.naxes[2] += params.range[0][i][1] - params.range[0][i][0] + 1;
      }
   }


   params.nrange[1] = 0;

   params.lbegin = 1;
   params.lend   = 1;

   strcpy(params.dConstraint[1], "");

   params.naxes[3] = 0;

   if(strlen(d4constraint) > 0)
   {
      strcpy(params.dConstraint[1], d4constraint);

      if(mSubCube_parseSelectList(4, &params))
      {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      params.lbegin = params.range[1][0][0];
      params.lend   = -1;

      params.naxes[3] = 0;

      for(i=0; i<params.nrange[1]; ++i)
      {
         if(params.range[1][i][0] < params.lbegin)
            params.lbegin = params.range[1][i][0];

         if(params.range[1][i][0] > params.lend)
            params.lend = params.range[1][i][0];

         if(params.range[1][i][1] != -1 && params.range[1][i][1] > params.lend)
            params.lend = params.range[1][i][1];

         if(params.range[1][i][1] == -1)
            ++params.naxes[3];
         else
            params.naxes[3] += params.range[1][i][1] - params.range[1][i][0] + 1;
      }
   }

   if(mSubCube_debug)
   {
      printf("DEBUG> mSubCube command parsing:\n\n");
      printf("DEBUG> nowcs      = %d\n", nowcs);
      printf("DEBUG> pixMode    = %d\n", pixMode);
      printf("DEBUG> shrinkWrap = %d\n", shrinkWrap);
      printf("DEBUG> allPixels  = %d\n", allPixels);
      printf("\nDEBUG> kbegin     = %d\n", params.kbegin);
      printf("DEBUG> kend       = %d\n", params.kend);
      printf("DEBUG> naxis[2]   = %ld\n", params.naxes[2]);
      printf("DEBUG> nrange3    = %d\n", params.nrange[0]);

      if(params.nrange[0] > 0)
      {
         printf("\n");

         for(i=0; i<params.nrange[0]; ++i)
            printf("%4d: %6d %6d\n", i, params.range[0][i][0], params.range[0][i][1]);

         printf("\n");
      }

      printf("\nDEBUG> lbegin     = %d\n", params.lbegin);
      printf("DEBUG> lend       = %d\n", params.lend);
      printf("DEBUG> naxis[3]   = %ld\n", params.naxes[3]);
      printf("DEBUG> nrange4    = %d\n", params.nrange[1]);

      if(params.nrange[1] > 0)
      {
         printf("\n");

         for(i=0; i<params.nrange[1]; ++i)
            printf("%4d: %6d %6d\n", i, params.range[1][i][0], params.range[1][i][1]);

         printf("\n");
      }

      fflush(stdout);
   }

   if(mSubCube_debug)
   {
      printf("DEBUG> infile     = [%s]\n", infile);
      printf("DEBUG> outfile    = [%s]\n", outfile);
      fflush(stdout);
   }


   /****************************************/
   /* Open the (unsubsetted) original file */
   /* to get the WCS info                  */
   /****************************************/

   if (!pixMode && !nowcs) {

      if(mSubCube_debug)
      {
         printf("DEBUG> calling checkHdr(\"%s\") for HDU %d\n", infile, hdu);
         fflush(stdout);
      }

      checkHdr = montage_checkHdr(infile, 0, hdu);

      if(checkHdr)
      {
         strcpy(returnStruct->msg, checkHdr);
         return returnStruct;
      }
   }
   
   header[0] = malloc(32768);
   header[1] = (char *)NULL;

   if(fits_open_file(&infptr, infile, READONLY, &status))
   {
      if(mSubCube_debug)
      {
         printf("DEBUG> Opening infile\n");
         fflush(stdout);
      }

      sprintf(returnStruct->msg, "Image file %s missing or invalid FITS", infile);
      return returnStruct;
   }

   if(hdu > 0)
   {
      if(mSubCube_debug)
      {
         printf("DEBUG> Moving to HDU %d\n", hdu);
         fflush(stdout);
      }

      if(fits_movabs_hdu(infptr, hdu+1, NULL, &status))
      {
         sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
         return returnStruct;
      }
   }

   fits_get_img_type(infptr, &bitpix, &status);

   haveBlank = 1; 
   if(fits_read_key_lng(infptr, "BLANK", &blank, (char *)NULL, &status))
   {
      haveBlank = 0; 
      status    = 0; 
   }

   if(mSubCube_debug)
   {
      printf("DEBUG> bitpix = %d\n", bitpix);
      printf("DEBUG> blank  = %ld (%d)\n", blank, haveBlank);
      fflush(stdout);
   }

   if(bitpix != -64 && shrinkWrap)
   {
      strcpy(returnStruct->msg, "Shrinkwrap mode only works for double precision floating point data.");
      return returnStruct;
   }

   if(shrinkWrap)
   {
      if(mSubCube_dataRange(infptr, &imin, &imax, &jmin, &jmax) > 0)
      {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      if(mSubCube_debug)
      {
         printf("imin  = %d\n", imin);
         printf("imax  = %d\n", imax);
         printf("jmin  = %d\n", jmin);
         printf("jmax  = %d\n", jmax);
         fflush(stdout);
      }
   }


   if (!nowcs) 
   {
      if(mSubCube_debug) 
      {
         printf("\nDEBUG> Checking WCS\n");
         fflush(stdout);
      }
  
      wcs = mSubCube_getFileInfo(infptr, header, &params);

      if(wcs == (struct WorldCoor *)NULL)
      {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   
      rotang = atan2(wcs->cd[2], wcs->cd[0])/dtr;

      while(rotang <   0.) rotang += 360.;
      while(rotang > 360.) rotang -= 360.;

      if((rotang >  45. && rotang < 135.) || 
         (rotang > 225. && rotang < 315.))
      {
         cdelt[0] = wcs->cd[2]/sin(rotang*dtr);
         cdelt[1] = wcs->cd[1]/sin(rotang*dtr);
      }
      else
      {
         cdelt[0] = wcs->cd[0]/cos(rotang*dtr);
         cdelt[1] = wcs->cd[3]/cos(rotang*dtr);
      }

      if(mSubCube_debug)
      {
         for(i=0; i<params.naxis; ++i)
         {
            printf("crpix%d = %-g\n", i+1, params.crpix[i]);
            printf("cdelt%d = %-g\n", i+1, cdelt[i]);
         }

         fflush(stdout);
      }


   /* Kludge to get around bug in WCS library:   */
   /* 360 degrees sometimes added to pixel coord */

      ix = 0.5;
      iy = 0.5;

      pix2wcs(wcs, ix, iy, &xpos, &ypos);

      offscl = wcs->offscl;

      x = 0.;
      y = 0.;

      if(!offscl)
         wcs2pix(wcs, xpos, ypos, &x, &y, &offscl);

      xcorrection = x-ix;
      ycorrection = y-iy;


   /* Extract the coordinate system and epoch info */

      if(wcs->syswcs == WCS_J2000)
      {
         sys   = EQUJ;
         epoch = 2000.;

         if(wcs->equinox == 1950.)
            epoch = 1950;
      }
      else if(wcs->syswcs == WCS_B1950)
      {
         sys   = EQUB;
         epoch = 1950.;

         if(wcs->equinox == 2000.)
            epoch = 2000;
      }
      else if(wcs->syswcs == WCS_GALACTIC)
      {
         sys   = GAL;
         epoch = 2000.;
      }
      else if(wcs->syswcs == WCS_ECLIPTIC)
      {
         sys   = ECLJ;
         epoch = 2000.;

         if(wcs->equinox == 1950.)
         {
            sys   = ECLB;
            epoch = 1950.;
         }
      }
      else       
      {
         sys   = EQUJ;
         epoch = 2000.;
      }
      
      if(mSubCube_debug)
      {
         printf("input coordinate system = %d\n", EQUJ);
         printf("input epoch             = %-g\n", 2000.);
         printf("image coordinate system = %d\n", sys);
         printf("image epoch             = %-g\n", epoch);
         fflush(stdout);
      }
   }
  

   /******************************************/
   /* If we are working in shrinkwrap mode,  */
   /* we use the ranges determined above.    */
   /*                                        */
   /* If we are working in pixel mode, we    */
   /* already have the info needed to subset */
   /* the image.                             */
   /*                                        */
   /* Otherwise, we need to convert the      */
   /* coordinates to pixel space.            */
   /******************************************/

   if(shrinkWrap)
   {
      params.ibegin = (int) imin;
      params.iend   = (int) imax;

      params.jbegin = (int) jmin;
      params.jend   = (int) jmax;
   }


   else if(pixMode)
   {
      if(mSubCube_debug) {
         printf("xsize= [%lf]\n", xsize);
         printf("ysize= [%lf]\n", ysize);

         printf("imin= [%d] imax = [%d]\n", imin, imax);
         printf("jmin= [%d] jmax = [%d]\n", jmin, jmax);
         fflush(stdout);
      }
 
      
      params.ibegin = (int)ra;
      params.iend   = (int)(ra + xsize + 0.5);

      params.jbegin = (int)dec;
      params.jend   = (int)(dec + ysize + 0.5);

      if(allPixels)
      {
         params.ibegin = 1;
         params.iend   = params.naxes[0];
         params.jbegin = 1;
         params.jend   = params.naxes[1];
      }

      if(params.ibegin < 1              ) params.ibegin = 1;
      if(params.ibegin > params.naxes[0]) params.ibegin = params.naxes[0];
      if(params.iend   > params.naxes[0]) params.iend   = params.naxes[0];
      if(params.iend   < 1              ) params.iend   = 1;

      if(params.jbegin < 1              ) params.jbegin = 1;
      if(params.jbegin > params.naxes[1]) params.jbegin = params.naxes[1];
      if(params.jend   > params.naxes[1]) params.jend   = params.naxes[1];
      if(params.jend   < 1              ) params.jend   = 1;

      if(mSubCube_debug)
      {
         printf("\npixMode = TRUE\n");
         printf("'ra'    = %-g\n", ra);
         printf("'dec'   = %-g\n", dec);
         printf("xsize   = %-g\n", xsize);
         printf("ysize   = %-g\n", ysize);
         printf("ibegin  = %d\n",  params.ibegin);
         printf("iend    = %d\n",  params.iend);
         printf("jbegin  = %d\n",  params.jbegin);
         printf("jend    = %d\n",  params.jend);
         fflush(stdout);
      }
   }
   
   else
   {
      /**********************************/
      /* Find the pixel location of the */
      /* sky coordinate specified       */
      /**********************************/

      convertCoordinates(EQUJ, 2000., ra, dec, sys, epoch, &lon, &lat, 0.);

      offscl = 0;

      wcs2pix(wcs, lon, lat, &xpix, &ypix, &offscl);

      mSubCube_fixxy(&xpix, &ypix, &offscl);

      /****** Skip this check: the location may be off the image but part of the region **********
      if(offscl == 1)
      {
         sprintf(returnStruct->msg, "Location is off image");
         return returnStruct;
      }
      ********************************************************************************************/

      if(mSubCube_debug)
      {
         printf("   ra   = %-g\n", ra);
         printf("   dec  = %-g\n", dec);
         printf("-> lon  = %-g\n", lon);
         printf("   lat  = %-g\n", lat);
         printf("-> xpix = %-g\n", xpix);
         printf("   ypix = %-g\n", ypix);
         fflush(stdout);
      }


      /************************************/
      /* Find the range of pixels to keep */
      /************************************/

      xoff = fabs(xsize/2./cdelt[0]);
      yoff = fabs(ysize/2./cdelt[1]);

      params.ibegin = xpix - xoff;
      params.iend   = params.ibegin + floor(2.*xoff + 1.0);

      params.jbegin = ypix - yoff;
      params.jend   = params.jbegin + floor(2.*yoff + 1.0);

      if(allPixels)
      {
         params.ibegin = 1;
         params.iend   = params.naxes[0];
         params.jbegin = 1;
         params.jend   = params.naxes[1];
      }

      if((   params.ibegin <              1
          && params.iend   <              1 )
      || (   params.ibegin > params.naxes[0]
          && params.iend   > params.naxes[0])
      || (   params.jbegin <              1
          && params.jend   <              1 )
      || (   params.jbegin > params.naxes[1]
          && params.jend   > params.naxes[1]))
      {
         sprintf(returnStruct->msg, "Region outside image.");
         return returnStruct;
      }

      if(params.ibegin < 1              ) params.ibegin = 1;
      if(params.ibegin > params.naxes[0]) params.ibegin = params.naxes[0];
      if(params.iend   > params.naxes[0]) params.iend   = params.naxes[0];
      if(params.iend   < 1              ) params.iend   = 1;

      if(params.jbegin < 1              ) params.jbegin = 1;
      if(params.jbegin > params.naxes[1]) params.jbegin = params.naxes[1];
      if(params.jend   > params.naxes[1]) params.jend   = params.naxes[1];
      if(params.jend   < 1              ) params.jend   = 1;

      if(mSubCube_debug)
      {
         printf("\npixMode = FALSE\n");
         printf("cdelt1  = %-g\n", cdelt[0]);
         printf("cdelt2  = %-g\n", cdelt[1]);
         printf("xsize   = %-g\n", xsize);
         printf("ysize   = %-g\n", ysize);
         printf("xoff    = %-g\n", xoff);
         printf("yoff    = %-g\n", yoff);
         printf("ibegin  = %d\n",  params.ibegin);
         printf("iend    = %d\n",  params.iend);
         printf("jbegin  = %d\n",  params.jbegin);
         printf("jend    = %d\n",  params.jend);
         fflush(stdout);
      }
   }

   if(params.ibegin > params.iend
   || params.jbegin > params.jend)
   {
      sprintf(returnStruct->msg, "No pixels match area.");
      return returnStruct;
   }

   if(params.iend - params.ibegin < 0
   && params.jend - params.jbegin < 0)
   {
      sprintf(returnStruct->msg, "Output area has no spatial extent.");
      return returnStruct;
   }

      
   params.nelements = params.iend - params.ibegin + 1;

   if(mSubCube_debug)
   {
      printf("ibegin    = %d\n",  params.ibegin);
      printf("iend      = %d\n",  params.iend);
      printf("nelements = %ld\n", params.nelements);
      printf("jbegin    = %d\n",  params.jbegin);
      printf("jend      = %d\n",  params.jend);
      fflush(stdout);
   }



   /**************************/
   /* Create the output file */
   /**************************/

   unlink(outfile);

   if(fits_create_file(&outfptr, outfile, &status))
   {
      sprintf(returnStruct->msg, "Can't create output file: %s", outfile);
      return returnStruct;
   }
   

   /********************************/
   /* Copy all the header keywords */
   /* from the input to the output */
   /********************************/

   if(mSubCube_debug)
   {
      printf("Calling mSubCube_copyHeaderInfo()\n");
      fflush(stdout);
   }

   mSubCube_copyHeaderInfo(infptr, outfptr, &params);


   /************************/
   /* Copy the data subset */
   /************************/


   if(mSubCube_debug)
   {
      printf("Calling mSubCube_copyData()\n");
      fflush(stdout);
   }

   if(mSubCube_copyData(infptr, outfptr, &params) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*******************/
   /* Close the files */
   /*******************/

   if(mSubCube_debug)
   {
      printf("Calling fits_close_file()\n");
      fflush(stdout);
   }

   if(fits_close_file(outfptr, &status))
      mSubCube_printFitsError(status);

   if(fits_close_file(infptr, &status))
      mSubCube_printFitsError(status);

   strcpy(warning, "");

   if(params.nrange[0] == 0 && params.nrange[1] == 0)
   {
      strcpy(warning, "Check CDELT, CRPIX values for axes 3 and 4.");

      sprintf(montage_msgstr, "content=\"%s\", warning=\"%s\"", content, warning);
      sprintf(montage_json, "{\"content\"=\"%s\", \"warning\"=\"%s\"}", content, warning);
   }
   else if(params.nrange[0] == 0 && params.nrange[1]  > 0)
   {
      strcpy(warning, "Check CDELT, CRPIX values for axis 4.");

      sprintf(montage_msgstr, "content=\"%s\", warning=\"%s\"", content, warning);
      sprintf(montage_json, "{\"content\"=\"%s\", \"warning\"=\"%s\"}", content, warning);
   }
   else
   {
      sprintf(montage_msgstr, "content=\"%s\"", content);
      sprintf(montage_json, "{\"content\":\"%s\"}", content);
   }

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   strcpy(returnStruct->content, content);
   strcpy(returnStruct->warning, warning);

   return returnStruct;
}


/**************************************************/
/*  Projections like CAR sometimes add an extra   */
/*  360 degrees worth of pixels to the return     */
/*  and call it off-scale.                        */
/**************************************************/

void mSubCube_fixxy(double *x, double *y, int *offscl)
{
   *x = *x - xcorrection;
   *y = *y - ycorrection;

   if(*x < 0.
   || *x > wcs->nxpix+1.
   || *y < 0.
   || *y > wcs->nypix+1.)
      *offscl = 1;

   return;
}


struct WorldCoor *mSubCube_getFileInfo(fitsfile *infptr, char *header[], struct mSubCubeParams *params)
{
   struct WorldCoor *wcs;
   int status = 0;

   if(fits_get_image_wcs_keys(infptr, header, &status))
      mSubCube_printFitsError(status);

   if(fits_read_key_lng(infptr, "NAXIS", &params->naxis, (char *)NULL, &status))
      mSubCube_printFitsError(status);
   
   if(fits_read_keys_lng(infptr, "NAXIS", 1, params->naxis, params->naxesin, &params->nfound, &status))
      mSubCube_printFitsError(status);
   
   params->naxes[0] = params->naxesin[0];
   params->naxes[1] = params->naxesin[1];

   if(params->naxis < 3 && strlen(params->dConstraint[0]) > 0)
   {
      sprintf(montage_msgstr, "D3 constraints set but this is a 2D image.");
      return (struct WorldCoor *)NULL;
   }

   if(params->naxis < 4 && strlen(params->dConstraint[1]) > 0)
   {
      sprintf(montage_msgstr, "D4 constraints set but this is a 3D datacube.");
      return (struct WorldCoor *)NULL;
   }

   if(params->naxis < 3)
      params->naxes[2] = 1;

   else if(params->naxes[2] == 0)
   {
      params->naxes[2] = params->naxesin[2];
      
      params->kbegin = 1;
      params->kend   = params->naxes[2];
   }

   else if(params->kend > params->naxesin[2])
   {
      sprintf(montage_msgstr, "Some select list values for axis 3 are greater than NAXIS3.");
      return (struct WorldCoor *)NULL;
   }

   if(params->naxis < 4)
      params->naxes[3] = 1;

   else if(params->naxes[3] == 0)
   {
      params->naxes[3] = params->naxesin[3];
      
      params->lbegin = 1;
      params->lend   = params->naxes[3];
   }

   else if(params->lend > params->naxesin[3])
   {
      sprintf(montage_msgstr, "Some select list values for axis 4 are greater than NAXIS4.");
      return (struct WorldCoor *)NULL;
   }


   /****************************************/
   /* Initialize the WCS transform library */
   /* and find the pixel location of the   */
   /* sky coordinate specified             */
   /****************************************/

   wcs = wcsinit(header[0]);

   params->isDSS = 0;
   if(wcs->prjcode == WCS_DSS)
      params->isDSS = 1;

   if(wcs == (struct WorldCoor *)NULL)
   {
      sprintf(montage_msgstr, "Output wcsinit() failed.");
      return wcs;
   }

   /* Extract the CRPIX and (equivalent) CDELT values */
   /* from the WCS structure                          */

   params->crpix[0] = wcs->xrefpix;
   params->crpix[1] = wcs->yrefpix;

   fits_read_key(infptr, TDOUBLE, "CRPIX3", &(params->crpix[2]), (char *)NULL, &status);
   status = 0;

   fits_read_key(infptr, TDOUBLE, "CRPIX4", &(params->crpix[3]), (char *)NULL, &status);
   status = 0;

   if(params->isDSS)
   {
      params->cnpix[0] = wcs->x_pixel_offset;
      params->cnpix[1] = wcs->y_pixel_offset;
   }
   return wcs;
}


int mSubCube_copyHeaderInfo(fitsfile *infptr, fitsfile *outfptr, struct mSubCubeParams *params)
{
   double tmp, tmp3, tmp4;
   int naxis2;
   int status = 0;
   
   if(fits_copy_header(infptr, outfptr, &status))
      mSubCube_printFitsError(status);


   /**********************/
   /* Update header info */
   /**********************/

   if(fits_update_key_lng(outfptr, "NAXIS", params->naxis,
                                  (char *)NULL, &status))
      mSubCube_printFitsError(status);

   if(fits_update_key_lng(outfptr, "NAXIS1", params->nelements,
                                  (char *)NULL, &status))
      mSubCube_printFitsError(status);

   naxis2 = params->jend - params->jbegin + 1;
   if(fits_update_key_lng(outfptr, "NAXIS2", naxis2,
                                  (char *)NULL, &status))
      mSubCube_printFitsError(status);

   if(params->isDSS)
   {
      tmp = params->cnpix[0] + params->ibegin - 1;

      if(fits_update_key_dbl(outfptr, "CNPIX1", tmp, -14,
                                     (char *)NULL, &status))
         mSubCube_printFitsError(status);

      tmp = params->cnpix[1] + params->jbegin - 1;

      if(fits_update_key_dbl(outfptr, "CNPIX2", tmp, -14,
                                     (char *)NULL, &status))
         mSubCube_printFitsError(status);
   }
   else
   {
      tmp = params->crpix[0] - params->ibegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX1", tmp, -14,
                                     (char *)NULL, &status))
         mSubCube_printFitsError(status);

      tmp = params->crpix[1] - params->jbegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX2", tmp, -14,
                                     (char *)NULL, &status))
         mSubCube_printFitsError(status);
   }

   if(params->naxis > 2)
   {
      if(fits_update_key_lng(outfptr, "NAXIS3", params->naxes[2],
                                     (char *)NULL, &status))
         mSubCube_printFitsError(status);

      tmp3 = params->crpix[2] - params->kbegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX3", tmp3, -14,
                                     (char *)NULL, &status))
         mSubCube_printFitsError(status);
   }

   if(params->naxis > 3)
   {
      if(fits_update_key_lng(outfptr, "NAXIS4", params->naxes[3],
                                     (char *)NULL, &status))
         mSubCube_printFitsError(status);

      tmp4 = params->crpix[3] - params->lbegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX4", tmp4, -14,
                                     (char *)NULL, &status))
         mSubCube_printFitsError(status);
   }

   if(mSubCube_debug)
   {
      printf("subCube> naxis1 -> %ld\n", params->nelements);
      printf("subCube> naxis2 -> %d\n",  naxis2);

      if(params->naxis > 2)
      {
         printf("subCube> naxis3 -> %ld\n",  params->naxes[2]);
         printf("subCube> crpix3 -> %-g\n",  tmp3);
      }

      if(params->naxis > 3)
      {
         printf("subCube> naxis4 -> %ld\n",  params->naxes[3]);
         printf("subCube> crpix4 -> %-g\n",  tmp4);
      }


      if(params->isDSS)
      {
         printf("subCube> cnpix1 -> %-g\n", params->cnpix[0]+params->ibegin-1);
         printf("subCube> cnpix2 -> %-g\n", params->cnpix[1]+params->jbegin-1);
      }
      else
      {
         printf("subCube> crpix1 -> %-g\n", params->crpix[0]-params->ibegin+1);
         printf("subCube> crpix2 -> %-g\n", params->crpix[1]-params->jbegin+1);
      }

      fflush(stdout);
   }

   return 0;
}


int mSubCube_copyData(fitsfile *infptr, fitsfile *outfptr, struct mSubCubeParams *params)
{
   long      fpixel[4], fpixelo[4];
   int       i, j, nullcnt;
   int       j3, j4, inRange;
   int       status = 0;

   double             *buffer_double,   refval_double;
   float              *buffer_float,    refval_float;
   unsigned long long *buffer_longlong, refval_longlong;
   unsigned long      *buffer_long,     refval_long;
   unsigned short     *buffer_short,    refval_short;
   unsigned char      *buffer_byte,     refval_byte;


   /*************************************************/
   /* Make a NaN value to use checking blank pixels */
   /*************************************************/

   union
   {
      double d8;
      float  d4[2];
      char   c[8];
   }
   value;

   double dnan;
   float  fnan;

   for(i=0; i<8; ++i)
      value.c[i] = 255;

   dnan = value.d8;
   fnan = value.d4[0];

   if(mSubCube_debug)
   {
      printf("copyData> lbegin, lend = %5d %5d\n", params->lbegin, params->lend);
      printf("copyData> kbegin, kend = %5d %5d\n", params->kbegin, params->kend);
      fflush(stdout);
   }

   fpixel[1] = params->jbegin;
   fpixel[2] = params->kbegin;

        if(bitpix == -64) buffer_double   = (double             *)malloc(params->nelements * sizeof(double));
   else if(bitpix == -32) buffer_float    = (float              *)malloc(params->nelements * sizeof(float));
   else if(bitpix ==  64) buffer_longlong = (unsigned long long *)malloc(params->nelements * sizeof(long long));
   else if(bitpix ==  32) buffer_long     = (unsigned long      *)malloc(params->nelements * sizeof(long));
   else if(bitpix ==  16) buffer_short    = (unsigned short     *)malloc(params->nelements * sizeof(short));
   else if(bitpix ==   8) buffer_byte     = (unsigned char      *)malloc(params->nelements * sizeof(char));

   fpixelo[1] = 1;

   isflat = 1;

   refval_double   = dnan;
   refval_float    = fnan;
   refval_longlong = blank;
   refval_long     = blank;
   refval_short    = blank;
   refval_byte     = blank;

   fpixel [0] = params->ibegin;  // Fixed
   fpixelo[0] = 1;               // Fixed

   fpixelo[3] = 1;

   if(bitpix > 0)
   {
      fits_set_bscale(infptr,  1., 0., &status);
      fits_set_bscale(outfptr, 1., 0., &status);
   }


   for (j4=params->lbegin; j4<=params->lend; ++j4)
   {
      fpixel[3] = j4;

      // If the dimension 4 value isn't in our range list,
      // we'll skip this one.

      if(params->nrange[1] > 0)
      {
         inRange = 0;

         for(i=0; i<params->nrange[1]; ++i)
         {
            if(params->range[1][i][1] == -1)
            {
               if(j4 == params->range[1][i][0])
               {
                  inRange = 1;
                  break;
               }
            }

            else
            {
               if(j4 >= params->range[1][i][0]
               && j4 <= params->range[1][i][1])
               {
                  inRange = 1;
                  break;
               }
            }
         }

         if(!inRange)
            continue;
      }


      // We want this dimension 3 value

      fpixelo[2] = 1;

      for (j3=params->kbegin; j3<=params->kend; ++j3)
      {
         fpixel[2] = j3;

         // If the dimension 3 value isn't in our range list,
         // we'll skip this one.

         if(params->nrange[0] > 0)
         {
            inRange = 0;

            for(i=0; i<params->nrange[0]; ++i)
            {
               if(params->range[0][i][1] == -1)
               {
                  if(j3 == params->range[0][i][0])
                  {
                     inRange = 1;
                     break;
                  }
               }

               else
               {
                  if(j3 >= params->range[0][i][0]
                  && j3 <= params->range[0][i][1])
                  {
                     inRange = 1;
                     break;
                  }
               }
            }

            if(!inRange)
               continue;
         }

         if(mSubCube_debug)
         {
            printf("copyData> Processing input 4/3  %5ld/%5ld",    fpixel[3],  fpixel[2]);
            printf(                     " to output %5ld/%5ld\n", fpixelo[3], fpixelo[2]);
            fflush(stdout);
         }


         // We want this dimension 4 value

         fpixelo[1] = 1;

         for (j=params->jbegin; j<=params->jend; ++j)
         {
            fpixel[1] = j;

            if(bitpix      == -64)
               fits_read_pix(infptr, TDOUBLE,   fpixel, params->nelements, &dnan, buffer_double,   &nullcnt, &status);
            else if(bitpix == -32)
               fits_read_pix(infptr, TFLOAT,    fpixel, params->nelements, &fnan, buffer_float,    &nullcnt, &status);
            else if(bitpix ==  64)
               fits_read_pix(infptr, TLONGLONG, fpixel, params->nelements,  NULL, buffer_longlong, &nullcnt, &status);
            else if(bitpix ==  32)
               fits_read_pix(infptr, TLONG,     fpixel, params->nelements,  NULL, buffer_long,     &nullcnt, &status);
            else if(bitpix ==  16)
               fits_read_pix(infptr, TSHORT,    fpixel, params->nelements,  NULL, buffer_short,    &nullcnt, &status);
            else if(bitpix ==   8)
               fits_read_pix(infptr, TBYTE,     fpixel, params->nelements,  NULL, buffer_byte,     &nullcnt, &status);

            if(status)
            {
               mSubCube_printFitsError(status);
               return 1;
            }

            if(bitpix == -64)
            {
               for(i=0; i<params->nelements; ++i)
               {
                  if(!mNaN(buffer_double[i]))
                  {
                     if(mNaN(refval_double))
                        refval_double = buffer_double[i];

                     if(buffer_double[i] != refval_double)
                        isflat = 0;
                  }
               }
            }

            else if(bitpix == -32)
            {
               for(i=0; i<params->nelements; ++i)
               {
                  if(!mNaN(buffer_float[i]))
                  {
                     if(mNaN(refval_float))
                        refval_float = buffer_float[i];

                     if(buffer_float[i] != refval_float)
                        isflat = 0;
                  }
               }
            }

            else if(bitpix == 64)
            {
               for(i=0; i<params->nelements; ++i)
               {
                  if(buffer_longlong[i] != refval_longlong)
                     isflat = 0;
               }
            }

            else if(bitpix == 32)
            {
               for(i=0; i<params->nelements; ++i)
               {
                  if(buffer_long[i] != refval_long)
                     isflat = 0;
               }
            }

            else if(bitpix == 16)
            {
               for(i=0; i<params->nelements; ++i)
               {
                  if(buffer_short[i] != refval_short)
                     isflat = 0;
               }
            }

            else if(bitpix == 8)
            {
               for(i=0; i<params->nelements; ++i)
               {
                  if(buffer_byte[i] != refval_byte)
                     isflat = 0;
               }
            }

            if(bitpix      == -64)
               fits_write_pix(outfptr, TDOUBLE,   fpixelo, params->nelements, (void *)buffer_double,   &status);
            else if(bitpix == -32)
               fits_write_pix(outfptr, TFLOAT,    fpixelo, params->nelements, (void *)buffer_float,    &status);
            else if(bitpix ==  64)
               fits_write_pix(outfptr, TLONGLONG, fpixelo, params->nelements, (void *)buffer_longlong, &status);
            else if(bitpix ==  32)
               fits_write_pix(outfptr, TLONG,     fpixelo, params->nelements, (void *)buffer_long,     &status);
            else if(bitpix ==  16)
               fits_write_pix(outfptr, TSHORT,    fpixelo, params->nelements, (void *)buffer_short,    &status);
            else if(bitpix ==   8)
               fits_write_pix(outfptr, TBYTE,     fpixelo, params->nelements, (void *)buffer_byte,     &status);

            if(status)
            {
               mSubCube_printFitsError(status);
               return 1;
            }

            ++fpixelo[1];
         }

         ++fpixelo[2];
      }

      ++fpixelo[3];
   }

        if(bitpix == -64) free(buffer_double);
   else if(bitpix == -32) free(buffer_float);
   else if(bitpix ==  64) free(buffer_longlong);
   else if(bitpix ==  32) free(buffer_long);
   else if(bitpix ==  16) free(buffer_short);
   else if(bitpix ==   8) free(buffer_byte);

   if(isflat)
   {
      if(mNaN(refval_double))
         strcpy(content, "blank");
      else
         strcpy(content, "flat");
   }
   else
      strcpy(content, "normal");

   return 0;
}


int mSubCube_dataRange(fitsfile *infptr, int *imin, int *imax, int *jmin, int *jmax)
{
   long    fpixel[4];
   long    naxis, naxes[10];
   int     i, j, nullcnt, nfound;
   int     j4, j3;
   double *buffer;

   int     status = 0;

   /*************************************************/
   /* Make a NaN value to use checking blank pixels */
   /*************************************************/

   union
   {
      double d;
      char   c[8];
   }
   value;

   double dnan;

   for(i=0; i<8; ++i)
      value.c[i] = 255;

   dnan = value.d;

   if(fits_read_key_lng(infptr, "NAXIS", &naxis, (char *)NULL, &status))
      mSubCube_printFitsError(status);
   
   if(fits_read_keys_lng(infptr, "NAXIS", 1, naxis, naxes, &nfound, &status))
      mSubCube_printFitsError(status);

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   *imin =  1000000000;
   *imax = -1;
   *jmin =  1000000000;
   *jmax = -1;

   buffer  = (double *)malloc(naxes[0] * sizeof(double));

   for (j4=1; j4<=naxes[3]; ++j4)
   {
      for (j3=1; j3<=naxes[2]; ++j3)
      {
         for (j=1; j<=naxes[1]; ++j)
         {
            if(mSubCube_debug)
            {
               printf("dataRange> input plane %5d/%5d, row %5d: \n", j4, j3, j);
               fflush(stdout);
            }

            if(fits_read_pix(infptr, TDOUBLE, fpixel, naxes[0], &dnan,
                             buffer, &nullcnt, &status))
               mSubCube_printFitsError(status);

            for(i=1; i<=naxes[0]; ++i)
            {
               if(mSubCube_debug && i<11)
                  printf(" %-g", buffer[i-1]);

               if(!mNaN(buffer[i-1]))
               {
                  if(buffer[i-1] != dnan)
                  {
                     if(i < *imin) *imin = i;
                     if(i > *imax) *imax = i;
                     if(j < *jmin) *jmin = j;
                     if(j > *jmax) *jmax = j;
                  }
               }
            }

            if(mSubCube_debug)
               printf("\n");

            ++fpixel[1];
         }

         ++fpixel[2];
      }

      ++fpixel[3];
   }

   free(buffer);

   return 0;
}


/*****************************/
/*                           */
/*  Parse D3/D4 select lists */
/*                           */
/*****************************/

int mSubCube_parseSelectList(int ind, struct mSubCubeParams *params)
{
   char *begin, *end, *split, *endstr, *ptr;

   char  list[MAXSTR];
   int   index, nrange, min, max;

   nrange = 0;

   index = ind - 3;

   if(index < 0 || index > 1)
   {
      sprintf(montage_msgstr, "Select list index can only be 3 or 4.");
      return 1;
   }

   strcpy(list, params->dConstraint[index]);

   endstr = list + strlen(list);

   begin = list;

   while(1)
   {
      min =  0;
      max = -1;

      while(*begin == ' ' && begin < endstr)
         ++begin;

      if(begin >= endstr)
         break;

      end = begin;

      while(*end != ',' && end < endstr)
         ++end;

      *end = '\0';

      split = begin;

      while(*split != ':' && split < end)
         ++split;

      if(*split == ':')
      {
         *split = '\0';
         ++split;
      }

      ptr = begin + strlen(begin) - 1;

      while(*ptr == ' ' && ptr >= begin) 
         *ptr = '\0';

      while(*split == ' ' && split >= end) 
         *split = '\0';

      ptr = split + strlen(split) - 1;

      while(*ptr == ' ' && ptr >= split) 
         *ptr = '\0';

      min = strtol(begin, &ptr, 10);

      if(ptr < begin + strlen(begin))
      {
         sprintf(montage_msgstr, "Invalid range string [%s].", begin);
         return 1;
      }

      if(split < end)
      {
         max = strtol(split, &ptr, 10);

         if(ptr < split + strlen(split))
         {
            sprintf(montage_msgstr, "Invalid range string [%s].", split);
            return 1;
         }
      }

      if(max != -1 && min > max)
      {
         sprintf(montage_msgstr, "Range max less than min.");
         return 1;
      }

      if(min < 1)
      {
         sprintf(montage_msgstr, "FITS index ranges cannot be less than one.");
         return 1;
      }

      params->range[index][nrange][0] = min;
      params->range[index][nrange][1] = max;

      ++nrange;

      begin = end;

      ++begin;

      if(begin >= endstr)
         break;
   }

   params->nrange[index] = nrange;

   return 0;
}


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mSubCube_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}
