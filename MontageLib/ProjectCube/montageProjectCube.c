/* Module: mCubeProj.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        29Jan15  Baseline code, based on mProject at that time.

*/

/* Module: overlapArea.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.2      John Good        03Nov04  Make sure nv never exceeds max (16)
1.1      John Good        25Nov03  Don't exit if segments are disjoint
1.0      John Good        29Jan03  Baseline code

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

#include <mProjectCube.h>
#include <montage.h>

#define MAXSTR  256
#define MAXFILE 256

#define FALSE             0
#define TRUE              1
#define FOREVER           1

#define COLINEAR_SEGMENTS 0
#define ENDPOINT_ONLY     1
#define NORMAL_INTERSECT  2
#define NO_INTERSECTION   3

#define CLOCKWISE         1
#define PARALLEL          0
#define COUNTERCLOCKWISE -1

#define UNKNOWN           0
#define P_IN_Q            1
#define Q_IN_P            2

#ifndef NAN
#define NAN (0.0 / 0.0)
#endif


static int    hdu;
static int    haveWeights;

static double offset;

static char   area_file[MAXSTR];


/* The two pixel polygons on the sky */
/* and the polygon of intersection   */

static Vec P[8], Q[8], V[16];

static int np = 4;
static int nq = 4;
static int nv;

static double pi, dtr;

static double tolerance = 4.424e-9;  /* sin(x) where x = 5e-4 arcsec */
                              /* or cos(x) when x is within   */
                              /* 1e-5 arcsec of 90 degrees    */

static int    inRow,  inColumn;
static int    outRow, outColumn;

static int    mProjectCube_debug;


/* Structure used to store relevant */
/* information about a FITS file    */

static struct
{
   fitsfile         *fptr;
   long              naxis;
   long              naxes[4];
   struct WorldCoor *wcs;
   int               sys;
   double            epoch;
   int               clockwise;
}
input, weight, output, output_area;


static double crpix1, crpix2;

static double refArea;


/* Structure contains the geometric       */
/* information for an input pixel         */
/* coordinate in (lat,lon), three-vector, */
/* and output pixel terms                 */

struct Ipos
{
   double lon;
   double lat;

   double x;
   double y;
   double z;

   double oxpix;
   double oypix;

   int    offscl;
};

static struct Ipos *topl, *bottoml;
static struct Ipos *topr, *bottomr;
static struct Ipos *postmp;

static double xcorrection;
static double ycorrection;

static double xcorrectionIn;
static double ycorrectionIn;

static time_t currtime, start;


