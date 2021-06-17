/* Module: mProjectQL.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        12Oct15  Baseline code.  Based on mProject but maintaining
                                   input in memory rather than output and using a 
                                   "nearest" neighbor based algorithm rather than 
                                   flux conserving.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>

#include <fitsio.h>
#include <wcs.h>
#include <coord.h>

#include <mProjectQL.h>
#include <montage.h>

#define MAXSTR  256
#define MAXFILE 256

#ifndef NAN
#define NAN (0.0 / 0.0)
#endif

#define FIXEDBORDER 0
#define POLYBORDER  1

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)


static int     hdu;
static int     haveWeights;

static double offset;

static char   area_file[MAXSTR];

static double dtr;

static int  mProjectQL_debug;


/* Optional border polygon data */
int nborder;

typedef struct
{
   int x, y;
}
BorderPoint;

static BorderPoint polygon[256];


/* Structure used to store relevant */
/* information about a FITS file    */

static struct
{
   fitsfile         *fptr;
   long              naxes[2];
   struct WorldCoor *wcs;
   int               sys;
   double            epoch;
   int               clockwise;
}
input, weight, output, output_area;

static double *buffer   = (double *)NULL;
static double *area     = (double *)NULL;

static double **data    = (double **)NULL;
static double **weights = (double **)NULL;

static double **lanczos = (double **)NULL;

static int    nfilter, noAreas;

static double cnpix1, cnpix2;
static double crpix1, crpix2;

static int    isDSS = 0;

static double xcorrection;
static double ycorrection;

static double xcorrectionIn;
static double ycorrectionIn;

static time_t currtime, start;


