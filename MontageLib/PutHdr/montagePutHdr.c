/* Module: mPutHdr.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
2.1      John Good        11Oct15  Add logic for 3rd and 4th dimension, if there
2.0      John Good        17Nov14  Cleanup to avoid compiler warnings, in proparation
                                   for new development cycle.
1.1      John Good        15Sep06  Cleaned up usage text
1.0      John Good        30May05  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <wcs.h>
#include <sys/types.h>

#include <fitsio.h>

#include <mPutHdr.h>
#include <montage.h>

#define MAXSTR  256


static int hdu;


/* Structure used to store relevant */
/* information about a FITS file    */

static struct
{
   fitsfile         *fptr;
   long              naxes[4];
   struct WorldCoor *wcs;
}
input, output;

static long  bitpix,  naxis,  naxis1,  naxis2,  naxis3,  naxis4;
static int  tbitpix, tnaxis, tnaxis1, tnaxis2, tnaxis3, tnaxis4;



static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mPutHdr                                                              */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mPutHdr, replaces the header of the input file with     */
/*  one supplied by the user (presumably a "corrected" version of the    */
/*  input).  Nothing else is changed.  The new header is in the form     */
/*  of a standard Montage template:  an ASCII file with the same content */
/*  as a FITS header but one card per line and no need to make the lines */
/*  80 characters long (i.e. an easily editable file).                   */
/*                                                                       */
/*   char  *input_file     Input FITS file                               */
/*                                                                       */
/*   char  *output_file    Output FITS file                              */
/*                                                                       */
/*   char  *template_file  New header to replace that from the input     */
/*                         in the output                                 */
/*                                                                       */
/*   int    hdu            Optional HDU offset for input file            */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mPutHdrReturn *mPutHdr(char *input_file, char *output_file, char *template_file, int hduin, int debug)
{
   int       i, j, j2, j3, nullcnt;
   long      fpixel[4], nelements;
   double    *buffer;

   int       status = 0;

   char      line[MAXSTR];

   FILE     *ftemp;

   struct mPutHdrReturn *returnStruct;


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

   returnStruct = (struct mPutHdrReturn *)malloc(sizeof(struct mPutHdrReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   hdu = hduin;

   if(debug >= 1)
   {
      printf("\ninput_file    = [%s]\n", input_file);
      printf("output_file   = [%s]\n",   output_file);
      printf("template_file = [%s]\n\n", template_file);
      fflush(stdout);
   }


   /************************************************/
   /* Get the template file BITPIX, NAXIS1, NAXIS2 */
   /************************************************/

   ftemp = fopen(template_file, "r");

   if(ftemp == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Cannot open template file %s.", template_file);
      return returnStruct;
   }

   tbitpix = -64;

   tnaxis3 = 1;
   tnaxis4 = 1;

   while(1)
   {
      if(fgets(line, MAXSTR, ftemp) == (char *)NULL)
         break;

      if(strncmp(line, "NAXIS   =", 9) == 0)
         tnaxis  = atoi(line+10);

      else if(strncmp(line, "NAXIS1  =", 9) == 0)
         tnaxis1 = atoi(line+10);

      else if(strncmp(line, "NAXIS2  =", 9) == 0)
         tnaxis2 = atoi(line+10);

      else if(strncmp(line, "NAXIS3  =", 9) == 0)
         tnaxis3 = atoi(line+10);

      else if(strncmp(line, "NAXIS4  =", 9) == 0)
         tnaxis4 = atoi(line+10);
   }

   fclose(ftemp);


   /*************************/
   /* Check the input image */
   /*************************/

   if(mPutHdr_readFits(input_file) > 0)
   {
      fclose(ftemp);
      strcpy(returnStruct->msg, montage_msgstr);
      return(returnStruct);
   }

   if(debug >= 1)
   {
      printf("input.naxes[0]   =  %ld\n",  input.naxes[0]);
      printf("input.naxes[1]   =  %ld\n",  input.naxes[1]);
      printf("input.naxes[2]   =  %ld\n",  input.naxes[2]);
      printf("input.naxes[3]   =  %ld\n",  input.naxes[3]);

      fflush(stdout);
   }

   if(debug >= 1)
   {
      printf("\nbitpix: %ld -> %d\n",   bitpix, tbitpix);
      printf(  "naxis:  %ld -> %d\n",   naxis, tnaxis);
      printf(  "naxis1: %ld -> %d\n",   input.naxes[0], tnaxis1);
      printf(  "naxis2: %ld -> %d\n",   input.naxes[1], tnaxis2);
      printf(  "naxis3: %ld -> %d\n",   input.naxes[2], tnaxis3);
      printf(  "naxis4: %ld -> %d\n\n", input.naxes[3], tnaxis4);
      fflush(stdout);
   }

   if(tnaxis  != naxis 
   || tnaxis1 != input.naxes[0]
   || tnaxis2 != input.naxes[1]
   || tnaxis3 != input.naxes[2]
   || tnaxis4 != input.naxes[3])
   {
      fclose(ftemp);
      sprintf(returnStruct->msg, "NAXIS/NAXES values cannot be modified using this program.\"]\n");
      return returnStruct;
   }

   output.naxes[0] = input.naxes[0];
   output.naxes[1] = input.naxes[1];
   output.naxes[2] = input.naxes[2];
   output.naxes[3] = input.naxes[3];

   if(debug >= 1)
   {
      printf("naxis            =  %ld\n", naxis);
      printf("bitpix           =  %d\n",  tbitpix);
      printf("output.naxes[0]  =  %ld\n", output.naxes[0]);
      printf("output.naxes[1]  =  %ld\n", output.naxes[1]);
      printf("output.naxes[2]  =  %ld\n", output.naxes[2]);
      printf("output.naxes[3]  =  %ld\n", output.naxes[3]);

      fflush(stdout);
   }


   /********************************/
   /* Create the output FITS files */
   /********************************/

   remove(output_file);               

   if(fits_create_file(&output.fptr, output_file, &status)) 
   {
      mPutHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*********************************************************/
   /* Create the FITS image.  All the required keywords are */
   /* handled automatically.                                */
   /*********************************************************/

   if (fits_create_img(output.fptr, (int)tbitpix, naxis,
      output.naxes, &status))
   {
      mPutHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("\nFITS data image created (not yet populated)\n"); 
      fflush(stdout);
   } 


   /****************************************/
   /* Set FITS header from a template file */
   /****************************************/

   if(fits_write_key_template(output.fptr, template_file, &status))
   {
      mPutHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("Template keywords written to FITS data image\n"); 
      fflush(stdout);
   }



   /*********************************************/
   /* Reset the basic parameters, in case the   */
   /* header file screwed them up.              */
   /*********************************************/

   if(fits_update_key_lng(output.fptr, "BITPIX", tbitpix,
                                  (char *)NULL, &status))
   {
      mPutHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS", naxis,
                                  (char *)NULL, &status))
   {
      mPutHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS1", output.naxes[0],
                                  (char *)NULL, &status))
   {
      mPutHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS2", output.naxes[1],
                                  (char *)NULL, &status))

   if(naxis > 2)
      if(fits_update_key_lng(output.fptr, "NAXIS3", output.naxes[2],
                                     (char *)NULL, &status))

   if(naxis > 3)
      if(fits_update_key_lng(output.fptr, "NAXIS4", output.naxes[3],
                                     (char *)NULL, &status))
   {
      mPutHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }



   /********************************************/ 
   /* Allocate memory for the data copy buffer */ 
   /********************************************/ 

   buffer = (double *)malloc(output.naxes[0] * sizeof(double));

   if(buffer == (void *)NULL)
   {
      sprintf(returnStruct->msg, "Not enough memory for output data image array\"]\n");
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("\n%ld bytes allocated for image pixels\n", 
          output.naxes[0] * sizeof(double));
      fflush(stdout);
   }



   /***************************/
   /* Copy the data unchanged */
   /***************************/

   fpixel[0] = 1;

   nelements = input.naxes[0];

   fpixel[3] = 1;
   
   for (j3=0; j3<input.naxes[3]; ++j3)
   {
      fpixel[2] = 1;

      for (j2=0; j2<input.naxes[2]; ++j2)
      {
         fpixel[1] = 1;

         for (j=0; j<input.naxes[1]; ++j)
         {
            if(debug >= 2)
            {
               printf("DEBUG> Reading/writing %ld pixels at %ld %ld %ld\n", nelements, fpixel[1], fpixel[2], fpixel[3]);
               fflush(stdout);
            }

            if(fits_read_pix(input.fptr, TDOUBLE, fpixel, nelements, &nan,
                             buffer, &nullcnt, &status))
            {
               free(buffer);
               mPutHdr_printFitsError(status);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            /*** Example data correction add-on *****

            for(i=0; i<nelements; ++i)
               if(buffer[i] < -10000) buffer[i] = nan;

            ****************************************/

            if (fits_write_pix(output.fptr, TDOUBLE, fpixel, nelements, 
                               buffer, &status))
            {
               free(buffer);
               mPutHdr_printFitsError(status);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            ++fpixel[1];
         }

         ++fpixel[2];
      }

      ++fpixel[3];
   }

   if(debug >= 1)
   {
      printf("Data copied from input FITS file to output FITS file\n"); 
      fflush(stdout);
   }

   free(buffer);



   /***********************/
   /* Close the FITS file */
   /***********************/

   if(fits_close_file(input.fptr, &status))
   {
      mPutHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_close_file(output.fptr, &status))
   {
      mPutHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("FITS data image finalized\n"); 
      fflush(stdout);
   }

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  "");
   strcpy(returnStruct->json, "{}");

   return returnStruct;
}



/**************************************************/
/*                                                */
/*  Read a FITS file and extract some of the      */
/*  header information.                           */
/*                                                */
/**************************************************/

int mPutHdr_readFits(char *filename)
{
   int       status;

   char     *header;

   char      errstr[MAXSTR];

   status = 0;


   /*****************************************/
   /* Open the FITS file and get the header */
   /* for WCS setup                         */
   /*****************************************/

   if(fits_open_file(&input.fptr, filename, READONLY, &status))
   {
      sprintf(errstr, "Image file %s missing or invalid FITS", filename);
      mPutHdr_printError(errstr);
      return 1;
   }

   if(hdu > 0)
   {
      if(fits_movabs_hdu(input.fptr, hdu+1, NULL, &status))
      {
         mPutHdr_printFitsError(status);
         return 1;
      }
   }

   if(fits_get_image_wcs_keys(input.fptr, &header, &status))
   {
      mPutHdr_printFitsError(status);
      return 1;
   }


   /*************************/
   /* Find the input BITPIX */
   /*************************/

   if(fits_read_key_lng(input.fptr, "BITPIX", &bitpix, (char *)NULL, &status))
   {
      mPutHdr_printFitsError(status);
      return 1;
   }

   if(fits_read_key_lng(input.fptr, "NAXIS", &naxis, (char *)NULL, &status))
   {
      mPutHdr_printFitsError(status);
      return 1;
   }

   if(fits_read_key(input.fptr, TLONG, "NAXIS1", &naxis1, (char *)NULL, &status))
   {
      mPutHdr_printFitsError(status);
      return 1;
   }

   if(fits_read_key(input.fptr, TLONG, "NAXIS2", &naxis2, (char *)NULL, &status))
   {
      mPutHdr_printFitsError(status);
      return 1;
   }

   naxis3 = 1;
   if(naxis > 2)
      if(fits_read_key(input.fptr, TLONG, "NAXIS3", &naxis3, (char *)NULL, &status))
      {
         mPutHdr_printFitsError(status);
         return 1;
      }

   naxis4 = 1;
   if(naxis > 3)
      if(fits_read_key(input.fptr, TLONG, "NAXIS4", &naxis4, (char *)NULL, &status))
      {
         mPutHdr_printFitsError(status);
         return 1;
      }


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   input.naxes[0] = naxis1;
   input.naxes[1] = naxis2;
   input.naxes[2] = naxis3;
   input.naxes[3] = naxis4;

   free(header);

   return 0;
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mPutHdr_printFitsError(int status)
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

void mPutHdr_printError(char *msg)
{
   strcpy(montage_msgstr, msg);
}
