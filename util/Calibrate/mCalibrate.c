/* Module: mCalibrate.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        15Nov15  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>

#include <mtbl.h>
#include <svc.h>

#define STRLEN 1024
#define NMAX   1024

void printError(char *msg);

int debug = 0;


/*************************************************************************/
/*                                                                       */
/*  mCalibrate                                                           */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.                                                 */
/*                                                                       */
/*  This module, mCalibrate, retrieves a catalog subset for the region   */
/*  covered by an image, then runs the mExamine aperture photometry      */
/*  routine on each source.                                              */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int    i, pid, ncols, ira, idec, ibmag, irmag;
   int    nstar, nbad;
   int    outtbl;

   double bmag, rmag;

   double *imgflux;
   double *bmagflux;
   double *rmagflux;
   double *x;
   double *y;
   int    nflux;
   int    nmax;

   double yval, B, Bold, sigma, fitflux;

   double sumn;
   double sumx;
   double sumy;
   double sumx2;
   double sumy2;
   double sumxy;

   char   input_file [STRLEN];
   char   tmptbl     [STRLEN];

   char   cmd        [STRLEN];
   char   status     [STRLEN];
   char   ra         [STRLEN];
   char   dec        [STRLEN];
   char   color      [STRLEN];

   FILE  *fout;


   /* Various time value variables */

   char   buffer[256];
   int    yr, mo, day, hr, min, sec;

   time_t     curtime;
   struct tm *loctime;


   outtbl = 0;

   if(debug)
      svc_debug(stdout);


   /*********************************************************/
   /* Get the current time and convert to a datetime string */
   /*********************************************************/

   curtime = time (NULL);
   loctime = localtime (&curtime);

   strftime(buffer, 256, "%Y", loctime);
   yr = atoi(buffer);

   strftime(buffer, 256, "%m", loctime);
   mo = atoi(buffer);

   strftime(buffer, 256, "%d", loctime);
   day = atoi(buffer);

   strftime(buffer, 256, "%H", loctime);
   hr = atoi(buffer);

   strftime(buffer, 256, "%M", loctime);
   min = atoi(buffer);

   strftime(buffer, 256, "%S", loctime);
   sec = atoi(buffer);

   pid = getpid();

   sprintf(tmptbl, "/tmp/CalTbl_%04d.%02d.%02d_%02d.%02d.%02d_%06d",
       yr, mo, day, hr, min, sec, pid);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   if (argc < 2) 
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: mCalibrate in.fits [out.tbl]\"]\n");
      exit(1);
   }

   strcpy(input_file,  argv[1]);

   if(argc > 2)
   {
      outtbl = 1;

      fout = fopen(argv[2], "w+");

      if(fout == (FILE *)NULL)
         printError("Cannot open output table file.");
   }


   /*****************************************/
   /* Get the catalog sources for the image */
   /*****************************************/

   sprintf(cmd, "mCatSearch %s %s", input_file, tmptbl);
   svc_run(cmd);

   strcpy(status, svc_value("stat"));

   if(strcmp(status, "OK") != 0)
   {
      unlink(tmptbl);
      printError(svc_value("msg"));
   }


   /********************************************************/
   /* Run aperture photometry for each source in the table */
   /********************************************************/

   ncols = topen(tmptbl);

   ira   = tcol("ra");
   idec  = tcol("dec");
   ibmag = tcol("b1_mag");
   irmag = tcol("r1_mag");

   if(ira < 0 || idec < 0 || ibmag < 0 || irmag < 0)
   {
      unlink(tmptbl);
      printError("Error reading source table.");
   }

   nstar = 0;
   nbad  = 0;

   nflux = 0;

   nmax = NMAX;

   imgflux  = (double *)malloc(nmax * sizeof(double));
   bmagflux = (double *)malloc(nmax * sizeof(double));
   rmagflux = (double *)malloc(nmax * sizeof(double));


   while(1)
   {
      if(tread() < 0)
         break;

      ++nstar;

      if(tnull(ibmag) || tnull(irmag))
      {
         ++nbad;
         continue;
      }

      strcpy(ra,  tval(ira));
      strcpy(dec, tval(idec));

      bmag = atof(tval(ibmag));
      rmag = atof(tval(irmag));

      bmagflux[nflux] = pow(10., -bmag/2.512);
      rmagflux[nflux] = pow(10., -rmag/2.512);

      sprintf(cmd, "mExamine -a %s %s %s", ra, dec, input_file);

      strcpy(status, svc_value("stat"));

      svc_run(cmd);

      strcpy(status, svc_value("stat"));

      if(strcmp(status, "OK") != 0)
      {
         ++nbad;
         continue;
      }

      imgflux[nflux] = atof(svc_value("totalflux"));

      if(imgflux[nflux] <= 0.)
      {
         ++nbad;
         continue;
      }

      ++nflux;

      if(nflux >= nmax)
      {
         nmax += NMAX;

         imgflux  = (double *)realloc(imgflux,  nmax * sizeof(double));
         bmagflux = (double *)realloc(bmagflux, nmax * sizeof(double));
         rmagflux = (double *)realloc(rmagflux, nmax * sizeof(double));
      }
   }

   unlink(tmptbl);


   // Convert fluxes to log, fit with a line (mid-averaged and slope identically one).
   // The intercept is the log of the calibration scale factor.

   x = (double *)malloc(nflux * sizeof(double));
   y = (double *)malloc(nflux * sizeof(double));

   for(i=0; i<nflux; ++i)
   {
      x[i] = log10(bmagflux[i]);
      y[i] = log10(imgflux[i]);
   }

   B     = 0.;
   sigma = 1.e99;

   while(1)
   {
      sumn  = 0.;
      sumx  = 0.;
      sumy  = 0.;
      sumy2 = 0.;

      for(i=0; i<nflux; ++i)
      {
         yval = x[i] + B;
         if(fabs(yval - y[i]) > 2.*sigma)
            continue;

         sumn  += 1.;
         sumx  += x[i];
         sumy  += y[i];
         sumy2 += y[i]*y[i];
      }

      Bold = B;

      B = (sumy - sumx)/sumn;

      sumn  = 0.;
      sumy2 = 0.;

      for(i=0; i<nflux; ++i)
      {
         yval = x[i] + B;
         if(fabs(yval - y[i]) > 2.*sigma)
            continue;

         sumn  += 1.;
         sumy2 += (yval-y[i])*(yval-y[i]);
      }

      sigma = sqrt(sumy2/sumn);

      if(fabs(Bold - B)/Bold < 1.e-8)
         break;
   }


   if(outtbl)
   {
      // For debugging purposes, print out the data and fit

      fprintf(fout, "|%14s|%14s|%14s|%14s|\n", "imgflux", "bmagflux", "rmagflux", "fitflux");
      fprintf(fout, "|%14s|%14s|%14s|%14s|\n", "double", "double", "double", "double");
      fprintf(fout, "|%14s|%14s|%14s|%14s|\n", "", "", "", "");
      fprintf(fout, "|%14s|%14s|%14s|%14s|\n", "null", "null", "null", "null");
      fflush(fout);

      for(i=0; i<nflux; ++i)
      {
         fitflux = pow(10., (x[i] + B));

         fprintf(fout, " %14.6e %14.6e %14e %14e \n", imgflux[i], bmagflux[i], rmagflux[i], fitflux);
         fflush(fout);
      }

      fclose(fout);
   }


   printf("[struct stat=\"OK\", file=\"%s\", nstar=%d, nbad=%d, scale=%.6e]\n", 
      input_file, nstar, nbad, pow(10., -B));
   fflush(stdout);

   exit(0);
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
