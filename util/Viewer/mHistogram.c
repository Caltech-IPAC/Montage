/* Module: mViewer.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
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

#include <montage.h>
#include <mNaN.h>

#define  MAXSTR 1024

#define  VALUE       0
#define  PERCENTILE  1
#define  SIGMA       2

#define  POWER       0
#define  GAUSSIAN    1
#define  GAUSSIANLOG 2
#define  ASINH       3

#define NBIN 20000


int    getPlanes         (char *, int *);
double percentileLevel   (double percentile);
double valuePercentile   (double value);
double snpinv            (double p);
double erfinv            (double p);
void   printFitsError    (int);

void    parseRange       (char const *str, char const *kind,
                          double *val, double *extra, int *type);

void    getRange         (fitsfile *fptr, char *minstr, char *maxstr,
                          double *rangemin, double *rangemax, 
                          int type, char *betastr, double *rangebeta, 
                          double *dataval, int imnaxis1, int imnaxis2,
                          double *datamin, double *datamax,
                          double *median, double *sigma,
                          int count, int *planes);

int     hdu;
int     grayPlaneCount;

int     grayPlanes[256];
char    histfile [1024];

int     naxis1, naxis2;

int     nbin;

int     hist    [NBIN];
double  chist   [NBIN];
double  datalev [NBIN];
double  gausslev[NBIN];

unsigned long npix;

double  delta, rmin, rmax;

FILE   *fout;
 

int debug = 0;


/****************************************************************************************/
/*                                                                                      */
/*  mHistogram                                                                          */
/*                                                                                      */
/*  This program is essentially a subset of mViewer containing the code that determines */
/*  the stretch for an image.  With it we can generate stretch information for a whole  */
/*  file (or one reference file), then use it when we create a set of JPEG/PNG files    */
/*  from a set of other subsets/files.                                                  */
/*                                                                                      */
/*  Only one file is processed, so if we want color we need to run this program three   */
/*  times.  The only arguments needed are the image / stretch min / stretch max / mode. */
/*                                                                                      */
/****************************************************************************************/


