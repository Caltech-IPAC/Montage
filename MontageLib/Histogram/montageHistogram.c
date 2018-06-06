/* Module: mViewer.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.1      John Good        08Sep15  fits_read_pix() incorrect null value
1.0      John Good        20Jun15  Baseline code (essentially extracted from mViewer)

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <errno.h>

#include <fitsio.h>

#include <mHistogram.h>
#include <montage.h>

#define NBIN 200000


static int     mHistogram_debug;
static int     hdu;
static int     grayPlaneCount;

static int     grayPlanes[256];

static int     naxis1, naxis2;

static int     nbin;

static int     hist    [NBIN];
static double  chist   [NBIN];
static double  datalev [NBIN];
static double  gausslev[NBIN];

static unsigned long npix;

static double  delta, rmin, rmax;

static FILE   *fout;
 

static char montage_msgstr[1024];


/*-*********************************************************************************************/
/*                                                                                             */
/*  mHistogram                                                                                 */
/*                                                                                             */
/*  This program is essentially a subset of mViewer containing the code that determines        */
/*  the stretch for an image.  With it we can generate stretch information for a whole         */
/*  file (or one reference file), then use it when we create a set of JPEG/PNG files           */
/*  from a set of other subsets/files.                                                         */
/*                                                                                             */
/*  Only one file is processed, so if we want color we need to run this program three          */
/*  times.  The only arguments needed are the image / stretch min / stretch max / mode.        */
/*                                                                                             */
/*   char *imgFile       Input FITS file.                                                      */
/*   char *histFile      Output histogram file.                                                */
/*   char *minString     Data range minimum as a string (to allow '%' and 's' suffix.          */
/*   char *maxString     Data range maximum as a string (to allow '%' and 's' suffix.          */
/*   char *stretchType   Stretch type (linear, power(log**N), sinh, gaussian or gaussian-log)  */
/*                                                                                             */
/*   int   grayLogPower  If the stretch type is log to a power, the power value.               */
/*   char *betaString    If the stretch type is asinh, the transition data value.              */
/*                                                                                             */
/*   int   debug         Debugging output level.                                               */
/*                                                                                             */
/***********************************************************************************************/

