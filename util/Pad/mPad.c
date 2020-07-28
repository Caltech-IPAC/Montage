/* Module: mPad.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.1      John Good        08Sep15  fits_read_pix() incorrect null value
1.0      John Good        04Apr11  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>

#include "fitsio.h"
#include "wcs.h"
#include "mNaN.h"

#define MAXSTR  256
#define MAXFILE 256

FILE *fstatus;

char input_file  [MAXSTR];
char output_file [MAXSTR];

void printFitsError(int err);
void printError    (char *msg);
int  readFits      (char *fluxfile);
int  checkHdr      (char *infile, int hdrflag, int hdu);

int  debug;

struct
{
   fitsfile *fptr;
   long      naxes[2];
   double    crpix1, crpix2;
   int       flip;
}
   input, output;

static time_t currtime, start;


/*************************************************************************/
/*                                                                       */
/*  mPad                                                                 */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mPad, can be used to pad around the image with NULLs.   */
/*  This is usually just for visual effect or to allow space for         */
/*  annotation, etc.                                                     */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int     i, j, jnorm, joffset, status, padline;
   int     nowcs, haveBar, index, offset;
   double  imin, imax, jmin, jmax;
   int     left, right, top, bottom;
   long    fpixelin[4], fpixelout[4], nelementsin, nelementsout;
   double *inbuffer;
   double  val, NaNvalue;
   double  dataval[256];

   double *outbuffer;

   char   *end;
   char    histfile  [1024];
   char    line      [1024];
   char    label     [1024];

   char    datavalStr[256][1024];

   FILE   *fhist;



   /**********************************************/
   /* Make a NaN value to use setting pad pixels */
   /**********************************************/

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


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   fstatus = stdout;

   debug    = 0;
   NaNvalue = nan;
   haveBar  = 0;
   nowcs    = 0;

   for(i=1; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
      {
         if(i+1 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"No debug level given\"]\n");
            exit(1);
         }

         debug = strtol(argv[i+1], &end, 0);

         if(end - argv[i+1] < strlen(argv[i+1]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Debug level string is invalid: '%s'\"]\n", argv[i+1]);
            exit(1);
         }

         if(debug < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"Debug level value cannot be negative\"]\n");
            exit(1);
         }

         ++i;
      }

      else if(strcmp(argv[i], "-bar") == 0)
      {
         haveBar = 1;

         if(i+6 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"Not enough information given to draw color bar\"]\n");
            exit(1);
         }

         imin = strtol(argv[i+1], &end, 0);

         if(end - argv[i+1] < strlen(argv[i+1]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Bar X min string is invalid: '%s'\"]\n", argv[i+1]);
            exit(1);
         }

         imax = strtol(argv[i+2], &end, 0);

         if(end - argv[i+2] < strlen(argv[i+2]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Bar X max string is invalid: '%s'\"]\n", argv[i+2]);
            exit(1);
         }

         jmin = strtol(argv[i+3], &end, 0);

         if(end - argv[i+3] < strlen(argv[i+3]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Bar Y min string is invalid: '%s'\"]\n", argv[i+3]);
            exit(1);
         }

         jmax = strtol(argv[i+4], &end, 0);

         if(end - argv[i+4] < strlen(argv[i+4]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Bar Y max string is invalid: '%s'\"]\n", argv[i+4]);
            exit(1);
         }

         strcpy(histfile, argv[i+5]);

         i += 5;
      }

      else if(strcmp(argv[i], "-val") == 0)
      {
         if(i+1 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"No value given for NaN conversion\"]\n");
            exit(1);
         }

         NaNvalue = strtod(argv[i+1], &end);

         if(end - argv[i+1] < strlen(argv[i+1]))
         {
            printf("[struct stat=\"ERROR\", msg=\"NaN conversion value string is invalid: '%s'\"]\n", argv[i+1]);
            exit(1);
         }

         ++i;
      }

      else if(strcmp(argv[i], "-nowcs") == 0)
         nowcs = 1;

      else
         break;
   }

   argc -= i;
   argv += i;

   if (argc < 6) 
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: mPad [-nowcs][-d level][-val NaN-value][-bar xmin xmax ymin ymax histfile] in.fits out.fits left right top bottom\"]\n");
      exit(1);
   }

   strcpy(input_file,  argv[0]);
   strcpy(output_file, argv[1]);

   left     = atoi(argv[2]);
   right    = atoi(argv[3]);
   top      = atoi(argv[4]);
   bottom   = atoi(argv[5]);

   if(debug >= 1)
   {
      printf("input_file       = [%s]\n", input_file);
      printf("output_file      = [%s]\n", output_file);
      printf("left             = %d\n",   left);
      printf("right            = %d\n",   right);
      printf("top              = %d\n",   top);
      printf("bottom           = %d\n",   bottom);
      printf("NaNvalue         = %-g\n",  NaNvalue);
      fflush(stdout);
   }


   /***************************/
   /* Read the histogram file */
   /***************************/

   if(haveBar)
   {
      fhist = fopen(histfile, "r");

      if(fhist == (FILE *)NULL)
      {
         printf ("[struct stat=\"ERROR\", msg=\"Cannot open histogram file %s\"]\n", histfile);
         exit(1);
      }

      for(i=0; i<15; ++i)
      fgets(line, 1024, fhist);

      for(i=0; i<256; ++i)
      {
         fgets(line, 1024, fhist);
         sscanf(line, "%s %s", label, datavalStr[i]);

         dataval[i] = atof(datavalStr[i]);
      }
   }


   /************************/
   /* Read the input image */
   /************************/

   time(&currtime);
   start = currtime;

   readFits(input_file);

   if(debug >= 1)
   {
      printf("input.naxes[0]       =  %ld\n",   input.naxes[0]);
      printf("input.naxes[1]       =  %ld\n",   input.naxes[1]);

      if(!nowcs)
      {
         printf("input.crpix1         =  %-g\n",   input.crpix1);
         printf("input.crpix2         =  %-g\n",   input.crpix2);
         printf("input.flip           =  %d\n",    input.flip);
      }

      fflush(stdout);
   }

   output.naxes[0] = input.naxes[0] + left + right;
   output.naxes[1] = input.naxes[1] + top  + bottom;

   if(imin < 0) imin = output.naxes[0] + 1 + imin;
   if(imax < 0) imax = output.naxes[0] + 1 + imax;
   if(jmin < 0) jmin = output.naxes[1] + 1 + jmin;
   if(jmax < 0) jmax = output.naxes[1] + 1 + jmax;

   if(debug >= 1)
   {
      printf("imin            -> %-g\n",   imin);
      printf("imax            -> %-g\n",   imax);
      printf("jmin            -> %-g\n",   jmin);
      printf("jmax            -> %-g\n",   jmax);
      fflush(stdout);
   }

   if(!nowcs)
   {
      output.crpix1   = input.crpix1 + left;

      if(input.flip)
         output.crpix2   = input.crpix2 + top;
      else
         output.crpix2   = input.crpix2 + bottom;
   }

   if(debug >= 1)
   {
      printf("output.naxes[0]       =  %ld\n",   output.naxes[0]);
      printf("output.naxes[1]       =  %ld\n",   output.naxes[1]);

      if(!nowcs)
      {
         printf("output.crpix1         =  %-g\n",   output.crpix1);
         printf("output.crpix2         =  %-g\n",   output.crpix2);
      }

      fflush(stdout);
   }

   /********************************/
   /* Create the output FITS files */
   /********************************/

   remove(output_file);               

   status = 0;
   if(fits_create_file(&output.fptr, output_file, &status)) 
      printFitsError(status);           

   if(debug >= 1)
   {
      printf("\nFITS output file created (not yet populated)\n"); 
      fflush(stdout);
   }


   /********************************/
   /* Copy all the header keywords */
   /* from the input to the output */
   /********************************/

   if(fits_copy_header(input.fptr, output.fptr, &status))
      printFitsError(status);           

   if(debug >= 1)
   {
      printf("Header keywords copied to FITS output file\n\n"); 
      fflush(stdout);
   }


   /***************************/
   /* Modify BITPIX to be -64 */
   /***************************/

   if(fits_update_key_lng(output.fptr, "BITPIX", -64,
                          (char *)NULL, &status))
      printFitsError(status);

   if(fits_update_key_lng(output.fptr, "NAXIS1", output.naxes[0],
                                  (char *)NULL, &status))
       printFitsError(status);

   if(fits_update_key_lng(output.fptr, "NAXIS2", output.naxes[1],
                                  (char *)NULL, &status))
       printFitsError(status);

   if(!nowcs)
   {
      if(fits_update_key_dbl(output.fptr, "CRPIX1", output.crpix1, -14,
                                     (char *)NULL, &status))
          printFitsError(status);

      if(fits_update_key_dbl(output.fptr, "CRPIX2", output.crpix2, -14,
                                     (char *)NULL, &status))
          printFitsError(status);
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

   fpixelin[0] = 1;
   fpixelin[1] = 1;
   fpixelin[2] = 1;
   fpixelin[3] = 1;

   fpixelout[0] = 1;
   fpixelout[1] = 1;
   fpixelout[2] = 1;
   fpixelout[3] = 1;

   nelementsin  =  input.naxes[0];
   nelementsout = output.naxes[0];

   status = 0;


   /**********************/
   /* Pad the top/bottom */
   /**********************/

   padline = bottom;

   if(input.flip)
      padline = top;

   for(j=0; j<padline; ++j)
   {
      if(debug >= 2)
      {
         if(debug >= 3)
            printf("\n");

         printf("\rPad initial row %d", j);

         if(debug >= 3)
            printf("\n");

         fflush(stdout);
      }

      jnorm = j;

      if(input.flip)
         jnorm = bottom + input.naxes[1] + (padline - j);

      for (i=0; i<output.naxes[0]; ++i)
         outbuffer[i] = NaNvalue;

      if(haveBar && jnorm >= jmin && jnorm <= jmax)
      {
         index = (jnorm - jmin) * 255 / (jmax - jmin);

         offset = (jnorm - jmin)/50;

         val = dataval[index];

         if(debug >= 1 && offset * 50 == (jnorm - jmin))
            printf("BAR LABEL> %d %s\n", jnorm, datavalStr[index]);

         for(i=imin; i<=imax; ++i)
            outbuffer[i] = val;
      }


      if (fits_write_pix(output.fptr, TDOUBLE, fpixelout, nelementsout, 
                         outbuffer, &status))
         printFitsError(status);

      ++fpixelout[1];
   }


   /*****************************/
   /* Loop over the input lines */
   /*****************************/

   status = 0;

   for (j=0; j<input.naxes[1]; ++j)
   {
      if(debug >= 2)
      {
         if(debug >= 3)
            printf("\n");

         printf("\rProcessing input row %5d", j);

         if(debug >= 3)
            printf("\n");

         fflush(stdout);
      }

      for (i=0; i<output.naxes[0]; ++i)
         outbuffer[i] = NaNvalue;


      /* Read a line from the input file */

      if(fits_read_pix(input.fptr, TDOUBLE, fpixelin, nelementsin, &nan,
                       inbuffer, NULL, &status))
         printFitsError(status);
      

      /* For each input pixel */

      jnorm = j + bottom;

      if(input.flip)
         jnorm = bottom + (input.naxes[1] - j);

      for (i=0; i<input.naxes[0]; ++i)
         outbuffer[i+left] = inbuffer[i];

      if(haveBar && jnorm >= jmin && jnorm <= jmax)
      {
         index = (jnorm - jmin) * 255 / (jmax - jmin);

         offset = (jnorm - jmin)/50;

         val = dataval[index];

         if(debug >= 1 && offset * 50 == (jnorm - jmin))
            printf("BAR LABEL> %d %s\n", jnorm, datavalStr[index]);


         for(i=imin; i<=imax; ++i)
            outbuffer[i] = val;
      }



      /* Write the output buffer */

      if (fits_write_pix(output.fptr, TDOUBLE, fpixelout, nelementsout, 
                         outbuffer, &status))
         printFitsError(status);

      ++fpixelin [1];
      ++fpixelout[1];
   }


   /**********************/
   /* Pad the bottom/top */
   /**********************/

   padline = top;
   if(input.flip)
      padline = bottom;

   for(j=0; j<padline; ++j)
   {
      if(debug >= 2)
      {
         if(debug >= 3)
            printf("\n");

         printf("\rPad final row %d", j);

         if(debug >= 3)
            printf("\n");

         fflush(stdout);
      }

      jnorm = j + bottom + input.naxes[1];

      if(input.flip)
         jnorm = padline - j;

      for (i=0; i<output.naxes[0]; ++i)
         outbuffer[i] = NaNvalue;

      if(haveBar && jnorm >= jmin && jnorm <= jmax)
      {
         index = (jnorm - jmin) * 255 / (jmax - jmin);

         offset = (jnorm - jmin)/50;

         val = dataval[index];

         if(debug >= 1 && offset * 50 == (jnorm - jmin))
            printf("BAR LABEL> %d %s\n", jnorm, datavalStr[index]);

         for(i=imin; i<=imax; ++i)
            outbuffer[i] = val;
      }

      if (fits_write_pix(output.fptr, TDOUBLE, fpixelout, nelementsout, 
                         outbuffer, &status))
         printFitsError(status);

      ++fpixelout[1];
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

   if(fits_close_file(input.fptr, &status))
      printFitsError(status);

   if(fits_close_file(output.fptr, &status))
      printFitsError(status);           

   if(debug >= 1)
   {
      time(&currtime);
      printf("Done (%d seconds total)\n", (int)(currtime - start));
      fflush(stdout);
   }

   free(inbuffer);
   free(outbuffer);

   printf("[struct stat=\"OK\"]\n");
   fflush(stdout);

   exit(0);
}


/*******************************************/
/*                                         */
/*  Open a FITS file pair and extract the  */
/*  pertinent header information.          */
/*                                         */
/*******************************************/

int readFits(char *fluxfile)
{
   int    status, nfound, keysexist, keynum;
   long   naxes[2];
   double crpix[2];
   char   errstr[MAXSTR];

   char  *header  = (char *)NULL;

   struct WorldCoor *wcs;

   input.flip = 0;

   status = 0;

   if(!nowcs)
      checkHdr(fluxfile, 0, 0);

   if(fits_open_file(&input.fptr, fluxfile, READONLY, &status))
   {
      sprintf(errstr, "Image file %s missing or invalid FITS", fluxfile);
      printError(errstr);
   }

   if(fits_read_keys_lng(input.fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
      printFitsError(status);
   
   input.naxes[0] = naxes[0];
   input.naxes[1] = naxes[1];

   if(!nowcs)
   {
      if(fits_read_keys_dbl(input.fptr, "CRPIX", 1, 2, crpix, &nfound, &status))
         printFitsError(status);

      input.crpix1 = crpix[0];
      input.crpix2 = crpix[1];

      if(fits_get_hdrpos(input.fptr, &keysexist, &keynum, &status))
         printFitsError(status);

      header  = malloc(keysexist * 80 + 1024);

      if(fits_get_image_wcs_keys(input.fptr, &header, &status))
         printFitsError(status);

      input.flip = wcs->imflip;

      wcs = wcsinit(header);
 
      input.flip = wcs->imflip;
   }

   return 0;
}



/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void printError(char *msg)
{
   fprintf(stderr, "[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
   exit(1);
}




/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   printf("[struct stat=\"ERROR\", status=%d, msg=\"%s\"]\n", status, status_str);

   exit(1);
}

