/* Module: mCombineHist.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        27Jun21  Baseline code

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

#include <mCombineHist.h>
#include <montage.h>

#define MAXSTR 1024

#define NBIN 200000


static int mCombineHist_debug;

static unsigned long  nbin;

static unsigned long long npix;

static unsigned long long hist [NBIN];
static unsigned long long chist[NBIN];

static double  datalev [NBIN];
static double  gausslev[NBIN];

static double  delta, rmin, rmax;

static char montage_msgstr[1024];


/*-*********************************************************************************************/
/*                                                                                             */
/*  mCombineHist                                                                               */
/*                                                                                             */
/*  This program combines the information is a collection of histogram files into a single     */
/*  histogram (and stretch).  In this way, we can then display a set of images with the same   */
/*  stretch (based on the correct stretch for all their pixels) or stretch a single image      */
/*  based on the brightness of it and its neighbors.  We use this latter approach to           */
/*  approximate a slowly varying stretch across a region.  This usually results in a uniform   */
/*  looking image where sub-regions of very different brightness are still still distinct      */
/*  (much like "dodging" while printing photographs in a darkroom).                            */
/*                                                                                             */
/*   char **histfile      Input FITS file.                                                     */
/*   int    nhist         Number of input histograms.                                          */
/*                                                                                             */
/*   char  *minString     Data range minimum as a string (to allow '%' and 's' suffix.         */
/*   char  *maxString     Data range maximum as a string (to allow '%' and 's' suffix.         */
/*                                                                                             */
/*   int    logpower      If the stretch type is log to a power, the power value.              */
/*   char  *betastr       If the stretch type is asinh, the transition data value.             */
/*                                                                                             */
/*   char  *stretchType   Stretch type (linear, power(log**N), sinh, gaussian or gaussian-log) */
/*                                                                                             */
/*   char  *outhist       Output histogram file.                                               */
/*                                                                                             */
/*   int    debug         Debugging output level.                                              */
/*                                                                                             */
/***********************************************************************************************/

