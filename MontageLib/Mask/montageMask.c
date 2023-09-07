/* Module: mMask.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        23Sep22  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <malloc.h>
#include <coord.h>

#include <mMask.h>
#include <montage.h>

#define MAXBOX 256
#define STRLEN 1024

static int mMask_debug;

struct Boxes
{
   int xmin;
   int xmax;
   int ymin; 
   int ymax;
};

struct Boxes *boxes;

FILE *fbox;

int nboxes, maxboxes;

static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mMask                                                                */
/*                                                                       */
/*  This program masks off regions (converts the values to NaN) based    */
/*  on an input ASCII file defining boxes in pixel space.                */
/*                                                                       */
/*   char  *infile         Input FITS file                               */
/*   char  *outfile        Output FITS file                              */
/*   char  *boxfile        ASCII file with set of boxe to mask off       */
/*                                                                       */
/*   int    hdu            Optional HDU offset for input file            */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mMaskReturn *mMask(char *infile, char *outfile, char *boxfile, int hdu, int debugin)
{
   fitsfile *infptr, *outfptr;

   char line[STRLEN];

   int status = 0;

   struct mMaskParams params;

   struct mMaskReturn *returnStruct;


   mMask_debug = debugin;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mMaskReturn *)malloc(sizeof(struct mMaskReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   // Open the original FITS file 

   if(fits_open_file(&infptr, infile, READONLY, &status))
   {
      sprintf(returnStruct->msg, "Image file %s missing or invalid FITS", infile);
      return returnStruct;
   }


   // Move to the right HDU
   
   if(hdu > 0)
   {
      if(fits_movabs_hdu(infptr, hdu+1, NULL, &status))
      {
         sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
         return returnStruct;
      }
   }


   // Get the image size before anything else, just in case
   
   if(fits_read_key_lng(infptr, "NAXIS", &params.naxis, (char *)NULL, &status))
   {
      sprintf(returnStruct->msg, "Header doesn't have NAXIS keyword.");
      return returnStruct;
   }
   
   if(fits_read_keys_lng(infptr, "NAXIS", 1, params.naxis, params.naxes, &params.nfound, &status))
   {
      sprintf(returnStruct->msg, "Header doesn't have NAXIS1,2 keywords.");
      return returnStruct;
   }

   if(mMask_debug)
   {
      printf("DEBUG> NAXIS  = %ld\n", params.naxis);
      printf("DEBUG> NAXIS1 = %ld\n", params.naxes[0]);
      printf("DEBUG> NAXIS2 = %ld\n", params.naxes[1]);
      fflush(stdout);
   }

   params.ibegin = 1;
   params.iend   = params.naxes[0];

   params.jbegin = 1;
   params.jend   = params.naxes[1];

   params.nelements = params.iend - params.ibegin + 1;

   if(mMask_debug)
   {
      printf("DEBUG> ibegin    = %d\n",  params.ibegin);
      printf("DEBUG> iend      = %d\n",  params.iend);
      printf("DEBUG> nelements = %ld\n", params.nelements);
      printf("DEBUG> jbegin    = %d\n",  params.jbegin);
      printf("DEBUG> jend      = %d\n",  params.jend);
      fflush(stdout);
   }


   /**********************/
   /* Read in the boxese */
   /**********************/

   fbox = fopen(boxfile, "r");

   nboxes = 0;

   maxboxes = MAXBOX;

   boxes = (struct Boxes *)malloc(maxboxes * sizeof(struct Boxes));

   while(1)
   {
      if(fgets(line, STRLEN, fbox) == (char *)NULL)
         break;

      sscanf(line, "%d %d %d %d",
            &boxes[nboxes].xmin, &boxes[nboxes].xmax, 
            &boxes[nboxes].ymin, &boxes[nboxes].ymax);

      ++nboxes;

      if(nboxes >= maxboxes)
      {
         maxboxes += MAXBOX;

         boxes = (struct Boxes *)realloc(boxes, maxboxes * sizeof(struct Boxes));
      }
   }

   fclose(fbox);

   if(mMask_debug)
   {
      printf("DEBUG> nboxes    = %d\n",  nboxes);
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

   if(mMask_debug)
   {
      printf("Calling copyHeaderInfo()\n");
      fflush(stdout);
   }

   mMask_copyHeaderInfo(infptr, outfptr, &params);


   /******************************************************************************/
   /* Copy the data, converting to TDOUBLE and masking off the regions specified */
   /******************************************************************************/

   if(mMask_debug)
   {
      printf("Calling copyData()\n");
      fflush(stdout);
   }

   if(mMask_copyData(infptr, outfptr, &params) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*******************/
   /* Close the files */
   /*******************/

   if(mMask_debug)
   {
      printf("Calling fits_close_file()\n");
      fflush(stdout);
   }

   if(fits_close_file(outfptr, &status))
   {
      mMask_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_close_file(infptr, &status))
   {
      mMask_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   returnStruct->status = 0;

   strcpy (returnStruct->msg,  "");
   sprintf(returnStruct->json, "{}");

   return returnStruct;
}


int mMask_getFileInfo(fitsfile *infptr, char *header[], struct mMaskParams *params)
{
   int i;
   int status = 0;

   if(fits_get_image_wcs_keys(infptr, header, &status))
   {
      mMask_printFitsError(status);
      return 1;
   }

   if(fits_read_key_lng(infptr, "NAXIS", &params->naxis, (char *)NULL, &status))
   {
      mMask_printFitsError(status);
      return 1;
   }
   
   if(fits_read_keys_lng(infptr, "NAXIS", 1, params->naxis, params->naxes, &params->nfound, &status))
   {
      mMask_printFitsError(status);
      return 1;
   }
   
   if(mMask_debug)
   {
      for(i=0; i<params->naxis; ++i)
         printf("naxis%d = %ld\n",  i+1, params->naxes[i]);

      fflush(stdout);
   }

   return 0;
}


int mMask_copyHeaderInfo(fitsfile *infptr, fitsfile *outfptr, struct mMaskParams *params)
{
   int status = 0;
   
   if(fits_copy_header(infptr, outfptr, &status))
   {
      mMask_printFitsError(status);
      return 1;
   }


   if(fits_update_key_lng(outfptr, "BITPIX", -64, (char *)NULL, &status))
   {
      mMask_printFitsError(status);
      return 1;
   }

   return 0;
}


int mMask_copyData(fitsfile *infptr, fitsfile *outfptr, struct mMaskParams *params)
{
   long       fpixel[4], fpixelo[4];
   int        i, j, k, nullcnt;
   int        status = 0;

   double    *buffer_double;


   /*************************************************/
   /* Make a NaN value to use checking blank pixels */
   /*************************************************/

   union
   {
      double d8;
      char   c[8];
   }
   value;

   double dnan;

   for(i=0; i<8; ++i)
      value.c[i] = (char)255;

   dnan = value.d8;

   buffer_double = (double *) NULL;

   fpixel[0] = params->ibegin;
   fpixel[1] = params->jbegin;
   fpixel[2] = 1;
   fpixel[3] = 1;

   if(mMask_debug)
   {
      printf("fpixel[0] = %ld\n", fpixel[0]);
      printf("fpixel[1] = %ld\n", fpixel[1]);
      fflush(stdout);
   }

   buffer_double = (double *)malloc(params->nelements * sizeof(double));

   fpixelo[0] = 1;
   fpixelo[1] = 1;
   fpixelo[2] = 1;
   fpixelo[3] = 1;

   for (j=params->jbegin; j<=params->jend; ++j)
   {
      if(mMask_debug)
      {
         printf("Reading  input image row %5d\n", j);
         fflush(stdout);
      }

      fits_read_pix(infptr, TDOUBLE, fpixel, params->nelements, &dnan, buffer_double, &nullcnt, &status);

      if(status)
      {
         mMask_printFitsError(status);
         return 1;
      }

      for(i=0; i<params->nelements; ++i) 
      {    
         if(!mNaN(buffer_double[i]))
         {    
            for(k=0; k<nboxes; ++k)
            {
               if(i>=boxes[k].xmin && i<=boxes[k].xmax 
               && j>=boxes[k].ymin && j<=boxes[k].ymax)
                  buffer_double[i] = dnan;
            }    
         }    
      }    

      if(mMask_debug)
      {
         printf("Writing output image row %5d\n", j);
         fflush(stdout);
      }

      fits_write_pix(outfptr, TDOUBLE, fpixelo, params->nelements, (void *)buffer_double, &status);

      if(status)
      {
         mMask_printFitsError(status);
         return 1;
      }

      ++fpixelo[1];
      ++fpixel [1];
   }


   free(buffer_double);

   return 0;
}

/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mMask_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}