struct mHistogramReturn *mHistogram(char *grayfile, char *histfile, 
                                    char *grayminstr, char *graymaxstr, char *graytype, int graylogpower, char *graybetastr, int debugin)
{
   int       i, grayType;

   int       status = 0;

   double    median, sigma;

   double    grayminval      = 0.;
   double    graymaxval      = 0.;
   double    grayminpercent  = 0.;
   double    graymaxpercent  = 0.;
   double    grayminsigma    = 0.;
   double    graymaxsigma    = 0.;
   double    graybetaval     = 0.;

   double    graydataval[256];

   double    graydatamin;
   double    graydatamax;

   fitsfile *grayfptr;

   struct mHistogramReturn *returnStruct;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mHistogramReturn *)malloc(sizeof(struct mHistogramReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mHistogram_debug = debugin;

     if(strcmp(graytype, "linear") == 0) graylogpower = 0;
       
        if(strcmp(graytype, "linear")       == 0) grayType = POWER;
   else if(strcmp(graytype, "power")        == 0) grayType = POWER;
   else if(strcmp(graytype, "gaussian")     == 0) grayType = GAUSSIAN;
   else if(strcmp(graytype, "gaussian-log") == 0) grayType = GAUSSIANLOG;
   else if(strcmp(graytype, "gaussianlog")  == 0) grayType = GAUSSIANLOG;
   else if(strcmp(graytype, "asinh")        == 0) grayType = ASINH;

   if(strlen(grayfile) == 0)
   {
      strcpy(returnStruct->msg, "No input FITS file name given");
      return returnStruct;
   }


   if(fits_open_file(&grayfptr, grayfile, READONLY, &status))
   {
      sprintf(returnStruct->msg, "Image file %s invalid FITS", grayfile);
      return returnStruct;
   }

   grayPlaneCount = mHistogram_getPlanes(grayfile, grayPlanes);

   if(grayPlaneCount > 0)
      hdu = grayPlanes[0];
   else
      hdu = 0;

   if(hdu > 0)
   {
      if(fits_movabs_hdu(grayfptr, hdu+1, NULL, &status))
      {
         sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
         return returnStruct;
      }
   }

   if(strlen(histfile) == 0)
   {
      strcpy(returnStruct->msg, "No output histogram file name given.");
      return returnStruct;
   }


   fout = fopen(histfile, "w+");

   if(fout == (FILE *)NULL)
   {
      strcpy(returnStruct->msg, "Cannot open output histogram file.");
      return returnStruct;
   }

   /* Grayscale/pseudocolor mode */

   /* Get the image dimensions */

   if(strlen(grayfile) == 0)
   {
      strcpy(returnStruct->msg, "Grayscale/pseudocolor mode but no gray image given");
      return returnStruct;
   }

   status = 0;
   if(fits_read_key(grayfptr, TLONG, "NAXIS1", &naxis1, (char *)NULL, &status))
   {
      mHistogram_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   status = 0;
   if(fits_read_key(grayfptr, TLONG, "NAXIS2", &naxis2, (char *)NULL, &status))
   {
      mHistogram_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mHistogram_debug)
   {
      printf("naxis1   = %d\n", naxis1);
      printf("naxis2   = %d\n", naxis2);
      printf("\n");

      fflush(stdout);
   }


   /* Now adjust the data range if the limits */
   /* were percentiles.  We had to wait until */
   /* we had naxis1, naxis2 which is why this */
   /* is here                                 */

   if(mHistogram_debug)
      printf("\n GRAY RANGE:\n");

   status = mHistogram_getRange(grayfptr,     grayminstr,  graymaxstr, 
                               &grayminval, &graymaxval,  grayType, 
                               graybetastr, &graybetaval, graydataval,
                               naxis1,       naxis2,
                              &graydatamin, &graydatamax,
                              &median,       &sigma,
                               grayPlaneCount, grayPlanes);

   if(status)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   grayminpercent = mHistogram_valuePercentile(grayminval);
   graymaxpercent = mHistogram_valuePercentile(graymaxval);

   grayminsigma = (grayminval - median) / sigma;
   graymaxsigma = (graymaxval - median) / sigma;

   if(mHistogram_debug)
   {
      printf("DEBUG> grayminval = %-g (%-g%%/%-gs)\n", grayminval, grayminpercent, grayminsigma);
      printf("DEBUG> graymaxval = %-g (%-g%%/%-gs)\n", graymaxval, graymaxpercent, graymaxsigma);
      fflush(stdout);
   }


   /* Output histogram data to file */

   fprintf(fout, "# HISTOGRAM DATA\n");
   fprintf(fout, "# \n");
   fprintf(fout, "# The following data is strictly formatted (after these initial comments).\n");
   fprintf(fout, "# The first line is the type of stretch, i.e., power law (linear/log/etc.):0,\n");
   fprintf(fout, "# gaussian:1, gaussian-log:2 or asinh:3.\n");
   fprintf(fout, "# \n");
   fprintf(fout, "# The second are the data ranges the user gave in the various units (data value,\n");
   fprintf(fout, "# percentiles, 'sigma' levels) plus the file data min, max, median and 'sigma'.\n");
   fprintf(fout, "# \n");
   fprintf(fout, "# The third (a little repetitive) are the file statistics used in the histogram\n");
   fprintf(fout, "# calculation: data min, max, the width of the bins ((max-min)/NBIN) and the \n");
   fprintf(fout, "# total number of pixels in the file.\n");
   fprintf(fout, "# \n");
   fprintf(fout, "# Then the conclusions, starting with the 256 data values that correspond to \n");
   fprintf(fout, "# the lowest data value associated with a 'grayscale' output value.\n");
   fprintf(fout, "# \n");
   fprintf(fout, "# Finally, the NBIN histogram values.  The first column is the bin number.\n");
   fprintf(fout, "# The second is the lowest data value that will go into that bin.  This is\n");
   fprintf(fout, "# the part that is stretch type dependent.  Next is the count of pixels that\n");
   fprintf(fout, "# ended up in that bin followed by the cumulative count, and finally the \n");
   fprintf(fout, "# chi^2 and sigma levels for the bin.\n");
   fprintf(fout, "# \n");

   if(grayType == POWER)
      fprintf(fout, "Type %d %d\n",  grayType, graylogpower);
   else
      fprintf(fout, "Type %d\n",  grayType);

   fprintf(fout, "\nRanges\n");

   fprintf(fout, "%s %-g %-g\n%s %-g %-g\n%s %-g %-g\n%s %-g %-g\n%s %-g %-g\n",
      "Value",        grayminval,     graymaxval,
      "Percentile",   grayminpercent, graymaxpercent,
      "Sigma",        grayminsigma,   graymaxsigma,
      "Min/Max",      graydatamin,    graydatamax,
      "Median/Sigma", median,         sigma);

   fprintf(fout, "\n");
   fprintf(fout, "rmin %-g\n",  rmin);
   fprintf(fout, "rmax %-g\n",  rmax);
   fprintf(fout, "delta %-g\n", delta);
   fprintf(fout, "npix %lu\n",  npix);

   fprintf(fout, "\nStretch Lookup\n");

   for(i=0; i<256; ++i)
      fprintf(fout, "%d %13.6e\n", i, graydataval[i]);

   fprintf(fout, "\n%d Histogram Bins\n", NBIN);

   for(i=0; i<NBIN; ++i)
      fprintf(fout, "%d %13.6e %d %13.6e %13.6e\n", i, datalev[i], hist[i], chist[i], gausslev[i]);

   fflush(fout);
   fclose(fout);


   returnStruct->status = 0;

   sprintf(returnStruct->msg, "min=%-g, minpercent=%.2f, minsigma=%.2f, max=%-g, maxpercent=%.2f, maxsigma=%.2f, datamin=%-g, datamax=%-g",
      grayminval,  grayminpercent, grayminsigma,
      graymaxval,  graymaxpercent, graymaxsigma,
      graydatamin, graydatamax);

   sprintf(returnStruct->json, "{\"min\":%-g, \"minpercent\":%.2f, \"minsigma\":%.2f, \"max\":%-g, \"maxpercent\":%.2f, \"maxsigma\":%.2f, \"datamin\":%-g, \"datamax\":%-g}",
      grayminval,  grayminpercent, grayminsigma,
      graymaxval,  graymaxpercent, graymaxsigma,
      graydatamin, graydatamax);

   returnStruct->minval     = grayminval;
   returnStruct->minpercent = grayminpercent;
   returnStruct->minsigma   = grayminsigma;
   returnStruct->maxval     = graymaxval;
   returnStruct->maxpercent = graymaxpercent;
   returnStruct->maxsigma   = graymaxsigma;
   returnStruct->datamin    = graydatamin;
   returnStruct->datamax    = graydatamax;

   return returnStruct;
}


/**************************************************/
/*                                                */
/*  Parse the HDU / plane info from the file name */
/*                                                */
/**************************************************/

int mHistogram_getPlanes(char *file, int *planes)
{
   int   count, len;
   char *ptr, *ptr1;

   count = 0;

   ptr = file;

   len = strlen(file);

   while(ptr < file+len && *ptr != '[')
      ++ptr;

   while(1)
   {
      if(ptr >= file+len)
         return count;

      if(*ptr != '[')
         return count;

      *ptr = '\0';
      ++ptr;

      ptr1 = ptr;

      while(ptr1 < file+len && *ptr1 != ']')
         ++ptr1;

      if(ptr1 >= file+len)
         return count;

      if(*ptr1 != ']')
         return count;

      *ptr1 = '\0';

      planes[count] = atoi(ptr);
      ++count;

      ptr = ptr1;
      ++ptr;
   }
}
      


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mHistogram_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}



/***********************************/
/*                                 */
/*  Parse a range string           */
/*                                 */
/***********************************/

int mHistogram_parseRange(char const *str, char const *kind, double *val, double *extra, int *type) 
{
   char const *ptr;

   char  *end;
   double sign = 1.0;

   ptr = str;

   while (isspace(*ptr)) ++ptr;

   if (*ptr == '-' || *ptr == '+') 
   {
      sign = (*ptr == '-') ? -1.0 : 1.0;
      ++ptr;
   }

   while (isspace(*ptr)) ++ptr;

   errno = 0;

   *val = strtod(ptr, &end) * sign;

   if (errno != 0) 
   {
      sprintf(montage_msgstr, "leading numeric term in %s '%s' "
            "cannot be converted to a finite floating point number",
            kind, str);
      return 1;
   }

   if (ptr == end) 
   {
      /* range string didn't start with a number, check for keywords */

      if (strncmp(ptr, "min", 3) == 0) 
      {
         *val = 0.0;
      } else if (strncmp(ptr, "max", 3) == 0) 
      {
         *val = 100.0;
      } else if (strncmp(ptr, "med", 3) == 0) 
      {
         *val = 50.0;
      } else 
      {
         sprintf(montage_msgstr, "'%s' is not a valid %s",
               str, kind);
         return 1;
      }
      *type = PERCENTILE;
      ptr += 3;
   }
   else 
   {
      /* range string began with a number, look for a type spec */

      ptr = end;

      while (isspace(*ptr)) ++ptr;

      switch (*ptr) 
      {
         case '%':
            if (*val < 0.0) 
            {
               sprintf(montage_msgstr, "'%s': negative "
                     "percentile %s", str, kind);
               return 1;
            }

            if (*val > 100.0) 
            {
               sprintf(montage_msgstr, "'%s': percentile %s "
                     "larger than 100", str, kind);
               return 1;
            }

            *type = PERCENTILE;
            ++ptr;
            break;

         case 's':
         case 'S':
            *type = SIGMA;
            ++ptr;
            break;

         case '+':
         case '-':
         case '\0':
            *type = VALUE;
            break;
         default:
            sprintf(montage_msgstr, "'%s' is not a valid %s",
                  str, kind);
            return 1;
      }
   }


   /* look for a trailing numeric term */

   *extra = 0.0;

   while (isspace(*ptr)) ++ptr;

   if (*ptr == '-' || *ptr == '+') 
   {
      sign = (*ptr == '-') ? -1.0 : 1.0;

      ++ptr;

      while (isspace(*ptr)) ++ptr;

      *extra = strtod(ptr, &end) * sign;

      if (errno != 0) 
      {
         sprintf(montage_msgstr, "extra numeric term in %s '%s' "
               "cannot be converted to a finite floating point number",
               kind, str);
         return 1;
      }

      if (ptr == end) 
      {
         sprintf(montage_msgstr, "%s '%s' contains trailing "
               "junk", kind, str);
         return 1;
      }

      ptr = end;
   }

   while (isspace(*ptr)) ++ptr;

   if (*ptr != '\0') 
   {
      sprintf(montage_msgstr, "%s '%s' contains trailing junk", kind, str);
      return 1;
   }

   return 0;
}


/***********************************/
/*                                 */
/*  Histogram percentile ranges    */
/*                                 */
/***********************************/

int mHistogram_getRange(fitsfile *fptr, char *minstr, char *maxstr,
                        double *rangemin, double *rangemax, 
                        int type, char *betastr, double *rangebeta, double *dataval,
                        int imnaxis1, int imnaxis2,  double *datamin, double *datamax,
                        double *median, double *sig,
                        int count, int *planes)
{
   int     i, j, k, mintype, maxtype, betatype, nullcnt, status;
   long    fpixel[4], nelements;
   double  d, diff, minval, maxval, betaval;
   double  lev16, lev50, lev84, sigma;
   double  minextra, maxextra, betaextra;
   double *data;

   double  glow, ghigh, gaussval, gaussstep;
   double  dlow, dhigh;
   double  gaussmin, gaussmax;


   /* Make a NaN value to use setting blank pixels */

   union
   {
      double d;
      char   c[8];
   }
   value;

   for(i=0; i<8; ++i)
      value.c[i] = 255;

   double nan;

   nan = value.d;



   nbin = NBIN - 1;

   /* MIN/MAX: Determine what type of  */
   /* range string we are dealing with */

   if(mHistogram_parseRange(minstr, "display min", &minval, &minextra, &mintype)) return 1;
   if(mHistogram_parseRange(maxstr, "display max", &maxval, &maxextra, &maxtype)) return 1;

   betaval   = 0.;
   betaextra = 0.;

   if (type == ASINH)
      if(mHistogram_parseRange(betastr, "beta value", &betaval, &betaextra, &betatype)) return 1;


   /* If we don't have to generate the image */
   /* histogram, get out now                 */

   *rangemin  = minval + minextra;
   *rangemax  = maxval + maxextra;
   *rangebeta = betaval + betaextra;

   if(mintype  == VALUE
   && maxtype  == VALUE
   && betatype == VALUE
   && (type == POWER || type == ASINH))
      return 0;


   /* Find the min, max values in the image */

   npix = 0;

   rmin =  1.0e10;
   rmax = -1.0e10;

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   if(count > 1)
      fpixel[2] = planes[1];
   if(count > 2)
      fpixel[3] = planes[2];

   nelements = imnaxis1;

   data = (double *)malloc(nelements * sizeof(double));

   status = 0;

   for(j=0; j<imnaxis2; ++j) 
   {
      if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, &nan, data, &nullcnt, &status))
      {
         mHistogram_printFitsError(status);
         return 1;
      }
      
      for(i=0; i<imnaxis1; ++i) 
      {
         if(!mNaN(data[i])) 
         {
            if (data[i] > rmax) rmax = data[i];
            if (data[i] < rmin) rmin = data[i];

            ++npix;
         }
      }

      ++fpixel[1];
   }

   *datamin = rmin;
   *datamax = rmax;

   diff = rmax - rmin;

   if(mHistogram_debug)
   {
      printf("DEBUG> mHistogram_getRange(): rmin = %-g, rmax = %-g (diff = %-g)\n",
         rmin, rmax, diff);
      fflush(stdout);
   }


   /* Populate the histogram */

   for(i=0; i<nbin+1; i++) 
      hist[i] = 0;
   
   fpixel[1] = 1;

   for(j=0; j<imnaxis2; ++j) 
   {
      if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, &nan, data, &nullcnt, &status))
      {
         mHistogram_printFitsError(status);
         return 1;
      }

      for(i=0; i<imnaxis1; ++i) 
      {
         if(!mNaN(data[i])) 
         {
            d = floor(nbin*(data[i]-rmin)/diff);
            k = (int)d;

            if(k > nbin-1)
               k = nbin-1;

            if(k < 0)
               k = 0;

            ++hist[k];
         }
      }

      ++fpixel[1];
   }


   /* Compute the cumulative histogram      */
   /* and the histogram bin edge boundaries */

   delta = diff/nbin;

   chist[0] = 0;

   for(i=1; i<=nbin; ++i)
      chist[i] = chist[i-1] + hist[i-1];


   /* Find the data value associated    */
   /* with the minimum percentile/sigma */

   lev16 = mHistogram_percentileLevel(16.);
   lev50 = mHistogram_percentileLevel(50.);
   lev84 = mHistogram_percentileLevel(84.);

   sigma = (lev84 - lev16) / 2.;

   *median = lev50;
   *sig    = sigma;

   if(mintype == PERCENTILE)
      *rangemin = mHistogram_percentileLevel(minval) + minextra;

   else if(mintype == SIGMA)
      *rangemin = lev50 + minval * sigma + minextra;


   /* Find the data value associated    */
   /* with the max percentile/sigma     */

   if(maxtype == PERCENTILE)
      *rangemax = mHistogram_percentileLevel(maxval) + maxextra;
   
   else if(maxtype == SIGMA)
      *rangemax = lev50 + maxval * sigma + maxextra;


   /* Find the data value associated    */
   /* with the beta percentile/sigma    */

   if(type == ASINH)
   {
      if(betatype == PERCENTILE)
         *rangebeta = mHistogram_percentileLevel(betaval) + betaextra;

      else if(betatype == SIGMA)
         *rangebeta = lev50 + betaval * sigma + betaextra;
   }

   if(*rangemin == *rangemax)
      *rangemax = *rangemin + 1.;
   
   if(mHistogram_debug)
   {
      if(type == ASINH)
         printf("DEBUG> mHistogram_getRange(): range = %-g to %-g (beta = %-g)\n", 
            *rangemin, *rangemax, *rangebeta);
      else
         printf("DEBUG> mHistogram_getRange(): range = %-g to %-g\n", 
            *rangemin, *rangemax);
   }


   /* Find the data levels associated with */
   /* a Guassian histogram stretch         */

   if(type == GAUSSIAN
   || type == GAUSSIANLOG)
   {
      for(i=0; i<nbin; ++i)
      {
         datalev [i] = rmin+delta*i;
         gausslev[i] = mHistogram_snpinv(chist[i+1]/npix);
      }


      /* Find the guassian levels associated */
      /* with the range min, max             */

      for(i=0; i<nbin-1; ++i)
      {
         if(datalev[i] > *rangemin)
         {
            gaussmin = gausslev[i];
            break;
         }
      }

      gaussmax = gausslev[nbin-2];

      for(i=0; i<nbin-1; ++i)
      {
         if(datalev[i] > *rangemax)
         {
            gaussmax = gausslev[i];
            break;
         }
      }

      if(type == GAUSSIAN)
      {
         gaussstep = (gaussmax - gaussmin)/255.;

         for(j=0; j<256; ++j)
         {
            gaussval = gaussmin + gaussstep * j;

            for(i=1; i<nbin-1; ++i)
               if(gausslev[i] >= gaussval)
                  break;

            glow  = gausslev[i-1];
            ghigh = gausslev[i];

            dlow  = datalev [i-1];
            dhigh = datalev [i];

            if(glow == ghigh)
               dataval[j] = dlow;
            else
               dataval[j] = dlow
                          + (gaussval - glow) * (dhigh - dlow) / (ghigh - glow);
         }
      }
      else
      {
         gaussstep = log10(gaussmax - gaussmin)/255.;

         for(j=0; j<256; ++j)
         {
            gaussval = gaussmax - pow(10., gaussstep * (256. - j));

            for(i=1; i<nbin-1; ++i)
               if(gausslev[i] >= gaussval)
                  break;

            glow  = gausslev[i-1];
            ghigh = gausslev[i];

            dlow  = datalev [i-1];
            dhigh = datalev [i];

            if(glow == ghigh)
               dataval[j] = dlow;
            else
               dataval[j] = dlow
                          + (gaussval - glow) * (dhigh - dlow) / (ghigh - glow);
         }
      }
   }

   return 0;
}