struct mCombineHistReturn *mCombineHist(char **histfile, int nhist, char *minstr, char *maxstr, char *stretchType,
                                        char *betastr, int logpower, char *outhist, int debugin)
{
   int    i, j, k, nbin, type;
   double val, dval;
   double datamin, datamax;
   double indatamin, indatamax;
   double minval, maxval, betaval, median, sigma;
   double minpercent, maxpercent, minsigma, maxsigma;
   double dataval[256];

   unsigned long long count;

   char   line [MAXSTR];
   char   label[MAXSTR];

   unsigned long long inhist[NBIN];

   static FILE *fhist, *fout;

   int    status = 0;

   struct mCombineHistReturn *returnStruct;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mCombineHistReturn *)malloc(sizeof(struct mCombineHistReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /**********************************/
   /* Process the calling parameters */
   /**********************************/

   mCombineHist_debug = debugin;

   if(strlen(outhist) == 0)
   {
      strcpy(returnStruct->msg, "No output histogram file name given.");
      return returnStruct;
   }

        if(strcmp(stretchType, "linear")       == 0) type = POWER;
   else if(strcmp(stretchType, "power")        == 0) type = POWER;
   else if(strcmp(stretchType, "gaussian")     == 0) type = GAUSSIAN;
   else if(strcmp(stretchType, "gaussian-log") == 0) type = GAUSSIANLOG;
   else if(strcmp(stretchType, "gaussianlog")  == 0) type = GAUSSIANLOG;
   else if(strcmp(stretchType, "asinh")        == 0) type = ASINH;

   fout = fopen(outhist, "w+");

   if(fout == (FILE *)NULL)
   {
      strcpy(returnStruct->msg, "Cannot open output histogram file.");
      return returnStruct;
   }


   /**************************************************************************/
   /* First, a simple loop over the histograms to find the data value range  */
   /* for the group.  We use this to set up the output histogram, which we   */
   /* will populate the second time we loop through the input.               */
   /**************************************************************************/
    
   nbin = NBIN - 1;

   for(i=0; i<nhist; ++i)
   {
      fhist = fopen(histfile[i], "r");

      if(fhist == (FILE *)NULL)
      {
         sprintf(returnStruct->msg, "Can't open histogram file [%s].", histfile[i]);
         return returnStruct;
      }

      while(1)
      {
         fgets(line, 1024, fhist);

         if(line[0] != '#')
            break;
      }

      for(j=0; j<9; ++j)
         fgets(line, 1024, fhist);

      sscanf(line, "%s %lf", label, &indatamin);

      fgets(line, 1024, fhist);

      sscanf(line, "%s %lf", label, &indatamax);

      if(i == 0 || indatamin < datamin)
         datamin = indatamin;

      if(i == 0 || indatamax > datamax)
         datamax = indatamax;

      fclose(fhist);
   }

   delta = (datamax - datamin)/nbin;

   rmin = datamin;
   rmax = datamax;

   if(mCombineHist_debug)
   {
      printf("DEBUG> datamin = %-g\n", datamin);
      printf("DEBUG> datamax = %-g\n", datamax);
      printf("DEBUG> rmin    = %-g\n", rmin);
      printf("DEBUG> rmax    = %-g\n", rmax);
      printf("DEBUG> delta   = %-g\n", delta);
      fflush(stdout);
   }


   /************************************/
   /* Set up the output histogram bins */
   /************************************/

   for(i=0; i<nbin+1; ++i)
   {
      datalev[i] = datamin + delta*i;
      hist   [i] = 0;
      chist  [i] = 0;
   }

   if(mCombineHist_debug)
   {
      printf("DEBUG> datalev and hist arrays initialized.\n");
      fflush(stdout);
   }


   /**************************************************************************/
   /* Loop over the input histograms again, readining in the histogram data  */
   /* We will ignore some of the parameters as we will later calculate those */
   /* for the combined data.                                                 */
   /**************************************************************************/
    
   npix = 0;

   for(j=0; j<nhist; ++j)
   {
      mCombineHist_readHist(histfile[j], inhist, &indatamin, &indatamax);

      
      // Loop over the data values, adding them into the new histogram
      
      dval = (indatamax - indatamin) / nbin;

      if(mCombineHist_debug)
      {
         printf("DEBUG> from readHist: file [%s]: inhist read (min: %-g, max: %-g, dval=%-g)\n",
                histfile[j], indatamin, indatamax, dval);
         fflush(stdout);
      }

      for(i=0; i<nbin; ++i)
      {
         val = indatamin + dval * i;

         count = inhist[i];

         k = (int)floor((val - datamin)/delta);

         hist[k] += count;

         if(mCombineHist_debug >= 2)
         {
            printf("DEBUG> %d (val=%-g): count=%llu (hist[%d] = %llu)\n", i, val, count, k, hist[k]);
            fflush(stdout);
         }

         npix += count;
      }
   }


   /************************************/
   /* Compute the cumulative histogram */
   /************************************/

   for(i=1; i<=nbin; ++i)
      chist[i] = chist[i-1] + hist[i-1];


   /******************************************************************/
   /* We no have all the data, though not net the gaussian equalized */
   /* histogram, and need to determine that, what the range strings  */
   /* translate to in terms of data ranges and finally the lookup    */
   /* table (for gaussian/gaussian-log stretches.                    */
   /******************************************************************/

   status = mCombineHist_getRange(type, minstr,  maxstr, betastr, &minval, &maxval, &betaval,
                                  &median,  &sigma, dataval);

   if(status)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      fclose(fout);
      return returnStruct;
   }

   minpercent = mCombineHist_valuePercentile(minval);
   maxpercent = mCombineHist_valuePercentile(maxval);

   minsigma = (minval - median) / sigma;
   maxsigma = (maxval - median) / sigma;

   if(mCombineHist_debug)
   {
      printf("DEBUG> minval = %-g (%-g%%/%-gs)\n", minval, minpercent, minsigma);
      printf("DEBUG> maxval = %-g (%-g%%/%-gs)\n", maxval, maxpercent, maxsigma);

      printf("\ndataval array:\n");

      for(i=0; i<256; ++i)
         printf("%3d: %-g\n", i, dataval[i]);

      printf("\n");
      fflush(stdout);
   }


   /*********************************/
   /* Output histogram data to file */
   /*********************************/

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

   if(type == POWER)
      fprintf(fout, "Type %d %d\n",  type, logpower);
   else
      fprintf(fout, "Type %d\n",  type);

   fprintf(fout, "\nRanges\n");

   fprintf(fout, "%s %-g %-g\n%s %-g %-g\n%s %-g %-g\n%s %-g %-g\n%s %-g %-g\n",
      "Value",        minval,     maxval,
      "Percentile",   minpercent, maxpercent,
      "Sigma",        minsigma,   maxsigma,
      "Min/Max",      datamin,    datamax,
      "Median/Sigma", median,     sigma);

   fprintf(fout, "\n");
   fprintf(fout, "rmin %-g\n",  rmin);
   fprintf(fout, "rmax %-g\n",  rmax);
   fprintf(fout, "delta %-g\n", delta);
   fprintf(fout, "npix %llu\n", npix);

   fprintf(fout, "\nStretch Lookup\n");

   for(i=0; i<256; ++i)
      fprintf(fout, "%d %13.6e\n", i, dataval[i]);

   fprintf(fout, "\n%d Combined Hist Bins\n", NBIN);

   for(i=0; i<NBIN; ++i)
      fprintf(fout, "%d %13.6e %llu %llu %13.6e\n", i, datalev[i], hist[i], chist[i], gausslev[i]);

   fflush(fout);
   fclose(fout);


   returnStruct->status = 0;

   sprintf(returnStruct->msg, "min=%-g, minpercent=%.2f, minsigma=%.2f, max=%-g, maxpercent=%.2f, maxsigma=%.2f, datamin=%-g, datamax=%-g",
      minval,  minpercent, minsigma,
      maxval,  maxpercent, maxsigma,
      datamin, datamax);

   sprintf(returnStruct->json, "{\"min\":%-g, \"minpercent\":%.2f, \"minsigma\":%.2f, \"max\":%-g, \"maxpercent\":%.2f, \"maxsigma\":%.2f, \"datamin\":%-g, \"datamax\":%-g}",
      minval,  minpercent, minsigma,
      maxval,  maxpercent, maxsigma,
      datamin, datamax);

   returnStruct->minval     = minval;
   returnStruct->minpercent = minpercent;
   returnStruct->minsigma   = minsigma;
   returnStruct->maxval     = maxval;
   returnStruct->maxpercent = maxpercent;
   returnStruct->maxsigma   = maxsigma;
   returnStruct->datamin    = datamin;
   returnStruct->datamax    = datamax;

   return returnStruct;
}



