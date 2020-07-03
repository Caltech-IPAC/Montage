/* Module: mAddMem.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        15Feb19  Baseline code

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
#include <mtbl.h>

#include <mAddMem.h>
#include <montage.h>

#define MAXSTR  256
#define MAXFILE 256


int  mAddMem_debug;
int  noAreas;

struct
{
   fitsfile *fptr;
   long      naxes[2];
   double    crpix1, crpix2;
}
   input, input_area, output, output_area;

static time_t currtime, start;


static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mAddMem                                                              */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mAddMem, is used as part of a background                */
/*  correction mechanism.  Pairwise, images that overlap are differenced */
/*  and the difference images fit with a surface (usually a plane).      */
/*  These planes are analyzed, and a correction determined for each      */
/*  input image (by mBgModel).                                           */
/*                                                                       */
/*   char  *path           Path to images in table file                  */
/*                                                                       */
/*   char  *table_file     List of files to coadd                        */
/*                                                                       */
/*   char  *template_file  FITS header file used to define the desired   */
/*                         output region                                 */
/*                                                                       */
/*   char  *output_file    Output mosaic image                           */
/*                                                                       */
/*   int    noAreas        Do not look for or create area images as part */
/*                         of the mosaicking                             */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

struct mAddMemReturn *mAddMem(char *path, char *table_file, char *template_file, char *ofile,
                              int noAreasin, int debugin)
{
   int       i, j, ncols, nfile, status;
   long      fpixel[4], nelements;
   int       nullcnt;
   int       ioffset, joffset;
   int       imin, jmin;
   int       imax, jmax;
   int       ilength;
   int       jlength;

   int       ifname;

   double   *buffer, *abuffer;

   char     *checkHdr;

   double  **data;
   double  **area;

   char      filename[MAXSTR];

   char      infile[MAXSTR];
   char      inarea[MAXSTR];

   char      output_file     [MAXSTR];
   char      output_area_file[MAXSTR];

   struct mAddMemReturn *returnStruct;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mAddMemReturn *)malloc(sizeof(struct mAddMemReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


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



   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mAddMem_debug   = debugin;
   noAreas         = noAreasin;

   strcpy(output_file, ofile);

   checkHdr = montage_checkHdr(template_file, 1, 0);

   if(checkHdr)
   {
      strcpy(returnStruct->msg, checkHdr);
      return returnStruct;
   }

   if(strlen(output_file) > 5 &&
      strncmp(output_file+strlen(output_file)-5, ".fits", 5) == 0)
         output_file[strlen(output_file)-5] = '\0';

   strcpy(output_area_file, output_file);
   strcat(output_file,  ".fits");
   strcat(output_area_file, "_area.fits");

   if(mAddMem_debug >= 1)
   {
      printf("table_file       = [%s]\n", table_file);
      printf("output_file      = [%s]\n", output_file);
      printf("output_area_file = [%s]\n", output_area_file);
      printf("template_file    = [%s]\n", template_file);
      fflush(stdout);
   }

   /*****************************/ 
   /* Open the image list table */
   /* to get metadata for input */
   /* files                     */      
   /*****************************/ 

   ncols = topen(table_file);

   if(ncols <= 0)
   {
      sprintf(returnStruct->msg, "Invalid image metadata file: %s", table_file);
      return returnStruct;
   }


   /**************************/
   /* Get indices of columns */
   /**************************/

   ifname  = tcol("fname");


   /***********************************/
   /* Look for alternate column names */
   /***********************************/

   if(ifname < 0)
      ifname = tcol( "file");


   /**************************************/
   /* Were all required columns present? */
   /**************************************/

   if(ifname  < 0)
   {
      strcpy(returnStruct->msg, "Need column: fname in image list");
      return returnStruct;
   }


   /*************************************************/ 
   /* Process the output header template to get the */ 
   /* image size, coordinate system and projection  */ 
   /*************************************************/ 

   if(mAddMem_readTemplate(template_file) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAddMem_debug >= 1)
   {
      printf("output.naxes[0] =  %ld\n", output.naxes[0]);
      printf("output.naxes[1] =  %ld\n", output.naxes[1]);
      printf("output.crpix1   =  %-g\n", output.crpix1);
      printf("output.crpix2   =  %-g\n", output.crpix2);
      fflush(stdout);
   }


   /***********************************************/ 
   /* Allocate memory for the output image pixels */ 
   /***********************************************/ 

   ilength = output.naxes[0];
   jlength = output.naxes[1];

   data = (double **)malloc(jlength * sizeof(double *));

   for(j=0; j<jlength; ++j)
      data[j] = (double *)malloc(ilength * sizeof(double));

   if(mAddMem_debug >= 1)
   {
      printf("\n%lu bytes allocated for image pixels\n", 
         ilength * jlength * sizeof(double));
      fflush(stdout);
   }


   /****************************/
   /* Initialize data to zeros */
   /****************************/

   for (j=0; j<jlength; ++j)
   {
      for (i=0; i<ilength; ++i)
      {
         data[j][i] = 0.;
      }
   }


   /**********************************************/ 
   /* Allocate memory for the output pixel areas */ 
   /**********************************************/ 

   area = (double **)malloc(jlength * sizeof(double *));

   for(j=0; j<jlength; ++j)
      area[j] = (double *)malloc(ilength * sizeof(double));

   if(mAddMem_debug >= 1)
   {
      printf("%lu bytes allocated for pixel areas\n\n", 
         ilength * jlength * sizeof(double));
      fflush(stdout);
   }


   /****************************/
   /* Initialize area to zeros */
   /****************************/

   for (j=0; j<jlength; ++j)
   {
      for (i=0; i<ilength; ++i)
      {
         area[j][i] = 0.;
      }
   }


   /*****************************/
   /* Loop over the input files */
   /*****************************/

   time(&currtime);
   start = currtime;

   nfile = 1;

   while(1)
   {
      status = tread();

      if(status < 0)
         break;


      /* Get filename */

      strcpy(filename, montage_filePath(path, tval(ifname)));

      if(mAddMem_debug >= 1)
      {
         printf("file %d: [%s]\n", nfile, filename);
         fflush(stdout);
      }


      /* Need to build _area filenames if we have area images */

      if (!noAreas)
      {
        if(strlen(filename) > 5 &&
             strncmp(filename+strlen(filename)-5, ".fits", 5) == 0)
            filename[strlen(filename)-5] = '\0';
      }

      strcpy(infile, filename);

      if (!noAreas)
      {
        strcat(infile,  ".fits");
        strcpy(inarea, filename);
        strcat(inarea, "_area.fits");
      }


      /************************/
      /* Read the input image */
      /************************/

      if(mAddMem_readFits(infile, inarea) > 0)
      {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      ioffset = output.crpix1 - input.crpix1;
      joffset = output.crpix2 - input.crpix2;

      if(mAddMem_debug >= 2)
      {
         printf("\nflux file            =  %s\n",  infile);
         printf("input.naxes[0]       =  %ld\n",   input.naxes[0]);
         printf("input.naxes[1]       =  %ld\n",   input.naxes[1]);
         printf("input.crpix1         =  %-g\n",   input.crpix1);
         printf("input.crpix2         =  %-g\n",   input.crpix2);
         printf("\narea file            =  %s\n",  inarea);
         printf("input_area.naxes[0]  =  %ld\n",   input.naxes[0]);
         printf("input_area.naxes[1]  =  %ld\n",   input.naxes[1]);
         printf("input_area.crpix1    =  %-g\n",   input.crpix1);
         printf("input_area.crpix2    =  %-g\n",   input.crpix2);
         printf("\nioffset              =  %d\n",  ioffset);
         printf("joffset              =  %d\n\n",  joffset);

         fflush(stdout);
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

      for (j=0; j<input.naxes[1]; ++j)
      {
         if(mAddMem_debug >= 2)
         {
            printf("\rProcessing input row %5d  ", j);
            fflush(stdout);
         }

         if(mAddMem_debug >= 3)
         {
            printf("\n\n");
            fflush(stdout);
         }


         /***********************************/
         /* Read a line from the input file */
         /***********************************/

         if(fits_read_pix(input.fptr, TDOUBLE, fpixel, nelements, &nan,
                          buffer, &nullcnt, &status))
         {
            free(buffer);
            free(abuffer);

            for(j=0; j<jlength; ++j)
            {
               free(data[j]);
               free(area[j]);
            }

            free(data);
            free(area);

            mAddMem_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
         
         
         if(noAreas)
         {
            for(i=0; i<input.naxes[0]; ++i)
               abuffer[i] = 1.;
         }
         else
         {
            if(fits_read_pix(input_area.fptr, TDOUBLE, fpixel, nelements, &nan,
                             abuffer, &nullcnt, &status))
            {
               free(buffer);
               free(abuffer);

               for(j=0; j<jlength; ++j)
               {
                  free(data[j]);
                  free(area[j]);
               }

               free(data);
               free(area);

               mAddMem_printFitsError(status);
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
            if(mAddMem_debug >= 4)
            {
               printf("input:       line %5d / pixel %5d, value: %10.2e (area: %10.2e) [array: %5d %5d]\n",
                  j, i, buffer[i], abuffer[i], j+joffset, i+ioffset);
               fflush(stdout);
            }

            if(i+ioffset < 0) continue;
            if(j+joffset < 0) continue;

            if(i+ioffset >= output.naxes[0]) continue;
            if(j+joffset >= output.naxes[1]) continue;

            if(mNaN(buffer[i]))
               continue;

            if(mAddMem_debug >= 3)
            {
               printf("keep:        line %5d / pixel %5d, value: %10.2e (area: %10.2e) [array: %5d %5d]\n",
                  j, i, buffer[i], abuffer[i], j+joffset, i+ioffset);
               fflush(stdout);
            }

            if(mAddMem_debug >= 5)
            {
               printf("File %d: Adding pixel value.\n", nfile);
               fflush(stdout);
            }

            data[j+joffset][i+ioffset] += buffer[i] * abuffer[i];
            area[j+joffset][i+ioffset] += abuffer[i];

            if(mAddMem_debug >= 3)
            {
               printf("cumulative:  line %5d / pixel %5d, value: %10.2e (area: %10.2e) [array: %5d %5d]\n",
                  j, i, data[j+joffset][i+ioffset], area[j+joffset][i+ioffset], j+joffset, i+ioffset);
               fflush(stdout);
            }
         }
      }

      free(buffer);
      free(abuffer);

      if(fits_close_file(input.fptr, &status))
      {
         for(j=0; j<jlength; ++j)
         {
            free(data[j]);
            free(area[j]);
         }

         free(data);
         free(area);

         mAddMem_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      if(!noAreas)
      {
         if(fits_close_file(input_area.fptr, &status))
         {
            for(j=0; j<jlength; ++j)
            {
               free(data[j]);
               free(area[j]);
            }

            free(data);
            free(area);

            mAddMem_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }

      ++nfile;
   }

   if(mAddMem_debug >= 1)
   {
      time(&currtime);
      printf("\nDone reading data (%.0f seconds)\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /****************************/
   /* Normalize the data array */
   /****************************/

   for (j=0; j<jlength; ++j)
   {
      for (i=0; i<ilength; ++i)
      {
         if(mNaN(area[j][i]) || area[j][i] == 0.)
            data[j][i] = nan;
         
         else
            data[j][i] = data[j][i]/area[j][i];
      }
   }


   /************************************/
   /* Find the pixel range of the data */
   /************************************/

   imin = 9999999;
   imax = 0;

   jmin = 9999999;
   jmax = 0;

   for (j=0; j<jlength; ++j)
   {
      for (i=0; i<ilength; ++i)
      {
         if(area[j][i] > 0.)
         {
            if(j < jmin) jmin = j;
            if(j > jmax) jmax = j;
            if(i < imin) imin = i;
            if(i > imax) imax = i;
         }
      }
   }

   if(mAddMem_debug >= 1)
   {
      printf("j min    = %d\n", jmin);
      printf("j max    = %d\n", jmax);
      printf("i min    = %d\n", imin);
      printf("i max    = %d\n", imax);
   }

   if(jmin > jmax || imin > imax)
   {
      mAddMem_printError("All pixels are blank.");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /********************************/
   /* Create the output FITS files */
   /********************************/

   remove(output_file);               
   remove(output_area_file);               

   if(fits_create_file(&output.fptr, output_file, &status)) 
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_create_file(&output_area.fptr, output_area_file, &status)) 
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*********************************************************/
   /* Create the FITS image.  All the required keywords are */
   /* handled automatically.                                */
   /*********************************************************/

   if (fits_create_img(output.fptr, bitpix, naxis, output.naxes, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);          
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAddMem_debug >= 1)
   {
      printf("\nFITS data image created (not yet populated)\n"); 
      fflush(stdout);
   }

   if (fits_create_img(output_area.fptr, bitpix, naxis, output_area.naxes, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);          
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAddMem_debug >= 1)
   {
      printf("FITS area image created (not yet populated)\n"); 
      fflush(stdout);
   }


   /****************************************/
   /* Set FITS header from a template file */
   /****************************************/

   if(fits_write_key_template(output.fptr, template_file, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAddMem_debug >= 1)
   {
      printf("Template keywords written to FITS data image\n"); 
      fflush(stdout);
   }

   if(fits_write_key_template(output_area.fptr, template_file, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAddMem_debug >= 1)
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
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output_area.fptr, "BITPIX", -64,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /***************************************************/
   /* Update NAXIS, NAXIS1, NAXIS2, CRPIX1 and CRPIX2 */
   /***************************************************/

   if(fits_update_key_lng(output.fptr, "NAXIS", 2,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS1", imax-imin+1,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output.fptr, "NAXIS2", jmax-jmin+1,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(output.fptr, "CRPIX1", output.crpix1-imin, -14,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(output.fptr, "CRPIX2", output.crpix2-jmin, -14,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output_area.fptr, "NAXIS", 2,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output_area.fptr, "NAXIS1", imax-imin+1,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(output_area.fptr, "NAXIS2", jmax-jmin+1,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(output_area.fptr, "CRPIX1", output.crpix1-imin, -14,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(output_area.fptr, "CRPIX2", output.crpix2-jmin, -14,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAddMem_debug >= 1)
   {
      printf("Template keywords BITPIX, CRPIX, and NAXIS updated\n");
      fflush(stdout);
   }


   /************************/
   /* Write the image data */
   /************************/

   fpixel[0] = 1;
   fpixel[1] = 1;
   nelements = imax - imin + 1;

   if(mAddMem_debug >= 2)
   {
      for(j=jmin; j<=jmax; ++j)
      {
         for(i=imin; i<=imax; ++i)
         {
            printf("final:       line %5d / pixel %5d, value: %10.2e (area: %10.2e)\n",
               j, i, data[j][i], area[j][i]);
            fflush(stdout);
         }
      }
   }

   for(j=jmin; j<=jmax; ++j)
   {
      if (fits_write_pix(output.fptr, TDOUBLE, fpixel, nelements, 
                         (void *)(&data[j][imin]), &status))
      {
         for(i=0; i<jlength; ++i)
         {
            free(data[i]);
            free(area[i]);
         }

         free(data);
         free(area);

         mAddMem_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      ++fpixel[1];
   }

   if(mAddMem_debug >= 1)
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
      nelements = imax - imin + 1;

      for(j=jmin; j<=jmax; ++j)
      {
         if (fits_write_pix(output_area.fptr, TDOUBLE, fpixel, nelements,
                            (void *)(&area[j][imin]), &status))
         {
            for(i=0; i<jlength; ++i)
            {
               free(data[i]);
               free(area[i]);
            }

            free(data);
            free(area);

            mAddMem_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }

         ++fpixel[1];
      }

      if(mAddMem_debug >= 1)
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
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mAddMem_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAddMem_debug >= 1)
   {
      printf("FITS data image finalized\n"); 
      fflush(stdout);
   }

   if(!noAreas)
   {
      if(fits_close_file(output_area.fptr, &status))
      {
         for(j=0; j<jlength; ++j)
         {
            free(data[j]);
            free(area[j]);
         }

         free(data);
         free(area);

         mAddMem_printFitsError(status);           
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      if(mAddMem_debug >= 1)
      {
         printf("FITS area image finalized\n\n"); 
         fflush(stdout);
      }
   }

   time(&currtime);

   for(j=0; j<jlength; ++j)
   {
      free(data[j]);
      free(area[j]);
   }

   free(data);
   free(area);

   sprintf(montage_msgstr, "time=%.1f", (double)(currtime - start));

   sprintf(montage_json, "{\"time\":%.1f\"}", (double)(currtime - start));

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   returnStruct->time      = (double)(currtime - start);

   return returnStruct;
}


/**************************************************/
/*                                                */
/*  Read the output header template file.         */
/*  Specifically extract the image size info.     */
/*                                                */
/**************************************************/

int mAddMem_readTemplate(char *filename)
{
   int       i;
   FILE     *fp;
   char      line[MAXSTR];


   /********************************************************/
   /* Open the template file, read and parse all the lines */
   /********************************************************/

   fp = fopen(filename, "r");

   if(fp == (FILE *)NULL)
   {
      mAddMem_printError("Template file not found.");
      return 1;
   }

   while(1)
   {
      if(fgets(line, MAXSTR, fp) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';
      
      if(line[strlen(line)-1] == '\r')
         line[strlen(line)-1]  = '\0';

      if(mAddMem_debug >= 3)
      {
         printf("Template line: [%s]\n", line);
         fflush(stdout);
      }

      for(i=strlen(line); i<80; ++i)
         line[i] = ' ';
      
      line[80] = '\0';

      mAddMem_parseLine(line);
   }

   fclose(fp);

   return 0;
}



/**************************************************/
/*                                                */
/*  Parse header lines from the template,         */
/*  looking for NAXIS1, NAXIS2, CRPIX1 and CRPIX2 */
/*                                                */
/**************************************************/

int mAddMem_parseLine(char *line)
{
   char *keyword;
   char *value;
   char *end;

   int   len;

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

   if(mAddMem_debug >= 2)
   {
      printf("keyword [%s] = value [%s]\n", keyword, value);
      fflush(stdout);
   }

   if(strcmp(keyword, "NAXIS1") == 0)
   {
      output.naxes[0] = atoi(value);
      output_area.naxes[0] = atoi(value);
   }

   if(strcmp(keyword, "NAXIS2") == 0)
   {
      output.naxes[1] = atoi(value);
      output_area.naxes[1] = atoi(value);
   }

   if(strcmp(keyword, "CRPIX1") == 0)
   {
      output.crpix1 = atof(value);
      output_area.crpix1 = atof(value);
   }

   if(strcmp(keyword, "CRPIX2") == 0)
   {
      output.crpix2 = atof(value);
      output_area.crpix2 = atof(value);
   }

   return 0;
}


/*******************************************/
/*                                         */
/*  Open a FITS file pair and extract the  */
/*  pertinent header information.          */
/*                                         */
/*******************************************/

int mAddMem_readFits(char *fluxfile, char *areafile)
{
   int    status, nfound;
   long   naxes[2];
   double crpix[2];
   char   errstr[MAXSTR];

   status = 0;

   if(!noAreas)
   {
      if(fits_open_file(&input_area.fptr, areafile, READONLY, &status))
      {
         sprintf(errstr, "Area file %s missing or invalid FITS", areafile);
         mAddMem_printError(errstr);
         return 1;
      }
   }

   if(fits_open_file(&input.fptr, fluxfile, READONLY, &status))
   {
      sprintf(errstr, "Image file %s missing or invalid FITS", fluxfile);
      mAddMem_printError(errstr);
      return 1;
   }

   if(fits_read_keys_lng(input.fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
   {
      mAddMem_printFitsError(status);
      return 1;
   }
   
   input.naxes[0] = naxes[0];
   input.naxes[1] = naxes[1];

   input_area.naxes[0] = naxes[0];
   input_area.naxes[1] = naxes[1];

   if(fits_read_keys_dbl(input.fptr, "CRPIX", 1, 2, crpix, &nfound, &status))
   {
      mAddMem_printFitsError(status);
      return 1;
   }

   input.crpix1 = crpix[0];
   input.crpix2 = crpix[1];

   input_area.crpix1 = crpix[0];
   input_area.crpix2 = crpix[1];
   
   return 0;
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mAddMem_printFitsError(int status)
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

void mAddMem_printError(char *msg)
{
   strcpy(montage_msgstr, msg);
}
