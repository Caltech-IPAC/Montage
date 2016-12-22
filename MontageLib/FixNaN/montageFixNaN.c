/* mFixNaN.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
2.2      John Good        08Sep15  fits_read_pix() incorrect null value
2.1      John Good        07Oct07  Bug fix: set fstatus value
2.0      John Good        11Aug05  Change the code to do a line at a time
                                   (to mimimize memory usage) and the allow 
                                   for no output image (just a count of pixel
                                   that would change) when the output file
                                   name is "-".
1.0      John Good        16Sep04  Baseline code

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

#include <mFixNaN.h>
#include <montage.h>

#define MAXSTR  256
#define MAXFILE 256

static struct
{
   fitsfile *fptr;
   long      naxes[2];
   double    crpix1, crpix2;
}
   input, output;

static time_t currtime, start;

static struct WorldCoor *wcs;

static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mFixNaN                                                              */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mFixNaN, converts NaNs found in the image to some       */
/*  other value (given by the user) or ranges of values to NaNs          */
/*                                                                       */
/*   char  *input_file     FITS file to fix                              */
/*   char  *output_file    Fixed FITS file                               */
/*                                                                       */
/*   int    boundaries     Some projections (e.g. Aitoff) have regions   */
/*                         of the image that are non-physical but which  */
/*                         may have non-blank values (e.g. 0 instead of  */
/*                         NaN).  If this flag is on, check for that and */
/*                         convert those pixels to proper NaNs.          */
/*                                                                       */
/*   int    haveVal        This flag indicates that NaNs should be       */
/*                         converted to a different value.               */
/*                                                                       */
/*   double NaNvalue       The value associated with the above flag.     */
/*                         This is for situations where the NaNs in the  */
/*                         image are incorrectly used.                   */
/*                                                                       */
/*   int    nMinMax        The next five arguments define a set of value */
/*                         ranges which will be converted to NaNs.  This */
/*                         one is the count of these ranges.             */
/*                                                                       */
/*   double *minblank      The "ranges" can either have min/max values   */
/*                         or can be just an upper (max) value (i.e.     */
/*                         with a min of -Infinity) or a lower (min)     */
/*                         value (i.e. with a max of +Infinity).  This   */
/*                         array of values are the minumums.             */
/*                                                                       */
/*   int    *ismin         This array is a set of booleans indicating    */
/*                         whether each min value above is part of a     */
/*                         min/max range (0) or a standalone lower limit */
/*                         (in which case the corresponding max is       */
/*                         ignored).                                     */
/*                                                                       */
/*   double *maxblank      Max values for min/max ranges or standalone   */
/*                         upper limits.                                 */
/*                                                                       */
/*   int    *ismax         Booleans indicating whether each max value is */
/*                         part of a min/max range (0) or a standalone   */
/*                         upper limit (1).                              */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mFixNaNReturn  *mFixNaN(char *input_file, char *output_file, int boundaries, int haveVal, double NaNvalue, 
                               int nMinMax, double *minblank, int *ismin, double *maxblank, int *ismax, int debug)
{
   int       i, j, k, countRange, countNaN, bcount, status;
   int       inRange, offscl;
   long      fpixel[4], nelements;

   double   *inbuffer;
   double   *outbuffer;
   double    xi, yj;
   double    xproj, yproj;
   double    lon, lat;

   char     *checkHdr;

   struct mFixNaNReturn *returnStruct;


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

   returnStruct = (struct mFixNaNReturn *)malloc(sizeof(struct mFixNaNReturn));

   bzero((void *)returnStruct, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /************************/
   /* Read the input image */
   /************************/

   time(&currtime);
   start = currtime;


   checkHdr = montage_checkHdr(input_file, 0, 0);

   if(checkHdr)
   {  
      strcpy(returnStruct->msg, checkHdr);
      return returnStruct;
   } 

   if(mFixNaN_readFits(input_file, boundaries) > 0)
   {  
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   if(debug >= 1)
   {
      printf("input.naxes[0]       =  %ld\n",   input.naxes[0]);
      printf("input.naxes[1]       =  %ld\n",   input.naxes[1]);
      printf("input.crpix1         =  %-g\n",   input.crpix1);
      printf("input.crpix2         =  %-g\n",   input.crpix2);

      fflush(stdout);
   }

   output.naxes[0] = input.naxes[0];
   output.naxes[1] = input.naxes[1];
   output.crpix1   = input.crpix1;
   output.crpix2   = input.crpix2;


   /********************************/
   /* Create the output FITS files */
   /********************************/

   remove(output_file);               

   status = 0;
   if(fits_create_file(&output.fptr, output_file, &status)) 
   {
      mFixNaN_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("\nFITS output file created (not yet populated)\n"); 
      fflush(stdout);
   }


   /********************************/
   /* Copy all the header keywords */
   /* from the input to the output */
   /********************************/

   status = 0;
   if(fits_copy_header(input.fptr, output.fptr, &status))
   {
      mFixNaN_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("Header keywords copied to FITS output file\n\n"); 
      fflush(stdout);
   }


   /***************************/
   /* Modify BITPIX to be -64 */
   /***************************/

   status = 0;
   if(fits_update_key_lng(output.fptr, "BITPIX", -64,
                          (char *)NULL, &status))
   {
      mFixNaN_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*****************************************************/ 
   /* Allocate memory for the input/output image pixels */ 
   /* (same size as the input image)                    */ 
   /*****************************************************/ 

   outbuffer = (double *)malloc(output.naxes[0] * sizeof(double));

   if(debug >= 1)
   {
      printf("%ld bytes allocated for row of output image pixels\n", 
         output.naxes[0] * sizeof(double));
      fflush(stdout);
   }

   inbuffer = (double *)malloc(input.naxes[0] * sizeof(double));

   if(debug >= 1)
   {
      printf("%ld bytes allocated for row of input image pixels\n", 
         input.naxes[0] * sizeof(double));
      fflush(stdout);
   }


   /*****************************/
   /* Loop over the input lines */
   /*****************************/

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   nelements = input.naxes[0];

   countRange = 0;
   countNaN   = 0;
   bcount     = 0;

   for (j=0; j<input.naxes[1]; ++j)
   {
      if(debug >= 2)
      {
         if(debug >= 3)
            printf("\n");

         printf("\rProcessing input row %5d [So far rangeCount=%d, nanCount=%d, boundaryCount=%d]",
            j, countRange, countNaN, bcount);

         if(debug >= 3)
            printf("\n");

         fflush(stdout);
      }

      for (i=0; i<output.naxes[0]; ++i)
         outbuffer[i] = 0.;


      /***********************************/
      /* Read a line from the input file */
      /***********************************/

      status = 0;
      if(fits_read_pix(input.fptr, TDOUBLE, fpixel, nelements, &nan,
                       inbuffer, NULL, &status))
      {
         mFixNaN_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
      

      /************************/
      /* For each input pixel */
      /************************/

      // If boundary checking is on (e.g. Aitoff pixels in the 
      // corners) or not-a-number and we have a replacement value

      for (i=0; i<input.naxes[0]; ++i)
      {
         // First convert the "boundary" values if desired

         if(boundaries)
         {
            xi = i;
            yj = j;

            pix2wcs(wcs, xi, yj, &lon, &lat);

            offscl = wcs->offscl;

            if(!offscl)
               wcs2pix(wcs, lon, lat, &xproj, &yproj, &offscl);

            if(offscl || fabs(xi-xproj) > 0.01 || fabs(yj-yproj) > 0.01)
            {
               inbuffer[i] = nan;
               ++bcount;
            }
         }


         // Now process the data

         if((haveVal == 1) && mNaN(inbuffer[i]))
         {
            ++countNaN;

            outbuffer[i] = NaNvalue;

            if(debug >= 3)
            {
               printf("pixel[%d][%d] converted to %-g\n", j, i, NaNvalue);
               fflush(stdout);
            }
         }


         // If in one of the "fix" ranges

         else if(nMinMax)
         {
            inRange = 0;

            for(k=0; k<nMinMax; ++k)
            {
               if(ismin[k] && inbuffer[i] <= maxblank[k])
               {
                  inRange = 1;
                  break;
               }
               else if(ismax[k] && inbuffer[i] >= minblank[k])
               {
                  inRange = 1;
                  break;
               }
               else if(inbuffer[i] >= minblank[k] 
                    && inbuffer[i] <= maxblank[k])
               {
                  inRange = 1;
                  break;
               }
            }

            if(inRange)
            {
               ++countRange;

               if(haveVal) 
               {
                  ++countNaN;

                  outbuffer[i] = NaNvalue;

                  if(debug >= 3)
                  {
                     printf("pixel[%d][%d] converted to NaN -> %-g\n", j, i, NaNvalue);
                     fflush(stdout);
                  }
               }
               else
               {
                  ++countNaN;

                  outbuffer[i] = nan;

                  if(debug >= 3)
                  {
                     printf("pixel[%d][%d] converted to NaN\n", j, i);
                     fflush(stdout);
                  }
               }
            }

            else
               outbuffer[i] = inbuffer[i];
         }


         // Otherwise, good data

         else
            outbuffer[i] = inbuffer[i];
      }


      /***************************/
      /* Write the output buffer */
      /***************************/

      status = 0;
      if (fits_write_pix(output.fptr, TDOUBLE, fpixel, nelements, 
                         outbuffer, &status))
      {
         mFixNaN_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      ++fpixel[1];
   }


   if(debug >= 1)
   {
      time(&currtime);
      printf("\nDone copying data (%d seconds)\n", 
         (int)(currtime - start));
      fflush(stdout);
   }


   /************************/
   /* Close the FITS files */
   /************************/

   status = 0;
   if(fits_close_file(input.fptr, &status))
   {
      mFixNaN_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_close_file(output.fptr, &status))
   {
      mFixNaN_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      time(&currtime);
      printf("Done (%d seconds total)\n", (int)(currtime - start));
      fflush(stdout);
   }

   free(inbuffer);
   free(outbuffer);

  
   sprintf(montage_msgstr, "rangeCount=%d, nanCount=%d, boundaryCount=%d", countRange, countNaN, bcount);
   sprintf(montage_json, "{\"rangeCount\":%d, \"nanCount\":%d, \"boundaryCount\":%d}", countRange, countNaN, bcount);
   
   returnStruct -> status = 0;
      
   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   returnStruct->rangeCount    = countRange;
   returnStruct->nanCount      = countNaN;
   returnStruct->boundaryCount = bcount;
   
   return returnStruct;
}


/*******************************************/
/*                                         */
/*  Open a FITS file pair and extract the  */
/*  pertinent header information.          */
/*                                         */
/*******************************************/

int mFixNaN_readFits(char *fluxfile, int boundaryFlag)
{
   int    status, nfound;
   long   naxes[2];
   double crpix[2];
   char   errstr[MAXSTR];
   char  *header;

   status = 0;
   if(fits_open_file(&input.fptr, fluxfile, READONLY, &status))
   {
      sprintf(errstr, "Image file %s missing or invalid FITS", fluxfile);
      mFixNaN_printError(errstr);
      return 1;
   }

   status = 0;
   if(fits_read_keys_lng(input.fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
   {
      mFixNaN_printFitsError(status);
      return 1;
   }
   
   input.naxes[0] = naxes[0];
   input.naxes[1] = naxes[1];

   status = 0;
   if(fits_read_keys_dbl(input.fptr, "CRPIX", 1, 2, crpix, &nfound, &status))
   {
      mFixNaN_printFitsError(status);
      return 1;
   }

   input.crpix1 = crpix[0];
   input.crpix2 = crpix[1];

   if(boundaryFlag)
   {
      if(fits_get_image_wcs_keys(input.fptr, &header, &status))
      {
         mFixNaN_printFitsError(status);
         return 1;
      }

      wcs = wcsinit(header);
   }

   return 0;
}



/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void mFixNaN_printError(char *msg)
{
   strcpy(montage_msgstr, msg);
   return;
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mFixNaN_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);

   return;
}