int mCombineHist_readHist(char *histfile,  unsigned long long *inhist, double *rmin, double *rmax)
{
   int   i;

   FILE *fhist;

   char  line [1024];
   char  label[1024];

   double level, comhist, gaussval;


   fhist = fopen(histfile, "r");

   if(fhist == (FILE *)NULL)
   {
      strcpy(montage_msgstr, "Cannot open histogram file.");
      return 1;
   }

   while(1)
   {
      fgets(line, 1024, fhist);

      if(line[0] != '#')
         break;
   }

   for(i=0; i<9; ++i)
      fgets(line, 1024, fhist);

   sscanf(line, "%s %lf", label, rmin);

   fgets(line, 1024, fhist);
   sscanf(line, "%s %lf", label, rmax);

   for(i=0; i<3; ++i)
      fgets(line, 1024, fhist);

   for(i=0; i<258; ++i)
      fgets(line, 1024, fhist);

   for(i=0; i<NBIN; ++i)
   {
      fgets(line, 1024, fhist);
      sscanf(line, "%s %lf %llu %lf %lf", label, &level, inhist+i, &comhist, &gaussval);
   }

   fclose(fhist);

   return 0;
}


/***********************************/
/*                                 */
/*  Parse a range string           */
/*                                 */
/***********************************/

int mCombineHist_parseRange(char const *str, char const *kind, double *val, double *extra, int *type)
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
/*  CombineHist percentile ranges  */
/*                                 */
/***********************************/