static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mProjectCube                                                         */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mProjectCube, processes a single input image and        */
/*  projects it onto the output space.  It's output is actually a pair   */
/*  of FITS files, one for the sky flux the other for the fractional     */
/*  pixel coverage. Once this has been done for all input images,        */
/*  mAdd can be used to coadd them into a composite output.              */
/*                                                                       */
/*  Each input pixel is projected onto the output pixel space and the    */
/*  exact area of overlap is computed.  Both the total 'flux' and the    */
/*  total sky area of input pixels added to each output pixel is         */
/*  tracked, and the flux is appropriately normalized before writing to  */
/*  the final output file.  This automatically corrects for any multiple */
/*  coverages that may occur.                                            */
/*                                                                       */
/*  In order to deal efficiently with cubes, mProjectCube differs from   */
/*  mProject in one major way.  Rather than try to minimize the amount   */
/*  of memory used, mProjectCube reads the whole cube in up front and    */
/*  then reprojects all of it.  While mProject could read a line at a    */
/*  time this would result in significant I/O thrashing and slowdown.    */
/*                                                                       */
/*   char  *input_file     FITS file to reproject                        */
/*   char  *output_file    Reprojected FITS file                         */
/*   char  *template_file  FITS header file used to define the desired   */
/*                         output                                        */
/*                                                                       */
/*   int    hdu            Optional HDU offset for input file            */
/*   char  *weight_file    Optional pixel weight FITS file (must match   */
/*                         input)                                        */
/*                                                                       */
/*   double fixedWeight    A weight value used for all pixels            */
/*   double threshold      Pixels with values below this level treated   */
/*                         as blank                                      */
/*                                                                       */
/*   double drizzle        Optional pixel area 'drizzle' factor          */
/*   double fluxScale      Scale factor applied to all pixels            */
/*   int    energyMode     Pixel values are total energy rather than     */
/*                         energy density                                */
/*   int    expand         Expand output image area to include all of    */
/*                         the input pixels                              */
/*   int    fullRegion     Do not 'shrink-wrap' output area to non-blank */
/*                         pixels                                        */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mProjectCubeReturn *mProjectCube(char *input_file, char *output_file, char *template_file,
                                        int hduin, char *weight_file, double fixedWeight, double threshold,
                                        double drizzle, double fluxScale, int energyMode, int expand, int fullRegion,
                                        int debugin)
{
   int        i, j, j2, j3, k, l, m;
   int        nullcnt;
   long       fpixel[4], nelements;
   double     lon, lat;
   double     oxpix, oypix;
   double     oxpixMin, oypixMin;
   double     oxpixMax, oypixMax;
   int        haveIn, haveOut, haveMinMax, haveTop;
   int        xrefin, yrefin;
   int        xrefout, yrefout;
   int        xpixIndMin, xpixIndMax;
   int        ypixIndMin, ypixIndMax;
   int        imin, imax, jmin, jmax;
   int        istart, ilength;
   int        jstart, jlength;
   double     xpos, ypos;
   int        offscl, use;
   double     datamin, datamax;
   double     areamin, areamax;
   double     areaRatio;

   double     xcw[]  = {0.5, 1.5, 1.5, 0.5};
   double     ycw[]  = {0.5, 0.5, 1.5, 1.5};

   double     xccw[] = {1.5, 0.5, 0.5, 1.5};
   double     yccw[] = {0.5, 0.5, 1.5, 1.5};

   double     xcorner[4];
   double     ycorner[4];

   double     ilon[4];
   double     ilat[4];

   double     olon[4];
   double     olat[4];

   double     pixel_value  = 0;
   double     weight_value = 1;

   double ****indata;
   double  **inweights;

   double ****outdata;
   double   **outarea;

   double     overlapArea;

   int        status = 0;

   char      *checkHdr;

   struct mProjectCubeReturn *returnStruct;



   /*************************************************/
   /* Initialize output FITS basic image parameters */
   /*************************************************/

   int  bitpix = DOUBLE_IMG; 



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
      value.c[i] = (char)255;

   nan = value.d;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mProjectCubeReturn *)malloc(sizeof(struct mProjectCubeReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /* These six parameters are for mProjectCube_debugging and have to be set */
   /* here and recompiled before running.  Not for general use. */

   haveIn    = 0;
   haveOut   = 0;

   xrefin    = 0;
   yrefin    = 0;

   xrefout   = 0;
   yrefout   = 0;

   /**************************************************************/


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mProjectCube_debug = debugin;

   hdu = hduin;

   dtr = atan(1.)/45.;

   time(&currtime);
   start = currtime;

   haveWeights = 0;
   if(strlen(weight_file) > 0)
      haveWeights = 1;
   
   checkHdr = montage_checkHdr(input_file, 0, hdu);

   if(checkHdr)
   {
      strcpy(returnStruct->msg, checkHdr);
      return returnStruct;
   }

   checkHdr = montage_checkHdr(template_file, 1, 0);

   if(checkHdr)
   {
      strcpy(returnStruct->msg, checkHdr);
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

   if(mProjectCube_debug >= 1)
   {
      printf("\ninput_file    = [%s]\n", input_file);
      printf("output_file   = [%s]\n", output_file);
      printf("area_file     = [%s]\n", area_file);
      printf("template_file = [%s]\n\n", template_file);
      fflush(stdout);
   }


   /************************/
   /* Read the input image */
   /************************/

   if(mProjectCube_debug >= 1)
   {
      time(&currtime);
      printf("\nStarting to process pixels (time %.0f)\n\n", 
         (double)(currtime - start));
      fflush(stdout);
   }

   if(mProjectCube_readFits(input_file, weight_file))
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mProjectCube_debug >= 1)
   {
      printf("input.naxis      =  %ld\n",  input.naxis);
      printf("input.naxes[0]   =  %ld\n",  input.naxes[0]);
      printf("input.naxes[1]   =  %ld\n",  input.naxes[1]);
      printf("input.naxes[2]   =  %ld\n",  input.naxes[2]);
      printf("input.naxes[3]   =  %ld\n",  input.naxes[3]);
      printf("input.sys        =  %d\n",   input.sys);
      printf("input.epoch      =  %-g\n",  input.epoch);
      printf("input.clockwise  =  %d\n",   input.clockwise);
      printf("input proj       =  %s\n\n", input.wcs->ptype);

      fflush(stdout);
   }

   if(haveIn)
   {
      if(mProjectCube_debug >= 1)
      {
         printf("xrefin           =  %d\n",  xrefin);
         printf("yrefin           =  %d\n\n",  yrefin);

         fflush(stdout);
      }

      if(xrefin < 0 || xrefin >= input.naxes[0]
      || yrefin < 0 || yrefin >= input.naxes[1])
      {
         sprintf(returnStruct->msg, "Debug input pixel coordinates out of range");
         return returnStruct;
      }
   }

   offset = 0.;


   /*************************************************/ 
   /* Process the output header template to get the */ 
   /* image size, coordinate system and projection  */ 
   /*************************************************/ 

   if(mProjectCube_readTemplate(template_file))
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   output.naxis    = input.naxis;
   output.naxes[2] = input.naxes[2];
   output.naxes[3] = input.naxes[3];


   if(output.clockwise)
   {
      for(k=0; k<4; ++k)
      {
         xcorner[k] = xcw[k];
         ycorner[k] = ycw[k];
      }
   }
   else
   {
      for(k=0; k<4; ++k)
      {
         xcorner[k] = xccw[k];
         ycorner[k] = yccw[k];
      }
   }

   if(mProjectCube_debug >= 1)
   {
      printf("\nOriginal template\n\n");
      printf("output.naxis     =  %ld\n", output.naxis);
      printf("output.naxes[0]  =  %ld\n", output.naxes[0]);
      printf("output.naxes[1]  =  %ld\n", output.naxes[1]);
      printf("output.naxes[2]  =  %ld\n", output.naxes[2]);
      printf("output.naxes[3]  =  %ld\n", output.naxes[3]);
      printf("output.sys       =  %d\n",  output.sys);
      printf("output.epoch     =  %-g\n", output.epoch);
      printf("output.clockwise =  %d\n",  output.clockwise);
      printf("output proj      =  %s\n",  output.wcs->ptype);

      fflush(stdout);
   }

   if(expand)
   {
      /* We need to expand the output area so we get all of the input image. */
      /* This implies rereading the template as well.                        */

      offset = (sqrt(input.naxes[0]*input.naxes[0] * input.wcs->xinc*input.wcs->xinc
                   + input.naxes[1]*input.naxes[1] * input.wcs->yinc*input.wcs->yinc));

      if(mProjectCube_debug >= 1)
      {
         printf("\nexpand output template by %-g degrees on all sides\n\n", offset);
         fflush(stdout);
      }

      offset = offset / sqrt(output.wcs->xinc * output.wcs->xinc + output.wcs->yinc * output.wcs->yinc);

      if(mProjectCube_debug >= 1)
      {
         printf("\nexpand output template by %-g pixels on all sides\n\n", offset);
         fflush(stdout);
      }

      if(mProjectCube_readTemplate(template_file))
      {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      if(mProjectCube_debug >= 1)
      {
         printf("\nExpanded template\n\n");
         printf("output.naxis     =  %ld\n", output.naxis);
         printf("output.naxes[0]  =  %ld\n", output.naxes[0]);
         printf("output.naxes[1]  =  %ld\n", output.naxes[1]);
         printf("output.naxes[2]  =  %ld\n", output.naxes[2]);
         printf("output.naxes[3]  =  %ld\n", output.naxes[3]);
         printf("output.sys       =  %d\n",  output.sys);
         printf("output.epoch     =  %-g\n", output.epoch);
         printf("output.clockwise =  %d\n",  output.clockwise);
         printf("output proj      =  %s\n",  output.wcs->ptype);

         fflush(stdout);
      }
   }

   if(haveOut)
   {
      if(xrefout < 0 || xrefout >= output.naxes[0]
      || yrefout < 0 || yrefout >= output.naxes[1])
      {
         sprintf(returnStruct->msg, "Debug output pixel coordinates out of range");
         return returnStruct;
      }
   }



   /********************************************************/
   /* Create the position information structures for the   */
   /* top and bottom corners of the current row of pixels. */
   /* As we step down the image, we move the current       */
   /* "bottom" row to the "top" and compute a new bottom.  */
   /********************************************************/

   topl    = (struct Ipos *)malloc((input.naxes[0]+1) * sizeof(struct Ipos));
   topr    = (struct Ipos *)malloc((input.naxes[0]+1) * sizeof(struct Ipos));
   bottoml = (struct Ipos *)malloc((input.naxes[0]+1) * sizeof(struct Ipos));
   bottomr = (struct Ipos *)malloc((input.naxes[0]+1) * sizeof(struct Ipos));


   /************************************************/
   /* Create memory space for the input image data */
   /* (and weights if there are any)               */
   /************************************************/

   indata = (double ****)malloc(input.naxes[3] * sizeof(double ***));

   if(indata == (void *)NULL)
   {
      sprintf(returnStruct->msg, "Not enough memory for input data image array");
      return returnStruct;
   }

   for(j3=0; j3<input.naxes[3]; j3++)
   {
      indata[j3] = (double ***)malloc(input.naxes[2] * sizeof(double **));

      for(j2=0; j2<input.naxes[2]; j2++)
      {
         indata[j3][j2] = (double **)malloc(input.naxes[1] * sizeof(double *));

         if(indata[j3][j2] == (void *)NULL)
         {
            sprintf(returnStruct->msg, "Not enough memory for input data image array");
            return returnStruct;
         }

         for(j=0; j<input.naxes[1]; j++)
         {
            indata[j3][j2][j] = (double *)malloc(input.naxes[0] * sizeof(double));

            if(indata[j3][j2][j] == (void *)NULL)
            {
               sprintf(returnStruct->msg, "Not enough memory for input data image array");
               return returnStruct;
            }
         }
      }
   }

   if(mProjectCube_debug >= 1)
   {
      printf("\n%lu bytes allocated for input image pixels\n", 
         input.naxes[0] * input.naxes[1] * input.naxes[2] * input.naxes[3] * sizeof(double));
      fflush(stdout);
   }

   if(haveWeights)
   {
      /*****************************************************/
      /* Create the weight buffer for line of input pixels */
      /*****************************************************/

      inweights = (double **)malloc(input.naxes[1] * sizeof(double *));

      if(inweights == (void *)NULL)
      {
         sprintf(returnStruct->msg, "Not enough memory for input weights array");
         return returnStruct;
      }

      for(j=0; j<input.naxes[1]; j++)
      {
         inweights[j] = (double *)malloc(input.naxes[0] * sizeof(double));                               

         if(inweights[j] == (void *)NULL)
         {
            sprintf(returnStruct->msg, "Not enough memory for input weights array");
            return returnStruct;
         }
      }

      if(mProjectCube_debug >= 1)
      {
         printf("%lu bytes allocated for input weight values\n", 
            input.naxes[0] * input.naxes[1] * sizeof(double));
         fflush(stdout);
      }
   }


   /************************************************/
   /* Go around the outside of the INPUT image,    */
   /* finding the range of output pixel locations  */
   /************************************************/

   oxpixMin =  100000000;
   oxpixMax = -100000000;
   oypixMin =  100000000;
   oypixMax = -100000000;


   /* Check input left and right */

   for (j=0; j<input.naxes[1]+1; ++j)
   {
      pix2wcs(input.wcs, 0.5, j+0.5, &xpos, &ypos);

      convertCoordinates(input.sys, input.epoch, xpos, ypos,
                         output.sys, output.epoch, &lon, &lat, 0.0);
      
      offscl = 0;

      wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

      mProjectCube_fixxy(&oxpix, &oypix, &offscl);

      if(input.wcs->offscl)
         offscl = 1;

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
      
      offscl = 0;

      wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

      mProjectCube_fixxy(&oxpix, &oypix, &offscl);

      if(input.wcs->offscl)
         offscl = 1;

      if(!offscl)
      {
         if(oxpix < oxpixMin) oxpixMin = oxpix;
         if(oxpix > oxpixMax) oxpixMax = oxpix;
         if(oypix < oypixMin) oypixMin = oypix;
         if(oypix > oypixMax) oypixMax = oypix;
      }
   }


   /* Check input top and bottom */

   for (i=0; i<input.naxes[0]+1; ++i)
   {
      pix2wcs(input.wcs, i+0.5, 0.5, &xpos, &ypos);

      convertCoordinates(input.sys, input.epoch, xpos, ypos,
                         output.sys, output.epoch, &lon, &lat, 0.0);
      
      offscl = 0;

      wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

      mProjectCube_fixxy(&oxpix, &oypix, &offscl);

      if(input.wcs->offscl)
         offscl = 1;

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
      
      offscl = 0;

      wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

      mProjectCube_fixxy(&oxpix, &oypix, &offscl);

      if(input.wcs->offscl)
         offscl = 1;

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

   for (j=0; j<output.naxes[1]+1; j++) {
     oxpix = 0.5;
     oypix = (double)j+0.5;
     mProjectCube_UpdateBounds (oxpix, oypix, &oxpixMin, &oxpixMax, &oypixMin, &oypixMax);
     oxpix = (double)output.naxes[0]+0.5;
     mProjectCube_UpdateBounds (oxpix, oypix, &oxpixMin, &oxpixMax, &oypixMin, &oypixMax);
   }

   /* 
    * Check output top and bottom 
    */

   for (i=0; i<output.naxes[0]+1; i++) {
     oxpix = (double)i+0.5;
     oypix = 0.5;
     mProjectCube_UpdateBounds (oxpix, oypix, &oxpixMin, &oxpixMax, &oypixMin, &oypixMax);
     oypix = (double)output.naxes[1]+0.5;
     mProjectCube_UpdateBounds (oxpix, oypix, &oxpixMin, &oxpixMax, &oypixMin, &oypixMax);
   }

   /*
    * ASSERT: Output bounding box now specified by
    *   (oxpixMin, oxpixMax, oypixMin, oypixMax)
    */

   if(fullRegion)
   {
      oxpixMin = 0.5;
      oxpixMax = output.naxes[0]+0.5+1;

      oypixMin = 0.5;
      oypixMax = output.naxes[1]+0.5+1;
   }

   istart = oxpixMin - 1;

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

   if(mProjectCube_debug >= 2)
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
      return returnStruct;
   }
    

   /***********************************************/ 
   /* Allocate memory for the output image pixels */ 
   /***********************************************/ 

   outdata = (double ****)malloc(output.naxes[3] * sizeof(double ***));

   if(outdata == (void *)NULL)
   {
      sprintf(returnStruct->msg, "Not enough memory for output data image array");
      return returnStruct;
   }

   for(j3=0; j3<output.naxes[3]; j3++)
   {
      outdata[j3] = (double ***)malloc(output.naxes[2] * sizeof(double **));

      for(j2=0; j2<output.naxes[2]; j2++)
      {
         outdata[j3][j2] = (double **)malloc(jlength * sizeof(double *));

         if(outdata[j3][j2] == (void *)NULL)
         {
            sprintf(returnStruct->msg, "Not enough memory for output data image array");
            return returnStruct;
         }

         for(j=0; j<jlength; j++)
         {
            outdata[j3][j2][j] = (double *)malloc(ilength * sizeof(double));

            if(outdata[j3][j2][j] == (void *)NULL)
            {
               sprintf(returnStruct->msg, "Not enough memory for output data image array");
               return returnStruct;
            }
         }
      }
   }

   if(mProjectCube_debug >= 1)
   {
      printf("\n%lu bytes allocated for output image pixels\n", 
         ilength * jlength * output.naxes[2] * output.naxes[3] * sizeof(double));
      fflush(stdout);
   }


   /*********************/
   /* Initialize pixels */
   /*********************/

   for (j3=0; j3<output.naxes[3]; ++j3)
   {
      for (j2=0; j2<output.naxes[2]; ++j2)
      {
         for (j=0; j<jlength; ++j)
         {
            for (i=0; i<ilength; ++i)
            {
               outdata[j3][j2][j][i] = NAN;
            }
         }
      }
   }

   if(mProjectCube_debug >= 1)
   {
      printf("output pixel values initialized\n"); 
      fflush(stdout);
   }


   /**********************************************/ 
   /* Allocate memory for the output pixel areas */ 
   /**********************************************/ 

   outarea = (double **)malloc(jlength * sizeof(double *));

   if(outarea == (void *)NULL)
   {
      sprintf(returnStruct->msg, "Not enough memory for output area image array");
      return returnStruct;
   }

   for(j=0; j<jlength; j++)
   {
      outarea[j] = (double *)malloc(ilength * sizeof(double));                               

      if(outarea[j] == (void *)NULL)
      {
         sprintf(returnStruct->msg, "Not enough memory for output area image array");
         return returnStruct;
      }
   }

   if(mProjectCube_debug >= 1)
   {
      printf("%lu bytes allocated for pixel areas\n", 
         ilength * jlength * sizeof(double));
      fflush(stdout);
   }


   /********************/
   /* Initialize areas */
   /********************/

   for (j=0; j<jlength; ++j)
   {
      for (i=0; i<ilength; ++i)
      {
         outarea[j][i] = 0.;
      }
   }


   /************************************/
   /* Read in all the input image data */
   /************************************/

   nelements = input.naxes[0];

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   for (j3=0; j3<input.naxes[3]; ++j3)
   {
      fpixel[2] = 1;

      for (j2=0; j2<input.naxes[2]; ++j2)
      {
         fpixel[1] = 1;

         for (j=0; j<input.naxes[1]; ++j)
         {
            if(mProjectCube_debug == 2 && !haveOut)
            {
               printf("\rReading input row %5d %5d %5d  ", j3, j2, j);
               fflush(stdout);
            }

            if(fits_read_pix(input.fptr, TDOUBLE, fpixel, nelements, &nan,
                             indata[j3][j2][j], &nullcnt, &status))
            {
               mProjectCube_printFitsError(status);
               return returnStruct;
            }

            ++fpixel[1];
         }

         ++fpixel[2];
      }

      ++fpixel[3];
   }

   if(mProjectCube_debug == 2 && !haveOut)
   {
      printf("\n");
      fflush(stdout);
   }


   if(haveWeights)
   {
      fpixel[0] = 1;
      fpixel[1] = 1;
      fpixel[2] = 1;
      fpixel[3] = 1;

      for (j=0; j<input.naxes[1]; ++j)
      {
         if(mProjectCube_debug == 2 && !haveOut)
         {
            printf("\rReading weight input row %5d  ", j);
            fflush(stdout);
         }

         if(fits_read_pix(weight.fptr, TDOUBLE, fpixel, nelements, &nan,
                          inweights[j], &nullcnt, &status))
         {
            mProjectCube_printFitsError(status);
            return returnStruct;
         }

         ++fpixel[1];
      }
   }

   if(mProjectCube_debug == 2 && !haveOut)
   {
      printf("\n");
      fflush(stdout);
   }



   /*****************************/
   /* Loop over the input lines */
   /*****************************/

   haveTop = 0;

   for (j=0; j<input.naxes[1]; ++j)
   {
      if(haveIn && j > yrefin)
         break;

      if(haveIn && j < yrefin)
         continue;

      inRow = j;

      if(mProjectCube_debug == 2 && !haveOut)
      {
         printf("\rProcessing input row %5d  ", j);
         fflush(stdout);
      }


      /*************************************************************/
      /*                                                           */
      /* Calculate the locations of the bottoms of the pixels      */
      /* (first time the tops, too) otherwise, switch top and      */
      /* bottom before recomputing bottoms.                        */
      /*                                                           */
      /* We use "bottom" and "top" (and "left" and "right")        */
      /* advisedly here.  If CDELT1 and CDELT2 are both negative,  */
      /* these descriptions are accurate.  If either is positive,  */
      /* the corresponding left/right or top/bottom roles are      */
      /* reversed.  What we really mean is increasing j (top to    */
      /* bottom) and increasing i (left to right).  The only place */
      /* it makes any difference is in making sure that we go      */
      /* around the pixel vertices counterclockwise, so that is    */
      /* where we check the CDELTs explicitly.                     */
      /*                                                           */
      /*************************************************************/


      /* 'TOPS' of the pixels */

      if(!haveTop || drizzle != 1.0)
      {
         for (i=0; i<input.naxes[0]+1; ++i)
         {
            /* For the general 'drizzle' algorithm we must       */
            /* project all four of the pixel corners separately. */
            /* However, in the default case where the input      */
            /* pixels are assumed to fill their area, we can     */
            /* save compute time by reusing the values for the   */
            /* shared corners of one pixel for the next.         */


            /* Project the top corners (if corners are shared) */

            if(drizzle == 1.)
            {
               pix2wcs(input.wcs, i+0.5, j+0.5, &xpos, &ypos);

               convertCoordinates(input.sys, input.epoch, xpos, ypos,
                                  output.sys, output.epoch, 
                                  &((topl+i)->lon), &((topl+i)->lat), 0.0);
               
               offscl = 0;

               wcs2pix(output.wcs, (topl+i)->lon,  (topl+i)->lat,
                       &((topl+i)->oxpix), &((topl+i)->oypix), &offscl);

               mProjectCube_fixxy(&((topl+i)->oxpix), &((topl+i)->oypix), &offscl);

               if(input.wcs->offscl)
                  offscl = 1;

               (topl+i)->offscl = offscl;

               if(i>0)
               {
                  (topr+i-1)->lon    = (topl+i)->lon;
                  (topr+i-1)->lat    = (topl+i)->lat;
                  (topr+i-1)->oxpix  = (topl+i)->oxpix;
                  (topr+i-1)->oypix  = (topl+i)->oypix;
                  (topr+i-1)->offscl = (topl+i)->offscl;
               }

               if(mProjectCube_debug >= 5)
               {
                  printf("    pixel (top)  = (%10.6f,%10.6f) [%d,%d]\n",
                     i+1.5, j+0.5, i, j);

                  printf(" -> input coord  = (%10.6f,%10.6f)\n", xpos, ypos);
                  printf(" -> output coord = (%10.6f,%10.6f)\n", 
                         (topl+i)->lon, (topl+i)->lat);

                  if((topl+i)->offscl)
                  {
                     printf(" -> opix         = (%10.6f,%10.6f) OFF SCALE\n\n",
                         (topl+i)->oxpix, (topl+i)->oypix);
                     fflush(stdout);
                  }
                  else
                  {
                     printf(" -> opix         = (%10.6f,%10.6f)\n\n",
                         (topl+i)->oxpix, (topl+i)->oypix);
                     fflush(stdout);
                  }
               }

               haveTop = 1;
            }


            /* Project the top corners (if corners aren't shared) */

            else
            {
               /* TOP LEFT */

               pix2wcs(input.wcs, i+1-0.5*drizzle, j+1-0.5*drizzle,
                       &xpos, &ypos);

               convertCoordinates(input.sys, input.epoch, xpos, ypos,
                                  output.sys, output.epoch, 
                                  &((topl+i)->lon), &((topl+i)->lat), 0.0);
               
               offscl = 0;

               wcs2pix(output.wcs, (topl+i)->lon,  (topl+i)->lat,
                       &((topl+i)->oxpix), &((topl+i)->oypix), &offscl);

               mProjectCube_fixxy(&((topl+i)->oxpix), &((topl+i)->oypix), &offscl);

               if(input.wcs->offscl)
                  offscl = 1;

               (topl+i)->offscl = offscl;

               if(mProjectCube_debug >= 5)
               {
                  printf("    pixel TL     = (%10.6f,%10.6f) [%d,%d]\n",  
                          i+1-0.5*drizzle, j+1-0.5*drizzle, i, j);

                  printf(" -> input coord  = (%10.6f,%10.6f)\n", xpos, ypos);
                  printf(" -> output coord = (%10.6f,%10.6f)\n", 
                         (topl+i)->lon, (topl+i)->lat);

                  if((topl+i)->offscl)
                  {
                     printf(" -> opix         = (%10.6f,%10.6f) OFF SCALE\n\n",
                         (topl+i)->oxpix, (topl+i)->oypix);
                     fflush(stdout);
                  }
                  else
                  {
                     printf(" -> opix         = (%10.6f,%10.6f)\n\n",
                         (topl+i)->oxpix, (topl+i)->oypix);
                     fflush(stdout);
                  }
               }


               /* TOP RIGHT */

               pix2wcs(input.wcs, i+1+0.5*drizzle, j+1-0.5*drizzle,
                       &xpos, &ypos);

               convertCoordinates(input.sys, input.epoch, xpos, ypos,
                                  output.sys, output.epoch, 
                                  &((topr+i)->lon), &((topr+i)->lat), 0.0);
               
               offscl = 0;

               wcs2pix(output.wcs, (topr+i)->lon,  (topr+i)->lat,
                       &((topr+i)->oxpix), &((topr+i)->oypix), &offscl);

               mProjectCube_fixxy(&((topr+i)->oxpix), &((topr+i)->oypix), &offscl);

               if(input.wcs->offscl)
                  offscl = 1;

               (topr+i)->offscl = offscl;

               if(mProjectCube_debug >= 5)
               {
                  printf("    pixel TR     = (%10.6f,%10.6f) [%d,%d]\n", 
                          i+1+0.5*drizzle, j+1-0.5*drizzle, i, j);

                  printf(" -> input coord  = (%10.6f,%10.6f)\n", xpos, ypos);
                  printf(" -> output coord = (%10.6f,%10.6f)\n", 
                         (topr+i)->lon, (topr+i)->lat);

                  if((topr+i)->offscl)
                  {
                     printf(" -> opix         = (%10.6f,%10.6f) OFF SCALE\n\n",
                         (topr+i)->oxpix, (topr+i)->oypix);
                     fflush(stdout);
                  }
                  else
                  {
                     printf(" -> opix         = (%10.6f,%10.6f)\n\n",
                         (topr+i)->oxpix, (topr+i)->oypix);
                     fflush(stdout);
                  }
               }
            }
         }
      }


      /* If the corners are shared, we don't need     */
      /* to recompute when we move down a row, rather */
      /* we move the 'bottom' to the 'top' and        */
      /* recompute the 'bottom'                       */

      else
      {
         postmp  = topl;
         topl    = bottoml;
         bottoml = postmp;

         postmp  = topr;
         topr    = bottomr;
         bottomr = postmp;
      }


      /* 'BOTTOMS' of the pixels */

      for (i=0; i<input.naxes[0]+1; ++i)
      {

         /* Project the bottom corners (if corners are shared) */

         if(drizzle == 1.)
         {
            pix2wcs(input.wcs, i+0.5, j+1.5, &xpos, &ypos);

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               output.sys, output.epoch, 
                               &((bottoml+i)->lon), &((bottoml+i)->lat), 0.0);

            offscl = 0;

            wcs2pix(output.wcs, (bottoml+i)->lon,  (bottoml+i)->lat,
                    &((bottoml+i)->oxpix), &((bottoml+i)->oypix), &offscl);

            mProjectCube_fixxy(&((bottoml+i)->oxpix), &((bottoml+i)->oypix), &offscl);

            if(input.wcs->offscl)
               offscl = 1;

            (bottoml+i)->offscl = offscl;

            if(i>0)
            {
               (bottomr+i-1)->lon    = (bottoml+i)->lon;
               (bottomr+i-1)->lat    = (bottoml+i)->lat;
               (bottomr+i-1)->oxpix  = (bottoml+i)->oxpix;
               (bottomr+i-1)->oypix  = (bottoml+i)->oypix;
               (bottomr+i-1)->offscl = (bottoml+i)->offscl;
            }

            if(mProjectCube_debug >= 5)
            {
               printf("    pixel (bot)  = (%10.6f,%10.6f) [%d,%d]\n", 
                  i+1.5, j+0.5, i, j-1);

               printf(" -> input coord  = (%10.6f,%10.6f)\n", xpos, ypos);
               printf(" -> output coord = (%10.6f,%10.6f)\n", 
                      (bottoml+i)->lon, (bottoml+i)->lat);

               if((bottoml+i)->offscl)
               {
                  printf(" -> opix         = (%10.6f,%10.6f) OFF SCALE\n\n",
                      (bottoml+i)->oxpix, (bottoml+i)->oypix);
                  fflush(stdout);
               }
               else
               {
                  printf(" -> opix         = (%10.6f,%10.6f)\n\n",
                      (bottoml+i)->oxpix, (bottoml+i)->oypix);
                  fflush(stdout);
               }
            }
         }


         /* Project the bottom corners (if corners aren't shared) */

         else
         {
            /* BOTTOM LEFT */

            pix2wcs(input.wcs, i+1-0.5*drizzle, j+1+0.5*drizzle,
                    &xpos, &ypos);

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               output.sys, output.epoch, 
                               &((bottoml+i)->lon), &((bottoml+i)->lat), 0.0);

            offscl = 0;

            wcs2pix(output.wcs, (bottoml+i)->lon,  (bottoml+i)->lat,
                    &((bottoml+i)->oxpix), &((bottoml+i)->oypix), &offscl);

            mProjectCube_fixxy(&((bottoml+i)->oxpix), &((bottoml+i)->oypix), &offscl);

            if(input.wcs->offscl)
               offscl = 1;

            (bottoml+i)->offscl = offscl;

            if(mProjectCube_debug >= 5)
            {
               printf("    pixel BL     = (%10.6f,%10.6f) [%d,%d]\n",
                       i+1-0.5*drizzle, j+1+0.5*drizzle, i, j);

               printf(" -> input coord  = (%10.6f,%10.6f)\n", xpos, ypos);
               printf(" -> output coord = (%10.6f,%10.6f)\n", 
                      (bottoml+i)->lon, (bottoml+i)->lat);

               if((bottoml+i)->offscl)
               {
                  printf(" -> opix         = (%10.6f,%10.6f) OFF SCALE\n\n",
                      (bottoml+i)->oxpix, (bottoml+i)->oypix);
                  fflush(stdout);
               }
               else
               {
                  printf(" -> opix         = (%10.6f,%10.6f)\n\n",
                      (bottoml+i)->oxpix, (bottoml+i)->oypix);
                  fflush(stdout);
               }
            }


            /* BOTTOM RIGHT */

            pix2wcs(input.wcs, i+1+0.5*drizzle, j+1+0.5*drizzle,
                    &xpos, &ypos);

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               output.sys, output.epoch, 
                               &((bottomr+i)->lon), &((bottomr+i)->lat), 0.0);

            offscl = 0;

            wcs2pix(output.wcs, (bottomr+i)->lon,  (bottomr+i)->lat,
                    &((bottomr+i)->oxpix), &((bottomr+i)->oypix), &offscl);

            mProjectCube_fixxy(&((bottomr+i)->oxpix), &((bottomr+i)->oypix), &offscl);

            if(input.wcs->offscl)
               offscl = 1;

            (bottomr+i)->offscl = offscl;

            if(mProjectCube_debug >= 5)
            {
               printf("    pixel BR     = (%10.6f,%10.6f) [%d,%d]\n", 
                       i+1+0.5*drizzle, j+1+0.5*drizzle, i, j);

               printf(" -> input coord  = (%10.6f,%10.6f)\n", xpos, ypos);
               printf(" -> output coord = (%10.6f,%10.6f)\n", 
                      (bottomr+i)->lon, (bottomr+i)->lat);

               if((bottomr+i)->offscl)
               {
                  printf(" -> opix         = (%10.6f,%10.6f) OFF SCALE\n\n",
                      (bottomr+i)->oxpix, (bottomr+i)->oypix);
                  fflush(stdout);
               }
               else
               {
                  printf(" -> opix         = (%10.6f,%10.6f)\n\n",
                      (bottomr+i)->oxpix, (bottomr+i)->oypix);
                  fflush(stdout);
               }
            }
         }
      }

      
      /************************/
      /* For each input pixel */
      /************************/

      for (i=0; i<input.naxes[0]; ++i)
      {
         if(haveIn && (j != yrefin || i != xrefin))
            continue;

         inColumn = i;

         if(haveWeights)
         {
            weight_value = inweights[j][i];

            if(weight_value < threshold)
               weight_value = 0.;

            weight_value *= fixedWeight;
         }

         if(mProjectCube_debug >= 3 && !haveOut)
         {
            if(haveWeights)
               printf("\nInput: line %d / pixel %d (weight: %-g)\n\n",
                  j, i,  weight_value);
            else
               printf("\nInput: line %d / pixel %d\n\n",
                  j, i);
            fflush(stdout);
         }



         /************************************/
         /* Find the four corners' locations */
         /* in output pixel coordinates      */
         /************************************/

         oxpixMin =  100000000;
         oxpixMax = -100000000;
         oypixMin =  100000000;
         oypixMax = -100000000;

         use = 1;

         if(input.clockwise)
         {
            ilon[0] = (bottomr+i)->lon;
            ilat[0] = (bottomr+i)->lat;

            ilon[1] = (bottoml+i)->lon;
            ilat[1] = (bottoml+i)->lat;

            ilon[2] = (topl+i)->lon;
            ilat[2] = (topl+i)->lat;

            ilon[3] = (topr+i)->lon;
            ilat[3] = (topr+i)->lat;
         }
         else
         {
            ilon[0] = (topr+i)->lon;
            ilat[0] = (topr+i)->lat;

            ilon[1] = (topl+i)->lon;
            ilat[1] = (topl+i)->lat;

            ilon[2] = (bottoml+i)->lon;
            ilat[2] = (bottoml+i)->lat;

            ilon[3] = (bottomr+i)->lon;
            ilat[3] = (bottomr+i)->lat;
         }

         if((topl+i)->oxpix < oxpixMin) oxpixMin = (topl+i)->oxpix;
         if((topl+i)->oxpix > oxpixMax) oxpixMax = (topl+i)->oxpix;
         if((topl+i)->oypix < oypixMin) oypixMin = (topl+i)->oypix;
         if((topl+i)->oypix > oypixMax) oypixMax = (topl+i)->oypix;

         if((topr+i)->oxpix < oxpixMin) oxpixMin = (topr+i)->oxpix;
         if((topr+i)->oxpix > oxpixMax) oxpixMax = (topr+i)->oxpix;
         if((topr+i)->oypix < oypixMin) oypixMin = (topr+i)->oypix;
         if((topr+i)->oypix > oypixMax) oypixMax = (topr+i)->oypix;

         if((bottoml+i)->oxpix < oxpixMin) oxpixMin = (bottoml+i)->oxpix;
         if((bottoml+i)->oxpix > oxpixMax) oxpixMax = (bottoml+i)->oxpix;
         if((bottoml+i)->oypix < oypixMin) oypixMin = (bottoml+i)->oypix;
         if((bottoml+i)->oypix > oypixMax) oypixMax = (bottoml+i)->oypix;

         if((bottomr+i)->oxpix < oxpixMin) oxpixMin = (bottomr+i)->oxpix;
         if((bottomr+i)->oxpix > oxpixMax) oxpixMax = (bottomr+i)->oxpix;
         if((bottomr+i)->oypix < oypixMin) oypixMin = (bottomr+i)->oypix;
         if((bottomr+i)->oypix > oypixMax) oypixMax = (bottomr+i)->oypix;

         if((topl+i)->offscl)    use = 0;
         if((topr+i)->offscl)    use = 0;
         if((bottoml+i)->offscl) use = 0;
         if((bottomr+i)->offscl) use = 0;


         if(use)
         {
            /************************************************/
            /* Determine the range of output pixels we need */
            /* to check against this input pixel            */
            /************************************************/

            xpixIndMin = floor(oxpixMin - 0.5);
            xpixIndMax = floor(oxpixMax - 0.5) + 1;
            ypixIndMin = floor(oypixMin - 0.5);
            ypixIndMax = floor(oypixMax - 0.5) + 1;

            if(mProjectCube_debug >= 3 && !haveOut)
            {
               printf("\n");
               printf(" oxpixMin = %20.13e\n", oxpixMin);
               printf(" oxpixMax = %20.13e\n", oxpixMax);
               printf(" oypixMin = %20.13e\n", oypixMin);
               printf(" oypixMax = %20.13e\n", oypixMax);
               printf("\n");
               printf("Output X range: %5d to %5d\n", xpixIndMin, xpixIndMax);
               printf("Output Y range: %5d to %5d\n", ypixIndMin, ypixIndMax);
               printf("\n");
            }


            /***************************************************/
            /* Loop over these, computing the fractional area  */
            /* of overlap (which we use to update the data and */
            /* area arrays)                                    */
            /***************************************************/

            for(m=ypixIndMin; m<ypixIndMax; ++m)
            {
               if(m-jstart < 0 || m-jstart >= jlength)
                  continue;

               outRow = m;

               for(l=xpixIndMin; l<xpixIndMax; ++l)
               {
                  if(l-istart < 0 || l-istart >= ilength)
                     continue;

                  if(haveOut && m == yrefout && l > xrefout)
                     break;

                  if(haveOut && (m != yrefout && l != xrefout))
                     continue;

                  outColumn = l;

                  for(k=0; k<4; ++k)
                  {
                     oxpix = l + xcorner[k];
                     oypix = m + ycorner[k];

                     pix2wcs(output.wcs, oxpix, oypix, &olon[k], &olat[k]);
                  }


                  /* If we've given a reference input/output pixel, print */
                  /* out all the info on it's overlap with corresponding  */
                  /* output/input pixels                                  */

                  if((haveIn  && j == yrefin  && i == xrefin )
                  || (haveOut && m == yrefout && l == xrefout))
                  {  
                     printf("\n\n\n===================================================\n\n");
                     printf("Input pixel:  (%d,%d)\n", j, i);

                     for(k=0; k<4; ++k)
                     {
                        offscl = 0;

                        wcs2pix(output.wcs, ilon[k], ilat[k],
                                &oxpix, &oypix, &offscl);

                        mProjectCube_fixxy(&oxpix, &oypix, &offscl);

                        printf("   corner %d: (%10.6f,%10.6f) -> [%10.6f,%10.6f]\n", 
                           k+1, ilon[k], ilat[k], oxpix, oypix);
                     }

                     printf("\nOutput pixel: (%d,%d)\n", m, l);

                     for(k=0; k<4; ++k)
                     {
                        offscl = 0;

                        wcs2pix(output.wcs, olon[k], olat[k],
                                &oxpix, &oypix, &offscl);

                        mProjectCube_fixxy(&oxpix, &oypix, &offscl);

                        printf("   corner %d: (%10.6f,%10.6f) -> [%10.6f,%10.6f]\n", 
                           k+1, olon[k], olat[k], oxpix, oypix);
                     }


                     fflush(stdout);
                  }
                    

                  /* Now compute the overlap area */

                  if(weight_value > 0)
                  {
                     if(!haveIn && !haveOut)
                        overlapArea = mProjectCube_computeOverlap(ilon, ilat, olon, olat, energyMode, refArea, &areaRatio);

                     if((haveIn  && j == yrefin  && i == xrefin )
                     || (haveOut && m == yrefout && l == xrefout))
                     {  
                        overlapArea = mProjectCube_computeOverlap(ilon, ilat, olon, olat, energyMode, refArea, &areaRatio);

                        printf("\n   => overlap area: %12.5e\n\n", overlapArea);
                        fflush(stdout);
                     }
                  }


                  /* Update the output data and area arrays */

                  outarea[m-jstart][l-istart] += overlapArea * weight_value;

                  for(j3=0; j3<input.naxes[3]; ++j3)
                  {
                     for(j2=0; j2<input.naxes[2]; ++j2)
                     {
                        pixel_value = indata[j3][j2][j][i];

                        if(mNaN(pixel_value))
                           continue;

                        pixel_value *= fluxScale;

                        if (mNaN(outdata[j3][j2][m-jstart][l-istart]))
                           outdata[j3][j2][m-jstart][l-istart] = pixel_value * overlapArea * areaRatio * weight_value;
                        else
                           outdata[j3][j2][m-jstart][l-istart] += pixel_value * overlapArea * areaRatio * weight_value;

                        if(mProjectCube_debug >= 3)
                        {
                           if((!haveIn && !haveOut)
                           || (haveIn  && j == yrefin  && i == xrefin )
                           || (haveOut && m == yrefout && l == xrefout))
                           {
                              printf("Compare out(%d,%d) to in(%d,%d), plane %d/%d => ", m, l, j, i, j3, j2);
                              printf("overlapArea = %12.5e (%12.5e / %12.5e)\n", overlapArea, 
                              outdata[j3][j2][m-jstart][l-istart], outarea[m-jstart][l-istart]);
                              fflush(stdout);
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

   if(mProjectCube_debug >= 1)
   {
      time(&currtime);
      printf("\n\nDone processing pixels (%.0f seconds)\n\n",
         (double)(currtime - start));
      fflush(stdout);
   }

   if(fits_close_file(input.fptr, &status))
   {
      mProjectCube_printFitsError(status);
      return returnStruct;
   }

   if(haveIn)
   {
      strcpy(returnStruct->msg, "Debugging output done.");
      return returnStruct;
   }


   /*********************************/
   /* Normalize image data based on */
   /* total area added to pixel     */
   /*********************************/

   haveMinMax = 0;

   datamax = 0.,
   datamin = 0.;
   areamin = 0.;
   areamax = 0.;

   imin = 99999;
   imax = 0;

   jmin = 99999;
   jmax = 0;

   for (j3=0; j3<output.naxes[3]; ++j3)
   {
      for (j2=0; j2<output.naxes[2]; ++j2)
      {
         for (j=0; j<jlength; ++j)
         {
            for (i=0; i<ilength; ++i)
            {
               if(outarea[j][i] > 0. && !mNaN(outdata[j3][j2][j][i]))
               {
                  outdata[j3][j2][j][i] 
                     = outdata[j3][j2][j][i] / outarea[j][i];

                  if(!haveMinMax)
                  {
                     datamin = outdata[j3][j2][j][i];
                     datamax = outdata[j3][j2][j][i];

                     areamin = outarea[j][i];
                     areamax = outarea[j][i];

                     haveMinMax = 1;
                  }

                  if(outdata[j3][j2][j][i] < datamin) 
                     datamin = outdata[j3][j2][j][i];

                  if(outdata[j3][j2][j][i] > datamax) 
                     datamax = outdata[j3][j2][j][i];

                  if(outarea[j][i] < areamin) 
                     areamin = outarea[j][i];

                  if(outarea[j][i] > areamax) 
                     areamax = outarea[j][i];

                  if(i < imin) imin = i;
                  if(i > imax) imax = i;
                  if(j < jmin) jmin = j;
                  if(j > jmax) jmax = j;
               }
               else
               {
                  outdata[j3][j2][j][i] = nan;

                  outarea[j][i] = 0.;
               }
            }
         }
      }
   }
   
   imin = imin + istart;
   imax = imax + istart;
   jmin = jmin + jstart;
   jmax = jmax + jstart;

   if(mProjectCube_debug >= 1)
   {
      printf("Data min = %-g\n", datamin);
      printf("Data max = %-g\n", datamax);
      printf("Area min = %-g\n", areamin);
      printf("Area max = %-g\n\n", areamax);
      printf("i min    = %d\n", imin);
      printf("i max    = %d\n", imax);
      printf("j min    = %d\n", jmin);
      printf("j max    = %d\n", jmax);
   }

   if(jmin > jmax || imin > imax)
   {
      mProjectCube_printError("All pixels are blank. Check for overlap of output template with image file.");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fullRegion)
   {
      imin = 0;
      imax = output.naxes[0]-1;

      jmin = 0;
      jmax = output.naxes[1]-1;

      if(mProjectCube_debug >= 1)
      {
         printf("Full region reset\n");
         printf("i min    = %d\n", imin);
         printf("i max    = %d\n", imax);
         printf("j min    = %d\n", jmin);
         printf("j max    = %d\n", jmax);
      }
   }



   /********************************/
   /* Create the output FITS files */
   /********************************/

   remove(output_file);               
   remove(area_file);               

   if(fits_create_file(&output.fptr, output_file, &status)) 
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_create_file(&output_area.fptr, area_file, &status)) 
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*********************************************************/
   /* Create the FITS image.  All the required keywords are */
   /* handled automatically.                                */
   /*********************************************************/

   if (fits_create_img(output.fptr, bitpix, output.naxis, output.naxes, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mProjectCube_debug >= 1)
   {
      printf("\nFITS data image created (not yet populated)\n"); 
      fflush(stdout);
   }

   if (fits_create_img(output_area.fptr, bitpix, 2, output_area.naxes, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mProjectCube_debug >= 1)
   {
      printf("FITS area image created (not yet populated)\n"); 
      fflush(stdout);
   }


   /****************************************/
   /* Set FITS header from a template file */
   /****************************************/

   if(fits_write_key_template(output.fptr, template_file, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mProjectCube_debug >= 1)
   {
      printf("Template keywords written to FITS data image\n"); 
      fflush(stdout);
   }

   if(fits_write_key_template(output_area.fptr, template_file, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mProjectCube_debug >= 1)
   {
      printf("Template keywords written to FITS area image\n\n"); 
      fflush(stdout);
   }


   /***************************/
   /* Modify BITPIX to be -64 */
   /***************************/

   if(fits_update_key_lng(output.fptr, "BITPIX", -64,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output_area.fptr, "BITPIX", -64,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /***********************************************************/
   /* Update NAXIS, NAXIS1, NAXIS2, NAXIS3, CRPIX1 and CRPIX2 */
   /***********************************************************/


   if(fits_update_key_lng(output.fptr, "NAXIS", output.naxis,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS1", imax-imin+1,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS2", jmax-jmin+1,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(output.fptr, "CRPIX1", crpix1-imin, -14,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(output.fptr, "CRPIX2", crpix2-jmin, -14,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }



   if(fits_update_key_lng(output_area.fptr, "NAXIS", 2,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output_area.fptr, "NAXIS1", imax-imin+1,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output_area.fptr, "NAXIS2", jmax-jmin+1,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(output_area.fptr, "CRPIX1", crpix1-imin, -14,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(output_area.fptr, "CRPIX2", crpix2-jmin, -14,
                                  (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   if(mProjectCube_debug)
   {
      printf("Template keywords BITPIX, CRPIX, and NAXIS updated\n");
      fflush(stdout);
   }


   /************************/
   /* Write the image data */
   /************************/

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   nelements = imax - imin + 1;

   for(j3=0; j3<output.naxes[3]; ++j3)
   {
      fpixel[2] = 1;

      for(j2=0; j2<output.naxes[2]; ++j2)
      {
         fpixel[1] = 1;

         for(j=jmin; j<=jmax; ++j)
         {
            if (fits_write_pix(output.fptr, TDOUBLE, fpixel, nelements, 
                               (void *)(&outdata[j3][j2][j-jstart][imin-istart]), &status))
            {
               mProjectCube_printFitsError(status);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            ++fpixel[1];
         }

         ++fpixel[2];
      }

      ++fpixel[3];
   }

   free(outdata[0]);

   if(mProjectCube_debug >= 1)
   {
      printf("Data written to FITS data image\n"); 
      fflush(stdout);
   }


   /***********************/
   /* Write the area data */
   /***********************/

   fpixel[0] = 1;
   fpixel[1] = 1;
   nelements = imax - imin + 1;

   for(j=jmin; j<=jmax; ++j)
   {
      if (fits_write_pix(output_area.fptr, TDOUBLE, fpixel, nelements,
                         (void *)(&outarea[j-jstart][imin-istart]), &status))
      {
         mProjectCube_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      ++fpixel[1];
   }

   free(outarea[0]);

   if(mProjectCube_debug >= 1)
   {
      printf("Data written to FITS area image\n\n"); 
      fflush(stdout);
   }


   /***********************/
   /* Close the FITS file */
   /***********************/

   if(fits_close_file(output.fptr, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mProjectCube_debug >= 1)
   {
      printf("FITS data image finalized\n"); 
      fflush(stdout);
   }

   if(fits_close_file(output_area.fptr, &status))
   {
      mProjectCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mProjectCube_debug >= 1)
   {
      printf("FITS area image finalized\n\n"); 
      fflush(stdout);
   }

   time(&currtime);

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "time=%.1f",       (double)(currtime - start));
   sprintf(returnStruct->json, "{\"time\":%.1f}", (double)(currtime - start));

   returnStruct->time = (double)(currtime - start);

   return returnStruct;
}



/**************************************************/
/*  Projections like CAR sometimes add an extra   */
/*  360 degrees worth of pixels to the return     */
/*  and call it off-scale.                        */
/**************************************************/

void mProjectCube_fixxy(double *x, double *y, int *offscl)
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

int mProjectCube_readTemplate(char *filename)
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
      mProjectCube_printError("Template file not found.");
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

      if(mProjectCube_debug >= 3)
      {
         printf("Template line: [%s]\n", line);
         fflush(stdout);
      }

      mProjectCube_parseLine(line);

      mProjectCube_stradd(header, line);
   }

   fclose(fp);


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   if(mProjectCube_debug >= 3)
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

   offscl = 0;

   xcorrection = 0;
   ycorrection = 0;

   pix2wcs(output.wcs, ix, iy, &xpos, &ypos);

   if(output.wcs->offscl == 0)
   {
      wcs2pix(output.wcs, xpos, ypos, &x, &y, &offscl);

      if(!offscl)
      {
         xcorrection = x-ix;
         ycorrection = y-iy;
      }
   }

   if(mProjectCube_debug)
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

   if(mProjectCube_debug >= 3)
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

int mProjectCube_parseLine(char *linein)
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

   if(mProjectCube_debug >= 2)
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

int mProjectCube_readFits(char *filename, char *weightfile)
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
      mProjectCube_printError(errstr);
      return 1;
   }

   if(hdu > 0)
   {
      if(fits_movabs_hdu(input.fptr, hdu+1, NULL, &status))
      {
         mProjectCube_printFitsError(status);
         return 1;
      }
   }

   if(fits_get_image_wcs_keys(input.fptr, &header, &status))
   {
      mProjectCube_printFitsError(status);
      return 1;
   }


   /************************/
   /* Open the weight file */
   /************************/

   if(haveWeights)
   {
      if(fits_open_file(&weight.fptr, weightfile, READONLY, &status))
      {
         sprintf(errstr, "Weight file %s missing or invalid FITS", weightfile);
         mProjectCube_printError(errstr);
         return 1;
      }

      if(hdu > 0)
      {
         if(fits_movabs_hdu(weight.fptr, hdu+1, NULL, &status))
         {
            mProjectCube_printFitsError(status);
            return 1;
         }
      }
   }


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   status = 0;
   if(fits_read_key(input.fptr, TLONG, "NAXIS", &(input.naxis), (char *)NULL, &status))
   {
      mProjectCube_printFitsError(status);
      return 1;
   }


   if(input.naxis < 2 || input.naxis > 4)
   {
      sprintf(errstr, "Image file %s missing or invalid FITS", filename);
      mProjectCube_printError(errstr);
      return 1;
   }
   

   input.wcs = wcsinit(header);

   if(input.wcs == (struct WorldCoor *)NULL)
   {
      sprintf(montage_msgstr, "Input wcsinit() failed.");
      return 1;
   }

   input.naxes[0] = input.wcs->nxpix;
   input.naxes[1] = input.wcs->nypix;

   refArea = fabs(input.wcs->xinc * input.wcs->yinc) * dtr * dtr;


   /* The WCS library doesn't track higher dimensions, */
   /* so we have to go back to the image header itself */

   status = 0;
   if(fits_read_key(input.fptr, TLONG, "NAXIS3", &(input.naxes[2]), (char *)NULL, &status))
      input.naxes[2] = 1;

   status = 0;
   if(fits_read_key(input.fptr, TLONG, "NAXIS4", &(input.naxes[3]), (char *)NULL, &status))
      input.naxes[3] = 1;


   /* Kludge to get around bug in WCS library:   */
   /* 360 degrees sometimes added to pixel coord */

   ix = (input.wcs->nxpix)/2.;
   iy = (input.wcs->nypix)/2.;

   offscl = 0;

   xcorrectionIn = 0;
   ycorrectionIn = 0;

   pix2wcs(input.wcs, ix, iy, &xpos, &ypos);

   if(input.wcs->offscl == 0)
   {
      wcs2pix(input.wcs, xpos, ypos, &x, &y, &offscl);

      if(!offscl)
      {
         xcorrectionIn = x-ix;
         ycorrectionIn = y-iy;
      }
   }

   if(mProjectCube_debug)
   {
      printf("xcorrectionIn = %.2f\n", xcorrectionIn);
      printf("ycorrectionIn = %.2f\n\n", ycorrectionIn);
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

   if(mProjectCube_debug >= 3)
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



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mProjectCube_printFitsError(int status)
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

void mProjectCube_printError(char *msg)
{
   strcpy(montage_msgstr, msg);
}



/* stradd adds the string "card" to a header line, and */
/* pads the header out to 80 characters.               */

int mProjectCube_stradd(char *header, char *card)
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

void mProjectCube_UpdateBounds (double oxpix, double oypix,
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

  wcs2pix (input.wcs, lon, lat, &ixpix, &iypix, &offscl);

  if(output.wcs->offscl)
     offscl = 1;

  ixpix = ixpix - xcorrectionIn;
  iypix = iypix - ycorrectionIn;

  if(ixpix < 0.
  || ixpix > input.wcs->nxpix+1.
  || iypix < 0.
  || iypix > input.wcs->nypix+1.)
     offscl = 1;


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




/***************************************************/
/*                                                 */
/* computeOverlap()                                */
/*                                                 */
/* Sets up the polygons, runs the overlap          */
/* computation, and returns the area of overlap.   */
/*                                                 */
/***************************************************/

double mProjectCube_computeOverlap(double *ilon, double *ilat,
                               double *olon, double *olat, 
                               int energyMode, double refArea, double *areaRatio)
{
   int    i;
   double thisPixelArea;

   pi  = atan(1.0) * 4.;
   dtr = pi / 180.;


   *areaRatio = 1.;

   if(energyMode)
   {
      nv = 0;

      for(i=0; i<4; ++i)
      mProjectCube_SaveVertex(&P[i]);

      thisPixelArea = mProjectCube_Girard();

      *areaRatio = thisPixelArea / refArea;
   }


   nv = 0;

   if(mProjectCube_debug >= 4)
   {
      printf("\n-----------------------------------------------\n\nAdding pixel (%d,%d) to pixel (%d,%d)\n\n",
         inRow, inColumn, outRow, outColumn);

      printf("Input (P):\n");
      for(i=0; i<4; ++i)
         printf("%10.6f %10.6f\n", ilon[i], ilat[i]);

      printf("\nOutput (Q):\n");
      for(i=0; i<4; ++i)
         printf("%10.6f %10.6f\n", olon[i], olat[i]);

      printf("\n");
      fflush(stdout);
   }

   for(i=0; i<4; ++i)
   {
      P[i].x = cos(ilon[i]*dtr) * cos(ilat[i]*dtr);
      P[i].y = sin(ilon[i]*dtr) * cos(ilat[i]*dtr);
      P[i].z = sin(ilat[i]*dtr);
   }

   for(i=0; i<4; ++i)
   {
      Q[i].x = cos(olon[i]*dtr) * cos(olat[i]*dtr);
      Q[i].y = sin(olon[i]*dtr) * cos(olat[i]*dtr);
      Q[i].z = sin(olat[i]*dtr);
   }

   mProjectCube_ComputeIntersection(P, Q);

   return(mProjectCube_Girard());
}



/***************************************************/
/*                                                 */
/* ComputeIntersection()                           */
/*                                                 */
/* Find the polygon defining the area of overlap   */
/* between the two input polygons.                 */
/*                                                 */
/***************************************************/

void  mProjectCube_ComputeIntersection(Vec *P, Vec *Q)
{
   Vec  Pdir, Qdir;             /* "Current" directed edges on P and Q   */
   Vec  other;                  /* Temporary "edge-like" variable        */
   int  ip, iq;                 /* Indices of ends of Pdir, Qdir         */
   int  ip_begin, iq_begin;     /* Indices of beginning of Pdir, Qdir    */
   int  PToQDir;                /* Qdir direction relative to Pdir       */
                                /* (e.g. CLOCKWISE)                      */
   int  qEndpointFromPdir;      /* End P vertex as viewed from beginning */
                                /* of Qdir relative to Qdir              */
   int  pEndpointFromQdir;      /* End Q vertex as viewed from beginning */
                                /* of Pdir relative to Pdir              */
   Vec  firstIntersection;      /* Point of intersection of Pdir, Qdir   */
   Vec  secondIntersection;     /* Second point of intersection          */
                                /* (if there is one)                     */
   int  interiorFlag;           /* Which polygon is inside the other     */
   int  contained;              /* Used for "completely contained" check */
   int  p_advances, q_advances; /* Number of times we've advanced        */
                                /* P and Q indices                       */
   int  isFirstPoint;           /* Is this the first point?              */
   int  intersectionCode;       /* SegSegIntersect() return code.        */ 


   /* Check for Q contained in P */

   contained = TRUE;

   for(ip=0; ip<np; ++ip)
   {
      ip_begin = (ip + np - 1) % np;

      mProjectCube_Cross(&P[ip_begin], &P[ip], &Pdir);
      mProjectCube_Normalize(&Pdir);

      for(iq=0; iq<nq; ++iq)
      {
         if(mProjectCube_debug >= 4)
         {
            printf("Q in P: Dot%d%d = %12.5e\n", ip, iq, mProjectCube_Dot(&Pdir, &Q[iq]));
            fflush(stdout);
         }

         if(mProjectCube_Dot(&Pdir, &Q[iq]) < -tolerance)
         {
            contained = FALSE;
            break;
         }
      }

      if(!contained)
         break;
   }

   if(contained)
   {
      if(mProjectCube_debug >= 4)
      {
         printf("Q is entirely contained in P (output pixel is in input pixel)\n");
         fflush(stdout);
      }

      for(iq=0; iq<nq; ++iq)
         mProjectCube_SaveVertex(&Q[iq]);
      
      return;
   }


   /* Check for P contained in Q */

   contained = TRUE;

   for(iq=0; iq<nq; ++iq)
   {
      iq_begin = (iq + nq - 1) % nq;

      mProjectCube_Cross(&Q[iq_begin], &Q[iq], &Qdir);
      mProjectCube_Normalize(&Qdir);

      for(ip=0; ip<np; ++ip)
      {
         if(mProjectCube_debug >= 4)
         {
            printf("P in Q: Dot%d%d = %12.5e\n", iq, ip, mProjectCube_Dot(&Qdir, &P[ip]));
            fflush(stdout);
         }

         if(mProjectCube_Dot(&Qdir, &P[ip]) < -tolerance)
         {
            contained = FALSE;
            break;
         }
      }

      if(!contained)
         break;
   }

   if(contained)
   {
      if(mProjectCube_debug >= 4)
      {
         printf("P is entirely contained in Q (input pixel is in output pixel)\n");
         fflush(stdout);
      }

      nv = 0;
      for(ip=0; ip<np; ++ip)
         mProjectCube_SaveVertex(&P[ip]);
      
      return;
   }


   /* Then check for polygon overlap */

   ip = 0;
   iq = 0;

   p_advances = 0;
   q_advances = 0;

   interiorFlag = UNKNOWN;
   isFirstPoint = TRUE;

   while(FOREVER)
   {
      if(p_advances >= 2*np) break;
      if(q_advances >= 2*nq) break;
      if(p_advances >= np && q_advances >= nq) break;

      if(mProjectCube_debug >= 4)
      {
         printf("-----\n");

         if(interiorFlag == UNKNOWN)
         {
            printf("Before advances (UNKNOWN interiorFlag): ip=%d, iq=%d ", ip, iq);
            printf("(p_advances=%d, q_advances=%d)\n", 
               p_advances, q_advances);
         }

         else if(interiorFlag == P_IN_Q)
         {
            printf("Before advances (P_IN_Q): ip=%d, iq=%d ", ip, iq);
            printf("(p_advances=%d, q_advances=%d)\n", 
               p_advances, q_advances);
         }

         else if(interiorFlag == Q_IN_P)
         {
            printf("Before advances (Q_IN_P): ip=%d, iq=%d ", ip, iq);
            printf("(p_advances=%d, q_advances=%d)\n", 
               p_advances, q_advances);
         }
         else
            printf("\nBAD INTERIOR FLAG.  Shouldn't get here\n");
            
         fflush(stdout);
      }


      /* Previous point in the polygon */

      ip_begin = (ip + np - 1) % np;
      iq_begin = (iq + nq - 1) % nq;


      /* The current polygon edges are given by  */
      /* the cross product of the vertex vectors */

      mProjectCube_Cross(&P[ip_begin], &P[ip], &Pdir);
      mProjectCube_Cross(&Q[iq_begin], &Q[iq], &Qdir);

      PToQDir = mProjectCube_DirectionCalculator(&P[ip], &Pdir, &Qdir);

      mProjectCube_Cross(&Q[iq_begin], &P[ip], &other);
      pEndpointFromQdir = mProjectCube_DirectionCalculator(&Q[iq_begin], &Qdir, &other);

      mProjectCube_Cross(&P[ip_begin], &Q[iq], &other);
      qEndpointFromPdir = mProjectCube_DirectionCalculator(&P[ip_begin], &Pdir, &other);

      if(mProjectCube_debug >= 4)
      {
         printf("   ");
         mProjectCube_printDir("P", "Q", PToQDir);
         mProjectCube_printDir("pEndpoint", "Q", pEndpointFromQdir);
         mProjectCube_printDir("qEndpoint", "P", qEndpointFromPdir);
         printf("\n");
         fflush(stdout);
      }


      /* Find point(s) of intersection between edges */

      intersectionCode = mProjectCube_SegSegIntersect(&Pdir,      &Qdir, 
                                                  &P[ip_begin], &P[ip],
                                                  &Q[iq_begin], &Q[iq], 
                                                  &firstIntersection, 
                                                  &secondIntersection);

      if(intersectionCode == NORMAL_INTERSECT 
      || intersectionCode == ENDPOINT_ONLY) 
      {
         if(interiorFlag == UNKNOWN && isFirstPoint) 
         {
            p_advances = 0;
            q_advances = 0;

            isFirstPoint = FALSE;
         }

         interiorFlag = mProjectCube_UpdateInteriorFlag(&firstIntersection, interiorFlag, 
                                                     pEndpointFromQdir, qEndpointFromPdir);

         if(mProjectCube_debug >= 4)
         {
            if(interiorFlag == UNKNOWN)
               printf("   interiorFlag -> UNKNOWN\n");

            else if(interiorFlag == P_IN_Q)
               printf("   interiorFlag -> P_IN_Q\n");

            else if(interiorFlag == Q_IN_P)
               printf("   interiorFlag -> Q_IN_P\n");

            else 
               printf("   BAD interiorFlag.  Shouldn't get here\n");

            fflush(stdout);
         }
      }


      /*-----Advance rules-----*/


      /* Special case: Pdir & Qdir overlap and oppositely oriented. */

      if((intersectionCode == COLINEAR_SEGMENTS)
      && (mProjectCube_Dot(&Pdir, &Qdir) < 0))
      {
         if(mProjectCube_debug >= 4)
         {
            printf("   ADVANCE: Pdir and Qdir are colinear.\n");
            fflush(stdout);
         }

         mProjectCube_SaveSharedSeg(&firstIntersection, &secondIntersection);

         mProjectCube_RemoveDups();
         return;
      }


      /* Special case: Pdir & Qdir parallel and separated. */
      
      if((PToQDir          == PARALLEL) 
      && (pEndpointFromQdir == CLOCKWISE) 
      && (qEndpointFromPdir == CLOCKWISE))
      {
         if(mProjectCube_debug >= 4)
         {
            printf("   ADVANCE: Pdir and Qdir are disjoint.\n");
            fflush(stdout);
         }

         mProjectCube_RemoveDups();
         return;
      }


      /* Special case: Pdir & Qdir colinear. */

      else if((PToQDir          == PARALLEL) 
           && (pEndpointFromQdir == PARALLEL) 
           && (qEndpointFromPdir == PARALLEL)) 
      {
         if(mProjectCube_debug >= 4)
         {
            printf("   ADVANCE: Pdir and Qdir are colinear.\n");
            fflush(stdout);
         }


         /* Advance but do not output point. */

         if(interiorFlag == P_IN_Q)
            iq = mProjectCube_Advance(iq, &q_advances, nq, interiorFlag == Q_IN_P, &Q[iq]);
         else
            ip = mProjectCube_Advance(ip, &p_advances, np, interiorFlag == P_IN_Q, &P[ip]);
      }


      /* Generic cases. */

      else if(PToQDir == COUNTERCLOCKWISE 
           || PToQDir == PARALLEL)
      {
         if(qEndpointFromPdir == COUNTERCLOCKWISE)
         {
            if(mProjectCube_debug >= 4)
            {
               printf("   ADVANCE: Generic: PToQDir is COUNTERCLOCKWISE ");
               printf("|| PToQDir is PARALLEL, ");
               printf("qEndpointFromPdir is COUNTERCLOCKWISE\n");
               fflush(stdout);
            }

            ip = mProjectCube_Advance(ip, &p_advances, np, interiorFlag == P_IN_Q, &P[ip]);
         }
         else
         {
            if(mProjectCube_debug >= 4)
            {
               printf("   ADVANCE: Generic: PToQDir is COUNTERCLOCKWISE ");
               printf("|| PToQDir is PARALLEL, qEndpointFromPdir is CLOCKWISE\n");
               fflush(stdout);
            }

            iq = mProjectCube_Advance(iq, &q_advances, nq, interiorFlag == Q_IN_P, &Q[iq]);
         }
      }

      else 
      {
         if(pEndpointFromQdir == COUNTERCLOCKWISE)
         {
            if(mProjectCube_debug >= 4)
            {
               printf("   ADVANCE: Generic: PToQDir is CLOCKWISE, ");
               printf("pEndpointFromQdir is COUNTERCLOCKWISE\n");
               fflush(stdout);
            }

            iq = mProjectCube_Advance(iq, &q_advances, nq, interiorFlag == Q_IN_P, &Q[iq]);
         }
         else
         {
            if(mProjectCube_debug >= 4)
            {
               printf("   ADVANCE: Generic: PToQDir is CLOCKWISE, ");
               printf("pEndpointFromQdir is CLOCKWISE\n");
               fflush(stdout);
            }

            ip = mProjectCube_Advance(ip, &p_advances, np, interiorFlag == P_IN_Q, &P[ip]);
         }
      }

      if(mProjectCube_debug >= 4)
      {
         if(interiorFlag == UNKNOWN)
         {
            printf("After  advances: ip=%d, iq=%d ", ip, iq);
            printf("(p_advances=%d, q_advances=%d) interiorFlag=UNKNOWN\n", 
               p_advances, q_advances);
         }

         else if(interiorFlag == P_IN_Q)
         {
            printf("After  advances: ip=%d, iq=%d ", ip, iq);
            printf("(p_advances=%d, q_advances=%d) interiorFlag=P_IN_Q\n", 
               p_advances, q_advances);
         }

         else if(interiorFlag == Q_IN_P)
         {
            printf("After  advances: ip=%d, iq=%d ", ip, iq);
            printf("(p_advances=%d, q_advances=%d) interiorFlag=Q_IN_P\n", 
               p_advances, q_advances);
         }
         else
            printf("BAD INTERIOR FLAG.  Shouldn't get here\n");

         printf("-----\n\n");
         fflush(stdout);
      }
   }


   mProjectCube_RemoveDups();
   return;
}




/***************************************************/
/*                                                 */
/* UpdateInteriorFlag()                            */
/*                                                 */
/* Print out the second point of intersection      */
/* and toggle in/out flag.                         */
/*                                                 */
/***************************************************/

int mProjectCube_UpdateInteriorFlag(Vec *p, int interiorFlag, 
                                int pEndpointFromQdir, int qEndpointFromPdir)
{
   double lon, lat;

   if(mProjectCube_debug >= 4)
   {
      lon = atan2(p->y, p->x)/dtr;
      lat = asin(p->z)/dtr;

      printf("   intersection [%13.6e,%13.6e,%13.6e]  -> (%10.6f,%10.6f) (UpdateInteriorFlag)\n",
         p->x, p->y, p->z, lon, lat);
      fflush(stdout);
   }

   mProjectCube_SaveVertex(p);


   /* Update interiorFlag. */

   if(pEndpointFromQdir == COUNTERCLOCKWISE)
      return P_IN_Q;

   else if(qEndpointFromPdir == COUNTERCLOCKWISE)
      return Q_IN_P;

   else /* Keep status quo. */
      return interiorFlag;
}


/***************************************************/
/*                                                 */
/* SaveSharedSeg()                                 */
/*                                                 */
/* Save the endpoints of a shared segment.         */
/*                                                 */
/***************************************************/

void mProjectCube_SaveSharedSeg(Vec *p, Vec *q)
{
   if(mProjectCube_debug >= 4)
   {
      printf("\n   SaveSharedSeg():  from [%13.6e,%13.6e,%13.6e]\n",
         p->x, p->y, p->z);

      printf("   SaveSharedSeg():  to   [%13.6e,%13.6e,%13.6e]\n\n",
         q->x, q->y, q->z);

      fflush(stdout);
   }

   mProjectCube_SaveVertex(p);
   mProjectCube_SaveVertex(q);
}



/***************************************************/
/*                                                 */
/* Advance()                                       */
/*                                                 */
/* Advances and prints out an inside vertex if     */
/* appropriate.                                    */
/*                                                 */
/***************************************************/

int mProjectCube_Advance(int ip, int *p_advances, int n, int inside, Vec *v)
{
   double lon, lat;

   lon = atan2(v->y, v->x)/dtr;
   lat = asin(v->z)/dtr;

   if(inside)
   {
      if(mProjectCube_debug >= 4)
      {
         printf("   Advance(): inside vertex [%13.6e,%13.6e,%13.6e] -> (%10.6f,%10.6f)n",
            v->x, v->y, v->z, lon, lat);

         fflush(stdout);
      }

      mProjectCube_SaveVertex(v);
   }

   (*p_advances)++;

   return (ip+1) % n;
}



/***************************************************/
/*                                                 */
/* SaveVertex()                                    */
/*                                                 */
/* Save the intersection polygon vertices          */
/*                                                 */
/***************************************************/

void mProjectCube_SaveVertex(Vec *v)
{
   int i, i_begin;
   Vec Dir;

   if(mProjectCube_debug >= 4)
      printf("   SaveVertex ... ");

   /* What with tolerance and roundoff    */
   /* problems, we need to double check   */
   /* that the point to be save is really */
   /* in or on the edge of both pixels    */
   /* P and Q                             */

   for(i=0; i<np; ++i)
   {
      i_begin = (i + np - 1) % np;

      mProjectCube_Cross(&P[i_begin], &P[i], &Dir);
      mProjectCube_Normalize(&Dir);

      if(mProjectCube_Dot(&Dir, v) < -1000.*tolerance)
      {
         if(mProjectCube_debug >= 4)
         {
            printf("rejected (not in P)\n");
            fflush(stdout);
         }

         return;
      }
   }


   for(i=0; i<nq; ++i)
   {
      i_begin = (i + nq - 1) % nq;

      mProjectCube_Cross(&Q[i_begin], &Q[i], &Dir);
      mProjectCube_Normalize(&Dir);

      if(mProjectCube_Dot(&Dir, v) < -1000.*tolerance)
      {
         if(mProjectCube_debug >= 4)
         {
            printf("rejected (not in Q)\n");
            fflush(stdout);
         }

         return;
      }
   }


   if(nv < 15)
   {
      V[nv].x = v->x;
      V[nv].y = v->y;
      V[nv].z = v->z;

      ++nv;
   }

   if(mProjectCube_debug >= 4)
   {
      printf("accepted (%d)\n", nv);
      fflush(stdout);
   }
}



/***************************************************/
/*                                                 */
/* PrintPolygon()                                  */
/*                                                 */
/* Print out the final intersection polygon        */
/*                                                 */
/***************************************************/

void mProjectCube_PrintPolygon()
{
   int    i;
   double lon, lat;

   for(i=0; i<nv; ++i)
   {
      lon = atan2(V[i].y, V[i].x)/dtr;
      lat = asin(V[i].z)/dtr;

      printf("[%13.6e,%13.6e,%13.6e] -> (%10.6f,%10.6f)\n", 
         V[i].x, V[i].y, V[i].z, lon, lat);
   }
}


/***************************************************/
/*                                                 */
/* DirectionCalculator()                           */
/*                                                 */
/* Computes whether ac is CLOCKWISE, etc. of ab    */
/*                                                 */
/***************************************************/

int mProjectCube_DirectionCalculator(Vec *a, Vec *b, Vec *c)
{
   Vec cross;
   int len;

   len = mProjectCube_Cross(b, c, &cross);

   if(len == 0)                 return PARALLEL;
   else if(mProjectCube_Dot(a, &cross) < 0.) return CLOCKWISE;
   else                         return COUNTERCLOCKWISE;
}


/****************************************************************************/
/*                                                                          */
/* SegSegIntersect()                                                        */
/*                                                                          */
/* Finds the point of intersection p between two closed                     */
/* segments ab and cd.  Returns p and a char with the following meaning:    */
/*                                                                          */
/*   COLINEAR_SEGMENTS: The segments colinearly overlap, sharing a point.   */
/*                                                                          */
/*   ENDPOINT_ONLY:     An endpoint (vertex) of one segment is on the other */
/*                      segment, but COLINEAR_SEGMENTS doesn't hold.        */
/*                                                                          */
/*   NORMAL_INTERSECT:  The segments intersect properly (i.e., they share   */
/*                      a point and neither ENDPOINT_ONLY nor               */
/*                      COLINEAR_SEGMENTS holds).                           */
/*                                                                          */
/*   NO_INTERSECTION:   The segments do not intersect (i.e., they share     */
/*                      no points).                                         */
/*                                                                          */
/* Note that two colinear segments that share just one point, an endpoint   */
/* of each, returns COLINEAR_SEGMENTS rather than ENDPOINT_ONLY as one      */
/* might expect.                                                            */
/*                                                                          */
/****************************************************************************/

int mProjectCube_SegSegIntersect(Vec *pEdge, Vec *qEdge, 
                             Vec *p0, Vec *p1, Vec *q0, Vec *q1, 
                             Vec *intersect1, Vec *intersect2)
{
   double pDot,  qDot;  /* Dot product [cos(length)] of the edge vertices */
   double p0Dot, p1Dot; /* Dot product from vertices to intersection      */
   double q0Dot, q1Dot; /* Dot product from vertices to intersection      */
   int    len;


   /* Get the edge lengths (actually cos(length)) */

   pDot = mProjectCube_Dot(p0, p1);
   qDot = mProjectCube_Dot(q0, q1);


   /* Find the point of intersection */

   len = mProjectCube_Cross(pEdge, qEdge, intersect1);


   /* If the two edges are colinear, */ 
   /* check to see if they overlap   */

   if(len == 0)
   {
      if(mProjectCube_Between(q0, p0, p1)
      && mProjectCube_Between(q1, p0, p1))
      {
         intersect1 = q0;
         intersect2 = q1;
         return COLINEAR_SEGMENTS;
      }

      if(mProjectCube_Between(p0, q0, q1)
      && mProjectCube_Between(p1, q0, q1))
      {
         intersect1 = p0;
         intersect2 = p1;
         return COLINEAR_SEGMENTS;
      }

      if(mProjectCube_Between(q0, p0, p1)
      && mProjectCube_Between(p1, q0, q1))
      {
         intersect1 = q0;
         intersect2 = p1;
         return COLINEAR_SEGMENTS;
      }

      if(mProjectCube_Between(p0, q0, q1)
      && mProjectCube_Between(q1, p0, p1))
      {
         intersect1 = p0;
         intersect2 = q1;
         return COLINEAR_SEGMENTS;
      }

      if(mProjectCube_Between(q1, p0, p1)
      && mProjectCube_Between(p1, q0, q1))
      {
         intersect1 = p0;
         intersect2 = p1;
         return COLINEAR_SEGMENTS;
      }

      if(mProjectCube_Between(q0, p0, p1)
      && mProjectCube_Between(p0, q0, q1))
      {
         intersect1 = p0;
         intersect2 = q0;
         return COLINEAR_SEGMENTS;
      }

      return NO_INTERSECTION;
   }


   /* If this is the wrong one of the two */
   /* (other side of the sky) reverse it  */

   mProjectCube_Normalize(intersect1);

   if(mProjectCube_Dot(intersect1, p0) < 0.)
      mProjectCube_Reverse(intersect1);


   /* Point has to be inside both sides to be an intersection */

   if((p0Dot = mProjectCube_Dot(intersect1, p0)) <  pDot) return NO_INTERSECTION;
   if((p1Dot = mProjectCube_Dot(intersect1, p1)) <  pDot) return NO_INTERSECTION;
   if((q0Dot = mProjectCube_Dot(intersect1, q0)) <  qDot) return NO_INTERSECTION;
   if((q1Dot = mProjectCube_Dot(intersect1, q1)) <  qDot) return NO_INTERSECTION;


   /* Otherwise, if the intersection is at an endpoint */

   if(p0Dot == pDot) return ENDPOINT_ONLY;
   if(p1Dot == pDot) return ENDPOINT_ONLY;
   if(q0Dot == qDot) return ENDPOINT_ONLY;
   if(q1Dot == qDot) return ENDPOINT_ONLY;


   /* Otherwise, it is a normal intersection */

   return NORMAL_INTERSECT;
}



/***************************************************/
/*                                                 */
/* printDir()                                      */
/*                                                 */
/* Formats a message about relative directions.    */
/*                                                 */
/***************************************************/

int mProjectCube_printDir(char *point, char *vector, int dir)
{
   if(dir == CLOCKWISE)
      printf("%s is CLOCKWISE of %s; ", point, vector);

   else if(dir == COUNTERCLOCKWISE)
      printf("%s is COUNTERCLOCKWISE of %s; ", point, vector);

   else if(dir == PARALLEL)
      printf("%s is PARALLEL to %s; ", point, vector);

   else 
      printf("Bad comparison (shouldn't get this; ");

   return 0;
}



/***************************************************/
/*                                                 */
/* Between()                                       */
/*                                                 */
/* Tests whether whether a point on an arc is      */
/* between two other points.                       */
/*                                                 */
/***************************************************/

int mProjectCube_Between(Vec *v, Vec *a, Vec *b)
{
   double abDot, avDot, bvDot;

   abDot = mProjectCube_Dot(a, b);
   avDot = mProjectCube_Dot(a, v);
   bvDot = mProjectCube_Dot(b, v);

   if(avDot > abDot
   && bvDot > abDot)
      return TRUE;
   else
      return FALSE;
}



/***************************************************/
/*                                                 */
/* Cross()                                         */
/*                                                 */
/* Vector cross product.                           */
/*                                                 */
/***************************************************/

int mProjectCube_Cross(Vec *v1, Vec *v2, Vec *v3)
{
   v3->x =  v1->y*v2->z - v2->y*v1->z;
   v3->y = -v1->x*v2->z + v2->x*v1->z;
   v3->z =  v1->x*v2->y - v2->x*v1->y;

   if(v3->x == 0.
   && v3->y == 0.
   && v3->z == 0.)
      return 0;
   
   return 1;
}


/***************************************************/
/*                                                 */
/* Dot()                                           */
/*                                                 */
/* Vector dot product.                             */
/*                                                 */
/***************************************************/

double mProjectCube_Dot(Vec *a, Vec *b)
{
   double sum = 0.0;

   sum = a->x * b->x
       + a->y * b->y
       + a->z * b->z;

   return sum;
}


/***************************************************/
/*                                                 */
/* Normalize()                                     */
/*                                                 */
/* Normalize the vector                            */
/*                                                 */
/***************************************************/

double mProjectCube_Normalize(Vec *v)
{
   double len;

   len = 0.;

   len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);

   if(len == 0.)
      len = 1.;

   v->x = v->x / len;
   v->y = v->y / len;
   v->z = v->z / len;
   
   return len;
}


/***************************************************/
/*                                                 */
/* Reverse()                                       */
/*                                                 */
/* Reverse the vector                              */
/*                                                 */
/***************************************************/

void mProjectCube_Reverse(Vec *v)
{
   v->x = -v->x;
   v->y = -v->y;
   v->z = -v->z;
}


/***************************************************/
/*                                                 */
/* Girard()                                        */
/*                                                 */
/* Use Girard's theorem to compute the area of a   */
/* sky polygon.                                    */
/*                                                 */
/***************************************************/

double mProjectCube_Girard()
{
   int    i, j, ibad;

   double area;
   double lon, lat;

   Vec    side[16];
   double ang [16];

   Vec    tmp;

   double pi, dtr, sumang, cosAng, sinAng;

   pi  = atan(1.0) * 4.;
   dtr = pi / 180.;

   sumang = 0.;

   if(nv < 3)
      return(0.);

   if(mProjectCube_debug >= 4)
   {
      for(i=0; i<nv; ++i)
      {
         lon = atan2(V[i].y, V[i].x)/dtr;
         lat = asin(V[i].z)/dtr;

         printf("Girard(): %3d [%13.6e,%13.6e,%13.6e] -> (%10.6f,%10.6f)\n", 
            i, V[i].x, V[i].y, V[i].z, lon, lat);
         
         fflush(stdout);
      }
   }

   for(i=0; i<nv; ++i)
   {
      mProjectCube_Cross (&V[i], &V[(i+1)%nv], &side[i]);

      (void) mProjectCube_Normalize(&side[i]);
   }

   for(i=0; i<nv; ++i)
   {
      mProjectCube_Cross (&side[i], &side[(i+1)%nv], &tmp);

      sinAng =  mProjectCube_Normalize(&tmp);
      cosAng = -mProjectCube_Dot(&side[i], &side[(i+1)%nv]);


      /* Remove center point of colinear segments */

      ang[i] = atan2(sinAng, cosAng);

      if(mProjectCube_debug >= 4)
      {
         if(i==0)
            printf("\n");

         printf("Girard(): angle[%d] = %13.6e -> %13.6e (from %13.6e / %13.6e)\n", i, ang[i], ang[i] - pi/2., sinAng, cosAng);
         fflush(stdout);
      }

      if(ang[i] > pi - 0.0175)  /* Direction changes of less than */
      {                         /* a degree can be tricky         */
         ibad = (i+1)%nv;

         if(mProjectCube_debug >= 4)
         {
            printf("Girard(): ---------- Corner %d bad; Remove point %d -------------\n", 
               i, ibad);
            fflush(stdout);
         }

         --nv;

         for(j=ibad; j<nv; ++j)
         {
            V[j].x = V[j+1].x;
            V[j].y = V[j+1].y;
            V[j].z = V[j+1].z;
         }

         return(mProjectCube_Girard());
      }

      sumang += ang[i];
   }

   area = sumang - (nv-2.)*pi;

   if(mNaN(area) || area < 0.)
      area = 0.;

   if(mProjectCube_debug >= 4)
   {
      printf("\nGirard(): area = %13.6e [%d]\n\n", area, nv);
      fflush(stdout);
   }

   return(area);
}




/***************************************************/
/*                                                 */
/* RemoveDups()                                    */
/*                                                 */
/* Check the vertex list for adjacent pairs of     */
/* points which are too close together for the     */
/* subsequent dot- and cross-product calculations  */
/* of Girard's theorem.                            */
/*                                                 */
/***************************************************/

void mProjectCube_RemoveDups()
{
   int    i, nvnew;
   Vec    Vnew[16];
   Vec    tmp;
   double lon, lat;

   double separation;

   if(mProjectCube_debug >= 4)
   {
      printf("RemoveDups() tolerance = %13.6e [%13.6e arcsec]\n\n", 
         tolerance, tolerance/dtr*3600.);

      for(i=0; i<nv; ++i)
      {
         lon = atan2(V[i].y, V[i].x)/dtr;
         lat = asin(V[i].z)/dtr;

         printf("RemoveDups() orig: %3d [%13.6e,%13.6e,%13.6e] -> (%10.6f,%10.6f)\n", 
            i, V[i].x, V[i].y, V[i].z, lon, lat);
         
         fflush(stdout);
      }

      printf("\n");
   }

   Vnew[0].x = V[0].x;
   Vnew[0].y = V[0].y;
   Vnew[0].z = V[0].z;

   nvnew = 0;

   for(i=0; i<nv; ++i)
   {
      ++nvnew;

      Vnew[nvnew].x = V[(i+1)%nv].x;
      Vnew[nvnew].y = V[(i+1)%nv].y;
      Vnew[nvnew].z = V[(i+1)%nv].z;

      mProjectCube_Cross (&V[i], &V[(i+1)%nv], &tmp);

      separation = mProjectCube_Normalize(&tmp);

      if(mProjectCube_debug >= 4)
      {
         printf("RemoveDups(): %3d x %3d: distance = %13.6e [%13.6e arcsec] (would become %d)\n", 
            (i+1)%nv, i, separation, separation/dtr*3600., nvnew);

         fflush(stdout);
      }

      if(separation < tolerance)
      {
         --nvnew;

         if(mProjectCube_debug >= 4)
         {
            printf("RemoveDups(): %3d is a duplicate (nvnew -> %d)\n",
               i, nvnew);

            fflush(stdout);
         }
      }
   }

   if(mProjectCube_debug >= 4)
   {
      printf("\n");
      fflush(stdout);
   }

   if(nvnew < nv)
   {
      for(i=0; i<nvnew; ++i)
      {
         V[i].x = Vnew[i].x;
         V[i].y = Vnew[i].y;
         V[i].z = Vnew[i].z;
      }

      nv = nvnew;
   }
}
