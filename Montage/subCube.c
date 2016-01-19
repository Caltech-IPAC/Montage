/* Module: subCube.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.1      John Good        08Sep15  fits_read_pix() incorrect null value
1.0      John Good        15May15  Baseline code, based on subImage.c of that date.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "fitsio.h"
#include "wcs.h"
#include "coord.h"

#include "montage.h"
#include "subCube.h"
#include "mNaN.h"

extern int debug;

int isflat;

char content[128];

extern FILE *fstatus;
        

struct WorldCoor *montage_getFileInfo(fitsfile *infptr, char *header[], struct imageParams *params)
{
   struct WorldCoor *wcs;
   int status = 0;
   int i;

   if(fits_get_image_wcs_keys(infptr, header, &status))
      montage_printFitsError(status);

   if(fits_read_key_lng(infptr, "NAXIS", &params->naxis, (char *)NULL, &status))
      montage_printFitsError(status);
   
   if(fits_read_keys_lng(infptr, "NAXIS", 1, params->naxis, params->naxesin, &params->nfound, &status))
      montage_printFitsError(status);
   
   params->naxes[0] = params->naxesin[0];
   params->naxes[1] = params->naxesin[1];

   if(params->naxis < 3)
      params->naxes[2] = 1;

   else if(params->naxes[2] == 0)
   {
      params->naxes[2] = params->naxesin[2];
      
      params->kbegin = 1;
      params->kend   = params->naxes[2];
   }

   else if(params->kend > params->naxesin[2])
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Some select list values for axis 3 are greater than NAXIS3.\"]\n");
      fflush(fstatus);
      exit(1);
   }

   if(params->naxis < 4)
      params->naxes[3] = 1;

   else if(params->naxes[3] == 0)
   {
      params->naxes[3] = params->naxesin[3];
      
      params->lbegin = 1;
      params->lend   = params->naxes[3];
   }

   else if(params->lend > params->naxesin[3])
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Some select list values for axis 4 are greater than NAXIS4.\"]\n");
      fflush(fstatus);
      exit(1);
   }

   if(debug)
   {
      printf("subCube> naxis1 = %ld\n",  params->naxes[0], params->ibegin, params->iend);
      printf("subCube> naxis2 = %ld\n",  params->naxes[1], params->jbegin, params->jend);

      if(params->naxis > 2)
      printf("subCube> naxis3 = %ld\n",  params->naxes[2], params->kbegin, params->kend);

      if(params->naxis > 3)
      printf("subCube> naxis4 = %ld\n",  params->naxes[3], params->lbegin, params->lend);

      fflush(stdout);
   }


   /****************************************/
   /* Initialize the WCS transform library */
   /* and find the pixel location of the   */
   /* sky coordinate specified             */
   /****************************************/

   wcs = wcsinit(header[0]);

   params->isDSS = 0;
   if(wcs->prjcode == WCS_DSS)
      params->isDSS = 1;

   if(wcs == (struct WorldCoor *)NULL)
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Output wcsinit() failed.\"]\n");
      fflush(stdout);
      exit(1);
   }

   /* Extract the CRPIX and (equivalent) CDELT values */
   /* from the WCS structure                          */

   params->crpix[0] = wcs->xrefpix;
   params->crpix[1] = wcs->yrefpix;

   fits_read_key(infptr, TDOUBLE, "CRPIX3", &(params->crpix[2]), (char *)NULL, &status);
   status = 0;

   fits_read_key(infptr, TDOUBLE, "CRPIX4", &(params->crpix[3]), (char *)NULL, &status);
   status = 0;

   if(params->isDSS)
   {
      params->cnpix[0] = wcs->x_pixel_offset;
      params->cnpix[1] = wcs->y_pixel_offset;
   }
   return wcs;
}