int mCombineHist_getRange(int type, char *minstr,  char *maxstr, char *betastr, 
                          double *rangemin, double *rangemax, double *rangebeta,
                          double *median,  double *sig, double *dataval)
{
   int     i, j, mintype, maxtype, betatype;
   double  minval, maxval, betaval;
   double  lev16, lev50, lev84, sigma;
   double  minextra, maxextra, betaextra;

   double  glow, ghigh, gaussval, gaussstep;
   double  dlow, dhigh;
   double  gaussmin, gaussmax, sumpix;

   nbin = NBIN - 1;

   /* MIN/MAX: Determine what type of  */
   /* range string we are dealing with */

   if(mCombineHist_parseRange(minstr, "display min", &minval, &minextra, &mintype)) return 1;
   if(mCombineHist_parseRange(maxstr, "display max", &maxval, &maxextra, &maxtype)) return 1;

   betaval   = 0.;
   betaextra = 0.;

   if (type == ASINH)
      if(mCombineHist_parseRange(betastr, "beta value", &betaval, &betaextra, &betatype)) return 1;

   *rangemin  = minval + minextra;
   *rangemax  = maxval + maxextra;
   *rangebeta = betaval + betaextra;

   if(mintype  == VALUE
   && maxtype  == VALUE
   && betatype == VALUE
   && (type == POWER || type == ASINH))
      return 0;


   /* Find the data value associated    */
   /* with the minimum percentile/sigma */

   lev16 = mCombineHist_percentileLevel(16.);
   lev50 = mCombineHist_percentileLevel(50.);
   lev84 = mCombineHist_percentileLevel(84.);

   sigma = (lev84 - lev16) / 2.;

   *median = lev50;
   *sig    = sigma;

   if(mintype == PERCENTILE)
      *rangemin = mCombineHist_percentileLevel(minval) + minextra;

   else if(mintype == SIGMA)
      *rangemin = lev50 + minval * sigma + minextra;


   /* Find the data value associated    */
   /* with the max percentile/sigma     */

   if(maxtype == PERCENTILE)
      *rangemax = mCombineHist_percentileLevel(maxval) + maxextra;
   
   else if(maxtype == SIGMA)
      *rangemax = lev50 + maxval * sigma + maxextra;


   /* Find the data value associated    */
   /* with the beta percentile/sigma    */

   if(type == ASINH)
   {
      if(betatype == PERCENTILE)
         *rangebeta = mCombineHist_percentileLevel(betaval) + betaextra;

      else if(betatype == SIGMA)
         *rangebeta = lev50 + betaval * sigma + betaextra;
   }

   if(*rangemin == *rangemax)
      *rangemax = *rangemin + 1.;
   
   if(mCombineHist_debug)
   {
      if(type == ASINH)
         printf("DEBUG> mCombineHist_getRange(): range = %-g to %-g (beta = %-g)\n", 
            *rangemin, *rangemax, *rangebeta);
      else
         printf("DEBUG> mCombineHist_getRange(): range = %-g to %-g\n", 
            *rangemin, *rangemax);
   }


   /* Find the values associated with */
   /* a Guassian histogram stretch    */

   if(type == GAUSSIAN
   || type == GAUSSIANLOG)
   {
      for(i=0; i<nbin; ++i)
      {
         sumpix = chist[i+1];
         
         if(sumpix > npix-1)
            sumpix = npix-1;

         gausslev[i] = mCombineHist_snpinv(sumpix/npix);
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

double mCombineHist_percentileLevel(double percentile)
{
   int    i;
   double percent, maxpercent, minpercent;
   double fraction, value;

   unsigned long long count;

   if(percentile <=   0) return rmin;
   if(percentile >= 100) return rmax;

   percent = percentile / 100.;

   count = npix * (unsigned long long) percentile / 100;

   i = 1;
   while(i < nbin+1 && chist[i] < count)
      ++i;
   
   minpercent = (double)chist[i-1] / npix;
   maxpercent = (double)chist[i]   / npix;

   fraction = (percent - minpercent) / (maxpercent - minpercent);

   value = rmin + (i-1+fraction) * delta;

   if(mCombineHist_debug)
   {
      printf("DEBUG> mCombineHist_percentileLevel(%-g):\n", percentile);
      printf("DEBUG> percent    = %-g -> count = %llu -> bin %d\n",
         percentile/100., count, i);
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

double mCombineHist_valuePercentile(double value)
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

   if(mCombineHist_debug)
   {
      printf("DEBUG> mCombineHist_valuePercentile(%-g):\n", value);
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

double mCombineHist_snpinv(double p)
{
   if(p > 0.5)
      return( sqrt(2) * mCombineHist_erfinv(2.*p-1.0));
   else
      return(-sqrt(2) * mCombineHist_erfinv(1.0-2.*p));
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

double mCombineHist_erfinv (double p)
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