/***********************************/
/* Find the data values associated */
/* with the desired percentile     */
/***********************************/

double mHistogram_percentileLevel(double percentile)
{
   int    i, count;
   double percent, maxpercent, minpercent;
   double fraction, value;

   if(percentile <=   0) return rmin;
   if(percentile >= 100) return rmax;

   percent = 0.01 * percentile;

   count = (int)(npix*percent);

   i = 1;
   while(i < nbin+1 && chist[i] < count)
      ++i;
   
   minpercent = (double)chist[i-1] / npix;
   maxpercent = (double)chist[i]   / npix;

   fraction = (percent - minpercent) / (maxpercent - minpercent);

   value = rmin + (i-1+fraction) * delta;

   if(mHistogram_debug)
   {
      printf("DEBUG> mHistogram_percentileLevel(%-g):\n", percentile);
      printf("DEBUG> percent    = %-g -> count = %d -> bin %d\n",
         percent, count, i);
      printf("DEBUG> minpercent = %-g\n", minpercent);
      printf("DEBUG> maxpercent = %-g\n", maxpercent);
      printf("DEBUG> fraction   = %-g\n", fraction);
      printf("DEBUG> rmin       = %-g\n", rmin);
      printf("DEBUG> delta      = %-g\n", delta);
      printf("DEBUG> value      = %-g\n\n", value);
      fflush(stdout);
   }

   return(value);
}