int main(int argc, char **argv)
{
   int       i;
   int       status    = 0;
   int       grayType  = 0;

   double    median, sigma;

   char      grayfile   [1024];

   char      grayminstr  [256];
   char      graymaxstr  [256];
   char      graybetastr [256];

   double    grayminval      = 0.;
   double    graymaxval      = 0.;
   double    grayminpercent  = 0.;
   double    graymaxpercent  = 0.;
   double    grayminsigma    = 0.;
   double    graymaxsigma    = 0.;
   double    graybetaval     = 0.;
   int       graylogpower    = 0;

   double    graydataval[256];

   double    graydatamin;
   double    graydatamax;

   fitsfile *grayfptr;

   char     *end;

   char     *ptr;


   /* Command-line arguments */

   debug   = 0;
   fstatus = stdout;

   strcpy(grayfile,   "");

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] -file in.fits minrange maxrange [logpower/gaussian/gaussian-log] -out out.hist\"]\n", argv[0]);
      exit(1);
   }

   for(i=0; i<argc; ++i)
   {
      /* DEBUG */

      if(strcmp(argv[i], "-d") == 0)
         debug = 1;
      

      /* GRAY */

      else if(strcmp(argv[i], "-file") == 0
           || strcmp(argv[i], "-gray") == 0
           || strcmp(argv[i], "-grey") == 0)
      {
         if(i+3 >= argc)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Too few arguments following -file flag\"]\n");
            fflush(stdout);
            exit(1);
         }

         strcpy(grayfile, argv[i+1]);

         grayPlaneCount = getPlanes(grayfile, grayPlanes);

         if(grayPlaneCount > 0)
            hdu = grayPlanes[0];
         else
            hdu = 0;

         strcpy(grayminstr, argv[i+2]);
         strcpy(graymaxstr, argv[i+3]);

         grayType = POWER;

         if(i+4 < argc)
         {
            if(argv[i+4][0] == 'g')
            {
               grayType = GAUSSIAN;

               if(strlen(argv[i+4]) > 1 
               && (   argv[i+4][strlen(argv[i+4])-1] == 'g'
                   || argv[i+4][strlen(argv[i+4])-1] == 'l'))
                  grayType = GAUSSIANLOG;

               i+=1;
            }
            
            else if(argv[i+4][0] == 'a')
            {
               grayType = ASINH;

               strcpy(graybetastr, "2s");

               if(i+5 < argc)
                  strcpy(graybetastr, argv[i+5]);
               
               i += 2;
            }
            
            else if(strcmp(argv[i+4], "lin") == 0)
               graylogpower = 0;
            
            else if(strcmp(argv[i+4], "log") == 0)
               graylogpower = 1;

            else if(strcmp(argv[i+4], "loglog") == 0)
               graylogpower = 2;

            else
            {
               graylogpower = strtol(argv[i+4], &end, 10);
 
               if(graylogpower < 0  || end < argv[i+4] + strlen(argv[i+4]))
                  graylogpower = 0;
               else
                  i += 1;
            }
         }
         
         i += 2;

         if(fits_open_file(&grayfptr, grayfile, READONLY, &status))
         {
            printf("[struct stat=\"ERROR\", msg=\"Image file %s invalid FITS\"]\n",
               grayfile);
            exit(1);
         }

         if(hdu > 0)
         {
            if(fits_movabs_hdu(grayfptr, hdu+1, NULL, &status))
            {
               printf("[struct stat=\"ERROR\", msg=\"Can't find HDU %d\"]\n", hdu);
               exit(1);
            }
         }
      }

      else if(strcmp(argv[i], "-out") == 0)
      {
         if(i+1 >= argc)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Too few arguments following -out flag\"]\n");
            fflush(stdout);
            exit(1);
         }

         strcpy(histfile, argv[i+1]);

         ++i;
      }
   }

   if(debug)
   {
      printf("DEBUG> grayfile        = [%s]\n", grayfile);
      printf("DEBUG> grayminstr      = [%s]\n", grayminstr);
      printf("DEBUG> graymaxstr      = [%s]\n", graymaxstr);
      printf("DEBUG> graylogpower    = [%d]\n", graylogpower);
      printf("DEBUG> grayType        = [%d]\n", grayType);
      printf("DEBUG> graybetastr     = [%s]\n", graybetastr);
      printf("\n");
      fflush(stdout);
   }


   if(strlen(grayfile) == 0)
   {
      printf ("[struct stat=\"ERROR\", msg=\"No input FITS file name given\"]\n");
      fflush(stdout);
      exit(1);
   }


   if(strlen(histfile) == 0)
   {
      printf ("[struct stat=\"ERROR\", msg=\"No output histogram file name given\"]\n");
      fflush(stdout);
      exit(1);
   }


   fout = fopen(histfile, "w+");

   if(fout == (FILE *)NULL)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Cannot open output histogram file.\"]\n");
      fflush(stdout);
      exit(1);
   }

   /* Grayscale/pseudocolor mode */

   /* Get the image dimensions */

   if(strlen(grayfile) == 0)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Grayscale/pseudocolor mode but no gray image given\"]\n");
      fflush(stdout);
      exit(1);
   }

   status = 0;
   if(fits_read_key(grayfptr, TLONG, "NAXIS1", &naxis1, (char *)NULL, &status))
      printFitsError(status);

   status = 0;
   if(fits_read_key(grayfptr, TLONG, "NAXIS2", &naxis2, (char *)NULL, &status))
      printFitsError(status);

   if(debug)
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

   if(debug)
      printf("\n GRAY RANGE:\n");

   getRange(grayfptr,     grayminstr,  graymaxstr, 
            &grayminval, &graymaxval,  grayType, 
            graybetastr, &graybetaval, graydataval,
            naxis1,       naxis2,
           &graydatamin, &graydatamax,
           &median,       &sigma,
            grayPlaneCount, grayPlanes);

   grayminpercent = valuePercentile(grayminval);
   graymaxpercent = valuePercentile(graymaxval);

   grayminsigma = (grayminval - median) / sigma;
   graymaxsigma = (graymaxval - median) / sigma;

   if(debug)
   {
      printf("DEBUG> grayminval = %-g (%-g%%/%-gs)\n", grayminval, grayminpercent, grayminsigma);
      printf("DEBUG> graymaxval = %-g (%-g%%/%-gs)\n", graymaxval, graymaxpercent, graymaxsigma);
      fflush(stdout);
   }


   /* Output histogram data to file */

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


   printf("[struct stat=\"OK\", min=%-g, minpercent=%.2f, minsigma=%.2f, max=%-g, maxpercent=%.2f, maxsigma=%.2f, datamin=%-g, datamax=%-g]\n",
      grayminval,  grayminpercent, grayminsigma,
      graymaxval,  graymaxpercent, graymaxsigma,
      graydatamin, graydatamax);

   fflush(stdout);
   exit(0);
}


