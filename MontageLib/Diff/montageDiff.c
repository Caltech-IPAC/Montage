/* Module: mDiff.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
3.1      John Good        08Sep15  fits_read_pix() incorrect null value
3.0      John Good        17Nov14  Cleanup to avoid compiler warnings, in proparation
                                   for new development cycle.
2.7      John Good        19Sep06  The area check for including a pixel was
                                   too stringent. Changed from 0.05 to 0.333
2.6      John Good        09Jul06  Make the WARNINGs into ERRORs
2.5      John Good        28Dec04  Deleted print lines that referred to
                                   uninitialized ilength, jlength (Ticket 252)
2.4      John Good        13Oct04  Changed format for printing time
2.3      John Good        24Sep04  Change a couple of messages to WARNINGs
2.2      John Good        03Aug04  Changed precision on updated keywords
2.1      John Good        07Jun04  Modified FITS key updating precision
2.0      John Good        16May04  Added "no areas" option
1.7      John Good        25Nov03  Added extern optarg references
1.6      John Good        21Nov03  Corrected bug in differencing;
                                   pixels outside the region of 
                                   overlap were sometimes getting
                                   kept.
1.5      John Good        15Sep03  Updated fits_read_pix() call
1.4      John Good        25Aug03  Added status file processing
1.3      John Good        08Apr03  Also remove <CR> from template lines
1.2      John Good        19Mar03  Check for no overlap
1.1      John Good        14Mar03  Modified command-line processing
                                   to use getopt() library.  Added
                                   specific messages for missing flux
                                   and area files.
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

#include <mDiff.h>
#include <montage.h>

#define MAXSTR  256
#define MAXFILE 256


int  mDiff_debug;
int  diff_noAreas;

struct
{
   int       bitpix;
   int       hdrsize;
   fitsfile *fptr;
   long      naxes[2];
   double    crpix1, crpix2;
}
   diff_input, diff_input_area, diff_output, diff_output_area;

static time_t currtime, start;


static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mDiff                                                                */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mDiff, is used as part of a background                  */
/*  correction mechanism.  Pairwise, images that overlap are differenced */
/*  and the difference images fit with a surface (usually a plane).      */
/*  These planes are analyzed, and a correction determined for each      */
/*  input image (by mBgModel).                                           */
/*                                                                       */
/*   char  *input_file1    First input file for differencing             */
/*   char  *input_file2    Second input file for differencing. Files     */
/*                         have to already have the same projection      */
/*                                                                       */
/*   char  *output_file    Output difference image                       */
/*                                                                       */
/*   char  *template_file  FITS header file used to define the desired   */
/*                         output region                                 */
/*                                                                       */
/*   int    noAreas        Do not look for or create area images as part */
/*                         of the differencing                           */
/*                                                                       */
/*   double factor         Optional scale factor to apply to the second  */
/*                         image before subtracting                      */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