int montage_copyHeaderInfo(fitsfile *infptr, fitsfile *outfptr, struct imageParams *params)
{
   double tmp, tmp3, tmp4;
   int status = 0;
   
   if(fits_copy_header(infptr, outfptr, &status))
      montage_printFitsError(status);


   /**********************/
   /* Update header info */
   /**********************/

   if(fits_update_key_lng(outfptr, "BITPIX", -64,
                                  (char *)NULL, &status))
      montage_printFitsError(status);

   if(fits_update_key_lng(outfptr, "NAXIS", params->naxis,
                                  (char *)NULL, &status))
      montage_printFitsError(status);

   if(fits_update_key_lng(outfptr, "NAXIS1", params->nelements,
                                  (char *)NULL, &status))
      montage_printFitsError(status);

   if(fits_update_key_lng(outfptr, "NAXIS2", params->naxes[1],
                                  (char *)NULL, &status))
      montage_printFitsError(status);

   if(params->isDSS)
   {
      tmp = params->cnpix[0] + params->ibegin - 1;

      if(fits_update_key_dbl(outfptr, "CNPIX1", tmp, -14,
                                     (char *)NULL, &status))
         montage_printFitsError(status);

      tmp = params->cnpix[1] + params->jbegin - 1;

      if(fits_update_key_dbl(outfptr, "CNPIX2", tmp, -14,
                                     (char *)NULL, &status))
         montage_printFitsError(status);
   }
   else
   {
      tmp = params->crpix[0] - params->ibegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX1", tmp, -14,
                                     (char *)NULL, &status))
         montage_printFitsError(status);

      tmp = params->crpix[1] - params->jbegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX2", tmp, -14,
                                     (char *)NULL, &status))
         montage_printFitsError(status);
   }

   if(params->naxis > 2)
   {
      if(fits_update_key_lng(outfptr, "NAXIS3", params->naxes[2],
                                     (char *)NULL, &status))
         montage_printFitsError(status);

      tmp3 = params->crpix[2] - params->kbegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX3", tmp3, -14,
                                     (char *)NULL, &status))
         montage_printFitsError(status);
   }

   if(params->naxis > 3)
   {
      if(fits_update_key_lng(outfptr, "NAXIS4", params->naxes[3],
                                     (char *)NULL, &status))
         montage_printFitsError(status);

      tmp4 = params->crpix[3] - params->lbegin + 1;

      if(fits_update_key_dbl(outfptr, "CRPIX4", tmp4, -14,
                                     (char *)NULL, &status))
         montage_printFitsError(status);
   }

   if(debug)
   {
      printf("subCube> naxis1 -> %ld\n", params->nelements);
      printf("subCube> naxis2 -> %d\n",  params->naxes[1]);

      if(params->naxis > 2)
      {
         printf("subCube> naxis3 -> %d\n",  params->naxes[2]);
         printf("subCube> crpix3 -> %-g\n",  tmp3);
      }

      if(params->naxis > 3)
      {
         printf("subCube> naxis4 -> %d\n",  params->naxes[3]);
         printf("subCube> crpix4 -> %-g\n",  tmp4);
      }


      if(params->isDSS)
      {
         printf("subCube> cnpix1 -> %-g\n", params->cnpix[0]+params->ibegin-1);
         printf("subCube> cnpix2 -> %-g\n", params->cnpix[1]+params->jbegin-1);
      }
      else
      {
         printf("subCube> crpix1 -> %-g\n", params->crpix[0]-params->ibegin+1);
         printf("subCube> crpix2 -> %-g\n", params->crpix[1]-params->jbegin+1);
      }

      fflush(stdout);
   }

   return 0;
}


