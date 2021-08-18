/* Module: mBackground.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
2.2      John Good        08Sep15  fits_read_pix() incorrect null value
2.1      John Good        24Apr06  Don't want to fail in table mode when
                                   the image is not in the list.
2.0      John Good        25Aug05  Updated flag handling (it was buggy)
                                   and "no area" to mean both input and 
                                   output areas
1.9      John Good        13Oct04  Changed format for printing time
1.8      John Good        18Sep04  Check argument count again after mode is known
1.7      John Good        27Aug04  Added "[-s statusfile]" to Usage statement
                                   and fixed status file usage (wasn't incrementing
                                   argv properly)
1.6      John Good        17Apr04  Added "no areas" mode
1.5      John Good        14Nov03  Forgot to initialize 'tableDriven' flag
1.4      John Good        10Oct03  Added variant file column name handling
1.3      John Good        15Sep03  Updated fits_read_pix() call
1.2      John Good        07Apr03  Processing of output_file parameter
                                   had minor order problem
1.1      John Good        14Mar03  Modified command-line processing
                                   to use getopt() library.  Also
                                   put in code to check if A,B,C 
                                   are valid numbers and added specific
                                   messages for missing flux and area files.
1.0      John Good        29Jan03  Baseline code

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

#include <mBackground.h>
#include <montage.h>

#define MAXSTR  256

static int  noAreas;

static struct
{
   fitsfile *fptr;
   long      naxes[2];
   double    crpix1, crpix2;
}
input, input_area, output, output_area;

static time_t currtime, start;

static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mBackground                                                          */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mBackground, removes a background plane from a single   */
/*  projected image.                                                     */
/*                                                                       */
/*   char  *input_file     Input FITS file                               */
/*   char  *output_file    Output background-removed FITS file           */
/*                                                                       */
/*   double A              A coefficient in (A*x + B*y + C) background   */
/*                         level equation                                */
/*   double B              B coefficient in (A*x + B*y + C) background   */
/*                         level equation                                */
/*   double C              C level in (A*x + B*y + C) background         */
/*                         level equation                                */
/*                                                                       */
/*   int    noAreas        Don't process associated area images          */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mBackgroundReturn *mBackground(char *input_file, char *ofile, double A, double B, double C, int noAreasin, int debug)
{
   int       i, j, status;
   long      fpixel[4], nelements;
   double   *buffer, *abuffer;
   int       nullcnt;

   double    pixel_value, background, x, y;

   double  **data;
   double  **area;

   char      output_file      [MAXSTR];
   char      output_area_file [MAXSTR];
   char      infile           [MAXSTR];
   char      inarea           [MAXSTR];
   char      line             [MAXSTR];

   struct mBackgroundReturn *returnStruct;


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

   returnStruct = (struct mBackgroundReturn *)malloc(sizeof(struct mBackgroundReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   strcpy(output_file, ofile);

   noAreas = noAreasin;

   if(strlen(input_file) > 5 
   && strcmp(input_file+strlen(input_file)-5, ".fits") == 0)
   {
      strcpy(line, input_file);

      line[strlen(line)-5] = '\0';

      strcpy(infile, line);
      strcat(infile,  ".fits");
      strcpy(inarea, line);
      strcat(inarea, "_area.fits");
   }
   else
   {
      strcpy(infile, input_file);
      strcat(infile,  ".fits");
      strcpy(inarea, input_file);
      strcat(inarea, "_area.fits");
   }

   if(strlen(output_file) > 5 &&
      strncmp(output_file+strlen(output_file)-5, ".fits", 5) == 0)
         output_file[strlen(output_file)-5] = '\0';

   strcpy(output_area_file, output_file);
   strcat(output_file,  ".fits");
   strcat(output_area_file, "_area.fits");

   if(debug >= 1)
   {
      printf("input_file       = [%s]\n", input_file);
      printf("output_file      = [%s]\n", output_file);
      printf("output_area_file = [%s]\n", output_area_file);
      printf("A                = %-g\n",  A);
      printf("B                = %-g\n",  B);
      printf("C                = %-g\n",  C);
      printf("noAreas          = %d\n",   noAreas);
      fflush(stdout);
   }


   /************************/
   /* Read the input image */
   /************************/

   time(&currtime);
   start = currtime;

   if(mBackground_readFits(infile, inarea) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("\nflux file            =  %s\n",  infile);
      printf("input.naxes[0]       =  %ld\n",    input.naxes[0]);
      printf("input.naxes[1]       =  %ld\n",    input.naxes[1]);
      printf("input.crpix1         =  %-g\n",   input.crpix1);
      printf("input.crpix2         =  %-g\n",   input.crpix2);

      printf("\narea file            =  %s\n",  inarea);
      printf("input_area.naxes[0]  =  %ld\n",    input.naxes[0]);
      printf("input_area.naxes[1]  =  %ld\n",    input.naxes[1]);
      printf("input_area.crpix1    =  %-g\n",   input.crpix1);
      printf("input_area.crpix2    =  %-g\n",   input.crpix2);

      fflush(stdout);
   }

   output.naxes[0] = input.naxes[0];
   output.naxes[1] = input.naxes[1];
   output.crpix1   = input.crpix1;
   output.crpix2   = input.crpix2;

   output_area.naxes[0] = output.naxes[0];
   output_area.naxes[1] = output.naxes[1];
   output_area.crpix1   = output.crpix1;
   output_area.crpix2   = output.crpix2;



   /***********************************************/ 
   /* Allocate memory for the output image pixels */ 
   /* (same size as the input image)              */ 
   /***********************************************/ 

   data = (double **)malloc(output.naxes[1] * sizeof(double *));

   for(j=0; j<output.naxes[1]; ++j)
      data[j] = (double *)malloc(output.naxes[0] * sizeof(double));

   if(debug >= 1)
   {
      printf("\n%lu bytes allocated for image pixels\n", 
         output.naxes[0] * output.naxes[1] * sizeof(double));
      fflush(stdout);
   }


   /****************************/
   /* Initialize data to zeros */
   /****************************/

   for (j=0; j<output.naxes[1]; ++j)
   {
      for (i=0; i<output.naxes[0]; ++i)
      {
         data[j][i] = 0.;
      }
   }


   /**********************************************/ 
   /* Allocate memory for the output pixel areas */ 
   /**********************************************/ 

   area = (double **)malloc(output.naxes[1] * sizeof(double *));

   for(j=0; j<output.naxes[1]; ++j)
      area[j] = (double *)malloc(output.naxes[0] * sizeof(double));

   if(debug >= 1)
   {
      printf("%lu bytes allocated for pixel areas\n", 
         output.naxes[0] * output.naxes[1] * sizeof(double));
      fflush(stdout);
   }


   /****************************/
   /* Initialize area to zeros */
   /****************************/

   for (j=0; j<output.naxes[1]; ++j)
   {
      for (i=0; i<output.naxes[0]; ++i)
      {
         area[j][i] = 0.;
      }
   }


   /**********************************************************/
   /* Create the output array by processing the input pixels */
   /**********************************************************/

   buffer  = (double *)malloc(input.naxes[0] * sizeof(double));
   abuffer = (double *)malloc(input.naxes[0] * sizeof(double));

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   nelements = input.naxes[0];

   status = 0;


   /*****************************/
   /* Loop over the input lines */
   /*****************************/

   if(debug >= 1)
   {
      x = input.naxes[0]/2. - input.crpix1;
      y = input.naxes[1]/2. - input.crpix2;

      background = A*x + B*y + C;

      printf("\nBackground offset for %s at center (%-g,%-g) = %-g\n\n", 
         infile, x, y, background);
      
      fflush(stdout);
   }

   for (j=0; j<input.naxes[1]; ++j)
   {
      if(debug >= 2)
      {
         if(debug >= 3)
            printf("\n");

         printf("\rProcessing input row %5d  ", j);

         if(debug >= 3)
            printf("\n");

         fflush(stdout);
      }


      /***********************************/
      /* Read a line from the input file */
      /***********************************/

      if(fits_read_pix(input.fptr, TDOUBLE, fpixel, nelements, &nan,
                       buffer, &nullcnt, &status))
      {
         mBackground_printFitsError(status);
   
         for(j=0; j<output.naxes[1]; ++j)
         {
            free(data[j]);
            free(area[j]);

            data[j] = (double *)NULL;
            area[j] = (double *)NULL;
         }

         free(data);
         free(area);

         data = (double **)NULL;
         area = (double **)NULL;

         free(buffer);
         free(abuffer);

         buffer  = (double *)NULL;
         abuffer = (double *)NULL;

         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
      
      if(noAreas)
      {
         for (i=0; i<input.naxes[0]; ++i)
            abuffer[i] = 1.;
      }
      else
      {
         if(fits_read_pix(input_area.fptr, TDOUBLE, fpixel, nelements, &nan,
                          abuffer, &nullcnt, &status))
         {
            mBackground_printFitsError(status);
   
            for(j=0; j<output.naxes[1]; ++j)
            {
               free(data[j]);
               free(area[j]);

               data[j] = (double *)NULL;
               area[j] = (double *)NULL;
            }

            free(data);
            free(area);

            data = (double **)NULL;
            area = (double **)NULL;

            free(buffer);
            free(abuffer);

            buffer  = (double *)NULL;
            abuffer = (double *)NULL;

            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }
      
      ++fpixel[1];


      /************************/
      /* For each input pixel */
      /************************/

      for (i=0; i<input.naxes[0]; ++i)
      {
         x = i - input.crpix1;
         y = j - input.crpix2;

         pixel_value = buffer[i];

         background = A*x + B*y + C;

         if(mNaN(buffer[i])
         || abuffer[i] <= 0.)
         {
            data[j][i] = nan;
            area[j][i] = 0.;
         }
         else
         {
            data[j][i] = pixel_value - background;
            area[j][i] = abuffer[i];
         }

         if(debug >= 3)
         {
            printf("(%4d,%4d): %10.3e (bg: %10.3e) at (%8.1f,%8.1f) -> %10.3e (%10.3e)\n",
               j, i, pixel_value, background, x, y, data[j][i], area[j][i]);
            fflush(stdout);
         }
      }
   }

   if(debug >= 1)
   {
      time(&currtime);
      printf("\nDone reading data (%.0f seconds)\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /********************************/
   /* Create the output FITS files */
   /********************************/

   remove(output_file);               

   if(fits_create_file(&output.fptr, output_file, &status)) 
   {
      mBackground_printFitsError(status);           
   
      for(j=0; j<output.naxes[1]; ++j)
      {
         free(data[j]);
         free(area[j]);

         data[j] = (double *)NULL;
         area[j] = (double *)NULL;
      }

      free(data);
      free(area);

      data = (double **)NULL;
      area = (double **)NULL;

      free(buffer);
      free(abuffer);

      buffer  = (double *)NULL;
      abuffer = (double *)NULL;

      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(!noAreas)
   {
      remove(output_area_file);               

      if(fits_create_file(&output_area.fptr, output_area_file, &status)) 
      {
         mBackground_printFitsError(status);           
   
         for(j=0; j<output.naxes[1]; ++j)
         {
            free(data[j]);
            free(area[j]);

            data[j] = (double *)NULL;
            area[j] = (double *)NULL;
         }

         free(data);
         free(area);

         data = (double **)NULL;
         area = (double **)NULL;

         free(buffer);
         free(abuffer);

         buffer  = (double *)NULL;
         abuffer = (double *)NULL;

         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }

   if(debug >= 1)
   {
      printf("\nFITS output files created (not yet populated)\n"); 
      fflush(stdout);
   }


   /********************************/
   /* Copy all the header keywords */
   /* from the input to the output */
   /********************************/

   if(fits_copy_header(input.fptr, output.fptr, &status))
   {
      mBackground_printFitsError(status);           
   
      for(j=0; j<output.naxes[1]; ++j)
      {
         free(data[j]);
         free(area[j]);

         data[j] = (double *)NULL;
         area[j] = (double *)NULL;
      }

      free(data);
      free(area);

      data = (double **)NULL;
      area = (double **)NULL;

      free(buffer);
      free(abuffer);

      buffer  = (double *)NULL;
      abuffer = (double *)NULL;

      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(!noAreas)
   {
      if(fits_copy_header(input.fptr, output_area.fptr, &status))
      {
         mBackground_printFitsError(status);           
   
         for(j=0; j<output.naxes[1]; ++j)
         {
            free(data[j]);
            free(area[j]);

            data[j] = (double *)NULL;
            area[j] = (double *)NULL;
         }

         free(data);
         free(area);

         data = (double **)NULL;
         area = (double **)NULL;

         free(buffer);
         free(abuffer);

         buffer  = (double *)NULL;
         abuffer = (double *)NULL;

         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }

   if(debug >= 1)
   {
      printf("Header keywords copied to FITS output files\n\n"); 
      fflush(stdout);
   }

   if(fits_close_file(input.fptr, &status))
   {
      mBackground_printFitsError(status);
   
      for(j=0; j<output.naxes[1]; ++j)
      {
         free(data[j]);
         free(area[j]);

         data[j] = (double *)NULL;
         area[j] = (double *)NULL;
      }

      free(data);
      free(area);

      data = (double **)NULL;
      area = (double **)NULL;

      free(buffer);
      free(abuffer);

      buffer  = (double *)NULL;
      abuffer = (double *)NULL;

      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(!noAreas)
      if(fits_close_file(input_area.fptr, &status))
      {
         mBackground_printFitsError(status);
   
         for(j=0; j<output.naxes[1]; ++j)
         {
            free(data[j]);
            free(area[j]);

            data[j] = (double *)NULL;
            area[j] = (double *)NULL;
         }

         free(data);
         free(area);

         data = (double **)NULL;
         area = (double **)NULL;

         free(buffer);
         free(abuffer);

         buffer  = (double *)NULL;
         abuffer = (double *)NULL;

         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }


   /***************************/
   /* Modify BITPIX to be -64 */
   /***************************/

   if(fits_update_key_lng(output.fptr, "BITPIX", -64,
                                  (char *)NULL, &status))
   {
      mBackground_printFitsError(status);
   
      for(j=0; j<output.naxes[1]; ++j)
      {
         free(data[j]);
         free(area[j]);

         data[j] = (double *)NULL;
         area[j] = (double *)NULL;
      }

      free(data);
      free(area);

      data = (double **)NULL;
      area = (double **)NULL;

      free(buffer);
      free(abuffer);

      buffer  = (double *)NULL;
      abuffer = (double *)NULL;

      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(!noAreas)
   {
      if(fits_update_key_lng(output_area.fptr, "BITPIX", -64,
                                     (char *)NULL, &status))
      {
         mBackground_printFitsError(status);
   
         for(j=0; j<output.naxes[1]; ++j)
         {
            free(data[j]);
            free(area[j]);

            data[j] = (double *)NULL;
            area[j] = (double *)NULL;
         }

         free(data);
         free(area);

         data = (double **)NULL;
         area = (double **)NULL;

         free(buffer);
         free(abuffer);

         buffer  = (double *)NULL;
         abuffer = (double *)NULL;

         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }


   /************************/
   /* Write the image data */
   /************************/

   fpixel[0] = 1;
   fpixel[1] = 1;
   nelements = output.naxes[0];

   for(j=0; j<output.naxes[1]; ++j)
   {
      if (fits_write_pix(output.fptr, TDOUBLE, fpixel, nelements, 
                         (void *)(&data[j][0]), &status))
      {
         mBackground_printFitsError(status);
   
         for(j=0; j<output.naxes[1]; ++j)
         {
            free(data[j]);
            free(area[j]);

            data[j] = (double *)NULL;
            area[j] = (double *)NULL;
         }

         free(data);
         free(area);

         data = (double **)NULL;
         area = (double **)NULL;

         free(buffer);
         free(abuffer);

         buffer  = (double *)NULL;
         abuffer = (double *)NULL;

         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      ++fpixel[1];
   }

   if(debug >= 1)
   {
      printf("Data written to FITS data image\n"); 
      fflush(stdout);
   }


   /***********************/
   /* Write the area data */
   /***********************/

   if(!noAreas)
   {
      fpixel[0] = 1;
      fpixel[1] = 1;
      nelements = output.naxes[0];

      for(j=0; j<output.naxes[1]; ++j)
      {
         if (fits_write_pix(output_area.fptr, TDOUBLE, fpixel, nelements,
                            (void *)(&area[j][0]), &status))
         {
            mBackground_printFitsError(status);
   
            for(j=0; j<output.naxes[1]; ++j)
            {
               free(data[j]);
               free(area[j]);

               data[j] = (double *)NULL;
               area[j] = (double *)NULL;
            }

            free(data);
            free(area);

            data = (double **)NULL;
            area = (double **)NULL;

            free(buffer);
            free(abuffer);

            buffer  = (double *)NULL;
            abuffer = (double *)NULL;

            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }

         ++fpixel[1];
      }

      if(debug >= 1)
      {
         printf("Data written to FITS area image\n\n"); 
         fflush(stdout);
      }
   }


   /***********************/
   /* Close the FITS file */
   /***********************/

   if(fits_close_file(output.fptr, &status))
   {
      mBackground_printFitsError(status);           
   
      for(j=0; j<output.naxes[1]; ++j)
      {
         free(data[j]);
         free(area[j]);

         data[j] = (double *)NULL;
         area[j] = (double *)NULL;
      }

      free(data);
      free(area);

      data = (double **)NULL;
      area = (double **)NULL;

      free(buffer);
      free(abuffer);

      buffer  = (double *)NULL;
      abuffer = (double *)NULL;

      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("FITS data image finalized\n"); 
      fflush(stdout);
   }

   if(!noAreas)
   {
      if(fits_close_file(output_area.fptr, &status))
      {
         mBackground_printFitsError(status);           
   
         for(j=0; j<output.naxes[1]; ++j)
         {
            free(data[j]);
            free(area[j]);

            data[j] = (double *)NULL;
            area[j] = (double *)NULL;
         }

         free(data);
         free(area);

         data = (double **)NULL;
         area = (double **)NULL;

         free(buffer);
         free(abuffer);

         buffer  = (double *)NULL;
         abuffer = (double *)NULL;

         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      if(debug >= 1)
      {
         printf("FITS area image finalized\n\n"); 
         fflush(stdout);
      }
   }

   time(&currtime);

   for(j=0; j<output.naxes[1]; ++j)
   {
      free(data[j]);
      free(area[j]);

      data[j] = (double *)NULL;
      area[j] = (double *)NULL;
   }

   free(data);
   free(area);

   data = (double **)NULL;
   area = (double **)NULL;

   free(buffer);
   free(abuffer);

   buffer  = (double *)NULL;
   abuffer = (double *)NULL;

   returnStruct->status = 0;

   sprintf(returnStruct->msg,    "time=%.1f",    (double)(currtime - start));
   sprintf(returnStruct->json, "{\"time\":%.1f}", (double)(currtime - start));

   returnStruct->time  = (double)(currtime - start);

   return returnStruct;
}


/*******************************************/
/*                                         */
/*  Open a FITS file pair and extract the  */
/*  pertinent header information.          */
/*                                         */
/*******************************************/

int mBackground_readFits(char *fluxfile, char *areafile)
{
   int    status, nfound;
   long   naxes[2];
   double crpix[2];
   char   errstr[MAXSTR];

   char  *checkHdr;

   status = 0;

   checkHdr =montage_checkHdr(fluxfile, 0, 0);

   if(checkHdr)
   {
      strcpy(montage_msgstr, checkHdr);
      return 1;
   }

   if(!noAreas)
   {
      checkHdr = montage_checkHdr(areafile, 0, 0);

      if(checkHdr)
      {
         strcpy(montage_msgstr, checkHdr);
         return 1;
      }

      if(fits_open_file(&input_area.fptr, areafile, READONLY, &status))
      {
         sprintf(errstr, "Area file %s missing or invalid FITS", areafile);
         mBackground_printError(errstr);
         return 1;
      }
   }

   if(fits_open_file(&input.fptr, fluxfile, READONLY, &status))
   {
      sprintf(errstr, "Image file %s missing or invalid FITS", fluxfile);
      mBackground_printError(errstr);
      return 1;
   }

   if(fits_read_keys_lng(input.fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
   {
      mBackground_printFitsError(status);
      return 1;
   }
   
   input.naxes[0] = naxes[0];
   input.naxes[1] = naxes[1];

   input_area.naxes[0] = naxes[0];
   input_area.naxes[1] = naxes[1];

   if(fits_read_keys_dbl(input.fptr, "CRPIX", 1, 2, crpix, &nfound, &status))
   {
      mBackground_printFitsError(status);
      return 1;
   }

   input.crpix1 = crpix[0];
   input.crpix2 = crpix[1];

   input_area.crpix1 = crpix[0];
   input_area.crpix2 = crpix[1];
   
   return 0;
}



/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void mBackground_printError(char *msg)
{
   strcpy(montage_msgstr, msg);
}




/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mBackground_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}