struct mDiffReturn *mDiff(char *input_file1, char *input_file2, char *ofile, char *template_file, 
                          int noAreasin, double fact, int debugin)
{
   int       i, j, ifile, status;
   long      fpixel[4], nelements;
   int       nullcnt, haveMinMax;
   int       imin, imax, jmin, jmax;
   int       bitpix1, hdrsize1;
   int       bitpix2, hdrsize2;
   int       imin1, imax1, jmin1, jmax1;
   int       imin2, imax2, jmin2, jmax2;
   int       istart, iend, ilength;
   int       jstart, jend, jlength;
   double   *buffer, *abuffer;
   double    datamin, datamax;
   double    areamin, areamax;
   double    factor;

   char     *checkHdr;

   int       narea1, narea2;
   double    avearea1, avearea2;

   double    pixel_value;
   double    min_pixel;
   double    max_pixel;
   double    min_diff;
   double    max_diff;

   double  **data;
   double  **area;

   char      line[MAXSTR];

   char      infile[2][MAXSTR];
   char      inarea[2][MAXSTR];

   char      output_file     [MAXSTR];
   char      output_area_file[MAXSTR];

   struct mDiffReturn *returnStruct;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mDiffReturn *)malloc(sizeof(struct mDiffReturn));

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
      value.c[i] = (char)255;

   nan = value.d;



   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mDiff_debug  = debugin;
   diff_noAreas = noAreasin;

   strcpy(output_file, ofile);

   factor = fact;

   if(factor == 0.)
      factor = 1.;

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

   if(mDiff_debug >= 1)
   {
      printf("input_file1      = [%s]\n", input_file1);
      printf("input_file2      = [%s]\n", input_file2);
      printf("output_file      = [%s]\n", output_file);
      printf("output_area_file = [%s]\n", output_area_file);
      printf("template_file    = [%s]\n", template_file);
      fflush(stdout);
   }


   /****************************/
   /* Get the input file names */
   /****************************/

   if(strlen(input_file1) > 5 
   && strcmp(input_file1+strlen(input_file1)-5, ".fits") == 0)
   {
      strcpy(line, input_file1);

      line[strlen(line)-5] = '\0';

      strcpy(infile[0], line);
      strcat(infile[0],  ".fits");
      strcpy(inarea[0], line);
      strcat(inarea[0], "_area.fits");
   }
   else
   {
      strcpy(infile[0], input_file1);
      strcat(infile[0],  ".fits");
      strcpy(inarea[0], input_file1);
      strcat(inarea[0], "_area.fits");
   }
      

   if(strlen(input_file2) > 5 
   && strcmp(input_file2+strlen(input_file2)-5, ".fits") == 0)
   {
      strcpy(line, input_file2);

      line[strlen(line)-5] = '\0';

      strcpy(infile[1], line);
      strcat(infile[1],  ".fits");
      strcpy(inarea[1], line);
      strcat(inarea[1], "_area.fits");
   }
   else
   {
      strcpy(infile[1], input_file2);
      strcat(infile[1],  ".fits");
      strcpy(inarea[1], input_file2);
      strcat(inarea[1], "_area.fits");
   }
      
   if(mDiff_debug >= 1)
   {
      printf("\ninput files:\n\n");

      printf("   [%s][%s]\n", infile[0], inarea[0]);
      printf("   [%s][%s]\n", infile[1], inarea[1]);
      
      printf("\n");
   }


   /*************************************************/ 
   /* Process the output header template to get the */ 
   /* image size, coordinate system and projection  */ 
   /*************************************************/ 

   if(mDiff_readTemplate(template_file) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mDiff_debug >= 1)
   {
      printf("diff_output.naxes[0] =  %ld\n", diff_output.naxes[0]);
      printf("diff_output.naxes[1] =  %ld\n", diff_output.naxes[1]);
      printf("diff_output.crpix1   =  %-g\n", diff_output.crpix1);
      printf("diff_output.crpix2   =  %-g\n", diff_output.crpix2);
      fflush(stdout);
   }


   /*****************************************************/
   /* We open the two input files briefly to get enough */
   /* information to determine the region of overlap    */
   /* (for memory allocation purposes)                  */
   /*****************************************************/

   if(mDiff_readFits(infile[0], inarea[0]) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   bitpix1  = diff_input.bitpix;
   hdrsize1 = diff_input.hdrsize;

   imin1 = diff_output.crpix1 - diff_input.crpix1;
   jmin1 = diff_output.crpix2 - diff_input.crpix2;

   imax1 = imin1 + diff_input.naxes[0];
   jmax1 = jmin1 + diff_input.naxes[1];

   istart = imin1;
   iend   = imax1;

   jstart = jmin1;
   jend   = jmax1;

   if(mDiff_debug >= 1)
   {
      printf("\nFile 1:\n");
      printf("bitpix1              =  %d\n",    bitpix1);
      printf("hdrsize1             =  %d\n",    hdrsize1);
      printf("diff_input.naxes[0]       =  %ld\n",   diff_input.naxes[0]);
      printf("diff_input.naxes[1]       =  %ld\n",   diff_input.naxes[1]);
      printf("diff_input.crpix1         =  %-g\n",   diff_input.crpix1);
      printf("diff_input.crpix2         =  %-g\n",   diff_input.crpix2);
      printf("imin1                =  %d\n",    imin1);
      printf("imax1                =  %d\n",    imax1);
      printf("jmin1                =  %d\n",    jmin1);
      printf("jmax1                =  %d\n\n",  jmax1);

      fflush(stdout);
   }

   status = 0;

   if(fits_close_file(diff_input.fptr, &status))
   {
      mDiff_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(!diff_noAreas)
   {
      if(fits_close_file(diff_input_area.fptr, &status))
      {
         mDiff_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }

   if(mDiff_readFits(infile[1], inarea[1]) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   bitpix2  = diff_input.bitpix;
   hdrsize2 = diff_input.hdrsize;

   imin2 = diff_output.crpix1 - diff_input.crpix1;
   jmin2 = diff_output.crpix2 - diff_input.crpix2;

   imax2 = imin2 + diff_input.naxes[0];
   jmax2 = jmin2 + diff_input.naxes[1];

   if(mDiff_debug >= 1)
   {
      printf("\nFile 2:\n");
      printf("bitpix2              =  %d\n",    bitpix2);
      printf("hdrsize2             =  %d\n",    hdrsize2);
      printf("diff_input.naxes[0]       =  %ld\n",   diff_input.naxes[0]);
      printf("diff_input.naxes[1]       =  %ld\n",   diff_input.naxes[1]);
      printf("diff_input.crpix1         =  %-g\n",   diff_input.crpix1);
      printf("diff_input.crpix2         =  %-g\n",   diff_input.crpix2);
      printf("imin2                =  %d\n",    imin2);
      printf("imax2                =  %d\n",    imax2);
      printf("jmin2                =  %d\n",    jmin2);
      printf("jmax2                =  %d\n\n",  jmax2);
      printf("\n");

      fflush(stdout);
   }

   if(imin2 > istart) istart = imin2;
   if(imax2 < iend  ) iend   = imax2;

   if(jmin2 > jstart) jstart = jmin2;
   if(jmax2 < jend  ) jend   = jmax2;

   if(istart < 0) istart = 0;
   if(jstart < 0) jstart = 0;

   if(iend > diff_output.naxes[0]-1) iend = diff_output.naxes[0]-1;
   if(jend > diff_output.naxes[1]-1) jend = diff_output.naxes[1]-1;

   ilength = iend - istart;
   jlength = jend - jstart;

   if(mDiff_debug >= 1)
   {
      printf("\nComposite:\n");
      printf("istart               =  %d\n",    istart);
      printf("iend                 =  %d\n",    iend);
      printf("jstart               =  %d\n",    jstart);
      printf("jend                 =  %d\n",    jend);
      printf("\n");
      printf("ilength              =  %d\n",    ilength);
      printf("jlength              =  %d\n",    jlength);
      printf("\n");

      fflush(stdout);
   }


   /* Parts of each image to read (image coordinates) */

   imax1 = iend   - imin1;
   imin1 = istart - imin1;

   jmax1 = jend   - jmin1;
   jmin1 = jstart - jmin1;

   imax2 = iend   - imin2;
   imin2 = istart - imin2;

   jmax2 = jend   - jmin2;
   jmin2 = jstart - jmin2;

   if(fits_close_file(diff_input.fptr, &status))
   {
      mDiff_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(!diff_noAreas)
   {
      if(fits_close_file(diff_input_area.fptr, &status))
      {
         mDiff_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }


   /*********************/
   /* Check for overlap */
   /*********************/

   if(ilength <= 0 || jlength <= 0)
   {
      mDiff_printError("Images don't overlap");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /***********************************************/ 
   /* Allocate memory for the output image pixels */ 
   /***********************************************/ 

   data = (double **)malloc(jlength * sizeof(double *));

   for(j=0; j<jlength; ++j)
      data[j] = (double *)malloc(ilength * sizeof(double));

   if(mDiff_debug >= 1)
   {
      printf("%lu bytes allocated for image pixels\n", 
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
         data[j][i] = nan;
      }
   }


   /**********************************************/ 
   /* Allocate memory for the output pixel areas */ 
   /**********************************************/ 

   area = (double **)malloc(jlength * sizeof(double *));

   for(j=0; j<jlength; ++j)
      area[j] = (double *)malloc(ilength * sizeof(double));

   if(mDiff_debug >= 1)
   {
      printf("%lu bytes allocated for pixel areas\n", 
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


   /***************************/
   /* For the two input files */
   /***************************/

   time(&currtime);
   start = currtime;

   avearea1 = 0.;
   avearea2 = 0.;
   narea1   = 0.;
   narea2   = 0.;

   min_pixel = nan;
   max_pixel = nan;
   min_diff  = nan;
   max_diff  = nan;


   /******************************/
   /* Read the first input image */
   /******************************/

   ifile = 0;

   if(mDiff_readFits(infile[ifile], inarea[ifile]) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /**********************************************************/
   /* Create the output array by processing the input pixels */
   /**********************************************************/

   buffer  = (double *)malloc(ilength * sizeof(double));
   abuffer = (double *)malloc(ilength * sizeof(double));

   for(i=0; i<ilength; ++i)
   {
      buffer[i]  = nan;
      abuffer[i] = 0.;
   }

   fpixel[0] = imin1+1;
   fpixel[1] = jmin1+1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   nelements = ilength;

   status = 0;


   /*****************************/
   /* Loop over the input lines */
   /*****************************/

   for (j=jmin1; j<jmax1; ++j)
   {
      if(mDiff_debug >= 2)
      {
         printf("Processing file %d [%s] / input row %5d\n", ifile, infile[ifile], j);
         fflush(stdout);
      }


      /***********************************/
      /* Read a line from the input file */
      /***********************************/

      if(fits_read_pix(diff_input.fptr, TDOUBLE, fpixel, nelements, &nan,
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

         mDiff_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
      
      
      if(diff_noAreas)
      {
         for(i=0; i<ilength; ++i)
            abuffer[i] = 1.;
      }
      else
      {
         if(fits_read_pix(diff_input_area.fptr, TDOUBLE, fpixel, nelements, &nan,
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

            mDiff_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }
      
      ++fpixel[1];


      /************************/
      /* For each input pixel */
      /************************/

      for (i=imin1; i<imax1; ++i)
      {
         pixel_value = buffer[i-imin1] * abuffer[i-imin1];

         if(mNaN(min_pixel)) min_pixel = pixel_value;
         if(mNaN(max_pixel)) max_pixel = pixel_value;

         if(abuffer[i-imin1] > 0. && pixel_value < min_pixel)
            min_pixel = pixel_value;

         if(abuffer[i-imin1] > 0. && pixel_value > max_pixel)
            max_pixel = pixel_value;

         if(mDiff_debug >= 4)
         {
            printf("input: line %5d / pixel %5d, value = %10.2e (%10.2e) [array: %5d %5d]\n",
               j, i, buffer[i-imin1], abuffer[i-imin1], j-jmin1, i-imin1);
            fflush(stdout);
         }

         if(mDiff_debug >= 3)
         {
            printf("keep: line %5d / pixel %5d, value = %10.2e (%10.2e) [array: %5d %5d]\n",
               j, i, buffer[i-imin1], abuffer[i-imin1], j-jmin1, i-imin1);
            fflush(stdout);
         }

         if(mNaN(buffer[i-imin1]) || abuffer[i-imin1] <= 0.)
         {
            if(mDiff_debug >= 5)
            {
               printf("First file. Setting data to NaN and area to zero.\n");
               fflush(stdout);
            }

            data[j-jmin1][i-imin1] = nan;
            area[j-jmin1][i-imin1] = 0.;

            if(mDiff_debug >= 5)
            {
               printf("done.\n");
               fflush(stdout);
            }

            continue;
         }
         else
         {
            if(mDiff_debug >= 5)
            {
               printf("First file. Setting data to pixel value.\n");
               fflush(stdout);
            }

            data[j-jmin1][i-imin1] = pixel_value;
            area[j-jmin1][i-imin1] = abuffer[i-imin1];

            ++narea1;
            avearea1 += abuffer[i-imin1];

            if(mDiff_debug >= 5)
            {
               printf("done.\n");
               fflush(stdout);
            }

         }
      }
   }

   if(fits_close_file(diff_input.fptr, &status))
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

      mDiff_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(!diff_noAreas)
   {
      if(fits_close_file(diff_input_area.fptr, &status))
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

         mDiff_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }



   /*******************************/
   /* Read the second input image */
   /*******************************/

   ifile = 1;

   if(mDiff_readFits(infile[ifile], inarea[ifile]) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /**********************************************************/
   /* Create the output array by processing the input pixels */
   /**********************************************************/

   fpixel[0] = imin2+1;
   fpixel[1] = jmin2+1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   nelements = ilength;

   status = 0;


   /*****************************/
   /* Loop over the input lines */
   /*****************************/

   for (j=jmin2; j<jmax2; ++j)
   {
      if(mDiff_debug >= 2)
      {
         printf("Processing file %d [%s] / input row %5d\n", ifile, infile[ifile], j);
         fflush(stdout);
      }


      /***********************************/
      /* Read a line from the input file */
      /***********************************/

      if(fits_read_pix(diff_input.fptr, TDOUBLE, fpixel, nelements, &nan,
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

         mDiff_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
      
      
      if(diff_noAreas)
      {
         for(i=0; i<ilength; ++i)
            abuffer[i] = 1.;
      }
      else
      {
         if(fits_read_pix(diff_input_area.fptr, TDOUBLE, fpixel, nelements, &nan,
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

            mDiff_printFitsError(status);
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }
      
      ++fpixel[1];


      /************************/
      /* For each input pixel */
      /************************/

      for (i=imin2; i<imax2; ++i)
      {
         pixel_value = buffer[i-imin2] * abuffer[i-imin2];

         if(mNaN(min_pixel)) min_pixel = pixel_value;
         if(mNaN(max_pixel)) max_pixel = pixel_value;

         if(abuffer[i-imin2] > 0. && pixel_value < min_pixel)
            min_pixel = pixel_value;

         if(abuffer[i-imin2] > 0. && pixel_value > max_pixel)
            max_pixel = pixel_value;

         if(mDiff_debug >= 4)
         {
            printf("input: line %5d / pixel %5d, value = %10.2e (%10.2e) [array: %5d %5d]\n",
               j, i, buffer[i-imin2], abuffer[i-imin2], j-jmin2, i-imin2);
            fflush(stdout);
         }

         if(mDiff_debug >= 3)
         {
            printf("keep: line %5d / pixel %5d, value = %10.2e (%10.2e) [array: %5d %5d]\n",
               j, i, buffer[i-imin2], abuffer[i-imin2], j-jmin2, i-imin2);
            fflush(stdout);
         }

        if(mNaN(buffer[i-imin2])
         || abuffer[i-imin2] <= 0.
         || data[j-jmin2][i-imin2] == nan
         || area[j-jmin2][i-imin2] == 0.)
         {
            if(mDiff_debug >= 5)
            {
               printf("Second file. One or the other value is NaN (or zero area).\n");
               fflush(stdout);
            }

            data[j-jmin2][i-imin2] = nan;
            area[j-jmin2][i-imin2] = 0.;

            continue;
         }
         else
         {
            if(mDiff_debug >= 5)
            {
               printf("Second file. Subtracting pixel value.\n");
               fflush(stdout);
            }

            data[j-jmin2][i-imin2] -= factor*pixel_value;
            area[j-jmin2][i-imin2] += abuffer[i-imin2];

            ++narea2;
            avearea2 += abuffer[i-imin2];

            if(mDiff_debug >= 5)
            {
               printf("done.\n");
               fflush(stdout);
            }
         }
      }
   }

   if(fits_close_file(diff_input.fptr, &status))
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

      mDiff_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(!diff_noAreas)
   {
      if(fits_close_file(diff_input_area.fptr, &status))
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

         mDiff_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }

   free(buffer);
   free(abuffer);

   if(mDiff_debug >= 1)
   {
      time(&currtime);
      printf("\nDone reading data (%.0f seconds)\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /************************************/
   /* Anly keep those pixels that were */
   /* covered twice reasonably fully   */
   /************************************/

   avearea1 = avearea1/narea1;
   avearea2 = avearea2/narea2;

   areamax = avearea1 + avearea2;

   if(mDiff_debug >= 2)
   {
      printf("\npixel areas: %-g + %-g = %-g\n\n", avearea1, avearea2, areamax);
      fflush(stdout);
   }

   for (j=0; j<jlength; ++j)
   {
      for (i=0; i<ilength; ++i)
      {
         if(mNaN(area[j][i]) || area[j][i] == 0.)
         {
            data[j][i] = 0.;
            area[j][i] = 0.;
         }
         else
         {
            if(fabs(area[j][i] - areamax)/areamax > 0.333)
            {
               data[j][i] = 0.;
               area[j][i] = 0.;
            }
         }
      }
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

   for (j=0; j<jlength; ++j)
   {
      for (i=0; i<ilength; ++i)
      {
         if(area[j][i] > 0.)
         {
            data[j][i] = 2. * data[j][i] / area[j][i];

            if(!haveMinMax)
            {
               datamin = data[j][i];
               datamax = data[j][i];
               areamin = area[j][i];
               areamax = area[j][i];

               min_diff = data[j][i];
               max_diff = data[j][i];

               haveMinMax = 1;
            }

            if(data[j][i] < datamin) datamin = data[j][i];
            if(data[j][i] > datamax) datamax = data[j][i];
            if(area[j][i] < areamin) areamin = area[j][i];
            if(area[j][i] > areamax) areamax = area[j][i];

            if(fabs(data[j][i]) < min_diff) min_diff = fabs(data[j][i]);
            if(fabs(data[j][i]) > max_diff) max_diff = fabs(data[j][i]);

            if(j < jmin) jmin = j;
            if(j > jmax) jmax = j;
            if(i < imin) imin = i;
            if(i > imax) imax = i;
         }
         else
         {
            data[j][i] = nan;
            area[j][i] = 0.;
         }
      }
   }

   imin += istart;
   jmin += jstart;

   imax += istart;
   jmax += jstart;

   if(mDiff_debug >= 1)
   {
      printf("Data min = %-g\n", datamin);
      printf("Data max = %-g\n", datamax);
      printf("Area min = %-g\n", areamin);
      printf("Area max = %-g\n\n", areamax);
      printf("j min    = %d\n", jmin);
      printf("j max    = %d\n", jmax);
      printf("i min    = %d\n", imin);
      printf("i max    = %d\n", imax);
   }

   if(jmin > jmax || imin > imax)
   {
      mDiff_printError("All pixels are blank.");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /********************************/
   /* Create the output FITS files */
   /********************************/

   remove(output_file);               
   remove(output_area_file);               

   if(fits_create_file(&diff_output.fptr, output_file, &status)) 
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_create_file(&diff_output_area.fptr, output_area_file, &status)) 
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*********************************************************/
   /* Create the FITS image.  All the required keywords are */
   /* handled automatically.                                */
   /*********************************************************/

   if (fits_create_img(diff_output.fptr, bitpix, naxis, diff_output.naxes, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);          
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mDiff_debug >= 1)
   {
      printf("\nFITS data image created (not yet populated)\n"); 
      fflush(stdout);
   }

   if (fits_create_img(diff_output_area.fptr, bitpix, naxis, diff_output_area.naxes, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);          
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mDiff_debug >= 1)
   {
      printf("FITS area image created (not yet populated)\n"); 
      fflush(stdout);
   }


   /****************************************/
   /* Set FITS header from a template file */
   /****************************************/

   if(fits_write_key_template(diff_output.fptr, template_file, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mDiff_debug >= 1)
   {
      printf("Template keywords written to FITS data image\n"); 
      fflush(stdout);
   }

   if(fits_write_key_template(diff_output_area.fptr, template_file, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mDiff_debug >= 1)
   {
      printf("Template keywords written to FITS area image\n\n"); 
      fflush(stdout);
   }


   /***************************/
   /* Modify BITPIX to be -64 */
   /***************************/

   if(fits_update_key_lng(diff_output.fptr, "BITPIX", -64,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(diff_output_area.fptr, "BITPIX", -64,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /***************************************************/
   /* Update NAXIS, NAXIS1, NAXIS2, CRPIX1 and CRPIX2 */
   /***************************************************/

   if(fits_update_key_lng(diff_output.fptr, "NAXIS", 2,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(diff_output.fptr, "NAXIS1", imax-imin+1,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(diff_output.fptr, "NAXIS2", jmax-jmin+1,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(diff_output.fptr, "CRPIX1", diff_output.crpix1-imin, -14,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(diff_output.fptr, "CRPIX2", diff_output.crpix2-jmin, -14,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(diff_output_area.fptr, "NAXIS", 2,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(diff_output_area.fptr, "NAXIS1", imax-imin+1,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_lng(diff_output_area.fptr, "NAXIS2", jmax-jmin+1,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(diff_output_area.fptr, "CRPIX1", diff_output.crpix1-imin, -14,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_update_key_dbl(diff_output_area.fptr, "CRPIX2", diff_output.crpix2-jmin, -14,
                                  (char *)NULL, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mDiff_debug >= 1)
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

   for(j=jmin; j<=jmax; ++j)
   {
      if (fits_write_pix(diff_output.fptr, TDOUBLE, fpixel, nelements, 
                         (void *)(&data[j-jstart][imin-istart]), &status))
      {
         for(i=0; i<jlength; ++i)
         {
            free(data[i]);
            free(area[i]);
         }

         free(data);
         free(area);

         mDiff_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      ++fpixel[1];
   }

   if(mDiff_debug >= 1)
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
      if (fits_write_pix(diff_output_area.fptr, TDOUBLE, fpixel, nelements,
                         (void *)(&area[j-jstart][imin-istart]), &status))
      {
         for(i=0; i<jlength; ++i)
         {
            free(data[i]);
            free(area[i]);
         }

         free(data);
         free(area);

         mDiff_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      ++fpixel[1];
   }

   if(mDiff_debug >= 1)
   {
      printf("Data written to FITS area image\n\n"); 
      fflush(stdout);
   }


   /***********************/
   /* Close the FITS file */
   /***********************/

   if(fits_close_file(diff_output.fptr, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mDiff_debug >= 1)
   {
      printf("FITS data image finalized\n"); 
      fflush(stdout);
   }

   if(fits_close_file(diff_output_area.fptr, &status))
   {
      for(j=0; j<jlength; ++j)
      {
         free(data[j]);
         free(area[j]);
      }

      free(data);
      free(area);

      mDiff_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mDiff_debug >= 1)
   {
      printf("FITS area image finalized\n\n"); 
      fflush(stdout);
   }

   time(&currtime);

   for(j=0; j<jlength; ++j)
   {
      free(data[j]);
      free(area[j]);
   }

   free(data);
   free(area);

   sprintf(montage_msgstr, "time=%.1f, min_pixel=%-g, max_pixel=%-g, min_diff=%-g, max_diff=%-g", 
      (double)(currtime - start), min_pixel, max_pixel, min_diff, max_diff);

   sprintf(montage_json, "{\"time\":%.1f, \"min_pixel\":\"%-g\", \"max_pixel\":\"%-g\", \"min_diff\":\"%-g\", \"max_diff\":\"%-g\"}", 
      (double)(currtime - start), min_pixel, max_pixel, min_diff, max_diff);

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   returnStruct->time      = (double)(currtime - start);
   returnStruct->min_pixel = min_pixel;
   returnStruct->max_pixel = max_pixel;
   returnStruct->min_diff  = min_diff;
   returnStruct->max_diff  = max_diff;

   return returnStruct;
}


/**************************************************/
/*                                                */
/*  Read the output header template file.         */
/*  Specifically extract the image size info.     */
/*                                                */
/**************************************************/

int mDiff_readTemplate(char *filename)
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
      mDiff_printError("Template file not found.");
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

      if(mDiff_debug >= 3)
      {
         printf("Template line: [%s]\n", line);
         fflush(stdout);
      }

      for(i=strlen(line); i<80; ++i)
         line[i] = ' ';
      
      line[80] = '\0';

      mDiff_parseLine(line);
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

int mDiff_parseLine(char *line)
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

   if(mDiff_debug >= 2)
   {
      printf("keyword [%s] = value [%s]\n", keyword, value);
      fflush(stdout);
   }

   if(strcmp(keyword, "NAXIS1") == 0)
   {
      diff_output.naxes[0] = atoi(value);
      diff_output_area.naxes[0] = atoi(value);
   }

   if(strcmp(keyword, "NAXIS2") == 0)
   {
      diff_output.naxes[1] = atoi(value);
      diff_output_area.naxes[1] = atoi(value);
   }

   if(strcmp(keyword, "CRPIX1") == 0)
   {
      diff_output.crpix1 = atof(value);
      diff_output_area.crpix1 = atof(value);
   }

   if(strcmp(keyword, "CRPIX2") == 0)
   {
      diff_output.crpix2 = atof(value);
      diff_output_area.crpix2 = atof(value);
   }

   return 0;
}


/*******************************************/
/*                                         */
/*  Open a FITS file pair and extract the  */
/*  pertinent header information.          */
/*                                         */
/*******************************************/

int mDiff_readFits(char *fluxfile, char *areafile)
{
   int    status, nfound, bitpix;
   long   naxes[2];
   double crpix[2];
   char   errstr[MAXSTR];
   char  *header;

   status = 0;

   if(!diff_noAreas)
   {
      if(fits_open_file(&diff_input_area.fptr, areafile, READONLY, &status))
      {
         sprintf(errstr, "Area file %s missing or invalid FITS", areafile);
         mDiff_printError(errstr);
         return 1;
      }
   }

   if(fits_open_file(&diff_input.fptr, fluxfile, READONLY, &status))
   {
      sprintf(errstr, "Image file %s missing or invalid FITS", fluxfile);
      mDiff_printError(errstr);
      return 1;
   }

   if(fits_get_img_type(diff_input.fptr, &bitpix, &status))
   {
      mDiff_printFitsError(status);
      return 1;
   }

   diff_input.bitpix = bitpix;

   if(fits_read_keys_lng(diff_input.fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
   {
      mDiff_printFitsError(status);
      return 1;
   }
   
   diff_input.naxes[0] = naxes[0];
   diff_input.naxes[1] = naxes[1];

   diff_input_area.naxes[0] = naxes[0];
   diff_input_area.naxes[1] = naxes[1];

   if(fits_read_keys_dbl(diff_input.fptr, "CRPIX", 1, 2, crpix, &nfound, &status))
   {
      mDiff_printFitsError(status);
      return 1;
   }

   diff_input.crpix1 = crpix[0];
   diff_input.crpix2 = crpix[1];

   diff_input_area.crpix1 = crpix[0];
   diff_input_area.crpix2 = crpix[1];

   if(fits_get_image_wcs_keys(diff_input.fptr, &header, &status))
   {
      mDiff_printFitsError(status);
      return 1;
   }

   diff_input.hdrsize = (int)strlen(header);

   free(header);
   
   return 0;
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mDiff_printFitsError(int status)
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

void mDiff_printError(char *msg)
{
   strcpy(montage_msgstr, msg);
}
