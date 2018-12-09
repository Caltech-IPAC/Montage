/* Module: mCubeShrink.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.1      John Good        08Sep15  fits_read_pix() incorrect null value
1.0      John Good        14Apr15  Baseline code, based on mShrink of that date

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>

#include <fitsio.h>
#include <wcs.h>

#include <mShrinkCube.h>
#include <montage.h>

#define MAXSTR 1024

static int  haveCtype;
static int  haveCrval;
static int  haveCrpix;
static int  haveCnpix;
static int  havePixelsz;
static int  havePP;
static int  haveCdelt;
static int  haveCdelt3;
static int  haveCrota2;
static int  haveCD11;
static int  haveCD12;
static int  haveCD21;
static int  haveCD22;
static int  havePC11;
static int  havePC12;
static int  havePC21;
static int  havePC22;
static int  haveEpoch;
static int  haveEquinox;
static int  haveBunit;
static int  haveBlank;

static struct
{
   fitsfile *fptr;
   long      bitpix;
   long      naxis;
   long      naxes[4];
   char      ctype1[16];
   char      ctype2[16];
   double    crval1, crval2;
   double    crpix1, crpix2;
   double    cnpix1, cnpix2;
   double    xpixelsz, ypixelsz;
   double    ppo3, ppo6;
   double    cdelt1, cdelt2, cdelt3;
   double    crota2;
   double    cd11;
   double    cd12;
   double    cd21;
   double    cd22;
   double    pc11;
   double    pc12;
   double    pc21;
   double    pc22;
   double    epoch;
   double    equinox;
   char      bunit[80];
   long      blank;
}
input, output;

static time_t currtime, start;

static int hdu;


static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mShrinkCube                                                          */
/*                                                                       */
/*  This module, is a utility program for making smaller versions of a   */
/*  FITS file by averaging NxN blocks of pixels spatially and M values   */
/*  in the third and fourth cube dimensions.  N can be fractional but    */
/*  M must be an integer.                                                */
/*                                                                       */
/*   char  *infile         Input FITS file                               */
/*   char  *output_file    Shrunken output FITS file                     */
/*                                                                       */
/*   double shrinkFactor   Scale factor for spatial shrinking.  Can be   */
/*                         any positive real number                      */
/*                                                                       */
/*   int    mfactor        Positive integer scale factor for shrinking   */
/*                         the third cube dimension                      */
/*                                                                       */
/*   int    hdu            Optional HDU offset for input file            */
/*                                                                       */
/*   int    fixedSize      Alternate mode: shrink so the output fits     */
/*                         in this many pixels                           */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mShrinkCubeReturn *mShrinkCube(char *input_file, char *output_file, double shrinkFactor, 
                                int mfactor, int hduin, int fixedSize, int debug)
{
   int       i, j, ii, jj, status, bufrow, split;
   int       ibuffer, jbuffer, ifactor, nbuf, nullcnt, k, l, imin, imax, jmin, jmax;
   int       m, j3, j4;
   long      fpixel[4], fpixelo[4], nelements, nelementso;
   double    obegin, oend;
   double   *colfact, *rowfact;
   double   *buffer;
   double   *mbuffer;
   double    xfactor, flux, area;

   double   *outdata;
   double  **indata;

   struct mShrinkCubeReturn *returnStruct;


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
   /**n****************************/

   returnStruct = (struct mShrinkCubeReturn *)malloc(sizeof(struct mShrinkCubeReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   time(&currtime);
   start = currtime;

   hdu = hduin;

   xfactor = shrinkFactor;

   if(!fixedSize)
   {
      ifactor = ceil(xfactor);

      if((double)ifactor < xfactor)
         xfactor += 2;
   }

   if(xfactor <= 0)
   {
      if(fixedSize)
         sprintf(returnStruct->msg, "Requested image size must be positive");
      else
         sprintf(returnStruct->msg, "Shrink factor must be positive");

      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("input_file       = [%s]\n", input_file);
      printf("output_file      = [%s]\n", output_file);
      printf("xfactor          = %-g\n",  xfactor);
      printf("ifactor          = %d\n",   ifactor);
      printf("mfactor          = %d\n",   mfactor);
      fflush(stdout);
   }


   /************************/
   /* Read the input image */
   /************************/

   if(mShrinkCube_readFits(input_file) > 0)
   {  
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   // Error if we are trying to shrink to less than one pixel

   if(!fixedSize
   && (   shrinkFactor > input.naxes[0]
       || shrinkFactor > input.naxes[1]))
   {
      mShrinkCube_printError("Trying to shrink image to smaller than one pixel");           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   if(debug >= 1)
   {
      printf("\nflux file            =  %s\n",  input_file);
      printf("input.bitpix         =  %ld\n",   input.bitpix);
      printf("input.naxes[0]       =  %ld\n",   input.naxes[0]);
      printf("input.naxes[1]       =  %ld\n",   input.naxes[1]);

      if(haveCtype)   printf("input.ctype1         =  %s\n",    input.ctype1);
      if(haveCtype)   printf("input.typel2         =  %s\n",    input.ctype2);
      if(haveCrval)   printf("input.crval1         =  %-g\n",   input.crval1);
      if(haveCrval)   printf("input.crval2         =  %-g\n",   input.crval2);
      if(haveCrpix)   printf("input.crpix1         =  %-g\n",   input.crpix1);
      if(haveCrpix)   printf("input.crpix2         =  %-g\n",   input.crpix2);
      if(haveCnpix)   printf("input.cnpix1         =  %-g\n",   input.cnpix1);
      if(haveCnpix)   printf("input.cnpix2         =  %-g\n",   input.cnpix2);
      if(havePixelsz) printf("input.xpixelsz       =  %-g\n",   input.xpixelsz);
      if(havePixelsz) printf("input.ypixelsz       =  %-g\n",   input.ypixelsz);
      if(havePP)      printf("input.ppo3           =  %-g\n",   input.ppo3);
      if(havePP)      printf("input.ppo6           =  %-g\n",   input.ppo6);
      if(haveCdelt)   printf("input.cdelt1         =  %-g\n",   input.cdelt1);
      if(haveCdelt)   printf("input.cdelt2         =  %-g\n",   input.cdelt2);
      if(haveCdelt3)  printf("input.cdelt3         =  %-g\n",   input.cdelt3);
      if(haveCrota2)  printf("input.crota2         =  %-g\n",   input.crota2);
      if(haveCD11)    printf("input.cd11           =  %-g\n",   input.cd11);
      if(haveCD12)    printf("input.cd12           =  %-g\n",   input.cd12);
      if(haveCD21)    printf("input.cd21           =  %-g\n",   input.cd21);
      if(haveCD22)    printf("input.cd22           =  %-g\n",   input.cd22);
      if(havePC11)    printf("input.pc11           =  %-g\n",   input.pc11);
      if(havePC12)    printf("input.pc12           =  %-g\n",   input.pc12);
      if(havePC21)    printf("input.pc21           =  %-g\n",   input.pc21);
      if(havePC22)    printf("input.pc22           =  %-g\n",   input.pc22);
      if(haveEpoch)   printf("input.epoch          =  %-g\n",   input.epoch);
      if(haveEquinox) printf("input.equinox        =  %-g\n",   input.equinox);
      if(haveBunit)   printf("input.bunit          =  %s\n",    input.bunit);
      if(haveBlank)   printf("input.blank          =  %ld\n",   input.blank);
      printf("\n");

      fflush(stdout);
   }


   /***********************************************/
   /* If we are going for a fixed size, the scale */
   /* factor needs to be computed.                */
   /***********************************************/

   if(fixedSize)
   {
      if(input.naxes[0] > input.naxes[1])
         xfactor = (double)input.naxes[0]/(int)xfactor;
      else
         xfactor = (double)input.naxes[1]/(int)xfactor;

      ifactor = ceil(xfactor);

      if((double)ifactor < xfactor)
         xfactor += 2;

      if(debug >= 1)
      {
         printf("xfactor         -> %-g\n",  xfactor);
         printf("ifactor         -> %d\n",   ifactor);
         fflush(stdout);
      }
   }

   /***********************************************/
   /* Compute all the parameters for the shrunken */
   /* output file.                                */
   /***********************************************/

   output.naxis    = input.naxis;
   output.naxes[0] = floor((double)input.naxes[0]/xfactor);
   output.naxes[1] = floor((double)input.naxes[1]/xfactor);
   output.naxes[2] = input.naxes[2]/mfactor;
   output.naxes[3] = input.naxes[3];

   if(debug >= 1)
   {
      printf("output.naxes[0] = %ld\n",  output.naxes[0]);
      printf("output.naxes[1] = %ld\n",  output.naxes[1]);
      fflush(stdout);
   }

   strcpy(output.ctype1, input.ctype1);
   strcpy(output.ctype2, input.ctype2);

   output.crval1   = input.crval1;
   output.crval2   = input.crval2;
   output.crpix1   = (input.crpix1-0.5)/xfactor + 0.5;
   output.crpix2   = (input.crpix2-0.5)/xfactor + 0.5;
   output.cdelt1   = input.cdelt1*xfactor;
   output.cdelt2   = input.cdelt2*xfactor;
   output.crota2   = input.crota2;
   output.cd11     = input.cd11*xfactor;
   output.cd12     = input.cd12*xfactor;
   output.cd21     = input.cd21*xfactor;
   output.cd22     = input.cd22*xfactor;
   output.pc11     = input.pc11;
   output.pc12     = input.pc12;
   output.pc21     = input.pc21;
   output.pc22     = input.pc22;
   output.epoch    = input.epoch;
   output.equinox  = input.equinox;

   output.cdelt3   = input.cdelt3 * mfactor;

   strcpy(output.bunit, input.bunit);

   if(haveCnpix)
   {
      input.crpix1    = input.ppo3 / input.xpixelsz - input.cnpix1 + 0.5; 
      input.crpix2    = input.ppo6 / input.ypixelsz - input.cnpix2 + 0.5; 

      output.crpix1   = (input.crpix1-0.5)/xfactor + 0.5;
      output.crpix2   = (input.crpix2-0.5)/xfactor + 0.5;

      output.xpixelsz = input.xpixelsz * xfactor;
      output.ypixelsz = input.ypixelsz * xfactor;

      output.cnpix1   = input.ppo3 / output.xpixelsz - output.crpix1 + 0.5;
      output.cnpix2   = input.ppo6 / output.ypixelsz - output.crpix2 + 0.5;
   }


   /********************************/
   /* Create the output FITS files */
   /********************************/

   status = 0;

   remove(output_file);               

   if(fits_create_file(&output.fptr, output_file, &status)) 
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /******************************************************/
   /* Create the FITS image.  Copy over the whole header */
   /******************************************************/

   if(fits_copy_header(input.fptr, output.fptr, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("\nFITS header copied to output\n"); 
      fflush(stdout);
   }


   /************************************/
   /* Reset all the WCS header kewords */
   /************************************/

   if(fits_update_key_lng(output.fptr, "NAXIS", output.naxis,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS1", output.naxes[0],
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS2", output.naxes[1],
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(output.naxis >= 3)
   {
      if(fits_update_key_lng(output.fptr, "NAXIS3", output.naxes[2],
                                     (char *)NULL, &status))
      {
         mShrinkCube_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }

   if(output.naxis == 4)
   {
      if(fits_update_key_lng(output.fptr, "NAXIS4", output.naxes[3],
                                     (char *)NULL, &status))
      {
         mShrinkCube_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }

   if(haveBunit && fits_update_key_str(output.fptr, "BUNIT", output.bunit,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveBlank && fits_update_key_lng(output.fptr, "BLANK", output.blank,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCtype && fits_update_key_str(output.fptr, "CTYPE1", output.ctype1,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCtype && fits_update_key_str(output.fptr, "CTYPE2", output.ctype2,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCrval && fits_update_key_dbl(output.fptr, "CRVAL1", output.crval1, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCrval && fits_update_key_dbl(output.fptr, "CRVAL2", output.crval2, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCrpix && fits_update_key_dbl(output.fptr, "CRPIX1", output.crpix1, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCrpix && fits_update_key_dbl(output.fptr, "CRPIX2", output.crpix2, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCnpix && fits_update_key_dbl(output.fptr, "CNPIX1", output.cnpix1, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCnpix && fits_update_key_dbl(output.fptr, "CNPIX2", output.cnpix2, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(havePixelsz && fits_update_key_dbl(output.fptr, "XPIXELSZ", output.xpixelsz, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(havePixelsz && fits_update_key_dbl(output.fptr, "YPIXELSZ", output.ypixelsz, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCdelt && fits_update_key_dbl(output.fptr, "CDELT1", output.cdelt1, -14,
                                     (char *)NULL, &status))
      {
         mShrinkCube_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

   if(haveCdelt && fits_update_key_dbl(output.fptr, "CDELT2", output.cdelt2, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCdelt3 && fits_update_key_dbl(output.fptr, "CDELT3", output.cdelt3, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCrota2 && fits_update_key_dbl(output.fptr, "CROTA2", output.crota2, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCD11 && fits_update_key_dbl(output.fptr, "CD1_1", output.cd11, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCD12 && fits_update_key_dbl(output.fptr, "CD1_2", output.cd12, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCD21 && fits_update_key_dbl(output.fptr, "CD2_1", output.cd21, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveCD22 && fits_update_key_dbl(output.fptr, "CD2_2", output.cd22, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(havePC11 && fits_update_key_dbl(output.fptr, "PC1_1", output.pc11, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(havePC12 && fits_update_key_dbl(output.fptr, "PC1_2", output.pc12, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(havePC21 && fits_update_key_dbl(output.fptr, "PC2_1", output.pc21, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(havePC22 && fits_update_key_dbl(output.fptr, "PC2_2", output.pc22, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveEpoch && fits_update_key_dbl(output.fptr, "EPOCH", output.epoch, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(haveEquinox && fits_update_key_dbl(output.fptr, "EQUINOX", output.equinox, -14,
                                  (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   if(debug >= 1)
   {
      printf("Output header keywords set\n\n");
      fflush(stdout);
   }



   /***********************************************/ 
   /* Allocate memory for a line of output pixels */ 
   /***********************************************/ 

   outdata = (double *)malloc(output.naxes[0] * sizeof(double));


   /*************************************************************************/ 
   /* We could probably come up with logic that would work for both scale   */
   /* factors of less than one and greater than one but the it would be too */
   /* hard to follow.  Instead, we put in a big switch here to deal with    */
   /* the two cases separately.                                             */
   /*************************************************************************/ 

   if(xfactor < 1.)
   {
      /************************************************/ 
      /* Allocate memory for "ifactor" lines of input */ 
      /************************************************/ 

      nbuf = 2;

      indata = (double **)malloc(nbuf * sizeof(double *));

      for(j=0; j<nbuf; ++j)
         indata[j] = (double *)malloc((input.naxes[0]+1) * sizeof(double));


      /**********************************************************/
      /* Create the output array by processing the input pixels */
      /**********************************************************/

      ibuffer = 0;

      buffer  = (double *)malloc(input.naxes[0] * sizeof(double));
      mbuffer = (double *)malloc(input.naxes[0] * sizeof(double));
      colfact = (double *)malloc(nbuf * sizeof(double));
      rowfact = (double *)malloc(nbuf * sizeof(double));

      fpixel [0] = 1;
      fpixelo[0] = 1;
      fpixelo[2] = 0;

      nelements = input.naxes[0];

      status = 0;


      /******************************/
      /* Loop over the output lines */
      /******************************/

      split = 0;

      for (j4=1; j4<=input.naxes[3]; ++j4)
      {
         fpixelo[2] = 1;

         for (j3=1; j3<=input.naxes[2]; j3+=mfactor)
         {
            fpixel[3] = j4;
            fpixel[1] = 1;

            fpixelo[3] = j4;
            fpixelo[1] = 1;

            for(l=0; l<output.naxes[1]; ++l)
            {
               obegin = (fpixelo[1] - 1.) * xfactor;
               oend   =  fpixelo[1] * xfactor;

               if(floor(oend) == oend)
                  oend = obegin;

               if(debug >= 2)
               {
                  printf("OUTPUT row %d: obegin = %.2f -> oend = %.3f\n\n", l, obegin, oend);
                  fflush(stdout);
               }

               rowfact[0] = 1.;
               rowfact[1] = 0.;


               /******************************************/
               /* If we have gone over into the next row */
               /******************************************/

               if(l == 0 || (int)oend > (int)obegin)
               {
                  rowfact[0] = 1.;
                  rowfact[1] = 0.;

                  if(l > 0)
                  {
                     split = 1;

                     jbuffer = (ibuffer + 1) % nbuf;

                     rowfact[1] = (oend - (int)(fpixelo[1] * xfactor))/xfactor;
                     rowfact[0] = 1. - rowfact[1];
                  }
                  else
                  {
                     jbuffer = 0;
                  }

                  if(debug >= 2)
                  {
                     printf("Reading input image row %5ld  (ibuffer %d)\n", fpixel[1], jbuffer);
                     fflush(stdout);
                  }

                  if(debug >= 2)
                  {
                     printf("Rowfact:  %-g %-g\n", rowfact[0], rowfact[1]);
                     fflush(stdout);
                  }


                  /***********************************/
                  /* Read a line from the input file */
                  /***********************************/

                  if(fpixel[1] <= input.naxes[1])
                  {
                     for(i=0; i<input.naxes[0]; ++i)
                        buffer[i] = 0.;

                     if(j3 + mfactor > input.naxes[2])
                        continue;

                     fpixel[2] = j3;

                     for(m=0; m<mfactor; ++m)
                     {
                        if(fits_read_pix(input.fptr, TDOUBLE, fpixel, nelements, &nan,
                                         mbuffer, &nullcnt, &status))
                        {
                           mShrinkCube_printFitsError(status);
                           strcpy(returnStruct->msg, montage_msgstr);
                           return returnStruct;
                        }

                        for(i=0; i<input.naxes[0]; ++i)
                           buffer[i] += mbuffer[i]/mfactor;

                        ++fpixel[2];
                     }
                  }
                  
                  ++fpixel[1];


                  /************************/
                  /* For each input pixel */
                  /************************/

                  indata[jbuffer][input.naxes[0]] = nan;

                  for (i=0; i<input.naxes[0]; ++i)
                  {
                     indata[jbuffer][i] = buffer[i];

                     if(debug >= 4)
                     {
                        printf("input: line %5ld / pixel %5d: indata[%d][%d] = %10.3e\n",
                           fpixel[1]-2, i, jbuffer, i, indata[jbuffer][i]);
                        fflush(stdout);
                     }
                  }

                  if(debug >= 4)
                  {
                     printf("---\n");
                     fflush(stdout);
                  }
               }


               /*************************************/
               /* Write out the next line of output */
               /*************************************/

               nelementso = output.naxes[0];

               for(k=0; k<nelementso; ++k)
               {
                  /* When "expanding" we never need to use more than two   */
                  /* pixels in more than two rows.  The row factors were   */
                  /* computed above and the column factors will be compute */
                  /* here as we go.                                        */

                  outdata[k] = nan;

                  colfact[0] = 1.;
                  colfact[1] = 0.;

                  obegin =  (double)k     * xfactor;
                  oend   = ((double)k+1.) * xfactor;

                  if(floor(oend) == oend)
                     oend = obegin;

                  imin = (int)obegin;

                  if((int)oend > (int)obegin)
                  {
                     colfact[1] = (oend - (int)(((double)k+1.) * xfactor))/xfactor;
                     colfact[0] = 1. - colfact[1];
                  }

                  flux = 0;
                  area = 0;

                  for(jj=0; jj<2; ++jj)
                  {
                     if(rowfact[jj] == 0.)
                        continue;

                     for(ii=0; ii<2; ++ii)
                     {
                        bufrow = (ibuffer + jj) % nbuf;

                        if(!mNaN(indata[bufrow][imin+ii]) && (colfact[ii] > 0.))
                        {
                           flux += indata[bufrow][imin+ii] * colfact[ii] * rowfact[jj];
                           area += colfact[ii] * rowfact[jj];

                           if(debug >= 3)
                           {
                              printf("output[%d][%d] -> %10.2e (area: %10.2e) (using indata[%d][%d] = %10.2e, colfact[%d] = %5.3f, rowfact[%d] = %5.3f)\n", 
                                 l, k, flux, area,
                                 bufrow, imin+ii, indata[bufrow][imin+ii], 
                                 imin+ii, colfact[ii],
                                 jj, rowfact[jj]);

                              fflush(stdout);
                           }
                        }
                     }
                  }

                  if(area > 0.)
                     outdata[k] = flux/area;
                  else
                     outdata[k] = nan;

                  if(debug >= 3)
                  {
                     printf("\nflux[%d] = %-g / area = %-g --> outdata[%d] = %-g\n",
                        k, flux, area, k, outdata[k]);
                     
                     printf("---\n");
                     fflush(stdout);
                  }
               }

               if(fpixelo[1] <= output.naxes[1])
               {
                  if(debug >= 2)
                  {
                     printf("\nWRITE output image row %5ld\n===========================================\n", fpixelo[1]);
                     fflush(stdout);
                  }

                  if (fits_write_pix(output.fptr, TDOUBLE, fpixelo, nelementso, 
                                     (void *)(outdata), &status))
                  {
                     mShrinkCube_printFitsError(status);
                     strcpy(returnStruct->msg, montage_msgstr);
                     return returnStruct;
                  }
               }

               ++fpixelo[1];

               if(split)
               {
                  ibuffer = jbuffer;
                  split = 0;
               }


               /***************************************************************/
               /* Special case:  The expansion factor is integral and we have */
               /* gotten to the point where we need the next line.            */
               /***************************************************************/

               oend   =  fpixelo[1] * xfactor;

               if(fpixel[1] <= input.naxes[1] && floor(oend) == oend)
               {
                  if(debug >= 2)
                  {
                     printf("Reading input image row %5ld  (ibuffer %d)\n", fpixel[1], jbuffer);
                     fflush(stdout);
                  }

                  if(fits_read_pix(input.fptr, TDOUBLE, fpixel, nelements, &nan,
                                   buffer, &nullcnt, &status))
                  {
                     mShrinkCube_printFitsError(status);
                     strcpy(returnStruct->msg, montage_msgstr);
                     return returnStruct;
                  }
                  
                  ++fpixel[1];

                  indata[jbuffer][input.naxes[0]] = nan;

                  for (i=0; i<input.naxes[0]; ++i)
                  {
                     indata[jbuffer][i] = buffer[i];

                     if(debug >= 4)
                     {
                        printf("input: line %5ld / pixel %5d: indata[%d][%d] = %10.3e\n",
                           fpixel[1]-2, i, jbuffer, i, indata[jbuffer][i]);
                        fflush(stdout);
                     }
                  }

                  if(debug >= 4)
                  {
                     printf("---\n");
                     fflush(stdout);
                  }
               }
            }
         }

         ++fpixelo[2];
      }
   }
   else
   {
      /************************************************/ 
      /* Allocate memory for "ifactor" lines of input */ 
      /************************************************/ 

      nbuf = ifactor + 1;

      indata = (double **)malloc(nbuf * sizeof(double *));

      for(j=0; j<nbuf; ++j)
         indata[j] = (double *)malloc(input.naxes[0] * sizeof(double));



      /**********************************************************/
      /* Create the output array by processing the input pixels */
      /**********************************************************/

      ibuffer = 0;

      buffer  = (double *)malloc(input.naxes[0] * sizeof(double));
      mbuffer = (double *)malloc(input.naxes[0] * sizeof(double));
      colfact = (double *)malloc(input.naxes[0] * sizeof(double));
      rowfact = (double *)malloc(input.naxes[1] * sizeof(double));

      fpixel [0] = 1;
      fpixelo[0] = 1;
      fpixelo[2] = 0;

      nelements = input.naxes[0];


      /*****************************/
      /* Loop over the input lines */
      /*****************************/

      for (j4=1; j4<=input.naxes[3]; ++j4)
      {
         fpixelo[2] = 1;

         for (j3=1; j3<=input.naxes[2]; j3+=mfactor)
         {
            fpixel[3] = j4;
            fpixel[1] = 1;

            fpixelo[3] = j4;
            fpixelo[1] = 1;

            status = 0;

            l = 0;

            obegin =  (double)l     * xfactor;
            oend   = ((double)l+1.) * xfactor;

            jmin = floor(obegin);
            jmax = ceil (oend);

            for(jj=jmin; jj<=jmax; ++jj)
            {
               rowfact[jj-jmin] = 1.;

                    if(jj <= obegin && jj+1 <= oend) rowfact[jj-jmin] = jj+1. - obegin;
               else if(jj <= obegin && jj+1 >= oend) rowfact[jj-jmin] = oend - obegin;
               else if(jj >= obegin && jj+1 >= oend) rowfact[jj-jmin] = oend - jj;

               if(rowfact[jj-jmin] < 0.)
                  rowfact[jj-jmin] = 0.;

               if(debug >= 4)
               {
                  printf("rowfact[%d]  %-g\n", jj, rowfact[jj]);
                  fflush(stdout);
               }
            }

            for (j=0; j<input.naxes[1]; ++j)
            {
               if(debug >= 2)
               {
                  printf("Reading input image row %5ld  (ibuffer %d)\n", fpixel[1], ibuffer);
                  fflush(stdout);
               }


               /***********************************/
               /* Read a line from the input file */
               /***********************************/

               for(i=0; i<input.naxes[0]; ++i)
                  buffer[i] = 0.;

               if(j3 + mfactor > input.naxes[2])
                  continue;

               fpixel[2] = j3;

               for(m=0; m<mfactor; ++m)
               {
                  if(fits_read_pix(input.fptr, TDOUBLE, fpixel, nelements, &nan,
                                   mbuffer, &nullcnt, &status))
                  {
                     mShrinkCube_printFitsError(status);
                     strcpy(returnStruct->msg, montage_msgstr);
                     return returnStruct;
                  }

                  for(i=0; i<input.naxes[0]; ++i)
                     buffer[i] += mbuffer[i]/mfactor;

                  ++fpixel[2];
               }
               
               ++fpixel[1];

               /************************/
               /* For each input pixel */
               /************************/

               for (i=0; i<input.naxes[0]; ++i)
               {
                  indata[ibuffer][i] = buffer[i];

                  if(debug >= 4)
                  {
                     printf("input: line %5d / pixel %5d: indata[%d][%d] = %10.2e\n",
                        j, i, ibuffer, i, indata[ibuffer][i]);
                     fflush(stdout);
                  }
               }

               if(debug >= 4)
               {
                  printf("---\n");
                  fflush(stdout);
               }


               /**************************************************/
               /* If we have enough for the next line of output, */
               /* compute and write it                           */
               /**************************************************/

               if(j == jmax || fpixel[1] == input.naxes[1])
               {
                  nelementso = output.naxes[0];

                  for(k=0; k<nelementso; ++k)
                  {
                     /* OK, we are trying to determine the correct flux   */
                     /* for output pixel k in output line l.  We have all */
                     /* the input lines we need (modulo looping back from */
                     /* indata[ibuffer])                                  */

                     outdata[k] = nan;

                     obegin =  (double)k     * xfactor;
                     oend   = ((double)k+1.) * xfactor;

                     imin = floor(obegin);
                     imax = ceil (oend);

                     if(debug >= 3)
                     {
                        printf("\nimin = %4d, imax = %4d, jmin = %4d, jmax = %4d\n", imin, imax, jmin, jmax);
                        fflush(stdout);
                     }

                     flux = 0;
                     area = 0;

                     for(ii=imin; ii<=imax; ++ii)
                     {
                        colfact[ii-imin] = 1.;

                             if(ii <= obegin && ii+1 <= oend) colfact[ii-imin] = ii+1. - obegin;
                        else if(ii <= obegin && ii+1 >= oend) colfact[ii-imin] = oend - obegin;
                        else if(ii >= obegin && ii+1 >= oend) colfact[ii-imin] = oend - ii;

                        if(colfact[ii-imin] < 0.)
                           colfact[ii-imin] = 0.;
                     }

                     for(jj=jmin; jj<=jmax; ++jj)
                     {
                        if(rowfact[jj-jmin] == 0.)
                           continue;

                        for(ii=imin; ii<=imax; ++ii)
                        {
                           bufrow = (ibuffer - jmax + jj + nbuf) % nbuf;

                           if(!mNaN(indata[bufrow][ii]) && (colfact[ii-imin] > 0.))
                           {
                              flux += indata[bufrow][ii] * colfact[ii-imin] * rowfact[jj-jmin];
                              area += colfact[ii-imin] * rowfact[jj-jmin];

                              if(debug >= 3)
                              {
                                 printf("output[%d][%d] -> %10.2e (area: %10.2e) (using indata[%d][%d] = %10.2e, colfact[%d-%d] = %5.3f, rowfact[%d-%d] = %5.3f)\n", 
                                    l, k, flux, area,
                                    bufrow, ii, indata[bufrow][ii], 
                                    ii, imin, colfact[ii-imin],
                                    jj, jmin, rowfact[jj-jmin]);

                                 fflush(stdout);
                              }
                           }
                        }

                        if(debug >= 3)
                        {
                           printf("---\n");
                           fflush(stdout);
                        }
                     }

                     if(area > 0.)
                        outdata[k] = flux/area;
                     else
                        outdata[k] = nan;

                     if(debug >= 3)
                     {
                        printf("\nflux = %-g / area = %-g --> outdata[%d] = %-g\n",
                           flux, area, k, outdata[k]);
                        
                        fflush(stdout);
                     }
                  }

                  if(fpixelo[1] <= output.naxes[1])
                  {
                     if(debug >= 2)
                     {
                        printf("\nWRITE output image row %5ld\n===========================================\n", fpixelo[1]);
                        fflush(stdout);
                     }

                     if (fits_write_pix(output.fptr, TDOUBLE, fpixelo, nelementso, 
                                        (void *)(outdata), &status))
                     {
                        mShrinkCube_printFitsError(status);
                        strcpy(returnStruct->msg, montage_msgstr);
                        return returnStruct;
                     }
                  }

                  ++fpixelo[1];

                  ++l;

                  obegin =  (double)l     * xfactor;
                  oend   = ((double)l+1.) * xfactor;

                  jmin = floor(obegin);
                  jmax = ceil (oend);

                  for(jj=jmin; jj<=jmax; ++jj)
                  {
                     rowfact[jj-jmin] = 1.;

                          if(jj <= obegin && jj+1 <= oend) rowfact[jj-jmin] = jj+1. - obegin;
                     else if(jj <= obegin && jj+1 >= oend) rowfact[jj-jmin] = oend - obegin;
                     else if(jj >= obegin && jj+1 >= oend) rowfact[jj-jmin] = oend - jj;

                     if(rowfact[jj-jmin] < 0.)
                        rowfact[jj-jmin] = 0.;

                     if(debug >= 4)
                     {
                        printf("rowfact[%d-%d] -> %-g\n", jj, jmin, rowfact[jj-jmin]);
                        fflush(stdout);
                     }
                  }
               }

               ibuffer = (ibuffer + 1) % nbuf;
            }

            ++fpixelo[2];
         }
      }
   }


   /*******************/
   /* Close the files */
   /*******************/

   if(fits_close_file(input.fptr, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_close_file(output.fptr, &status))
   {
      mShrinkCube_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("FITS data image finalized\n"); 
      fflush(stdout);
   }

   time(&currtime);

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "time=%.1f",       (double)(currtime - start));
   sprintf(returnStruct->json, "{\"time\":%.1f}", (double)(currtime - start));

   returnStruct->time = (double)(currtime - start);

   return returnStruct;
}



/**************************************/
/*                                    */
/*  Open a FITS file and extract the  */
/*  pertinent header information.     */
/*                                    */
/**************************************/

int mShrinkCube_readFits(char *fluxfile)
{
   int      status;
   long     bitpix;
   char     ctype1[32], ctype2[32];
   double   crval1, crval2;
   double   crpix1, crpix2;
   double   cnpix1, cnpix2;
   double   xpixelsz, ypixelsz;
   double   ppo3, ppo6;
   double   cdelt1, cdelt2, cdelt3;
   double   crota2;
   double   cd11;
   double   cd12;
   double   cd21;
   double   cd22;
   double   pc11;
   double   pc12;
   double   pc21;
   double   pc22;
   double   epoch;
   double   equinox;
   long     blank;

   char     bunit[80];

   char    msg [1024];

   status = 0;

   haveCtype   = 1;
   haveCrval   = 1;
   haveCrpix   = 1;
   haveCnpix   = 1;
   havePixelsz = 1;
   havePP      = 1;
   haveCdelt   = 1;
   haveCdelt3  = 1;
   haveCrota2  = 1;
   haveCD11    = 1;
   haveCD12    = 1;
   haveCD21    = 1;
   haveCD22    = 1;
   havePC11    = 1;
   havePC12    = 1;
   havePC21    = 1;
   havePC22    = 1;
   haveEpoch   = 1;
   haveEquinox = 1;
   haveBunit   = 1;
   haveBlank   = 1;

   input.cdelt1  = 0;
   input.cdelt2  = 0;
   input.cdelt3  = 0;
   input.crota2  = 0;
   input.cd11    = 0;
   input.cd12    = 0;
   input.cd21    = 0;
   input.cd22    = 0;
   input.pc11    = 0;
   input.pc12    = 0;
   input.pc21    = 0;
   input.pc22    = 0;
   input.epoch   = 0;
   input.equinox = 0;
   input.blank   = 0;

   strcpy(input.bunit, "");

   if(fits_open_file(&input.fptr, fluxfile, READONLY, &status))
   {
      sprintf(msg, "Image file %s missing or invalid FITS", fluxfile);
      mShrinkCube_printError(msg);
      return 1;
   }

   if(hdu > 0)
   {
      if(fits_movabs_hdu(input.fptr, hdu+1, NULL, &status))
      {
         mShrinkCube_printFitsError(status);
         return 1;
      }
   }

   status = 0;
   if(fits_read_key(input.fptr, TLONG, "BITPIX", &bitpix, (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      return 1;
   }

   input.bitpix = bitpix;

   status = 0;
   if(fits_read_key(input.fptr, TLONG, "NAXIS", &(input.naxis), (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      return 1;
   }

   status = 0;
   if(fits_read_key(input.fptr, TLONG, "NAXIS1", &(input.naxes[0]), (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      return 1;
   }

   status = 0;
   if(fits_read_key(input.fptr, TLONG, "NAXIS2", &(input.naxes[1]), (char *)NULL, &status))
   {
      mShrinkCube_printFitsError(status);
      return 1;
   }

   status = 0;
   if(fits_read_key(input.fptr, TLONG, "NAXIS3", &(input.naxes[2]), (char *)NULL, &status))
      input.naxes[2] = 1;

   status = 0;
   if(fits_read_key(input.fptr, TLONG, "NAXIS4", &(input.naxes[3]), (char *)NULL, &status))
      input.naxes[3] = 1;

   status = 0;
   fits_read_key(input.fptr, TSTRING, "CTYPE1", ctype1, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCtype = 0;
   else strcpy(input.ctype1, ctype1);

   status = 0;
   fits_read_key(input.fptr, TSTRING, "CTYPE2", ctype2, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCtype = 0;
   else strcpy(input.ctype2, ctype2);

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CRVAL1", &crval1, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCrval = 0;
   else input.crval1 = crval1;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CRVAL2", &crval2, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCrval = 0;
   else input.crval2 = crval2;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CRPIX1", &crpix1, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCrpix = 0;
   else input.crpix1 = crpix1;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CRPIX2", &crpix2, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCrpix = 0;
   else input.crpix2 = crpix2;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CNPIX1", &cnpix1, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCnpix = 0;
   else input.cnpix1 = cnpix1;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CNPIX2", &cnpix2, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCnpix = 0;
   else input.cnpix2 = cnpix2;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "XPIXELSZ", &xpixelsz, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      havePixelsz = 0;
   else input.xpixelsz = xpixelsz;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "YPIXELSZ", &ypixelsz, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      havePixelsz = 0;
   else input.ypixelsz = ypixelsz;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "PPO3", &ppo3, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      havePP = 0;
   else input.ppo3 = ppo3;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "PPO6", &ppo6, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      havePP = 0;
   else input.ppo6 = ppo6;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CDELT1", &cdelt1, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCdelt = 0;
   else input.cdelt1 = cdelt1;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CDELT2", &cdelt2, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCdelt = 0;
   else input.cdelt2 = cdelt2;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CDELT3", &cdelt3, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCdelt3 = 0;
   else input.cdelt3 = cdelt3;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CD1_1", &cd11, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCD11 = 0;
   else input.cd11 = cd11;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CD1_2", &cd12, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCD12 = 0;
   else input.cd12 = cd12;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CD2_1", &cd21, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCD21 = 0;
   else input.cd21 = cd21;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CD2_2", &cd22, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCD22 = 0;
   else input.cd22 = cd22;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "PC1_1", &pc11, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      havePC11 = 0;
   else input.pc11 = pc11;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "PC1_2", &pc12, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      havePC12 = 0;
   else input.pc12 = pc12;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "PC2_1", &pc21, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      havePC21 = 0;
   else input.pc21 = pc21;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "PC2_2", &pc22, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      havePC22 = 0;
   else input.pc22 = pc22;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "CROTA2", &crota2, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveCrota2 = 0;
   else input.crota2 = crota2;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "EPOCH", &epoch, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveEpoch = 0;
   else input.epoch = epoch;

   status = 0;
   fits_read_key(input.fptr, TDOUBLE, "EQUINOX", &equinox, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveEquinox = 0;
   else input.equinox = equinox;

   status = 0;
   fits_read_key(input.fptr, TSTRING, "BUNIT", bunit, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveBunit = 0;
   else strcpy(input.bunit, bunit);

   status = 0;
   fits_read_key(input.fptr, TLONG, "BLANK", &blank, (char *)NULL, &status);
   if(status == KEY_NO_EXIST)
      haveBlank = 0;
   else input.blank = blank;

   return 0;
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mShrinkCube_printFitsError(int status)
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

void mShrinkCube_printError(char *msg)
{
   strcpy(montage_msgstr, msg);
}
