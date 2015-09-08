
/* Module: mExamine.c

Version  Developer        Date     Change

2.1      John Good        08Sep15  fits_read_pix() incorrect null value
2.0      John Good        15Apr15  Complete revamp, with more image info 
                                   and region statistics.
1.0      John Good        13Feb08  Baseline code
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <fitsio.h>
#include <wcs.h>
#include <coord.h>

#include "montage.h"
#include "mNaN.h"

#define STRLEN  1024

int getPlanes(char *, int *);



/*************************************************************************/
/*                                                                       */
/*  mExamine                                                             */
/*                                                                       */
/*  Opens a FITS file (using the cfitsio library), finds the coordinates */
/*  on the sky of the corners (using the WCS library) and converts them  */
/*  to equatorial J2000 (using the coord library).                       */
/*                                                                       */
/*  Outputs these corners plus all the image projection information.     */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv) 
{
   int    debug = 0;

   int    i, j, offscl, nullcnt;
   int    status, clockwise;
   int    locinpix, radinpix;
   int    npix, nnull, first;
   int    ixpix, iypix;

   int    planes[256];
   int    planeCount, hdu;

   char   infile[1024];

   char  *header;

   char   proj[32];
   int    csys;
   char   csys_str[64];

   char   ctype1[256];
   char   ctype2[256];

   double equinox;

   double naxis1;
   double naxis2;

   double crval1;
   double crval2;

   double crpix1;
   double crpix2;

   double cdelt1;
   double cdelt2;

   double crota2;

   double lon, lat;
   double lonc, latc;
   double ra, dec, radius;
   double rac, decc;
   double racp, deccp;
   double ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4;
   double xpix, ypix, rpix;

   double x, y, z;
   double xp, yp, zp;

   double rot, beta, dtr;
   double dx, dy, r;

   double sumflux, sumflux2, mean, rms, dot;
   double sigmaref, sigmamax, sigmamin;
   double val, valx, valy, valra, valdec;
   double min, minx, miny, minra, mindec;
   double max, maxx, maxy, maxra, maxdec;
   double xc, yc, zc;
   double x0, y0, z0;

   struct WorldCoor *wcs;

   fitsfile *fptr;
   int       ibegin, iend, jbegin, jend;
   long      fpixel[4], nelements;
   double   *data;


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


   dtr = atan(1.)/45.;


   /* Process basic command-line arguments */

   radius = 0.;

   locinpix = 0;
   radinpix = 0;

   for(i=0; i<argc; ++i)
   {
      if(strcmp(argv[i], "-p") == 0)
      {
         if(i+4 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"No ra, dec, radius location given or file name missing\"]\n");
            exit(1);
         }

         ra     = atof(argv[i+1]);
         dec    = atof(argv[i+2]);
         radius = atof(argv[i+3]);

         if(strstr(argv[i+1], "p") != 0) locinpix = 1;
         if(strstr(argv[i+2], "p") != 0) locinpix = 1;
         if(strstr(argv[i+3], "p") != 0) radinpix = 1;

         i += 3;
      }

      else if(strcmp(argv[i], "-d") == 0)
      {
         if(i+1 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"File name missing\"]\n");
            exit(1);
         }

         debug = 1;
      }
   }

   if(radius > 0.)
   {
      argv += 4;
      argc -= 4;
   }

   if(debug)
   {
      argv += 1;
      argc -= 1;
   }

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-p ra dec radius] image.fits\"]\n", argv[0]);
      exit(1);
   }

   
   strcpy(infile,  argv[1]);

   if(debug)
   {
      printf("DEBUG> infile = %s\n", infile);
      fflush(stdout);
   }

   planeCount = getPlanes(infile, planes);

   if(planeCount > 0)
      hdu = planes[0];
   else
      hdu = 0;


   /* Open the FITS file and initialize the WCS transform */

   status = 0;
   if(fits_open_file(&fptr, infile, READONLY, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open FITS file %s\"]\n", infile);
      exit(1);
   }

   if(hdu > 0)
   {
      if(fits_movabs_hdu(fptr, hdu+1, NULL, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Can't find HDU %d\"]\n", hdu);
         exit(1);
      }
   }

   status = 0;
   if(fits_get_image_wcs_keys(fptr, &header, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot find WCS keys in FITS file %s\"]\n", infile);
      exit(1);
   }


   wcs = wcsinit(header);

   if(wcs == (struct WorldCoor *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"WCS initialization failed.\"]\n");
      fflush(stdout);
      exit(1);
   }


   /* A bunch of the parameters we want are in the WCS structure */

   clockwise = 0;

   strcpy(ctype1, wcs->ctype[0]);
   strcpy(ctype2, wcs->ctype[1]);

   naxis1 = wcs->nxpix;
   naxis2 = wcs->nypix;

   crval1 = wcs->xref;
   crval2 = wcs->yref;

   crpix1 = wcs->xrefpix;
   crpix2 = wcs->yrefpix;

   cdelt1 = wcs->xinc;
   cdelt2 = wcs->yinc;

   crota2 = wcs->rot;

   if((cdelt1 < 0 && cdelt2 < 0)
   || (cdelt1 > 0 && cdelt2 > 0)) clockwise = 1;

   strcpy(proj, "");

   if(strlen(ctype1) > 5)
   strcpy (proj, ctype1+5);  


   /* We get the Equinox from the WCS.  If not    */
   /* there we take the command-line value. We    */
   /* then infer Julian/Besselian as best we can. */

   equinox = wcs->equinox;

   csys = EQUJ;
   strcpy(csys_str, "EQUJ");

   if(strncmp(ctype1, "RA--", 4) == 0)
   {
      csys = EQUJ;

      strcpy(csys_str, "EQUJ");

      if(equinox < 1975.)
      {
         csys = EQUB;

         strcpy(csys_str, "EQUB");
      }
   }

   if(strncmp(ctype1, "LON-", 4) == 0)
   {
      csys = GAL;
      strcpy(csys_str, "GAL");
   }

   if(strncmp(ctype1, "GLON", 4) == 0)
   {
      csys = GAL;
      strcpy(csys_str, "GAL");
   }

   if(strncmp(ctype1, "ELON", 4) == 0)
   {
      csys = ECLJ;

      strcpy(csys_str, "ECLJ");

      if(equinox < 1975.)
      {
         csys = ECLB;

         strcpy(csys_str, "ECLB");
      }
   }

   if(debug)
   {
      printf("DEBUG> proj      = [%s]\n", proj);
      printf("DEBUG> csys      = %d\n",   csys);
      printf("DEBUG> clockwise = %d\n",   clockwise);
      printf("DEBUG> ctype1    = [%s]\n", ctype1);
      printf("DEBUG> ctype2    = [%s]\n", ctype2);
      printf("DEBUG> equinox   = %-g\n",  equinox);

      printf("\n");
      fflush(stdout);
   }


   /* To get corners and rotation in EQUJ we need the     */
   /* locations of the center pixel and the one above it. */

   pix2wcs(wcs, wcs->nxpix/2.+0.5, wcs->nypix/2.+0.5, &lonc, &latc);
   convertCoordinates (csys, equinox, lonc, latc, 
                       EQUJ, 2000., &rac, &decc, 0.);

   pix2wcs(wcs, wcs->nxpix/2.+0.5, wcs->nypix/2.+1.5, &lonc, &latc);
   convertCoordinates (csys, equinox, lonc, latc, 
                       EQUJ, 2000., &racp, &deccp, 0.);


   /* Use spherical trig to get the Equatorial rotation angle */

   x = cos(decc*dtr) * cos(rac*dtr);
   y = cos(decc*dtr) * sin(rac*dtr);
   z = sin(decc*dtr);

   xp = cos(deccp*dtr) * cos(racp*dtr);
   yp = cos(deccp*dtr) * sin(racp*dtr);
   zp = sin(deccp*dtr);

   beta = acos(x*xp + y*yp + z*zp) / dtr;

   rot = asin(cos(deccp*dtr) * sin((rac-racp)*dtr)/sin(beta*dtr)) / dtr;


   /* And for the four corners we want them uniformly clockwise */

   if(!clockwise)
   {
      pix2wcs(wcs, -0.5, -0.5, &lon, &lat);
      convertCoordinates (csys, equinox, lon, lat, 
                          EQUJ, 2000., &ra1, &dec1, 0.);


      pix2wcs(wcs, wcs->nxpix+0.5, -0.5, &lon, &lat);
      convertCoordinates (csys, equinox, lon, lat, 
                          EQUJ, 2000., &ra2, &dec2, 0.);


      pix2wcs(wcs, wcs->nxpix+0.5, wcs->nypix+0.5, &lon, &lat);
      convertCoordinates (csys, equinox, lon, lat, 
                          EQUJ, 2000., &ra3, &dec3, 0.);


      pix2wcs(wcs, -0.5, wcs->nypix+0.5, &lon, &lat);
      convertCoordinates (csys, equinox, lon, lat, 
                          EQUJ, 2000., &ra4, &dec4, 0.);
   }
   else
   {
      pix2wcs(wcs, -0.5, -0.5, &lon, &lat);
      convertCoordinates (csys, equinox, lon, lat, 
                          EQUJ, 2000., &ra2, &dec2, 0.);


      pix2wcs(wcs, wcs->nxpix+0.5, -0.5, &lon, &lat);
      convertCoordinates (csys, equinox, lon, lat, 
                          EQUJ, 2000., &ra1, &dec1, 0.);


      pix2wcs(wcs, wcs->nxpix+0.5, wcs->nypix+0.5, &lon, &lat);
      convertCoordinates (csys, equinox, lon, lat, 
                          EQUJ, 2000., &ra4, &dec4, 0.);


      pix2wcs(wcs, -0.5, wcs->nypix+0.5, &lon, &lat);
      convertCoordinates (csys, equinox, lon, lat, 
                          EQUJ, 2000., &ra3, &dec3, 0.);
   }


   /* If a location was given, get region statistics */

   // First find the pixel center/radius

   if(radius > 0.)
   {
      if(radinpix) 
      {
         rpix   = radius;
         radius = rpix * fabs(cdelt2);
      }
      else
         rpix = radius / fabs(cdelt2);

      if(locinpix)
      {
         xpix = ra;
         ypix = dec;

         pix2wcs(wcs, xpix, ypix, &lon, &lat);

         convertCoordinates (csys, equinox, lon, lat,
                             EQUJ, 2000., &ra, &dec, 0);
      }
      else
      {
         convertCoordinates (EQUJ, 2000., ra, dec,  
                             csys, equinox, &lon, &lat, 0.);

         wcs2pix(wcs, lon, lat, &xpix, &ypix, &offscl);

         if(offscl)
         {
            printf("[struct stat=\"ERROR\", msg=\"Location off the image.\"]\n");
            fflush(stdout);
            exit(1);
         }
      }

      x0 = cos(ra*dtr) * cos(dec*dtr);
      y0 = sin(ra*dtr) * cos(dec*dtr);
      z0 = sin(dec*dtr);

      ixpix = (int)(xpix+0.5);
      iypix = (int)(ypix+0.5);

      if(debug)
      {
         printf("DEBUG> Region statististics for %-g pixels around (%-g,%-g) [%d,%d] [Equatorial (%-g, %-g)\n",
            rpix, xpix, ypix, ixpix, iypix, ra, dec);
         fflush(stdout);
      }
   }


   // Then read the data

   jbegin = iypix - rpix - 1;
   jend   = iypix + rpix + 1;

   ibegin = ixpix - rpix - 1;
   iend   = ixpix + rpix + 1;

   nelements = iend - ibegin + 1;

   fpixel[0] = ibegin;
   fpixel[1] = jbegin;
   fpixel[2] = 1;
   fpixel[3] = 1;

   if(planeCount > 1)
      fpixel[2] = planes[1];
   if(planeCount > 2)
      fpixel[3] = planes[2];

   data = (double *)malloc(nelements * sizeof(double));

   status = 0;

   sumflux  = 0.;
   sumflux2 = 0.;
   npix     = 0;
   nnull    = 0;

   val      = 0.;
   valx     = 0.;
   valy     = 0.;
   valra    = 0.;
   valdec   = 0.;

   max      = 0.;
   maxx     = 0.;
   maxy     = 0.;
   maxra    = 0.;
   maxdec   = 0.;

   min      = 0.;
   minx     = 0.;
   miny     = 0.;
   minra    = 0.;
   mindec   = 0.;

   xc = cos(rac*dtr) * cos(decc*dtr);
   yc = sin(rac*dtr) * cos(decc*dtr);
   zc = sin(decc*dtr);

   first = 1;

   if(radius > 0.)
   {
      if(debug)
      {
         printf("\nDEBUG> Location: (%.6f %.6f) -> (%d,%d)\n\n", xpix, ypix, ixpix, iypix);
         printf("DEBUG> Radius: %.6f\n\n", rpix);
      }

      for (j=jbegin; j<=jend; ++j)
      {
         status = 0;
         if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, &nan,
                          (void *)data, &nullcnt, &status))
         {
            printf("[struct stat=\"ERROR\", msg=\"Error reading FITS data.\"]\n");
            exit(1);
         }

         for(i=0; i<nelements; ++i)
         {
            if(mNaN(data[i]))
            {
               if(debug)
                  printf("%10s ", "NULL");

               ++nnull;
               continue;
            }

            sumflux  += data[i];
            sumflux2 += data[i]*data[i];
            ++npix;

            x  = ibegin + i;
            y  = j;

            pix2wcs(wcs, x, y, &lon, &lat);

            convertCoordinates(csys, equinox, lon, lat, EQUJ, 2000., &ra, &dec, 0.);

            xp = cos(ra*dtr) * cos(dec*dtr);
            yp = sin(ra*dtr) * cos(dec*dtr);
            zp = sin(dec*dtr);

            dot = xp*x0 + yp*y0 + zp*z0;

            if(dot > 1.)
               dot = 1.;

            r = acos(dot)/dtr / fabs(cdelt2);

            if(debug)
            {
               if(r <= rpix)
                  printf("%10.3f ", data[i]);
               else
                  printf("%10s ", ".");
            }

            if(r > rpix)
               continue;

            if(x == ixpix && y == iypix)
            {
               val = data[i];

               valx = x;
               valy = y;
            }
            
            if(first)
            {
               first = 0;

               min = data[i];

               minx = x;
               miny = y;

               max = data[i];

               maxx = x;
               maxy = y;
            }
            
            if(data[i] > max)
            {
               max = data[i];

               maxx = x;
               maxy = y;
            }
            
            if(data[i] < min)
            {
               min = data[i];

               minx = x;
               miny = y;
            }
         }

         pix2wcs(wcs, valx, valy, &lon, &lat);
         convertCoordinates(csys, equinox, lon, lat, EQUJ, 2000., &valra, &valdec, 0.);

         pix2wcs(wcs, minx, miny, &lon, &lat);
         convertCoordinates(csys, equinox, lon, lat, EQUJ, 2000., &minra, &mindec, 0.);

         pix2wcs(wcs, maxx, maxy, &lon, &lat);
         convertCoordinates(csys, equinox, lon, lat, EQUJ, 2000., &maxra, &maxdec, 0.);

         if(debug)
            printf("\n");

         ++fpixel[1];
      }

      mean = sumflux / npix;
      rms  = sqrt(sumflux2/npix - mean*mean);

      sigmaref = (val - mean) / rms; 
      sigmamin = (min - mean) / rms; 
      sigmamax = (max - mean) / rms; 
   }
   

   /* Finally, print out parameters */

   printf("[struct stat=\"OK\","); 

   printf(" proj=\"%s\",",     proj);
   printf(" csys=\"%s\",",     csys_str);
   printf(" equinox=%.1f,",    equinox);
   printf(" naxis1=%d,",       (int)naxis1);
   printf(" naxis2=%d,",       (int)naxis2);
   printf(" crval1=%.7f,",     crval1);
   printf(" crval2=%.7f,",     crval2);
   printf(" crpix1=%-g,",      crpix1);
   printf(" crpix2=%-g,",      crpix2);
   printf(" cdelt1=%.7f,",     fabs(cdelt1));
   printf(" cdelt2=%.7f,",     fabs(cdelt2));
   printf(" crota2=%.4f,",     crota2);
   printf(" lonc=%.7f,",       lonc);
   printf(" latc=%.7f,",       latc);
   printf(" ximgsize=%.6f,",   fabs(naxis1*cdelt1));
   printf(" yimgsize=%.6f,",   fabs(naxis1*cdelt2));
   printf(" rotequ=%.4f,",     rot);
   printf(" rac=%.7f,",        rac);
   printf(" decc=%.7f,",       decc);
   printf(" ra1=%.7f,",        ra1);
   printf(" dec1=%.7f,",       dec1);
   printf(" ra2=%.7f,",        ra2);
   printf(" dec2=%.7f,",       dec2);
   printf(" ra3=%.7f,",        ra3);
   printf(" dec3=%.7f,",       dec3);
   printf(" ra4=%.7f,",        ra4);
   printf(" dec4=%.7f",        dec4);

   if(radius > 0)
   {
      printf(", radius=%.7f,", radius);
      printf(" radpix=%.2f,",  rpix);
      printf(" npixel=%d,",    npix);
      printf(" nnull=%d,",     nnull);
      printf(" aveflux=%-g,",  mean);
      printf(" rmsflux=%-g,",  rms);
      printf(" fluxref=%-g,",  val);
      printf(" sigmaref=%-g,", sigmaref);
      printf(" xref=%.0f,",    valx);
      printf(" yref=%.0f,",    valy);
      printf(" raref=%.7f,",   valra);
      printf(" decref=%.7f,",  valdec);
      printf(" fluxmin=%-g,",  min);
      printf(" sigmamin=%-g,", sigmamin);
      printf(" xmin=%.0f,",    minx);
      printf(" ymin=%.0f,",    miny);
      printf(" ramin=%.7f,",   minra);
      printf(" decmin=%.7f,",  mindec);
      printf(" fluxmax=%-g,",  max);
      printf(" sigmamax=%-g,", sigmamax);
      printf(" xmax=%.0f,",    maxx);
      printf(" ymax=%.0f,",    maxy);
      printf(" ramax=%.7f,",   maxra);
      printf(" decmax=%.7f",   maxdec);
   }

   printf("]\n");

   fflush(stdout);

   return (0);
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