void montage_copyData(fitsfile *infptr, fitsfile *outfptr, struct imageParams *params)
{
   long    fpixel[4], fpixelo[4];
   int     i, j, nullcnt;
   int     j3, j4, inRange;
   int     status = 0;
   double *buffer, refval;


   /*************************************************/
   /* Make a NaN value to use checking blank pixels */
   /*************************************************/

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


   fpixel[1] = params->jbegin;
   fpixel[2] = params->kbegin;

   buffer  = (double *)malloc(params->nelements * sizeof(double));

   fpixelo[1] = 1;

   isflat = 1;

   refval = nan;

   fpixel [0] = params->ibegin;  // Fixed
   fpixelo[0] = 1;               // Fixed

   fpixelo[3] = 1;

   for (j4=params->lbegin; j4<=params->lend; ++j4)
   {
      fpixel[3] = j4;

      // If the dimension 4 value isn't in our range list,
      // we'll skip this one.

      if(params->nrange[1] > 0)
      {
         inRange = 0;

         for(i=0; i<params->nrange[1]; ++i)
         {
            if(params->range[1][i][1] == -1)
            {
               if(j4 == params->range[1][i][0])
               {
                  inRange = 1;
                  break;
               }
            }

            else
            {
               if(j4 >= params->range[1][i][0]
               && j4 <= params->range[1][i][1])
               {
                  inRange = 1;
                  break;
               }
            }
         }

         if(!inRange)
            continue;
      }


      // We want this dimension 4 value

      fpixelo[2] = 1;

      for (j3=params->kbegin; j3<=params->kend; ++j3)
      {
         fpixel[2] = j3;

         // If the dimension 4 value isn't in our range list,
         // we'll skip this one.

         if(params->nrange[0] > 0)
         {
            inRange = 0;

            for(i=0; i<params->nrange[0]; ++i)
            {
               if(params->range[0][i][1] == -1)
               {
                  if(j3 == params->range[0][i][0])
                  {
                     inRange = 1;
                     break;
                  }
               }

               else
               {
                  if(j3 >= params->range[0][i][0]
                  && j3 <= params->range[0][i][1])
                  {
                     inRange = 1;
                     break;
                  }
               }
            }

            if(!inRange)
               continue;
         }

         if(debug)
         {
            printf("copyData> Processing input 4/3  %5d %5d",    fpixel[3],  fpixel[2]);
            printf(                     " to output %5d %5d\n", fpixelo[3], fpixelo[2]);
            fflush(stdout);
         }


         // We want this dimension 4 value

         fpixelo[1] = 1;

         for (j=params->jbegin; j<=params->jend; ++j)
         {
            fpixel[1] = j;

            if(debug)
            {
               printf("copyData> Processing input %5d/%5d, row %5d",             fpixel[3],  fpixel[2],  fpixel[1]);
               printf(                " to output %5d/%5d, row %5d: read ... ", fpixelo[3], fpixelo[2], fpixelo[1]);
               fflush(stdout);
            }

            if(fits_read_pix(infptr, TDOUBLE, fpixel, params->nelements, &nan,
                             buffer, &nullcnt, &status))
               montage_printFitsError(status);

            for(i=0; i<params->nelements; ++i)
            {
               if(!mNaN(buffer[i]))
               {
                  if(mNaN(refval))
                     refval = buffer[i];

                  if(buffer[i] != refval)
                     isflat = 0;
               }
            }

            if(debug)
            {
               printf("write.\n");
               fflush(stdout);
            }

            if (fits_write_pix(outfptr, TDOUBLE, fpixelo, params->nelements,
                               (void *)buffer, &status))
               montage_printFitsError(status);

            ++fpixelo[1];
         }

         ++fpixelo[2];
      }

      ++fpixelo[3];
   }


   free(buffer);

   if(isflat)
   {
      if(mNaN(refval))
         strcpy(content, "blank");
      else
         strcpy(content, "flat");
   }
   else
      strcpy(content, "normal");
}


