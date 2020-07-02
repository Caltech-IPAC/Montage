/* Module: mSubimage.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
6.0      John Good        17Nov14  Cleanup to avoid compiler warnings, in proparation
                                   for new development cycle.
5.0      John Good        24Jun07  Add -c (shrinkwrap) mode where the
                                   subimage is chosen based on the range
                                   of non-blank pixels
4.6      John Good        24Jun07  Added correction for CAR projection error
4.5      John Good        15Sep06  Corrected usage message
4.4      John Good        08Sep05  Adjusted rounding so that subimages in
                                   different parts of the image will more
                                   often have the same number of pixels.
4.3      John Good        25Aug05  Added one more pixel in both X and Y so
                                   we can get the edges of the input and
                                   added -a flag to force getting the whole
                                   image (mostly for extracting whole HDUs)
4.2      John Good        04Jun05  Added output keyword ("content") to signal
                                   when the cutout region is blank or flat
4.1      John Good        16May05  Added checks for correct number of args
                                   in pixel range mode and for pixel ranges
                                   completely outside input image
4.0      John Good        13Feb05  Modified code to preserve original BITPIX
                                   (originally, we were converting to 64-bit)
                                   actually in subImage.c but we note it here
3.3      Loring Craymer   20Jan05  Factored out image extraction code
                                   (subImage.h, subImage.c) so that it could
                                   also be used in  a new version of mTile
                                   that properly handles DSS images.
3.2      John Good        02Dec04  Added special processing for DSS CNPIX 
3.1      John Good        03Aug04  Changed precision on CRPIX; was getting
                                   low-level round-off treated as precision
3.0      John Good        28Jun04  Changed to do our own subsetting.
                                   FITS library failing for large files.
2.1      John Good        07Jun04  Modified FITS key updating precision
2.0      John Good        25May04  Added "pixel mode" subsetting
1.5      John Good        25Aug03  Added status file processing
1.4      John Good        30Apr03  Removed the getopt() library use; it
                                   screws up on negative dec values.
1.3      John Good        30Apr03  Forgot to exit when no overlap. 
                                   Also, wrong message text for bad output
                                   file path.
1.2      John Good        22Mar03  Replaced wcsCheck with checkHdr
1.1      John Good        13Mar03  Added WCS header check and
                                   modified command-line processing
                                   to use getopt() library.  Check for
                                   valid image sizes. Added more informative
                                   FITS open error messages.  Removed
                                   extra printFitsError() call.
1.0      John Good        29Jan03  Baseline code

*/