/***********************************/
/* Find the percentile level       */
/* associated with a data value    */
/***********************************/

double mHistogram_valuePercentile(double value)
{
   int    i;
   double maxpercent, minpercent;
   double ival, fraction, percentile;

   if(value <= rmin) return   0.0;
   if(value >= rmax) return 100.0;

   ival = (value - rmin) / delta;

   i = ival;

   fraction = ival - i;

   minpercent = (double)chist[i]   / npix;
   maxpercent = (double)chist[i+1] / npix;

   percentile = 100. *(minpercent * (1. - fraction) + maxpercent * fraction);

   if(mHistogram_debug)
   {
      printf("DEBUG> mHistogram_valuePercentile(%-g):\n", value);
      printf("DEBUG> rmin       = %-g\n", rmin);
      printf("DEBUG> delta      = %-g\n", delta);
      printf("DEBUG> value      = %-g -> bin %d (fraction %-g)\n",
         value, i, fraction);
      printf("DEBUG> minpercent = %-g\n", minpercent);
      printf("DEBUG> maxpercent = %-g\n", maxpercent);
      printf("DEBUG> percentile = %-g\n\n", percentile);
      fflush(stdout);
   }

   return(percentile);
}



/*********************************************************************/
/*                                                                   */
/* Wrapper around ERFINV to turn it into the inverse of the          */
/* standard normal probability function.                             */
/*                                                                   */
/*********************************************************************/