static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mProjectQL                                                           */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mProject, processes a single input image and            */
/*  projects it onto the output space.  Other Montage modules do this    */
/*  with full flux-conserving accuracy but pay the price in speed.       */
/*  This module produces a quick approximation which is nevertheless     */
/*  adquate even for some science applications.                          */
/*                                                                       */
/*  Each output pixel is project to the input pixel space and its value  */
/*  approximated based on surrounding input pixels via interpolation.    */
/*  Unlike the other modules, no area file is generated since no         */
/*  overlap areas are determined.  When coadding overlapping images      */
/*  later, this will effect the accuracy of edge pixels but this cannot  */
/*  be helped.                                                           */
/*                                                                       */
/*  The input can come from from arbitrarily disparate sources.  It is   */
/*  assumed that the flux scales in the input images match, but this is  */
/*  not required (leading to some interesting combinations).             */
/*                                                                       */
/*   char  *input_file     FITS file to reproject                        */
/*   char  *output_file    Reprojected FITS file                         */
/*   char  *template_file  FITS header file used to define the desired   */
/*                         output                                        */
/*                                                                       */
/*   int    hdu            Optional HDU offset for input file            */
/*   int    interp         Interpolation scheme for value lookup.        */
/*                         Currently NEAREST or LANCZOS.                 */
/*                                                                       */
/*   char  *weight_file    Optional pixel weight FITS file (must match   */
/*                         input)                                        */
/*                                                                       */
/*   double fixedWeight    A weight value used for all pixels            */
/*   double threshold      Pixels with values below this level treated   */
/*                         as blank                                      */
/*                                                                       */
/*   char  *borderstr      Optional string that contains either a border */
/*                         width or comma-separated 'x1,y1,x2,y2, ...'   */
/*                         pairs defining a pixel region polygon where   */
/*                         we keep only the data inside.                 */
/*                                                                       */
/*   double fluxScale      Scale factor applied to all pixels            */
/*                                                                       */
/*   int    expand         Expand output image area to include all of    */
/*                         the input pixels                              */
/*                                                                       */
/*   int    fullRegion     Do not 'shrink-wrap' output area to non-blank */
/*   int    noAreas        In the interest of speed, generation of area  */
/*                         images is turned off.  This turns it back on. */
/*                         pixels                                        */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mProjectQLReturn *mProjectQL(char *input_file, char *ofile, char *template_file, int hduin, int interp,
                                    char *weight_file, double fixedWeight, double threshold, char *borderstr,
                                    double fluxScale, int expand, int fullRegion, int noAreasin, int debugin)
{
   int       i, j;
   int       imx, jmy;
   int       imgi, imgj, keri, kerj;
   int       ix, jy;
   int       nullcnt;
   int       hpx, hpxPix, hpxLevel;
   long      fpixel[4], nelements;
   double    lon, lat;
   double    ixpix, iypix;
   double    oxpix, oypix;
   double    oxpixTest, oypixTest;
   double    oxpixMin, oypixMin;
   double    oxpixMax, oypixMax;
   double    xoff, yoff;
   int       imin, imax, jmin, jmax;
   int       ibmin, ibmax, ibfound;
   int       istart, ilength;
   int       jstart, jlength;
   double    xpos, ypos;
   int       offscl, border, bordertype;

   int       status;

   char      output_file[MAXSTR];

   char     *end;

   char     *checkHdr;

   double    pi, x, a;
   int       ia;

   int       nsamp;

   struct mProjectQLReturn *returnStruct;

   input.fptr       = (fitsfile *)NULL;
   weight.fptr      = (fitsfile *)NULL;
   output.fptr      = (fitsfile *)NULL;
   output_area.fptr = (fitsfile *)NULL;

   input.wcs  = (struct WorldCoor *)NULL;
   output.wcs = (struct WorldCoor *)NULL;

   hpx    = 0;
   hpxPix = 0;

   
   /*************************************************/
   /* Initialize output FITS basic image parameters */
   /*************************************************/

   int  bitpix = DOUBLE_IMG; 
   long naxis  = 2;  



   /************************************************/
   /* Make a NaN value to use setting blank pixels */
   /************************************************/

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


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mProjectQLReturn *)malloc(sizeof(struct mProjectQLReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /*********************************/
   /* Make the Lanczos filter array */
   /*********************************/

   pi = atan(1.) * 4.;

   ia = 3;

   a = ia;

   nsamp = 100;

   nfilter = (int) (a * nsamp);

   lanczos = (double **) malloc(nfilter * sizeof(double *));

   for(i=0; i<nfilter; ++i)
   {
      lanczos[i] = (double *) malloc(nfilter * sizeof(double));

      for(j=0; j<nfilter; ++j)
         lanczos[i][j] = 0.;
   }

   lanczos[0][0] = 1.;

   for(j=1; j<nfilter; ++j)
   {
      x = a * (double) j / (double) nfilter;

      lanczos[0][j] = sin(pi*x) / (pi*x) * sin(pi*x/a) / (pi*x/a);
   }
   
   
   for(j=1; j<nfilter; ++j)
      for(i=0; i<nfilter; ++i)
         lanczos[j][i] = lanczos[0][j] * lanczos[0][i];

   
   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mProjectQL_debug = debugin;

   noAreas = noAreasin;
   
   hdu = hduin;

   strcpy(output_file, ofile);

   if(fixedWeight == 0.)
      fixedWeight = 1.;

   if(fluxScale == 0.)
      fluxScale = 1.;

   dtr = atan(1.)/45.;

   time(&currtime);
   start = currtime;

   border     = 0;
   bordertype = FIXEDBORDER;

   if(borderstr)
   {
      border = strtol(borderstr, &end, 10);

      if(end < borderstr + strlen(borderstr))
      {
         if(mProjectQL_BorderSetup(borderstr) <= 3)
         {
            sprintf(returnStruct->msg, "Border value string (%s) cannot be interpreted as an integer or a set of polygon vertices",
               borderstr);
            mProjectQL_cleanup();
            return returnStruct;
         }
         else
         {
            border = 0;
            bordertype = POLYBORDER;
         }
      }
   }

   if(border < 0)
   {
      sprintf(returnStruct->msg, "Border value (%d) must be greater than or equal to zero", border);
      mProjectQL_cleanup();
      return returnStruct;
   }

   haveWeights = 0;
   if(weight_file && strlen(weight_file) > 0)
      haveWeights = 1;

   checkHdr = montage_checkHdr(input_file, 0, hdu);

   if(checkHdr)
   {
      strcpy(returnStruct->msg, checkHdr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   checkHdr = montage_checkHdr(template_file, 1, 0);

   if(checkHdr)
   {
      strcpy(returnStruct->msg, checkHdr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   if(strlen(output_file) > 5 &&
      strncmp(output_file+strlen(output_file)-5, ".FITS", 5) == 0)
         output_file[strlen(output_file)-5] = '\0';
      
   else if(strlen(output_file) > 5 &&
      strncmp(output_file+strlen(output_file)-5, ".fits", 5) == 0)
         output_file[strlen(output_file)-5] = '\0';
      
   else if(strlen(output_file) > 4 &&
      strncmp(output_file+strlen(output_file)-4, ".FIT", 4) == 0)
         output_file[strlen(output_file)-4] = '\0';
      
   else if(strlen(output_file) > 4 &&
      strncmp(output_file+strlen(output_file)-4, ".fit", 4) == 0)
         output_file[strlen(output_file)-4] = '\0';
      
   strcpy(area_file,     output_file);
   strcat(output_file,  ".fits");
   strcat(area_file,    "_area.fits");

   if(mProjectQL_debug >= 1)
   {
      printf("\ninput_file    = [%s]\n", input_file);

      if(haveWeights)
         printf("weight_file   = [%s]\n", weight_file);

      printf("input_file    = [%s]\n", input_file);
      printf("output_file   = [%s]\n", output_file);
      printf("area_file     = [%s]\n", area_file);
      printf("template_file = [%s]\n\n", template_file);
      fflush(stdout);
   }


   /*******************************/
   /* Read the input image header */
   /*******************************/

   if(mProjectQL_debug >= 1)
   {
      time(&currtime);
      printf("\nStarting to process pixels (time %.0f)\n\n", 
         (double)(currtime - start));
      fflush(stdout);
   }

   if(mProjectQL_readFits(input_file, weight_file) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   if(mProjectQL_debug >= 1)
   {
      printf("input.naxes[0]   =  %ld\n",  input.naxes[0]);
      printf("input.naxes[1]   =  %ld\n",  input.naxes[1]);
      printf("input.sys        =  %d\n",   input.sys);
      printf("input.epoch      =  %-g\n",  input.epoch);
      printf("input.clockwise  =  %d\n",   input.clockwise);
      printf("input proj       =  %s\n\n", input.wcs->ptype);

      fflush(stdout);
   }

   offset = 0.;


   /*************************************************/ 
   /* Process the output header template to get the */ 
   /* image size, coordinate system and projection  */ 
   /*************************************************/ 

   mProjectQL_readTemplate(template_file);

   if(mProjectQL_debug >= 1)
   {
      printf("\nOriginal template\n");
      printf("\noutput.naxes[0]  =  %ld\n", output.naxes[0]);
      printf("output.naxes[1]  =  %ld\n",   output.naxes[1]);
      printf("output.sys       =  %d\n",    output.sys);
      printf("output.epoch     =  %-g\n",   output.epoch);
      printf("output.clockwise =  %d\n",    output.clockwise);
      printf("output proj      =  %s\n",    output.wcs->ptype);

      fflush(stdout);
   }

   if(expand)
   {
      /* We need to expand the output area so we get all of the input image. */
      /* This implies rereading the template as well.                        */

      offset = (sqrt(input.naxes[0]*input.naxes[0] * input.wcs->xinc*input.wcs->xinc
                   + input.naxes[1]*input.naxes[1] * input.wcs->yinc*input.wcs->yinc));

      if(mProjectQL_debug >= 1)
      {
         printf("\nexpand output template by %-g degrees on all sides\n\n", offset);
         fflush(stdout);
      }

      offset = offset / sqrt(output.wcs->xinc * output.wcs->xinc + output.wcs->yinc * output.wcs->yinc);

      if(mProjectQL_debug >= 1)
      {
         printf("\nexpand output template by %-g pixels on all sides\n\n", offset);
         fflush(stdout);
      }

      mProjectQL_readTemplate(template_file);

      if(mProjectQL_debug >= 1)
      {
         printf("\nExpanded template\n");
         printf("\noutput.naxes[0]  =  %ld\n", output.naxes[0]);
         printf("output.naxes[1]  =  %ld\n",   output.naxes[1]);
         printf("output.sys       =  %d\n",    output.sys);
         printf("output.epoch     =  %-g\n",   output.epoch);
         printf("output.clockwise =  %d\n",    output.clockwise);
         printf("output proj      =  %s\n",    output.wcs->ptype);

         fflush(stdout);
      }
   }


   /**********************************************************/
   /* Check the WCS header to see if we need to set up       */
   /* special HPX handling.  The projection type tells us    */
   /* if this is HPX and the pixel size tells us what order. */
   /**********************************************************/

   if(strcmp(output.wcs->ptype, "HPX") == 0)
   {
      hpx = 1;

      hpxPix = 90.0 / fabs(output.wcs->xinc) / sqrt(2.0) + 0.5;

      hpxLevel = log10((double)hpxPix)/log10(2.) + 0.5;

      hpxPix = pow(2., (double)hpxLevel) + 0.5;
      
      hpxPix = 4 * hpxPix;
   }


   /******************************************************************/
   /* Create the data and area buffers for one line of output pixels */
   /******************************************************************/

   buffer = (double *)malloc(output.naxes[0] * sizeof(double));

   if(!noAreas)
      area = (double *)malloc(output.naxes[0] * sizeof(double));


   /************************************************/
   /* Go around the outside of the INPUT image,    */
   /* finding the range of output pixel locations  */
   /************************************************/

   oxpixMin =  100000000;
   oxpixMax = -100000000;
   oypixMin =  100000000;
   oypixMax = -100000000;

   /* Check input left and right */

   for (j=0; j<input.naxes[1]; ++j)
   {
      pix2wcs(input.wcs, 0.5, j+0.5, &xpos, &ypos);

      convertCoordinates(input.sys, input.epoch, xpos, ypos,
                         output.sys, output.epoch, &lon, &lat, 0.0);
      

      offscl = input.wcs->offscl;

      if(!offscl)
         wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);


      // Special check for HPX.  This allows us to fill in the wraparound area in the
      // lower left and upper right of the projection.

      if(hpx)
      {
         offscl = 0;

         if(oxpix < -(double)hpxPix/2.) oxpix += (double)hpxPix;
         if(oxpix >  (double)hpxPix/2.) oxpix -= (double)hpxPix;

         if(oypix < -(double)hpxPix/2.) oypix += (double)hpxPix;
         if(oypix >  (double)hpxPix/2.) oypix -= (double)hpxPix;
      }

      mProjectQL_fixxy(&oxpix, &oypix, &offscl);

      if(!offscl)
      {
         if(oxpix < oxpixMin) oxpixMin = oxpix;
         if(oxpix > oxpixMax) oxpixMax = oxpix;
         if(oypix < oypixMin) oypixMin = oypix;
         if(oypix > oypixMax) oypixMax = oypix;
      }

      pix2wcs(input.wcs, input.naxes[0]+0.5, j+0.5, &xpos, &ypos);

      convertCoordinates(input.sys, input.epoch, xpos, ypos,
                         output.sys, output.epoch, &lon, &lat, 0.0);
      
      offscl = input.wcs->offscl;

      if(!offscl)
         wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

      if(hpx)
      {
         offscl = 0;
         
         if(oxpix < -(double)hpxPix/2.) oxpix += (double)hpxPix;
         if(oxpix >  (double)hpxPix/2.) oxpix -= (double)hpxPix;

         if(oypix < -(double)hpxPix/2.) oypix += (double)hpxPix;
         if(oypix >  (double)hpxPix/2.) oypix -= (double)hpxPix;
      }

      mProjectQL_fixxy(&oxpix, &oypix, &offscl);

      if(!offscl)
      {
         if(oxpix < oxpixMin) oxpixMin = oxpix;
         if(oxpix > oxpixMax) oxpixMax = oxpix;
         if(oypix < oypixMin) oypixMin = oypix;
         if(oypix > oypixMax) oypixMax = oypix;
      }
   }


   /* Check input top and bottom */

   for (i=0; i<input.naxes[0]; ++i)
   {
      pix2wcs(input.wcs, i+0.5, 0.5, &xpos, &ypos);

      convertCoordinates(input.sys, input.epoch, xpos, ypos,
                         output.sys, output.epoch, &lon, &lat, 0.0);
      
      offscl = input.wcs->offscl;

      if(!offscl)
         wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

      if(hpx)
      {
         offscl = 0;

         if(oxpix < -(double)hpxPix/2.) oxpix += (double)hpxPix;
         if(oxpix >  (double)hpxPix/2.) oxpix -= (double)hpxPix;

         if(oypix < -(double)hpxPix/2.) oypix += (double)hpxPix;
         if(oypix >  (double)hpxPix/2.) oypix -= (double)hpxPix;
      }

      mProjectQL_fixxy(&oxpix, &oypix, &offscl);

      if(!offscl)
      {
         if(oxpix < oxpixMin) oxpixMin = oxpix;
         if(oxpix > oxpixMax) oxpixMax = oxpix;
         if(oypix < oypixMin) oypixMin = oypix;
         if(oypix > oypixMax) oypixMax = oypix;
      }

      pix2wcs(input.wcs, i+0.5, input.naxes[1]+0.5, &xpos, &ypos);

      convertCoordinates(input.sys, input.epoch, xpos, ypos,
                         output.sys, output.epoch, &lon, &lat, 0.0);
      
      offscl = input.wcs->offscl;

      if(!offscl)
         wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

      if(hpx)
      {
         offscl = 0;

         if(oxpix < -(double)hpxPix/2.) oxpix += (double)hpxPix;
         if(oxpix >  (double)hpxPix/2.) oxpix -= (double)hpxPix;

         if(oypix < -(double)hpxPix/2.) oypix += (double)hpxPix;
         if(oypix >  (double)hpxPix/2.) oypix -= (double)hpxPix;
      }

      mProjectQL_fixxy(&oxpix, &oypix, &offscl);

      if(!offscl)
      {
         if(oxpix < oxpixMin) oxpixMin = oxpix;
         if(oxpix > oxpixMax) oxpixMax = oxpix;
         if(oypix < oypixMin) oypixMin = oypix;
         if(oypix > oypixMax) oypixMax = oypix;
      }
   }


   /************************************************/
   /* Go around the outside of the OUTPUT image,   */
   /* finding the range of output pixel locations  */
   /************************************************/

   /* 
    * Check output left and right 
    */

   for (j=0; j<output.naxes[1]; j++) {
     oxpix = 0.5;
     oypix = (double)j+0.5;
     mProjectQL_UpdateBounds (oxpix, oypix, &oxpixMin, &oxpixMax, &oypixMin, &oypixMax);
     oxpix = (double)output.naxes[0]+0.5;
     mProjectQL_UpdateBounds (oxpix, oypix, &oxpixMin, &oxpixMax, &oypixMin, &oypixMax);
   }

   /* 
    * Check output top and bottom 
    */

   for (i=0; i<output.naxes[0]; i++) {
     oxpix = (double)i+0.5;
     oypix = 0.5;
     mProjectQL_UpdateBounds (oxpix, oypix, &oxpixMin, &oxpixMax, &oypixMin, &oypixMax);
     oypix = (double)output.naxes[1]+0.5;
     mProjectQL_UpdateBounds (oxpix, oypix, &oxpixMin, &oxpixMax, &oypixMin, &oypixMax);
   }

   /*
    * ASSERT: Output bounding box now specified by
    *   (oxpixMin, oxpixMax, oypixMin, oypixMax)
    */

   if(fullRegion)
   {
      oxpixMin = 0.5;
      oxpixMax = output.naxes[0]+0.5;

      oypixMin = 0.5;
      oypixMax = output.naxes[1]+0.5;
   }

   istart = oxpixMin - 1;

   if(istart < 0) 
      istart = 0;
   
   ilength = oxpixMax - oxpixMin + 2;

   if(ilength > output.naxes[0])
      ilength = output.naxes[0];


   jstart = oypixMin - 1;

   if(jstart < 0) 
   if(istart < 0) 
      istart = 0;
   
   ilength = oxpixMax - oxpixMin + 2;

   if(ilength > output.naxes[0])
      ilength = output.naxes[0];


   jstart = oypixMin - 1;

   if(jstart < 0) 
      jstart = 0;
   
   jlength = oypixMax - oypixMin + 2;

   if(jlength > output.naxes[1])
      jlength = output.naxes[1];

   if(mProjectQL_debug >= 2)
   {
      printf("\nOutput range:\n");
      printf(" oxpixMin = %-g\n", oxpixMin);
      printf(" oxpixMax = %-g\n", oxpixMax);
      printf(" oypixMin = %-g\n", oypixMin);
      printf(" oypixMax = %-g\n", oypixMax);
      printf(" istart   = %-d\n", istart);
      printf(" ilength  = %-d\n", ilength);
      printf(" jstart   = %-d\n", jstart);
      printf(" jlength  = %-d\n", jlength);
      fflush(stdout);
   }

   if(oxpixMin > oxpixMax || oypixMin > oypixMax)
   {
      sprintf(returnStruct->msg, "No overlap");
      mProjectQL_cleanup();
      return returnStruct;
   }


   // In mProject/mProjectPP, we write the image out after having
   // populated the array in memory.  This allows us the strip off
   // blank areas around the outside, so the pixel range can end up
   // smaller than the oxpix min/max values we calculated earlier.
   // Here we don't have that luxury, but we will maintain the 
   // same variable names even though they are just duplicates in
   // value.

   imin = oxpixMin;
   imax = oxpixMax;
   jmin = oypixMin;
   jmax = oypixMax;

    

   /**********************************************/ 
   /* Allocate memory for the input image pixels */ 
   /**********************************************/ 

   data = (double **)malloc(input.naxes[1] * sizeof(double *));

   if(data == (void *)NULL)
   {
      sprintf(returnStruct->msg, "Not enough memory for input data image array");
      mProjectQL_cleanup();
      return returnStruct;
   }

   for(j=0; j<input.naxes[1]; j++)
   {
      data[j] = (double *)malloc(input.naxes[0] * sizeof(double));

      if(data[j] == (void *)NULL)
      {
         sprintf(returnStruct->msg, "Not enough memory for input data image array");
         mProjectQL_cleanup();
         return returnStruct;
      }
   }

   if(mProjectQL_debug >= 1)
   {
      printf("\n%lu bytes allocated for image pixel values\n", input.naxes[0] * input.naxes[1] * sizeof(double));
      fflush(stdout);
   }


   /********************************************************/ 
   /* Allocate memory for the input pixel weights (if any) */ 
   /********************************************************/ 

   if(haveWeights)
   {
      weights = (double **)malloc(input.naxes[1] * sizeof(double *));

      if(weights == (void *)NULL)
      {
         sprintf(returnStruct->msg, "Not enough memory for input weights array");
         mProjectQL_cleanup();
         return returnStruct;
      }

      for(j=0; j<input.naxes[1]; j++)
      {
         weights[j] = (double *)malloc(input.naxes[0] * sizeof(double));                               

         if(weights[j] == (void *)NULL)
         {
            sprintf(returnStruct->msg, "Not enough memory for input weights array");
            mProjectQL_cleanup();
            return returnStruct;
         }
      }

      if(mProjectQL_debug >= 1)
      {
         printf("%lu bytes allocated for pixel weights\n", ilength * jlength * sizeof(double));
         fflush(stdout);
      }
   }


   /*********************************/
   /* Read in input image / weights */
   /*********************************/

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   status = 0;

   nelements = input.naxes[0];

   for (j=0; j<input.naxes[1]; ++j)
   {
      if(mProjectQL_debug == 2)
      {
         printf("\rReading input row %5d  ", j);
         fflush(stdout);
      }
      else if(mProjectQL_debug)
      {
         printf("Reading input row %5d\n", j);
         fflush(stdout);
      }

      if(fits_read_pix(input.fptr, TDOUBLE, fpixel, nelements, &nan,
                       data[j], &nullcnt, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      if(haveWeights)
      {
         if(fits_read_pix(weight.fptr, TDOUBLE, fpixel, nelements, &nan,
                          weights[j], &nullcnt, &status))
         {
            mProjectQL_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            mProjectQL_cleanup();
            return returnStruct;
         }
      }

      ++fpixel[1];
   }

   if(fits_close_file(input.fptr, &status))
   {
      mProjectQL_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   input.fptr = (fitsfile *)NULL;

   if(haveWeights)
   {
      if(fits_close_file(weight.fptr, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      weight.fptr = (fitsfile *)NULL;
   }


   /*****************************************************/
   /* If we have a border, change border values to NaNs */
   /*****************************************************/

   if(bordertype == FIXEDBORDER && border > 0)
   {
      for(j=0; j<input.naxes[1]; ++j)
      {
         for(i=0; i<input.naxes[0]; ++i)
         {
            if(j<border || j>input.naxes[1]-border
            || i<border || i>input.naxes[0]-border)
               data[j][i] = nan;
         }
      }
   }

   else if(bordertype == POLYBORDER)
   {
      for(j=0; j<input.naxes[1]; ++j)
      {
         ibfound = mProjectQL_BorderRange(j, input.naxes[0]-1, &ibmin, &ibmax);

         for(i=0; i<input.naxes[0]; ++i)
         {
            if(ibfound == 0 || (i >= ibmin && i <= ibmax))
               data[j][i] = nan;
         }
      }
   }
      

   /********************************/
   /* Create the output FITS files */
   /********************************/

   remove(output_file);               

   remove(area_file);               

   if(fits_create_file(&output.fptr, output_file, &status)) 
   {
      mProjectQL_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   if(!noAreas)
   {
      if(fits_create_file(&output_area.fptr, area_file, &status)) 
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);

      }
   }


   /*********************************************************/
   /* Create the FITS image.  All the required keywords are */
   /* handled automatically.                                */
   /*********************************************************/

   if (fits_create_img(output.fptr, bitpix, naxis, output.naxes, &status))
   {
      mProjectQL_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   if(mProjectQL_debug >= 1)
   {
      printf("\nFITS data image created (not yet populated)\n"); 
      fflush(stdout);
   }

   if(!noAreas)
   {
      if (fits_create_img(output_area.fptr, bitpix, naxis, output_area.naxes, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      if(mProjectQL_debug >= 1)
      {
         printf("FITS area image created (not yet populated)\n"); 
         fflush(stdout);
      }
   }


   /****************************************/
   /* Set FITS header from a template file */
   /****************************************/

   if(fits_write_key_template(output.fptr, template_file, &status))
   {
      mProjectQL_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   if(mProjectQL_debug >= 1)
   {
      printf("Template keywords written to FITS data image\n"); 
      fflush(stdout);
   }

   if(!noAreas)
   {
      if(fits_write_key_template(output_area.fptr, template_file, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      if(mProjectQL_debug >= 1)
      {
         printf("Template keywords written to FITS area image\n\n"); 
         fflush(stdout);
      }
   }


   /***************************/
   /* Modify BITPIX to be -64 */
   /***************************/

   if(fits_update_key_lng(output.fptr, "BITPIX", -64,
                                  (char *)NULL, &status))
   {
      mProjectQL_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   if(!noAreas)
      if(fits_update_key_lng(output_area.fptr, "BITPIX", -64,
                                     (char *)NULL, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }


   /***************************************************/
   /* Update NAXIS, NAXIS1, NAXIS2, CRPIX1 and CRPIX2 */
   /***************************************************/


   if(fits_update_key_lng(output.fptr, "NAXIS", 2,
                                  (char *)NULL, &status))
   {
      mProjectQL_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS1", imax-imin,
                                  (char *)NULL, &status))
   {
      mProjectQL_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS2", jmax-jmin,
                                  (char *)NULL, &status))
   {
      mProjectQL_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   if(isDSS)
   {
      if(fits_update_key_dbl(output.fptr, "CNPIX1", cnpix1+imin, -14,
                                     (char *)NULL, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      if(fits_update_key_dbl(output.fptr, "CNPIX2", cnpix2+jmin, -14,
                                     (char *)NULL, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }
   }
   else
   {
      if(fits_update_key_dbl(output.fptr, "CRPIX1", crpix1-imin, -14,
                                     (char *)NULL, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      if(fits_update_key_dbl(output.fptr, "CRPIX2", crpix2-jmin, -14,
                                     (char *)NULL, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }
   }



   if(!noAreas)
   {
      if(fits_update_key_lng(output_area.fptr, "NAXIS", 2,
                                     (char *)NULL, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      if(fits_update_key_lng(output_area.fptr, "NAXIS1", imax-imin,
                                     (char *)NULL, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      if(fits_update_key_lng(output_area.fptr, "NAXIS2", jmax-jmin,
                                     (char *)NULL, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      if(isDSS)
      {
         if(fits_update_key_dbl(output_area.fptr, "CNPIX1", cnpix1+imin, -14,
                                        (char *)NULL, &status))
         {
            mProjectQL_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            mProjectQL_cleanup();
            return returnStruct;
         }

         if(fits_update_key_dbl(output_area.fptr, "CNPIX2", cnpix2+jmin, -14,
                                        (char *)NULL, &status))
         {
            mProjectQL_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            mProjectQL_cleanup();
            return returnStruct;
         }
      }
      else
      {
         if(fits_update_key_dbl(output_area.fptr, "CRPIX1", crpix1-imin, -14,
                                        (char *)NULL, &status))
         {
            mProjectQL_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            mProjectQL_cleanup();
            return returnStruct;
         }

         if(fits_update_key_dbl(output_area.fptr, "CRPIX2", crpix2-jmin, -14,
                                        (char *)NULL, &status))
         {
            mProjectQL_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            mProjectQL_cleanup();
            return returnStruct;
         }
      }
   }


   if(mProjectQL_debug)
   {
      printf("Template keywords BITPIX, CRPIX, and NAXIS updated\n");
      fflush(stdout);
   }




   /* Now we loop over the output image, determining */
   /* and writing out a row at a time.               */

   fpixel[0] = 1;
   fpixel[1] = 1;

   nelements = imax - imin;

   for(j=jmin; j<jmax; ++j)
   {
      for(i=0; i<nelements; ++i)
      {
         buffer[i] = nan;

         if(!noAreas)
            area[i] = nan;
      }

      for(i=imin; i<imax; ++i)
      {
         buffer[i] = nan;

         oxpix = i+1.0;  // Since the first pixel in a FITS image (index 0)
         oypix = j+1.0;  // is at coordinate 1 according to the WCS library

         pix2wcs(output.wcs, oxpix, oypix, &xpos, &ypos);


         // Convert it back to make sure we weren't off scale

         if(!hpx)
         {
            offscl = output.wcs->offscl;

            oxpixTest = 999.;
            oypixTest = 999.;

            if(!offscl)
               wcs2pix(output.wcs, xpos, ypos, &oxpixTest, &oypixTest, &offscl);


            // The offscl parameter seems to be too sensitive in some
            // cases, so we will replace it with the following.

            offscl = 0;

            if(fabs(oxpixTest - oxpix) > 1.)
               offscl = 1;

            if(fabs(oypixTest - oypix) > 1.)
               offscl = 1;

            if(offscl)
               continue;
         }


         // Convert to the input coordinate system

         convertCoordinates(output.sys, output.epoch, xpos, ypos,
                            input.sys, input.epoch, &lon, &lat, 0.0);
         

         // Convert to input pixel space

         offscl = 0;

         wcs2pix(input.wcs, lon, lat, &ixpix, &iypix, &offscl);

         if(!hpx)
         {
            if(offscl)
               continue;
         }

         ixpix = ixpix - 1.0;  // Similarly, the input pixel location is 
         iypix = iypix - 1.0;  // one offset from the array index

         ix = (int)(ixpix+0.5);  // The extra 0.5 here is to make it round
         jy = (int)(iypix+0.5);  // correctly to the nearest integer value


         // If using nearest neighbor

         if(ix >= 0 && ix < input.naxes[0]
         && jy >= 0 && jy < input.naxes[1])
         {
            if(interp == NEAREST)
            {
               buffer[i-imin] = data[jy][ix];

               if(!noAreas)
                  area[i-imin] = 1.;
            }

            else if(interp == LANCZOS)
            {
               if(mNaN(data[jy][ix]))
                  buffer[i-imin] = data[jy][ix];
               else
               {
                  xoff = ixpix - ix;
                  yoff = iypix - jy;

                  buffer[i-imin] = 0.;

                  for(imx=-ia; imx<=ia; ++imx)
                  {
                     imgi = ix + imx;
                     keri = abs((imx+xoff)*nsamp);

                     if(imgi < 0 || imgi > input.naxes[0]
                     || keri < 0 || keri > nfilter)
                        continue;
                        
                     for(jmy=-ia; jmy<=ia; ++jmy)
                     {
                        imgj = jy + jmy;
                        kerj = abs((jmy+yoff)*nsamp);

                        if(imgj < 0 || imgj >= input.naxes[1]
                        || kerj < 0 || kerj >= nfilter)
                           continue;
                           
                        buffer[i-imin] += data[imgj][imgi] * lanczos[kerj][keri];
                     }
                  }
               }
            }
         }

         else
            buffer[i-imin] = nan;
      }


      /*********************************/
      /* Write the image and area data */
      /*********************************/

      if (fits_write_pix(output.fptr, TDOUBLE, fpixel, nelements, 
                         (void *)buffer, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      if(!noAreas)
         if (fits_write_pix(output_area.fptr, TDOUBLE, fpixel, nelements,
                            (void *)area, &status))
         {
            mProjectQL_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            mProjectQL_cleanup();
            return returnStruct;
         }

      ++fpixel[1];
   }

   if(mProjectQL_debug >= 1)
   {
      printf("Data written to FITS data (and area) images\n\n"); 
      fflush(stdout);
   }

   if(mProjectQL_debug >= 1)
   {
      time(&currtime);
      printf("\n\nDone processing pixels (%.0f seconds)\n\n",
         (double)(currtime - start));
      fflush(stdout);
   }


   /***********************/
   /* Close the FITS file */
   /***********************/

   if(fits_close_file(output.fptr, &status))
   {
      mProjectQL_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      mProjectQL_cleanup();
      return returnStruct;
   }

   output.fptr = (fitsfile *)NULL;

   if(mProjectQL_debug >= 1)
   {
      printf("FITS data image finalized\n"); 
      fflush(stdout);
   }

   if(!noAreas)
   {
      if(fits_close_file(output_area.fptr, &status))
      {
         mProjectQL_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         mProjectQL_cleanup();
         return returnStruct;
      }

      output_area.fptr = (fitsfile *)NULL;

      if(mProjectQL_debug >= 1)
      {
         printf("FITS area image finalized\n\n"); 
         fflush(stdout);
      }
   }

   time(&currtime);

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "time=%.1f",       (double)(currtime - start));
   sprintf(returnStruct->json, "{\"time\":%.1f}", (double)(currtime - start));

   returnStruct->time = (double)(currtime - start);

   mProjectQL_cleanup();
   return returnStruct;
}


/**************************************************/
/*  Projections like CAR sometimes add an extra   */
/*  360 degrees worth of pixels to the return     */
/*  and call it off-scale.                        */
/**************************************************/

void mProjectQL_fixxy(double *x, double *y, int *offscl)
{
   *x = *x - xcorrection;
   *y = *y - ycorrection;

   if(*x < 0.
   || *x > output.wcs->nxpix+1.
   || *y < 0.
   || *y > output.wcs->nypix+1.)
      *offscl = 1;

   return;
}


/**************************************************/
/*                                                */
/*  Read the output header template file.         */
/*  Specifically extract the image size info.     */
/*  Also, create a single-string version of the   */
/*  header data and use it to initialize the      */
/*  output WCS transform.                         */
/*                                                */
/**************************************************/

int mProjectQL_readTemplate(char *filename)
{
   int       j;

   FILE     *fp;

   char      line[MAXSTR];

   char      header[80000];

   int       sys;
   double    epoch;

   double    x, y;
   double    ix, iy;
   double    xpos, ypos;
   int       offscl;


   /********************************************************/
   /* Open the template file, read and parse all the lines */
   /********************************************************/

   fp = fopen(filename, "r");

   if(fp == (FILE *)NULL)
   {
      mProjectQL_printError("Template file not found.");
      return 1;
   }

   strcpy(header, "");

   for(j=0; j<1000; ++j)
   {
      if(fgets(line, MAXSTR, fp) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';
      
      if(line[strlen(line)-1] == '\r')
         line[strlen(line)-1]  = '\0';

      if(mProjectQL_debug >= 3)
      {
         printf("Template line: [%s]\n", line);
         fflush(stdout);
      }

      mProjectQL_parseLine(line);

      mProjectQL_stradd(header, line);
   }

   fclose(fp);


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   if(mProjectQL_debug >= 3)
   {
      printf("Output Header to wcsinit():\n%s\n", header);
      fflush(stdout);
   }

   output.wcs = wcsinit(header);

   if(output.wcs == (struct WorldCoor *)NULL)
   {
      sprintf(montage_msgstr, "Output wcsinit() failed.");
      return 1;
   }

   output_area.wcs = output.wcs;


   /* Kludge to get around bug in WCS library:   */
   /* 360 degrees sometimes added to pixel coord */

   ix = (output.wcs->nxpix)/2.;
   iy = (output.wcs->nypix)/2.;

   pix2wcs(output.wcs, ix, iy, &xpos, &ypos);

   offscl = output.wcs->offscl;

   if(!offscl)
      wcs2pix(output.wcs, xpos, ypos, &x, &y, &offscl);

   xcorrection = 0;
   ycorrection = 0;

   if(!offscl)
   {
      xcorrection = x-ix;
      ycorrection = y-iy;
   }

   if(mProjectQL_debug)
   {
      printf("xcorrection = %.2f\n", xcorrection);
      printf(" ycorrection = %.2f\n\n", ycorrection);
      fflush(stdout);
   }


   /*************************************/
   /*  Set up the coordinate transform  */
   /*************************************/

   if(output.wcs->syswcs == WCS_J2000)
   {
      sys   = EQUJ;
      epoch = 2000.;

      if(output.wcs->equinox == 1950.)
         epoch = 1950;
   }
   else if(output.wcs->syswcs == WCS_B1950)
   {
      sys   = EQUB;
      epoch = 1950.;

      if(output.wcs->equinox == 2000.)
         epoch = 2000;
   }
   else if(output.wcs->syswcs == WCS_GALACTIC)
   {
      sys   = GAL;
      epoch = 2000.;
   }
   else if(output.wcs->syswcs == WCS_ECLIPTIC)
   {
      sys   = ECLJ;
      epoch = 2000.;

      if(output.wcs->equinox == 1950.)
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

   output.sys   = sys;
   output.epoch = epoch;

   output_area.sys   = sys;
   output_area.epoch = epoch;


   /***************************************************/
   /*  Determine whether these pixels are 'clockwise' */
   /* or 'counterclockwise'                           */
   /***************************************************/

   output.clockwise = 0;

   if((output.wcs->xinc < 0 && output.wcs->yinc < 0)
   || (output.wcs->xinc > 0 && output.wcs->yinc > 0)) output.clockwise = 1;

   if(strcmp(output.wcs->c1type, "DEC") == 0
   || output.wcs->c1type[strlen(output.wcs->c1type)-1] == 'T')
      output.clockwise = !output.clockwise;

   if(mProjectQL_debug >= 3)
   {
      if(output.clockwise)
         printf("Output pixels are clockwise.\n");
      else
         printf("Output pixels are counterclockwise.\n");
   }

   return 0;
}


/**********************************************/
/*                                            */
/*  Parse header lines from the template,     */
/*  looking for NAXIS1, NAXIS2, CRPIX1 CRPIX2 */
/*                                            */
/**********************************************/

int mProjectQL_parseLine(char *linein)
{
   char *keyword;
   char *value;
   char *end;

   int   len;

   char  line[MAXSTR];

   strcpy(line, linein);

   len = strlen(line);

   keyword = line;

   while(*keyword == ' ' && keyword < line+len)
      ++keyword;
   
   end = keyword;

   while(*end != ' ' && *end != '=' && end < line+len)
      ++end;

   value = end;

   while((*value == '=' || *value == ' ' || *value == '\'')
         && value < line+len)
      ++value;
   
   *end = '\0';
   end = value;

   if(*end == '\'')
      ++end;

   while(*end != ' ' && *end != '\'' && end < line+len)
      ++end;
   
   *end = '\0';

   if(mProjectQL_debug >= 2)
   {
      printf("keyword [%s] = value [%s]\n", keyword, value);
      fflush(stdout);
   }


   if(strcmp(keyword, "NAXIS1") == 0)
   {
      output.naxes[0]      = atoi(value) + 2 * offset;
      output_area.naxes[0] = atoi(value) + 2 * offset;

      sprintf(linein, "NAXIS1  = %ld", output.naxes[0]);
   }

   if(strcmp(keyword, "NAXIS2") == 0)
   {
      output.naxes[1]      = atoi(value) + 2 * offset;
      output_area.naxes[1] = atoi(value) + 2 * offset;

      sprintf(linein, "NAXIS2  = %ld", output.naxes[1]);
   }

   if(strcmp(keyword, "CRPIX1") == 0)
   {
      crpix1 = atof(value) + offset;

      sprintf(linein, "CRPIX1  = %11.6f", crpix1);
   }

   if(strcmp(keyword, "CRPIX2") == 0)
   {
      crpix2 = atof(value) + offset;

      sprintf(linein, "CRPIX2  = %11.6f", crpix2);
   }

   return 0;
}


/**************************************************/
/*                                                */
/*  Read a FITS file and extract some of the      */
/*  header information.                           */
/*                                                */
/**************************************************/

int mProjectQL_readFits(char *filename, char *weightfile)
{
   int       status;

   char     *header;

   char      errstr[MAXSTR];

   int       sys;
   double    epoch;
   
   double    x, y;
   double    ix, iy;
   double    xpos, ypos;
   int       offscl;

   status = 0;

   
   /*****************************************/
   /* Open the FITS file and get the header */
   /* for WCS setup                         */
   /*****************************************/

   if(fits_open_file(&input.fptr, filename, READONLY, &status))
   {
      sprintf(errstr, "Image file %s missing or invalid FITS", filename);
      mProjectQL_printError(errstr);
      return 1;
   }

   if(hdu > 0)
   {
      if(fits_movabs_hdu(input.fptr, hdu+1, NULL, &status))
      {
         mProjectQL_printFitsError(status);
         return 1;
      }
   }

   if(fits_get_image_wcs_keys(input.fptr, &header, &status))
   {
      mProjectQL_printFitsError(status);
      return 1;
   }


   /*******************************************/
   /* Check for EPOCH used instead of EQUINOX */
   /*******************************************/

   char   *ptr;
   char    replace[80];
   int     ir;

   if(strstr(header, "EQUINOX = ") == (char *)NULL)
   {
      if((ptr = strstr(header, "EPOCH   =")) != (char *)NULL)
      {
         epoch = atof(ptr+9);

         if(epoch == 1950.)
         {
            strcpy(replace, "EQUINOX");

            for(ir=0; ir<strlen(replace); ++ir)
               *(ptr + ir) = replace[ir];
         }
       }     
    }     


   /************************/
   /* Open the weight file */
   /************************/

   if(haveWeights)
   {
      if(fits_open_file(&weight.fptr, weightfile, READONLY, &status))
      {
         sprintf(errstr, "Weight file %s missing or invalid FITS", weightfile);
         mProjectQL_printError(errstr);
         return 1;
      }

      if(hdu > 0)
      {
         if(fits_movabs_hdu(weight.fptr, hdu+1, NULL, &status))
         {
            mProjectQL_printFitsError(status);
            return 1;
         }
      }
   }


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   input.wcs = wcsinit(header);

   if(input.wcs == (struct WorldCoor *)NULL)
   {
      sprintf(montage_msgstr, "Input wcsinit() failed.");
      return 1;
   }

   input.naxes[0] = input.wcs->nxpix;
   input.naxes[1] = input.wcs->nypix;


   /* Kludge to get around bug in WCS library:   */
   /* 360 degrees sometimes added to pixel coord */

   ix = (input.wcs->nxpix)/2.;
   iy = (input.wcs->nypix)/2.;

   pix2wcs(input.wcs, ix, iy, &xpos, &ypos);

   offscl = input.wcs->offscl;

   if(!offscl)
      wcs2pix(input.wcs, xpos, ypos, &x, &y, &offscl);

   xcorrectionIn = 0;
   ycorrectionIn = 0;

   if(!offscl)
   {
      xcorrectionIn = x-ix;
      ycorrectionIn = y-iy;
   }

   if(mProjectQL_debug)
   {
      printf("xcorrectionIn = %.2f\n", xcorrectionIn);
      printf(" ycorrectionIn = %.2f\n\n", ycorrectionIn);
      fflush(stdout);
   }
   

   /***************************************************/
   /*  Determine whether these pixels are 'clockwise' */
   /* or 'counterclockwise'                           */
   /***************************************************/

   input.clockwise = 0;

   if((input.wcs->xinc < 0 && input.wcs->yinc < 0)
   || (input.wcs->xinc > 0 && input.wcs->yinc > 0)) input.clockwise = 1;

   if(strcmp(input.wcs->c1type, "DEC") == 0
   || input.wcs->c1type[strlen(input.wcs->c1type)-1] == 'T')
      input.clockwise = !input.clockwise;

   if(isDSS)
      input.clockwise = 0;

   if(mProjectQL_debug >= 3)
   {
      if(input.clockwise)
         printf("Input pixels are clockwise.\n");
      else
         printf("Input pixels are counterclockwise.\n");
   }


   /*************************************/
   /*  Set up the coordinate transform  */
   /*************************************/

   if(input.wcs->syswcs == WCS_J2000)
   {
      sys   = EQUJ;
      epoch = 2000.;

      if(input.wcs->equinox == 1950.)
         epoch = 1950;
   }
   else if(input.wcs->syswcs == WCS_B1950)
   {
      sys   = EQUB;
      epoch = 1950.;

      if(input.wcs->equinox == 2000.)
         epoch = 2000;
   }
   else if(input.wcs->syswcs == WCS_GALACTIC)
   {
      sys   = GAL;
      epoch = 2000.;
   }
   else if(input.wcs->syswcs == WCS_ECLIPTIC)
   {
      sys   = ECLJ;
      epoch = 2000.;

      if(input.wcs->equinox == 1950.)
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

   input.sys   = sys;
   input.epoch = epoch;

   free(header);

   return 0;
}


/************************************/
/*                                  */
/*  Make sure FITS files are closed */
/*                                  */
/************************************/

void mProjectQL_cleanup()
{
   int     status, i;

   if(input.fptr != (fitsfile *)NULL)
      fits_close_file(input.fptr, &status);

   if(weight.fptr != (fitsfile *)NULL)
      fits_close_file(weight.fptr, &status);

   if(output.fptr != (fitsfile *)NULL)
      fits_close_file(output.fptr, &status);

   if(output_area.fptr != (fitsfile *)NULL)
      fits_close_file(output_area.fptr, &status);


   input.fptr       = (fitsfile *)NULL;
   weight.fptr      = (fitsfile *)NULL;
   output.fptr      = (fitsfile *)NULL;
   output_area.fptr = (fitsfile *)NULL;

   if(input.wcs)
      wcsfree(input.wcs);

   if(output.wcs)
      wcsfree(output.wcs);

   if(lanczos)
   {
      for(i=0; i<nfilter; ++i)
         free(lanczos[i]);

      free(lanczos);
   }


   if(area)   free(area);
   if(buffer) free(buffer);


   if(data)
   {
      for(i=0; i<input.naxes[1]; ++i)
         free(data[i]);

      free(data);
   }


   if(weights)
   {
      for(i=0; i<input.naxes[1]; ++i)
         free(weights[i]);

      free(weights);
   }

   lanczos = (double **)NULL;
   area    = (double  *)NULL;
   buffer  = (double  *)NULL;
   data    = (double **)NULL;
   weights = (double **)NULL;

   return;
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mProjectQL_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}



/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void mProjectQL_printError(char *msg)
{
   strcpy(montage_msgstr, msg);
}



/* stradd adds the string "card" to a header line, and */
/* pads the header out to 80 characters.               */

int mProjectQL_stradd(char *header, char *card)
{
   int i;

   int hlen = strlen(header);
   int clen = strlen(card);

   for(i=0; i<clen; ++i)
      header[hlen+i] = card[i];

   if(clen < 80)
      for(i=clen; i<80; ++i)
         header[hlen+i] = ' ';
   
   header[hlen+80] = '\0';

   return(strlen(header));
}


/*
 * Given an image coordinate in the output image, (oxpix,oypix), update
 * output bounding box (oxpixMin,oypixMax,oypixMin,oypixMax) to include 
 * (oxpix,oypix) only if that coordinate maps in the bounds of the input 
 * image.
 */

void mProjectQL_UpdateBounds (double oxpix, double oypix,
                              double *oxpixMin, double *oxpixMax,
                              double *oypixMin, double *oypixMax)
{
  double xpos, ypos;   /* sky coordinate in output space */
  double lon, lat;     /* sky coordinates in input epoch */
  double ixpix, iypix; /* image coordinates in input space */
  int offscl;          /* out of input image bounds flag */
  /*
   * Convert output image coordinates to sky coordinates
   */
  pix2wcs (output.wcs, oxpix, oypix, &xpos, &ypos);

  convertCoordinates (output.sys, output.epoch, xpos, ypos,
                      input.sys, input.epoch, &lon, &lat, 0.0);

  /* 
   * Convert sky coordinates to input image coordinates
   */
  offscl = output.wcs->offscl;

  if(!offscl)
  {
     wcs2pix (input.wcs, lon, lat, &ixpix, &iypix, &offscl);

     ixpix = ixpix - xcorrectionIn;
     iypix = iypix - ycorrectionIn;

     if(ixpix < 0.
     || ixpix > input.wcs->nxpix
     || iypix < 0.
     || iypix > input.wcs->nypix)
        offscl = 1;
  }


  /*
   * Update output bounding box if in bounds
   */
  if (!offscl) {
    if (oxpix < *oxpixMin) *oxpixMin = oxpix;
    if (oxpix > *oxpixMax) *oxpixMax = oxpix;
    if (oypix < *oypixMin) *oypixMin = oypix;
    if (oypix > *oypixMax) *oypixMax = oypix;
  }
}



/* Boundary polygon handling */

int mProjectQL_BorderSetup(char *strin) 
{
   int   len;
   char  str[8192];
   char *ptr, *end;

   nborder = 0;

   strcpy(str, strin);

   if(mProjectQL_debug >= 3)
   {
      printf("Polygon string: [%s]\n", str);

      fflush(stdout);
   }

   ptr = str;

   len = strlen(str);

   while(*ptr == ' ' && ptr < str+len)
      ++ptr;

   end = ptr;

   while(1)
   {
      if(ptr >= str+len)
         break;


      /* find the next x coordinate */

      while(*end != ' ' &&  *end != ',' && end < str+len)
         ++end;

      *end = '\0';


      polygon[nborder].x = atoi(ptr);


      /* find the corresponding y coordinate */

      ptr = end+1;

      while(*ptr == ' ' && ptr < str+len)
         ++ptr;

      if(ptr >= str+len)
         break;

      end = ptr;

      while(*end != ' ' &&  *end != ',' && end < str+len)
         ++end;

      *end = '\0';

      polygon[nborder].y = atoi(ptr);

      if(mProjectQL_debug)
      {
         printf("Polygon border  %3d: %6d %6d\n",
            nborder,
            polygon[nborder].x,
            polygon[nborder].y);

         fflush(stdout);
      }

      ++nborder;

      ptr = end+1;
   }

   return nborder;
}


int mProjectQL_BorderRange(int jrow, int maxpix,
                           int *imin, int *imax)
{
   int          i, found;
   double       xinters, y;
   BorderPoint  p1, p2;
   double       xmin, xmax;

   y = (double)jrow;

   found = 0;
   p1    = polygon[0];
   xmin  = (double)maxpix + 1.;
   xmax  = 0.;

   for (i=1; i<=nborder; ++i)
   {
      p2 = polygon[i % nborder];

      if(y > MIN((double)p1.y, (double)p2.y))
      {
         if(y < MAX((double)p1.y, (double)p2.y))
         {
            found = 1;

            xinters =  (double)(y-p1.y)*(double)(p2.x-p1.x)
                      /(double)(p2.y-p1.y)+(double)p1.x;

            xmin = MIN(xmin, xinters);
            xmax = MAX(xmax, xinters);
         }
      }

      p1 = p2;
   }

   if(found)
   {
      *imin = (int)(xmin);

      if(*imin < 0) *imin = 0;

      *imax = (int)(xmax + 0.5);

      if(*imax > maxpix) *imax = maxpix;
   }

   else
   {
      *imin = 0;
      *imax = maxpix;
   }

   return found;
}