/* Module: subImage.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.5      John Good        08Sep15  fits_read_pix() incorrect null value
1.4      John Good        24Jun07  Added shrinkwrap support
1.3      John Good        25Aug05  Bug fix: temporary variable tmp should
                                   have been a double for CRPIX calculations
1.2      John Good        04Jun05  Added logic to determine if cutout is 
                                   blank or flat (all pixels the same value)
1.1      John Good        02Feb05  Modified code to preserve original BITPIX
                                   (originally, we were converting to 64-bit)
1.0      Loring Craymer   2OJan03  Baseline code factored out of
                                   mSubImage.c (v 3.2)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <wcs.h>
#include <coord.h>

#include <mSubimage.h>
#include <montage.h>


static struct WorldCoor *wcs;

static double xcorrection;
static double ycorrection;

static int mSubimage_debug;
static int isflat;
static int bitpix;

int haveBlank;
static long blank;

static char content[128];


static char montage_msgstr[1024];


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
/*                         In IMGPIX mode (relative to CRPIX), the ra,   */
/*                         dec parameters are the offset of the          */
/*                         beginning pixel and xsize,ysize are the       */
/*                         offset of the end pixel.                      */
/*                                                                       */
/*   int    mode           Processing mode. The two main modes are       */
/*                         0 (SKY) and 1 (PIX), corresponding to cutouts */
/*                         are in sky coordinate or pixel space. The two */
/*                         other modes are 2 (HDU) and 3 (SHRINK), where */
/*                         the region parameters are ignored and you get */
/*                         back either a single HDU or an image that has */
/*                         had all the blank border pixels removed. Mode */
/*                         4 (IMGPIX) was added later as a variant of    */
/*                         PIX, where the coordinates are relative to    */
/*                         CRPIX1,CRPIX2.                                */
/*                                                                       */
/*   int    hdu            Optional HDU offset for input file            */
/*   int    nowcs          Indicates that the image has no WCS info      */
/*                         (only makes sense in PIX mode)                */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mSubimageReturn *mSubimage(char *infile, char *outfile, double ra, double dec, 
                                  double xsize, double ysize, int mode, int hdu, int nowcs, int debugin)
{
   fitsfile *infptr, *outfptr;

   int       i, offscl;
   double    cdelt[10];
   int       pixMode, shrinkWrap;
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

   char    *checkHdr;

   char     *header[2];

   int       status = 0;

   struct mSubimageParams params;

   struct mSubimageReturn *returnStruct;


   dtr = atan(1.)/45.;

   mSubimage_debug = debugin;

   pixMode    = 0;
   shrinkWrap = 0;
   
   if(mode == PIX)    pixMode    = 1;
   if(mode == IMGPIX) pixMode    = 2;
   if(mode == SHRINK) shrinkWrap = 1;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mSubimageReturn *)malloc(sizeof(struct mSubimageReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /****************************************/
   /* Open the (unsubsetted) original file */
   /* to get the WCS info                  */
   /****************************************/

   if (!pixMode && !nowcs) 
   {
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
      sprintf(returnStruct->msg, "Image file %s missing or invalid FITS", infile);
      return returnStruct;
   }

   if(hdu > 0)
   {
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

   if(mSubimage_debug)
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
      if(mSubimage_dataRange(infptr, &imin, &imax, &jmin, &jmax) > 0)
      {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      if(mSubimage_debug)
      {
         printf("imin  = %d\n", imin);
         printf("imax  = %d\n", imax);
         printf("jmin  = %d\n", jmin);
         printf("jmax  = %d\n", jmax);
         fflush(stdout);
      }
   }

   wcs = mSubimage_getFileInfo(infptr, header, &params);

   if (!nowcs) 
   {
      if(mSubimage_debug) 
      {
         printf("WCS handling\n");
         fflush(stdout);
      }

      if(wcs == (struct WorldCoor *)NULL)
      {
         strcpy(returnStruct->msg, "Input file invalid WCS.");
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

      if(mSubimage_debug)
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
      
      if(mSubimage_debug)
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
      if(mSubimage_debug) {
         printf("xsize= [%lf]\n", xsize);
         printf("ysize= [%lf]\n", ysize);

         printf("imin= [%d] imax = [%d]\n", imin, imax);
         printf("jmin= [%d] jmax = [%d]\n", jmin, jmax);
         fflush(stdout);
      }
 
      
      if(pixMode == 1)
      {
         params.ibegin = (int)ra;
         params.iend   = (int)(ra + xsize + 0.5);

         params.jbegin = (int)dec;
         params.jend   = (int)(dec + ysize + 0.5);
      }
      else  // pixMode = 2
      {
         params.ibegin = params.crpix[0] + ra;
         params.jbegin = params.crpix[1] + dec;

         params.iend   = params.crpix[0] + xsize + 0.5;
         params.jend   = params.crpix[1] + ysize + 0.5;
      }

      if(mSubimage_debug)
      {
         printf("\npixMode = %d\n", pixMode);
         printf("'ra'    = %-g\n", ra);
         printf("'dec'   = %-g\n", dec);
         printf("'xsize' = %-g\n", xsize);
         printf("'ysize' = %-g\n", ysize);
         printf("crpix1  = %-g\n", params.crpix[0]);
         printf("crpix2  = %-g\n", params.crpix[1]);
         printf("ibegin  = %d\n",  params.ibegin);
         printf("iend    = %d\n",  params.iend);
         printf("jbegin  = %d\n",  params.jbegin);
         printf("jend    = %d\n",  params.jend);
         fflush(stdout);
      }

      if(params.ibegin < 1              ) params.ibegin = 1;
      if(params.ibegin > params.naxes[0]) params.ibegin = params.naxes[0];
      if(params.iend   > params.naxes[0]) params.iend   = params.naxes[0];
      if(params.iend   < 1              ) params.iend   = 1;

      if(params.jbegin < 1              ) params.jbegin = 1;
      if(params.jbegin > params.naxes[1]) params.jbegin = params.naxes[1];
      if(params.jend   > params.naxes[1]) params.jend   = params.naxes[1];
      if(params.jend   < 1              ) params.jend   = 1;

      if(mSubimage_debug)
      {
         printf("\nclipped:\n");
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

      mSubimage_fixxy(&xpix, &ypix, &offscl);

      /****** Skip this check: the location may be off the image but part of the region **********
      if(offscl == 1)
      {
         sprintf(returnStruct->msg, "Location is off image");
         return returnStruct;
      }
      ********************************************************************************************/

      if(mSubimage_debug)
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

      if(mSubimage_debug)
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

      if(params.ibegin > params.iend
      || params.jbegin > params.jend)
      {
         sprintf(returnStruct->msg, "No pixels match area.");
         return returnStruct;
      }
   }

      
   params.nelements = params.iend - params.ibegin + 1;

   if(mSubimage_debug)
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

   if(mSubimage_debug)
   {
      printf("Calling copyHeaderInfo()\n");
      fflush(stdout);
   }

   mSubimage_copyHeaderInfo(infptr, outfptr, &params);


   /************************/
   /* Copy the data subset */
   /************************/


   if(mSubimage_debug)
   {
      printf("Calling copyData()\n");
      fflush(stdout);
   }

   if(mSubimage_copyData(infptr, outfptr, &params) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*******************/
   /* Close the files */
   /*******************/

   if(mSubimage_debug)
   {
      printf("Calling fits_close_file()\n");
      fflush(stdout);
   }

   if(fits_close_file(outfptr, &status))
   {
      mSubimage_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_close_file(infptr, &status))
   {
      mSubimage_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "content=\"%s\"",       content);
   sprintf(returnStruct->json, "{\"content\":\"%s\"}", content);

   strcpy(returnStruct->content, content);

   return returnStruct;
}


/**************************************************/
/*  Projections like CAR sometimes add an extra   */
/*  360 degrees worth of pixels to the return     */
/*  and call it off-scale.                        */
/**************************************************/

void mSubimage_fixxy(double *x, double *y, int *offscl)
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


struct WorldCoor *mSubimage_getFileInfo(fitsfile *infptr, char *header[], struct mSubimageParams *params)
{
   struct WorldCoor *wcs;
   int status = 0;
   int i;

   if(fits_get_image_wcs_keys(infptr, header, &status))
   {
      mSubimage_printFitsError(status);
      return (struct WorldCoor *)NULL;
   }

   if(fits_read_key_lng(infptr, "NAXIS", &params->naxis, (char *)NULL, &status))
   {
      mSubimage_printFitsError(status);
      return (struct WorldCoor *)NULL;
   }
   
   if(fits_read_keys_lng(infptr, "NAXIS", 1, params->naxis, params->naxes, &params->nfound, &status))
   {
      mSubimage_printFitsError(status);
      return (struct WorldCoor *)NULL;
   }
   
   if(mSubimage_debug)
   {
      for(i=0; i<params->naxis; ++i)
         printf("naxis%d = %ld\n",  i+1, params->naxes[i]);

      fflush(stdout);
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

   if(params->isDSS)
   {
      params->cnpix[0] = wcs->x_pixel_offset;
      params->cnpix[1] = wcs->y_pixel_offset;
   }

   return wcs;
}


int mSubimage_copyHeaderInfo(fitsfile *infptr, fitsfile *outfptr, struct mSubimageParams *params)
{
   double tmp;
   int naxis2;
   int status = 0;
   
   if(fits_copy_header(infptr, outfptr, &status))
   {
      mSubimage_printFitsError(status);
      return 1;
   }


   /**********************/
   /* Update header info */
   /**********************/

   if(fits_update_key_lng(outfptr, "NAXIS", 2,
                                  (char *)NULL, &status))
   {
      mSubimage_printFitsError(status);
      return 1;
   }

   if(fits_update_key_lng(outfptr, "NAXIS1", params->nelements,
                                  (char *)NULL, &status))
   {
      mSubimage_printFitsError(status);
      return 1;
   }

   naxis2 = params->jend - params->jbegin + 1;
   if(fits_update_key_lng(outfptr, "NAXIS2", naxis2,
                                  (char *)NULL, &status))
   {
      mSubimage_printFitsError(status);
      return 1;
   }

   if(params->isDSS)
   {
      tmp = params->cnpix[0] + params->ibegin - 1;

      if(fits_update_key_dbl(outfptr, "CNPIX1", tmp, -14,
                                     (char *)NULL, &status))
      {
         mSubimage_printFitsError(status);
         return 1;
      }

      tmp = params->cnpix[1] + params->jbegin - 1;

      if(fits_update_key_dbl(outfptr, "CNPIX2", tmp, -14,
                                     (char *)NULL, &status))
      {
         mSubimage_printFitsError(status);
         return 1;
      }
   }
   else
   {
      tmp = params->crpix[0] - params->ibegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX1", tmp, -14,
                                     (char *)NULL, &status))
      {
         mSubimage_printFitsError(status);
         return 1;
      }

      tmp = params->crpix[1] - params->jbegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX2", tmp, -14,
                                     (char *)NULL, &status))
      {
         mSubimage_printFitsError(status);
         return 1;
      }
   }

   if(mSubimage_debug)
   {
      printf("naxis1 -> %ld\n", params->nelements);
      printf("naxis2 -> %d\n",  naxis2);

      if(params->isDSS)
      {
         printf("cnpix1 -> %-g\n", params->cnpix[0]+params->ibegin-1);
         printf("cnpix2 -> %-g\n", params->cnpix[1]+params->jbegin-1);
      }
      else
      {
         printf("crpix1 -> %-g\n", params->crpix[0]-params->ibegin+1);
         printf("crpix2 -> %-g\n", params->crpix[1]-params->jbegin+1);
      }

      fflush(stdout);
   }

   return 0;
}


int mSubimage_copyData(fitsfile *infptr, fitsfile *outfptr, struct mSubimageParams *params)
{
   long       fpixel[4], fpixelo[4];
   int        i, j, nullcnt;
   int        status = 0;

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


   fpixel[0] = params->ibegin;
   fpixel[1] = params->jbegin;
   fpixel[2] = 1;
   fpixel[3] = 1;

        if(bitpix == -64) buffer_double   = (double             *)malloc(params->nelements * sizeof(double));
   else if(bitpix == -32) buffer_float    = (float              *)malloc(params->nelements * sizeof(float));
   else if(bitpix ==  64) buffer_longlong = (unsigned long long *)malloc(params->nelements * sizeof(long long));
   else if(bitpix ==  32) buffer_long     = (unsigned long      *)malloc(params->nelements * sizeof(long));
   else if(bitpix ==  16) buffer_short    = (unsigned short     *)malloc(params->nelements * sizeof(short));
   else if(bitpix ==   8) buffer_byte     = (unsigned char      *)malloc(params->nelements * sizeof(char));

   fpixelo[0] = 1;
   fpixelo[1] = 1;
   fpixelo[2] = 1;
   fpixelo[3] = 1;

   isflat = 1;

   refval_double   = dnan;
   refval_float    = fnan;
   refval_longlong = blank;
   refval_long     = blank;
   refval_short    = blank;
   refval_byte     = blank;

   for (j=params->jbegin; j<=params->jend; ++j)
   {
      if(mSubimage_debug >= 2)
      {
         printf("Processing input image row %5d\n", j);
         fflush(stdout);
      }

      if(bitpix > 0)
         fits_set_bscale(infptr, 1., 0., &status);

      if(bitpix      == -64) 
         fits_read_pix(infptr, TDOUBLE,   fpixel, params->nelements, &dnan, buffer_double,   &nullcnt, &status);
      else if(bitpix == -32) 
         fits_read_pix(infptr, TFLOAT,    fpixel, params->nelements, &dnan, buffer_float,    &nullcnt, &status);
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
         mSubimage_printFitsError(status);
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
         mSubimage_printFitsError(status);
         return 1;
      }

      ++fpixelo[1];
      ++fpixel [1];
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


int mSubimage_dataRange(fitsfile *infptr, int *imin, int *imax, int *jmin, int *jmax)
{
   long    fpixel[4];
   long    naxis, naxes[10];
   int     i, j, nullcnt, nfound;
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

   double nan;

   for(i=0; i<8; ++i)
      value.c[i] = 255;

   nan = value.d;

   if(fits_read_key_lng(infptr, "NAXIS", &naxis, (char *)NULL, &status))
   {
      mSubimage_printFitsError(status);
      return 1;
   }
   
   if(fits_read_keys_lng(infptr, "NAXIS", 1, naxis, naxes, &nfound, &status))
   {
      mSubimage_printFitsError(status);
      return 1;
   }

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   *imin =  1000000000;
   *imax = -1;
   *jmin =  1000000000;
   *jmax = -1;

   buffer  = (double *)malloc(naxes[0] * sizeof(double));

   for (j=1; j<=naxes[1]; ++j)
   {
      if(mSubimage_debug >= 2)
      {
         printf("Processing image row %5d\n", j);
         fflush(stdout);
      }

      if(fits_read_pix(infptr, TDOUBLE, fpixel, naxes[0], &nan,
                       buffer, &nullcnt, &status))
      {
         mSubimage_printFitsError(status);
         return 1;
      }

      for(i=1; i<=naxes[0]; ++i)
      {
         if(!mNaN(buffer[i-1]))
         {
            if(buffer[i-1] != nan)
            {
               if(i < *imin) *imin = i;
               if(i > *imax) *imax = i;
               if(j < *jmin) *jmin = j;
               if(j > *jmax) *jmax = j;
            }
         }
      }

      ++fpixel [1];
   }

   free(buffer);

   return 0;
}


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mSubimage_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}