double mHistogram_snpinv(double p)
{
   if(p > 0.5)
      return( sqrt(2) * mHistogram_erfinv(2.*p-1.0));
   else
      return(-sqrt(2) * mHistogram_erfinv(1.0-2.*p));
}



/*********************************************************************/
/*                                                                   */
/* ERFINV: Evaluation of the inverse error function                  */
/*                                                                   */
/*        For 0 <= p <= 1,  w = erfinv(p) where erf(w) = p.          */
/*        If either inequality on p is violated then erfinv(p)       */
/*        is set to a negative value.                                */
/*                                                                   */
/*    reference. mathematics of computation,oct.1976,pp.827-830.     */
/*                 j.m.blair,c.a.edwards,j.h.johnson                 */
/*                                                                   */
/*********************************************************************/

double mHistogram_erfinv (double p)
{
   double q, t, v, v1, s, retval;

   /*  c2 = ln(1.e-100)  */

   double c  =  0.5625;
   double c1 =  0.87890625;
   double c2 = -0.2302585092994046e+03;

   double a[6]  = { 0.1400216916161353e+03, -0.7204275515686407e+03,
                    0.1296708621660511e+04, -0.9697932901514031e+03,
                    0.2762427049269425e+03, -0.2012940180552054e+02 };

   double b[6]  = { 0.1291046303114685e+03, -0.7312308064260973e+03,
                    0.1494970492915789e+04, -0.1337793793683419e+04,
                    0.5033747142783567e+03, -0.6220205554529216e+02 };

   double a1[7] = {-0.1690478046781745e+00,  0.3524374318100228e+01,
                   -0.2698143370550352e+02,  0.9340783041018743e+02,
                   -0.1455364428646732e+03,  0.8805852004723659e+02,
                   -0.1349018591231947e+02 };
     
   double b1[7] = {-0.1203221171313429e+00,  0.2684812231556632e+01,
                   -0.2242485268704865e+02,  0.8723495028643494e+02,
                   -0.1604352408444319e+03,  0.1259117982101525e+03,
                   -0.3184861786248824e+02 };

   double a2[9] = { 0.3100808562552958e-04,  0.4097487603011940e-02,
                    0.1214902662897276e+00,  0.1109167694639028e+01,
                    0.3228379855663924e+01,  0.2881691815651599e+01,
                    0.2047972087262996e+01,  0.8545922081972148e+00,
                    0.3551095884622383e-02 };

   double b2[8] = { 0.3100809298564522e-04,  0.4097528678663915e-02,
                    0.1215907800748757e+00,  0.1118627167631696e+01,
                    0.3432363984305290e+01,  0.4140284677116202e+01,
                    0.4119797271272204e+01,  0.2162961962641435e+01 };

   double a3[9] = { 0.3205405422062050e-08,  0.1899479322632128e-05,
                    0.2814223189858532e-03,  0.1370504879067817e-01,
                    0.2268143542005976e+00,  0.1098421959892340e+01,
                    0.6791143397056208e+00, -0.8343341891677210e+00,
                    0.3421951267240343e+00 };

   double b3[6] = { 0.3205405053282398e-08,  0.1899480592260143e-05,
                    0.2814349691098940e-03,  0.1371092249602266e-01,
                    0.2275172815174473e+00,  0.1125348514036959e+01 };


   /* Error conditions */

   q = 1.0 - p;

   if(p < 0.0 || q < 0.0)
     return(-1.e99);


   /* Extremely large value */

   if (q == 0.0)
      return(1.e99);


   /* p between 0 and 0.75 */

   if(p <= 0.75)
   {
      v = p * p - c;

      t = p *  (((((a[5] * v + a[4]) * v + a[3]) * v + a[2]) * v
                             + a[1]) * v + a[0]);

      s = (((((v + b[5]) * v + b[4]) * v + b[3]) * v + b[2]) * v
                             + b[1]) * v + b[0];
    
      retval = t/s;
    
      return(retval);
   }


   /* p between 0.75 and 0.9375 */

   if (p <= 0.9375)
   {
      v = p*p - c1;

      t = p *  ((((((a1[6] * v + a1[5]) * v + a1[4]) * v + a1[3]) * v
                               + a1[2]) * v + a1[1]) * v + a1[0]);

      s = ((((((v + b1[6]) * v + b1[5]) * v + b1[4]) * v + b1[3]) * v
                  + b1[2]) * v + b1[1]) * v + b1[0];
    
      retval = t/s;
    
      return(retval);
   }


   /* q between 1.e-100 and 0.0625 */

   v1 = log(q);

   v  = 1.0/sqrt(-v1);

   if (v1 < c2)
   {
      t = (((((((a2[8] * v + a2[7]) * v + a2[6]) * v + a2[5]) * v + a2[4]) * v
                           + a2[3]) * v + a2[2]) * v + a2[1]) * v + a2[0];

      s = v *  ((((((((  v + b2[7]) * v + b2[6]) * v + b2[5]) * v + b2[4]) * v
                           + b2[3]) * v + b2[2]) * v + b2[1]) * v + b2[0]);
  
      retval = t/s;
    
      return(retval);
   }


   /* q between 1.e-10000 and 1.e-100 */

   t = (((((((a3[8] * v + a3[7]) * v + a3[6]) * v + a3[5]) * v + a3[4]) * v
                        + a3[3]) * v + a3[2]) * v + a3[1]) * v + a3[0];

   s = v *    ((((((  v + b3[5]) * v + b3[4]) * v + b3[3]) * v + b3[2]) * v
                        + b3[1]) * v + b3[0]);
   retval = t/s;

   return retval;
}
