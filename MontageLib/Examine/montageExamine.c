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
#include <strings.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <fitsio.h>
#include <wcs.h>
#include <coord.h>

#include <mExamine.h>
#include <montage.h>

#define STRLEN  1024
#define MAXFLUX 1024

struct apPhoto 
{
   double rad;
   double flux;
   double fit;
   double sum;
};


static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mExamine                                                             */
/*                                                                       */
/*  Opens a FITS file (using the cfitsio library), finds the coordinates */
/*  on the sky of the corners (using the WCS library) and converts them  */
/*  to equatorial J2000 (using the coord library).                       */
/*                                                                       */
/*  Outputs these corners plus all the image projection information.     */
/*                                                                       */
/*   int    areaMode       We can examine the image in general (0:NONE)  */
/*                         a specific region with radius (1:AREA) or     */
/*                         perform aperture photometry out to a fixed    */
/*                         radius (2:APPHOT)                             */
/*                                                                       */
/*   char  *infile         FITS file to examine                          */
/*   int    hdu            Optional HDU offset for input file            */
/*                                                                       */
/*   int    plane3         If datacube, the plane index for dimension 3  */
/*   int    plane4         If datacube, the plane index for dimension 4  */
/*                                                                       */
/*   double ra             RA for region statistics                      */
/*   double dec            Dec for region statistics                     */
/*   double radius         Radius for region statistics                  */
/*                                                                       */
/*   int    locinpix       The coordinates are actually in pixels        */
/*   int    radinpix       The radius is actually in pixels              */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mExamineReturn * mExamine(int areaMode, char *infile, int hdu, int plane3, int plane4, 
                                 double ra, double dec, double radius, int locinpix, int radinpix, int debug)
{
   int    i, j, offscl, nullcnt;
   int    status, clockwise, nfound;
   int    npix, nnull, first;
   int    ixpix, iypix;
   int    maxi, maxj;

   char  *header;
   char   tmpstr[32768];

   char   proj[32];
   int    csys;
   char   csys_str[64];

   char   ctype1[256];
   char   ctype2[256];

   double equinox;

   long   naxis;
   long   naxes[10];

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
   double rac, decc;
   double racp, deccp;
   double ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4;
   double xpix, ypix, rpix, rap;

   double x, y, z;
   double xp, yp, zp;

   double rot, beta, dtr;
   double r;

   double sumflux, sumflux2, sumn, mean, background, oldbackground, rms, dot;
   double sigmaref, sigmamax, sigmamin;
   double val, valx, valy, valra, valdec;
   double min, minx, miny, minra, mindec;
   double max, maxx, maxy, maxra, maxdec;
   double x0, y0, z0;

   double  fluxMin, fluxMax, fluxRef, sigma;

   struct WorldCoor *wcs;

   fitsfile *fptr;
   int       ibegin, iend, jbegin, jend;
   long      fpixel[4], nelements;

   double   *data = (double *)NULL;

   struct apPhoto *ap = (struct apPhoto *)NULL;

   int nflux, maxflux;


   struct mExamineReturn *returnStruct;

   returnStruct = (struct mExamineReturn *)malloc(sizeof(struct mExamineReturn));

   bzero((void *)returnStruct, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   nflux   = 0;
   maxflux = MAXFLUX;


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

   rpix = 0.;

   if(areaMode == APPHOT)
      ap = (struct apPhoto *)malloc(maxflux * sizeof(struct apPhoto));

   if(debug)
   {
      printf("DEBUG> infile = %s\n", infile);
      fflush(stdout);
   }

   /* Open the FITS file and initialize the WCS transform */

   status = 0;
   if(fits_open_file(&fptr, infile, READONLY, &status))
   {
      if(ap) free(ap);
      sprintf(returnStruct->msg, "Cannot open FITS file %s", infile);
      return returnStruct;
   }

   if(hdu > 0)
   {
      if(fits_movabs_hdu(fptr, hdu+1, NULL, &status))
      {
         if(ap) free(ap);
         sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
         return returnStruct;
      }
   }

   status = 0;
   if(fits_get_image_wcs_keys(fptr, &header, &status))
   {
      if(ap) free(ap);
      sprintf(returnStruct->msg, "Cannot find WCS keys in FITS file %s", infile);
      return returnStruct;
   }

   status = 0;
   if(fits_read_key_lng(fptr, "NAXIS", &naxis, (char *)NULL, &status))
   {
      if(ap) free(ap);
      sprintf(returnStruct->msg, "Cannot find NAXIS keyword in FITS file %s", infile);
      return returnStruct;
   }

   status = 0;
   if(fits_read_keys_lng(fptr, "NAXIS", 1, naxis, naxes, &nfound, &status))
   {
      if(ap) free(ap);
      sprintf(returnStruct->msg, "Cannot find NAXIS1,2 keywords in FITS file %s", infile);
      return returnStruct;
   }


   wcs = wcsinit(header);

   if(wcs == (struct WorldCoor *)NULL)
   {
      if(ap) free(ap);
      sprintf(returnStruct->msg, "WCS initialization failed.");
      return returnStruct;
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

   ixpix = 1;
   iypix = 1;

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

         if(wcs->offscl)
         {
            if(ap) free(ap);
            sprintf(returnStruct->msg, "Location off the image.");
            return returnStruct;
         }

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
            if(ap) free(ap);
            sprintf(returnStruct->msg, "Location off the image.");
            return returnStruct;
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

   if(ibegin < 1         ) ibegin = 1;
   if(iend   > wcs->nxpix) iend   = wcs->nxpix;

   nelements = iend - ibegin + 1;

   if(jbegin < 1         ) jbegin = 1;
   if(jend   > wcs->nypix) jend   = wcs->nxpix;

   fpixel[0] = ibegin;
   fpixel[1] = jbegin;
   fpixel[2] = 1;
   fpixel[3] = 1;
   fpixel[2] = plane3;
   fpixel[3] = plane4;


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

   first = 1;

   if(radius > 0.)
   {
      if(debug)
      {
         printf("\nDEBUG> Location: (%.6f %.6f) -> (%d,%d)\n\n", xpix, ypix, ixpix, iypix);
         printf("DEBUG> Radius: %.6f\n\n", rpix);

         printf("DEBUG> i: %d to %d\n", ibegin, iend);
         printf("DEBUG> j: %d to %d\n", jbegin, jend);
      }

      for (j=jbegin; j<=jend; ++j)
      {
         status = 0;
         if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, &nan,
                          (void *)data, &nullcnt, &status))
         {
            ++fpixel[1];

            continue;
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

               maxi = i+ibegin;
               maxj = j;
            }
            
            if(data[i] > max)
            {
               max = data[i];

               maxx = x;
               maxy = y;

               maxi = i+ibegin;
               maxj = j;
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

      mean     = 0.;
      rms      = 0.;
      sigmaref = 0.;
      sigmamin = 0.;
      sigmamax = 0.;

      if(npix > 0)
      {
         mean = sumflux / npix;
         rms  = sqrt(sumflux2/npix - mean*mean);

         sigmaref = (val - mean) / rms; 
         sigmamin = (min - mean) / rms; 
         sigmamax = (max - mean) / rms; 
      }
   }
   

   /* Finally, print out parameters */

   strcpy(montage_json,  "{");
   strcpy(montage_msgstr,  "");

   if(areaMode == REGION && radius == 0)
   {
      sprintf(tmpstr, "\"proj\":\"%s\",",     proj);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"csys\":\"%s\",",    csys_str);             strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"equinox\":%.1f,",   equinox);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"naxis\":%ld,",      naxis);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"naxis1\":%d,",      (int)naxis1);          strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"naxis2\":%d,",      (int)naxis2);          strcat(montage_json, tmpstr);

      if(naxis > 2)
         sprintf(tmpstr, " \"naxis3\":%ld,",   naxes[2]);            strcat(montage_json, tmpstr);

      if(naxis > 3)
         sprintf(tmpstr, " \"naxis4\":%ld,",   naxes[3]);            strcat(montage_json, tmpstr);

      sprintf(tmpstr, " \"crval1\":%.7f,",    crval1);               strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crval2\":%.7f,",    crval2);               strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crpix1\":%-g,",     crpix1);               strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crpix2\":%-g,",     crpix2);               strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"cdelt1\":%.7f,",    fabs(cdelt1));         strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"cdelt2\":%.7f,",    fabs(cdelt2));         strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crota2\":%.4f,",    crota2);               strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"lonc\":%.7f,",      lonc);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"latc\":%.7f,",      latc);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ximgsize\":%.6f,",  fabs(naxis1*cdelt1));  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"yimgsize\":%.6f,",  fabs(naxis1*cdelt2));  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"rotequ\":%.4f,",    rot);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"rac\":%.7f,",       rac);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"decc\":%.7f,",      decc);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra1\":%.7f,",       ra1);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec1\":%.7f,",      dec1);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra2\":%.7f,",       ra2);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec2\":%.7f,",      dec2);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra3\":%.7f,",       ra3);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec3\":%.7f,",      dec3);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra4\":%.7f,",       ra4);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec4\":%.7f",       dec4);                 strcat(montage_json, tmpstr);


      sprintf(tmpstr, "proj=\"%s\",",     proj);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " csys=\"%s\",",    csys_str);             strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " equinox=%.1f,",   equinox);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " naxis=%ld,",      naxis);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " naxis1=%d,",      (int)naxis1);          strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " naxis2=%d,",      (int)naxis2);          strcat(montage_msgstr, tmpstr);

      if(naxis > 2)
         sprintf(tmpstr, " naxis3=%ld,",   naxes[2]);            strcat(montage_msgstr, tmpstr);

      if(naxis > 3)
         sprintf(tmpstr, " naxis4=%ld,",   naxes[3]);            strcat(montage_msgstr, tmpstr);

      sprintf(tmpstr, " crval1=%.7f,",    crval1);               strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crval2=%.7f,",    crval2);               strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crpix1=%-g,",     crpix1);               strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crpix2=%-g,",     crpix2);               strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " cdelt1=%.7f,",    fabs(cdelt1));         strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " cdelt2=%.7f,",    fabs(cdelt2));         strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crota2=%.4f,",    crota2);               strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " lonc=%.7f,",      lonc);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " latc=%.7f,",      latc);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ximgsize=%.6f,",  fabs(naxis1*cdelt1));  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " yimgsize=%.6f,",  fabs(naxis1*cdelt2));  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " rotequ=%.4f,",    rot);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " rac=%.7f,",       rac);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " decc=%.7f,",      decc);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra1=%.7f,",       ra1);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec1=%.7f,",      dec1);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra2=%.7f,",       ra2);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec2=%.7f,",      dec2);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra3=%.7f,",       ra3);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec3=%.7f,",      dec3);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra4=%.7f,",       ra4);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec4=%.7f",       dec4);                 strcat(montage_msgstr, tmpstr);
   }

   else if(areaMode == REGION && radius > 0)
   {
      sprintf(tmpstr, "\"proj\":\"%s\",",   proj);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"csys\":\"%s\",",   csys_str);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"equinox\":%.1f,",  equinox);               strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"naxis\":%ld,",     naxis);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"naxis1\":%d,",     (int)naxis1);           strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"naxis2\":%d,",     (int)naxis2);           strcat(montage_json, tmpstr);

      if(naxis > 2)
         sprintf(tmpstr, " \"naxis3\":%ld,", naxes[2]);              strcat(montage_json, tmpstr);

      if(naxis > 3)
         sprintf(tmpstr, " \"naxis4\":%ld,", naxes[3]);              strcat(montage_json, tmpstr);

      sprintf(tmpstr, " \"crval1\":%.7f,",   crval1);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crval2\":%.7f,",   crval2);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crpix1\":%-g,",    crpix1);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crpix2\":%-g,",    crpix2);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"cdelt1\":%.7f,",   fabs(cdelt1));          strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"cdelt2\":%.7f,",   fabs(cdelt2));          strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crota2\":%.4f,",   crota2);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"lonc\":%.7f,",     lonc);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"latc\":%.7f,",     latc);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ximgsize\":%.6f,", fabs(naxis1*cdelt1));   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"yimgsize\":%.6f,", fabs(naxis1*cdelt2));   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"rotequ\":%.4f,",   rot);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"rac\":%.7f,",      rac);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"decc\":%.7f,",     decc);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra1\":%.7f,",      ra1);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec1\":%.7f,",     dec1);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra2\":%.7f,",      ra2);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec2\":%.7f,",     dec2);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra3\":%.7f,",      ra3);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec3\":%.7f,",     dec3);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra4\":%.7f,",      ra4);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec4\":%.7f,",     dec4);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"radius\":%.7f,",   radius);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"radpix\":%.2f,",   rpix);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"npixel\":%d,",     npix);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"nnull\":%d,",      nnull);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"aveflux\":%-g,",   mean);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"rmsflux\":%-g,",   rms);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"fluxref\":%-g,",   val);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"sigmaref\":%-g,",  sigmaref);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"xref\":%.0f,",     valx);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"yref\":%.0f,",     valy);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"raref\":%.7f,",    valra);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"decref\":%.7f,",   valdec);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"fluxmin\":%-g,",   min);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"sigmamin\":%-g,",  sigmamin);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"xmin\":%.0f,",     minx);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ymin\":%.0f,",     miny);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ramin\":%.7f,",    minra);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"decmin\":%.7f,",   mindec);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"fluxmax\":%-g,",   max);                   strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"sigmamax\":%-g,",  sigmamax);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"xmax\":%.0f,",     maxx);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ymax\":%.0f,",     maxy);                  strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ramax\":%.7f,",    maxra);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"decmax\":%.7f",    maxdec);                strcat(montage_json, tmpstr);
   

      sprintf(tmpstr, "proj=\"%s\",",   proj);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " csys=\"%s\",",   csys_str);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " equinox=%.1f,",  equinox);               strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " naxis=%ld,",     naxis);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " naxis1=%d,",     (int)naxis1);           strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " naxis2=%d,",     (int)naxis2);           strcat(montage_msgstr, tmpstr);

      if(naxis > 2)
         sprintf(tmpstr, " naxis3=%ld,", naxes[2]);              strcat(montage_msgstr, tmpstr);

      if(naxis > 3)
         sprintf(tmpstr, " naxis4=%ld,", naxes[3]);              strcat(montage_msgstr, tmpstr);

      sprintf(tmpstr, " crval1=%.7f,",   crval1);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crval2=%.7f,",   crval2);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crpix1=%-g,",    crpix1);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crpix2=%-g,",    crpix2);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " cdelt1=%.7f,",   fabs(cdelt1));          strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " cdelt2=%.7f,",   fabs(cdelt2));          strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crota2=%.4f,",   crota2);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " lonc=%.7f,",     lonc);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " latc=%.7f,",     latc);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ximgsize=%.6f,", fabs(naxis1*cdelt1));   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " yimgsize=%.6f,", fabs(naxis1*cdelt2));   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " rotequ=%.4f,",   rot);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " rac=%.7f,",      rac);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " decc=%.7f,",     decc);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra1=%.7f,",      ra1);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec1=%.7f,",     dec1);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra2=%.7f,",      ra2);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec2=%.7f,",     dec2);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra3=%.7f,",      ra3);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec3=%.7f,",     dec3);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra4=%.7f,",      ra4);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec4=%.7f,",     dec4);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " radius=%.7f,",   radius);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " radpix=%.2f,",   rpix);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " npixel=%d,",     npix);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " nnull=%d,",      nnull);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " aveflux=%-g,",   mean);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " rmsflux=%-g,",   rms);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " fluxref=%-g,",   val);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " sigmaref=%-g,",  sigmaref);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " xref=%.0f,",     valx);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " yref=%.0f,",     valy);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " raref=%.7f,",    valra);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " decref=%.7f,",   valdec);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " fluxmin=%-g,",   min);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " sigmamin=%-g,",  sigmamin);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " xmin=%.0f,",     minx);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ymin=%.0f,",     miny);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ramin=%.7f,",    minra);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " decmin=%.7f,",   mindec);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " fluxmax=%-g,",   max);                   strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " sigmamax=%-g,",  sigmamax);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " xmax=%.0f,",     maxx);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ymax=%.0f,",     maxy);                  strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ramax=%.7f,",    maxra);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " decmax=%.7f",    maxdec);                strcat(montage_msgstr, tmpstr);
   }


   // Now we process the flux vs. radius data to get
   // the integrated source flux. 
   
   else if(areaMode == APPHOT)
   {
      rap = radius;

      jbegin = maxj - rap - 1;
      jend   = maxj + rap + 1;

      ibegin = maxi - rap - 1;
      iend   = maxi + rap + 1;

      nelements = iend - ibegin + 1;

      fpixel[0] = ibegin;
      fpixel[1] = jbegin;
      fpixel[2] = 1;
      fpixel[3] = 1;

      for (j=jbegin; j<=jend; ++j)
      {
         status = 0;
         if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, &nan,
                          (void *)data, &nullcnt, &status))
         {
            if(ap) free(ap);
            free(data);
            sprintf(returnStruct->msg, "Error reading FITS data.");
            return returnStruct;
         }

         for(i=0; i<nelements; ++i)
         {
            if(mNaN(data[i]))
               continue;

            r = sqrt(((double)i+ibegin-maxi)*((double)i+ibegin-maxi) + ((double)j-maxj)*((double)j-maxj));

            if(r > rap)
               continue;

            ap[nflux].rad  = r;
            ap[nflux].flux = data[i];
            ap[nflux].fit  = 0.;

            ++nflux;
            if(nflux >= maxflux)
            {
               maxflux += MAXFLUX;

               ap = (struct apPhoto *)realloc(ap, maxflux * sizeof(struct apPhoto));
            }
         }

         ++fpixel[1];
      }


      // Sort the data by radius

      qsort(ap, (size_t)nflux, sizeof(struct apPhoto), mExamine_radCompare);


      // Find the peak flux and the minimum flux.

      // The max is constrained to be in the first quarter of the
      // data, just to be careful.

      fluxMax = -1.e99;
      fluxMin =  1.e99;

      for(i=0; i<nflux; ++i)
      {
         if(i < nflux/4 && ap[i].flux > fluxMax)
            fluxMax = ap[i].flux;

         if(ap[i].flux < fluxMin)
            fluxMin = ap[i].flux;
      }


      // Fit the minimum a little more carefully, iterating
      // and excluding points more than 3 sigma above the 
      // minimum.

      background = fluxMin;
      sigma      = fluxMax - fluxMin;

      for(j=0; j<1000; ++j)
      {
         oldbackground = background;

         sumn     = 0.;
         sumflux  = 0.;
         sumflux2 = 0.;

         for(i=0; i<nflux; ++i)
         {
            if(fabs(ap[i].flux - background) > 3.*sigma)
               continue;

            sumn     += 1.;
            sumflux  += ap[i].flux;
            sumflux2 += ap[i].flux * ap[i].flux;
         }

         background = sumflux / sumn;
         sigma      = sqrt(sumflux2/sumn - background*background);

         if(fabs((background - oldbackground) / oldbackground) < 1.e-3)
            break;
      }


      fluxRef = (fluxMax-fluxMin) / exp(1.);

      for(i=0; i<nflux; ++i)
      {
         if(ap[i].flux < fluxRef)
         {
            sigma = ap[i].rad;
            break;
         }
      }

      for(i=0; i<nflux; ++i)
      {
         ap[i].fit = (fluxMax-background) * exp(-(ap[i].rad * ap[i].rad) / (sigma * sigma)) + background;

         if(i == 0)
            ap[i].sum = ap[i].flux - background;
         else
            ap[i].sum = ap[i-1].sum + (ap[i].flux - background);
      }


      if(debug)
      {
         printf("|    rad    |    flux    |     fit     |     sum     |\n");
         fflush(stdout);

         for(i=0; i<nflux; ++i)
         {
            printf("%12.6f %12.6f %12.6f %12.6f  \n", ap[i].rad, ap[i].flux, ap[i].fit, ap[i].sum);
            fflush(stdout);
         }
      }

      sprintf(tmpstr, "\"proj\":\"%s\",",    proj);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"csys\":\"%s\",",    csys_str);            strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"equinox\":%.1f,",   equinox);             strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"naxis\":%ld,",      naxis);               strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"naxis1\":%d,",      (int)naxis1);         strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"naxis2\":%d,",      (int)naxis2);         strcat(montage_json, tmpstr);

      if(naxis > 2)
         sprintf(tmpstr, " \"naxis3\":%ld,",   naxes[2]);           strcat(montage_json, tmpstr);

      if(naxis > 3)
         sprintf(tmpstr, " \"naxis4\":%ld,",   naxes[3]);           strcat(montage_json, tmpstr);

      sprintf(tmpstr, " \"crval1\":%.7f,",    crval1);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crval2\":%.7f,",    crval2);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crpix1\":%-g,",     crpix1);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crpix2\":%-g,",     crpix2);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"cdelt1\":%.7f,",    fabs(cdelt1));        strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"cdelt2\":%.7f,",    fabs(cdelt2));        strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"crota2\":%.4f,",    crota2);              strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"lonc\":%.7f,",      lonc);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"latc\":%.7f,",      latc);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ximgsize\":%.6f,",  fabs(naxis1*cdelt1)); strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"yimgsize\":%.6f,",  fabs(naxis1*cdelt2)); strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"rotequ\":%.4f,",    rot);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"rac\":%.7f,",       rac);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"decc\":%.7f,",      decc);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra1\":%.7f,",       ra1);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec1\":%.7f,",      dec1);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra2\":%.7f,",       ra2);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec2\":%.7f,",      dec2);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra3\":%.7f,",       ra3);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec3\":%.7f,",      dec3);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"ra4\":%.7f,",       ra4);                 strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"dec4\":%.7f,",      dec4);                strcat(montage_json, tmpstr);
      sprintf(tmpstr, " \"totalflux\":%.7e",  ap[nflux/2].sum);     strcat(montage_json, tmpstr);


      sprintf(tmpstr, "proj:\"%s\",",    proj);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " csys:\"%s\",",    csys_str);            strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " equinox:%.1f,",   equinox);             strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " naxis:%ld,",      naxis);               strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " naxis1:%d,",      (int)naxis1);         strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " naxis2:%d,",      (int)naxis2);         strcat(montage_msgstr, tmpstr);

      if(naxis > 2)
         sprintf(tmpstr, " naxis3:%ld,",   naxes[2]);           strcat(montage_msgstr, tmpstr);

      if(naxis > 3)
         sprintf(tmpstr, " naxis4:%ld,",   naxes[3]);           strcat(montage_msgstr, tmpstr);

      sprintf(tmpstr, " crval1:%.7f,",    crval1);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crval2:%.7f,",    crval2);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crpix1:%-g,",     crpix1);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crpix2:%-g,",     crpix2);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " cdelt1:%.7f,",    fabs(cdelt1));        strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " cdelt2:%.7f,",    fabs(cdelt2));        strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " crota2:%.4f,",    crota2);              strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " lonc:%.7f,",      lonc);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " latc:%.7f,",      latc);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ximgsize:%.6f,",  fabs(naxis1*cdelt1)); strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " yimgsize:%.6f,",  fabs(naxis1*cdelt2)); strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " rotequ:%.4f,",    rot);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " rac:%.7f,",       rac);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " decc:%.7f,",      decc);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra1:%.7f,",       ra1);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec1:%.7f,",      dec1);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra2:%.7f,",       ra2);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec2:%.7f,",      dec2);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra3:%.7f,",       ra3);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec3:%.7f,",      dec3);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " ra4:%.7f,",       ra4);                 strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " dec4:%.7f,",      dec4);                strcat(montage_msgstr, tmpstr);
      sprintf(tmpstr, " totalflux:%.7e",  ap[nflux/2].sum);     strcat(montage_msgstr, tmpstr);
   }

   strcat(montage_json, "}");

   if(ap) free(ap);
   free(data);

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);
   strcpy(returnStruct->proj, proj);
   strcpy(returnStruct->csys, csys_str);

   returnStruct->equinox   = equinox;
   returnStruct->naxis     = naxis;
   returnStruct->naxis1    = naxis1;
   returnStruct->naxis2    = naxis2;
   returnStruct->naxis3    = naxes[2];
   returnStruct->naxis4    = naxes[3];
   returnStruct->crval1    = crval1;
   returnStruct->crval2    = crval2;
   returnStruct->crpix1    = crpix1;
   returnStruct->crpix2    = crpix2;
   returnStruct->cdelt1    = cdelt1;
   returnStruct->cdelt2    = cdelt2;
   returnStruct->crota2    = crota2;
   returnStruct->lonc      = lonc;
   returnStruct->latc      = latc;
   returnStruct->ximgsize  = fabs(naxis1*cdelt1);
   returnStruct->yimgsize  = fabs(naxis2*cdelt2);
   returnStruct->rotequ    = rot;
   returnStruct->rac       = rac;
   returnStruct->decc      = decc;
   returnStruct->ra1       = ra1;
   returnStruct->dec1      = dec1;
   returnStruct->ra2       = ra2;
   returnStruct->dec2      = dec2;
   returnStruct->ra3       = ra3;
   returnStruct->dec3      = dec3;
   returnStruct->ra4       = ra4;
   returnStruct->dec4      = dec4;
   returnStruct->radius    = radius;
   returnStruct->radpix    = rpix;
   returnStruct->npixel    = npix;
   returnStruct->nnull     = nnull;
   returnStruct->aveflux   = mean;
   returnStruct->rmsflux   = rms;
   returnStruct->fluxref   = val;
   returnStruct->sigmaref  = sigmaref;
   returnStruct->xref      = valx;
   returnStruct->yref      = valy;
   returnStruct->raref     = valra;
   returnStruct->decref    = valdec;
   returnStruct->fluxmin   = min;
   returnStruct->sigmamin  = sigmamin;
   returnStruct->xmin      = minx;
   returnStruct->ymin      = miny;
   returnStruct->ramin     = minra;
   returnStruct->decmin    = mindec;
   returnStruct->fluxmax   = max;
   returnStruct->sigmamax  = sigmamax;
   returnStruct->xmax      = maxx;
   returnStruct->ymax      = maxy;
   returnStruct->ramax     = maxra;
   returnStruct->decmax    = maxdec;

   if(areaMode == APPHOT)
      returnStruct->totalflux = ap[nflux/2].sum;
   else
      returnStruct->totalflux = 0.;

   return returnStruct;
}


/**************************************************/
/*                                                */
/*  Parse the HDU / plane info from the file name */
/*                                                */
/**************************************************/

int mExamine_getPlanes(char *file, int *planes)
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



int mExamine_radCompare(const void *p1, const void *p2)
{
   struct apPhoto *ap1;
   struct apPhoto *ap2;

   ap1 = (struct apPhoto *)p1;
   ap2 = (struct apPhoto *)p2;

   if(ap1->rad < ap2->rad)
      return -1;

   else if(ap1->rad == ap2->rad)
      return 0;

   else
      return 1;
}