/**************************************************/
/*                                                */
/*  Parse the HDU / plane info from the file name */
/*                                                */
/**************************************************/

int getPlanes(char *file, int *planes)
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

void printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   fprintf(fstatus, "[struct stat=\"ERROR\", status=%d, msg=\"%s\"]\n", status, status_str);

   exit(1);
}



/***********************************/
/*                                 */
/*  Parse a range string           */
/*                                 */
/***********************************/

void parseRange(char const *str, char const *kind, double *val, double *extra, int *type) {
   char const *ptr;
   char *end;
   double sign = 1.0;

   ptr = str;
   while (isspace(*ptr)) ++ptr;
   if (*ptr == '-' || *ptr == '+') {
      sign = (*ptr == '-') ? -1.0 : 1.0;
      ++ptr;
   }
   while (isspace(*ptr)) ++ptr;
   errno = 0;
   *val = strtod(ptr, &end) * sign;
   if (errno != 0) {
      printf("[struct stat=\"ERROR\", msg=\"leading numeric term in %s '%s' "
             "cannot be converted to a finite floating point number\"]\n",
             kind, str);
      fflush(stdout);
      exit(1);
   }
   if (ptr == end) {
      /* range string didn't start with a number, check for keywords */
      if (strncmp(ptr, "min", 3) == 0) {
         *val = 0.0;
      } else if (strncmp(ptr, "max", 3) == 0) {
         *val = 100.0;
      } else if (strncmp(ptr, "med", 3) == 0) {
         *val = 50.0;
      } else {
         printf("[struct stat=\"ERROR\", msg=\"'%s' is not a valid %s\"]\n",
                str, kind);
         fflush(stdout);
         exit(1);
      }
      *type = PERCENTILE;
      ptr += 3;
   } else {
      /* range string began with a number, look for a type spec */
      ptr = end;
      while (isspace(*ptr)) ++ptr;
      switch (*ptr) {
         case '%':
            if (*val < 0.0) {
               printf("[struct stat=\"ERROR\", msg=\"'%s': negative "
                      "percentile %s\"]\n", str, kind);
               fflush(stdout);
               exit(1);
            }
            if (*val > 100.0) {
               printf("[struct stat=\"ERROR\", msg=\"'%s': percentile %s "
                      "larger than 100\"]\n", str, kind);
               fflush(stdout);
               exit(1);
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
            printf("[struct stat=\"ERROR\", msg=\"'%s' is not a valid %s\"]\n",
                   str, kind);
            fflush(stdout);
            exit(1);
      }
   }

   /* look for a trailing numeric term */
   *extra = 0.0;
   while (isspace(*ptr)) ++ptr;
   if (*ptr == '-' || *ptr == '+') {
      sign = (*ptr == '-') ? -1.0 : 1.0;
      ++ptr;
      while (isspace(*ptr)) ++ptr;
      *extra = strtod(ptr, &end) * sign;
      if (errno != 0) {
         printf("[struct stat=\"ERROR\", msg=\"extra numeric term in %s '%s' "
                "cannot be converted to a finite floating point number\"]\n",
                kind, str);
         fflush(stdout);
         exit(1);
      }
      if (ptr == end) {
         printf("[struct stat=\"ERROR\", msg=\"%s '%s' contains trailing "
                "junk\"]\n", kind, str);
         fflush(stdout);
         exit(1);
      }
      ptr = end;
   }
   while (isspace(*ptr)) ++ptr;
   if (*ptr != '\0') {
      printf("[struct stat=\"ERROR\", msg=\"%s '%s' contains trailing "
             "junk\"]\n", kind, str);
      fflush(stdout);
      exit(1);
   }
}


/***********************************/
/*                                 */
/*  Histogram percentile ranges    */
/*                                 */
/***********************************/

void getRange(fitsfile *fptr, char *minstr, char *maxstr,
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
   char    valstr[1024];
   char   *end;
   char   *ptr;

   double  glow, ghigh, gaussval, gaussstep;
   double  dlow, dhigh;
   double  gaussmin, gaussmax;


   nbin = NBIN - 1;

   /* MIN/MAX: Determine what type of  */
   /* range string we are dealing with */

   parseRange(minstr, "display min", &minval, &minextra, &mintype);
   parseRange(maxstr, "display max", &maxval, &maxextra, &maxtype);
   if (type == ASINH) {
      parseRange(betastr, "beta value", &betaval, &betaextra, &betatype);
   }

   /* If we don't have to generate the image */
   /* histogram, get out now                 */

   *rangemin  = minval + minextra;
   *rangemax  = maxval + maxextra;
   *rangebeta = betaval + betaextra;

   if(mintype  == VALUE
   && maxtype  == VALUE
   && betatype == VALUE
   && (type == POWER || type == ASINH))
      return;


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
      if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, NULL,
                       data, &nullcnt, &status))
         printFitsError(status);
      
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

   if(debug)
   {
      printf("DEBUG> getRange(): rmin = %-g, rmax = %-g (diff = %-g)\n",
         rmin, rmax, diff);
      fflush(stdout);
   }


   /* Populate the histogram */

   for(i=0; i<nbin+1; i++) 
      hist[i] = 0;
   
   fpixel[1] = 1;

   for(j=0; j<imnaxis2; ++j) 
   {
      if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, NULL,
                       data, &nullcnt, &status))
         printFitsError(status);

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

   lev16 = percentileLevel(16.);
   lev50 = percentileLevel(50.);
   lev84 = percentileLevel(84.);

   sigma = (lev84 - lev16) / 2.;

   *median = lev50;
   *sig    = sigma;

   if(mintype == PERCENTILE)
      *rangemin = percentileLevel(minval) + minextra;

   else if(mintype == SIGMA)
      *rangemin = lev50 + minval * sigma + minextra;


   /* Find the data value associated    */
   /* with the max percentile/sigma     */

   if(maxtype == PERCENTILE)
      *rangemax = percentileLevel(maxval) + maxextra;
   
   else if(maxtype == SIGMA)
      *rangemax = lev50 + maxval * sigma + maxextra;


   /* Find the data value associated    */
   /* with the beta percentile/sigma    */

   if(type == ASINH)
   {
      if(betatype == PERCENTILE)
         *rangebeta = percentileLevel(betaval) + betaextra;

      else if(betatype == SIGMA)
         *rangebeta = lev50 + betaval * sigma + betaextra;
   }

   if(*rangemin == *rangemax)
      *rangemax = *rangemin + 1.;
   
   if(debug)
   {
      if(type == ASINH)
         printf("DEBUG> getRange(): range = %-g to %-g (beta = %-g)\n", 
            *rangemin, *rangemax, *rangebeta);
      else
         printf("DEBUG> getRange(): range = %-g to %-g\n", 
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
         gausslev[i] = snpinv(chist[i+1]/npix);
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

   return;
}



/***********************************/
/* Find the data values associated */
/* with the desired percentile     */
/***********************************/

double percentileLevel(double percentile)
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

   if(debug)
   {
      printf("DEBUG> percentileLevel(%-g):\n", percentile);
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

double valuePercentile(double value)
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

   if(debug)
   {
      printf("DEBUG> valuePercentile(%-g):\n", value);
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

double snpinv(double p)
{
   if(p > 0.5)
      return( sqrt(2) * erfinv(2.*p-1.0));
   else
      return(-sqrt(2) * erfinv(1.0-2.*p));
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

double erfinv (double p)
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