void montage_dataRange(fitsfile *infptr, int *imin, int *imax, int *jmin, int *jmax)
{
   long    fpixel[4];
   long    naxis, naxes[10];
   int     i, j, nullcnt, nfound;
   int     j4, j3;
   double *buffer;

   int     status = 0;

   /*************************************************/
   /* Make a NaN value to use checking blank pixels */
   /*************************************************/

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

   if(fits_read_key_lng(infptr, "NAXIS", &naxis, (char *)NULL, &status))
      montage_printFitsError(status);
   
   if(fits_read_keys_lng(infptr, "NAXIS", 1, naxis, naxes, &nfound, &status))
      montage_printFitsError(status);

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   *imin =  1000000000;
   *imax = -1;
   *jmin =  1000000000;
   *jmax = -1;

   buffer  = (double *)malloc(naxes[0] * sizeof(double));

   for (j4=1; j4<=naxes[3]; ++j4)
   {
      for (j3=1; j3<=naxes[2]; ++j3)
      {
         for (j=1; j<=naxes[1]; ++j)
         {
            if(debug)
            {
               printf("dataRange> input plane %5d/%5d, row %5d: \n", j4, j3, j);
               fflush(stdout);
            }

            if(fits_read_pix(infptr, TDOUBLE, fpixel, naxes[0], &nan,
                             buffer, &nullcnt, &status))
               montage_printFitsError(status);

            for(i=1; i<=naxes[0]; ++i)
            {
               if(debug && i<11)
                  printf(" %-g", buffer[i-1]);

               if(!mNaN(buffer[i-1]))
               {
                  if(buffer[i-1] != nan)
                  {
                     if(i < *imin) *imin = i;
                     if(i > *imax) *imax = i;
                     if(j < *jmin) *jmin = j;
                     if(j > *jmax) *jmax = j;
                  }
               }
            }

            if(debug)
               printf("\n");

            ++fpixel[1];
         }

         ++fpixel[2];
      }

      ++fpixel[3];
   }

   free(buffer);
}


/*****************************/
/*                           */
/*  Parse D3/D4 select lists */
/*                           */
/*****************************/

int montage_parseSelectList(int ind, struct imageParams *params)
{
   char *begin, *end, *split, *endstr, *ptr;

   char  list[STRLEN];
   int   index, nrange, min, max;

   nrange = 0;

   index = ind - 3;

   if(index < 0 || index > 1)
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Select list index can only be 3 or 4.");
      fflush(fstatus);
      exit(1);
   }

   strcpy(list, params->dConstraint[index]);

   endstr = list + strlen(list);

   begin = list;

   while(1)
   {
      min =  0;
      max = -1;

      while(*begin == ' ' && begin < endstr)
         ++begin;

      if(begin >= endstr)
         break;

      end = begin;

      while(*end != ',' && end < endstr)
         ++end;

      *end = '\0';

      split = begin;

      while(*split != ':' && split < end)
         ++split;

      if(*split == ':')
      {
         *split = '\0';
         ++split;
      }

      ptr = begin + strlen(begin) - 1;

      while(*ptr == ' ' && ptr >= begin) 
         *ptr = '\0';

      while(*split == ' ' && split >= end) 
         *split = '\0';

      ptr = split + strlen(split) - 1;

      while(*ptr == ' ' && ptr >= split) 
         *ptr = '\0';

      min = strtol(begin, &ptr, 10);

      if(ptr < begin + strlen(begin))
      {
         fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Invalid range string [%s].\"]\n", begin);
         fflush(fstatus);
         exit(1);
      }

      if(split < end)
      {
         max = strtol(split, &ptr, 10);

         if(ptr < split + strlen(split))
         {
            fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Invalid range string [%s].\"]\n", split);
            fflush(fstatus);
            exit(1);
         }
      }

      if(max != -1 && min > max)
      {
         fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Range max less than min.\"]\n");
         fflush(fstatus);
         exit(1);
      }

      if(min < 1)
      {
         fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"FITS index ranges cannot be less than one.\"]\n");
         fflush(fstatus);
         exit(1);
      }

      params->range[index][nrange][0] = min;
      params->range[index][nrange][1] = max;

      ++nrange;

      begin = end;

      ++begin;

      if(begin >= endstr)
         break;
   }

   params->nrange[index] = nrange;
}


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void montage_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   fprintf(fstatus, "[struct stat=\"ERROR\", flag=%d, msg=\"%s\"]\n", status, status_str);

   exit(1);
}
