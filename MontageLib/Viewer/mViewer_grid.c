#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <cmd.h>
#include <mNaN.h>

#include "fitsio.h"
#include "coord.h"
#include "wcs.h"

#define MAXSTR  1024
#define MAXHDR 80000

#define NEITHER 0
#define NORTH   1
#define SOUTH   2
#define BOTH    3

void  mViewer_makeGrid       (struct WorldCoor *wcs, 
                              int csysimg,  double epochimg, 
                              int csysgrid, double epochgrid,  
                              double red, double green, double blue,
                              char *fontfile, double fontscale, double linewidth);

char *mViewer_longitude_label(double lon, int ishr);
char *mViewer_latitude_label (double lat);

void mViewer_coord_label     (char *face_path, int fontsize,
                              double lonlab, double latlab, char *label,
                              int csysimg, double epochimg, int csysgrid, double epochgrid,
                              double red, double green, double blue);

void mViewer_great_circle    (struct WorldCoor *wcs, int flipY,
                              int csysimg,  double epochimg, 
                              int csysgrid, double epochgrid,  
                              double lon0,  double lat0, double lon1, double lat1,
                              double red,   double green, double blue, double linewidth);

void mViewer_symbol          (struct WorldCoor *wcs, int flipY,
                              int csysimg,  double epochimg,
                              int csyssym, double epochsym,   
                              double clon,  double clat, int inpix,
                              double symsize, int symnpnt, int symmax, int symtype, double symang,
                              double red,   double green, double blue, double linewidth);

void mViewer_drawing         (char *filename, int flipY, struct WorldCoor *wcs, int csysimg, double epochimg, char *fontfile);

void mViewer_smooth_line     (double x1, double y1, 
                              double x2, double y2,
                              double red, double green, double blue,
                              double linewidth);

void mViewer_curve           (double *xcurve, double *ycurve, int npt,
                              double red, double green, double blue, double linewidth);

void mViewer_latitude_line   (double lat, double lonmin, double lonmax,
                              int csysimg, double epochimg, int csysgrid, double epochgrid,
                              double red, double green, double blue, double linewidth);

void mViewer_longitude_line  (double lon, double latmin, double latmax,
                              int csysimg, double epochimg, int csysgrid, double epochgrid,
                              double red, double green, double blue, double linewidth);

void mViewer_draw_label      (char *fontfile, int fontsize,
                              int xlab, int ylab, char *text,
                              double red, double green, double blue);

void mViewer_draw_boundary   (double red, double green, double blue, double linewidth);

void mViewer_hpxCheck        (int *offscl, double *x, double *y);

void mViewer_addOverlay      (double alpha);


struct Pix
{
   double x;
   double y;
};


double grid_range[] = {0.6/3600.,   1.5/3600.,   3./3600.,   6./3600.,  15./3600.,   30./3600.,
                       60./3600.,      3./60.,     6./60.,    15./60.,    30./60.,         1.5,
                              3.,          6.,        15.,        30.,        60.,         90.,      360.};

double grid_space[] = {0.1/3600.,   0.2/3600.,  0.5/3600.,   1./3600.,   2./3600.,    5./3600.,
                       10./3600.,   20./3600.,     1./60.,     2./60.,     5./60.,     10./60.,
                         30./60.,          1.,         2.,         5.,        10.,         15.,       30.};

/*                         -> 6       3 -> 7.5    3 -> 6      3 -> 6     3 - > 7.5     3 -> 6
                         3 -> 6       3 -> 9      3 -> 6      3 -> 6     3 - > 6       3 -> 9
                         3 -> 6       3 -> 6      3 -> 7.5    3 -> 6     3 - > 6       4 -> 6    4 -> 12 
*/

int ngrid = 190;

double compass_segments[9][4] = {{ 0.0,  0.0,  0.0,  8.0},
                                 { 0.0,  0.0, -4.0,  0.0},
                                 {-4.0,  0.0, -3.0, -0.5},
                                 {-4.0,  0.0, -3.0,  0.5},
                                 { 0.0,  8.0,  0.5,  7.0},
                                 { 0.0,  8.0, -0.5,  7.0},
                                 {-0.7,  9.0, -0.7, 11.0},
                                 {-0.7, 11.0,  0.7,  9.0},
                                 { 0.7,  9.0,  0.7, 11.0}};

extern int hpx;

int gdebug = 0;


/********************************************************************/
/*                                                                  */
/* Find the right set of latitude and longitude lines for the image */
/* and generate an appropriate set of labels                        */
/*                                                                  */
/********************************************************************/

void mViewer_makeGrid(struct WorldCoor *wcs, 
                      int csysimg,  double epochimg, 
                      int csysgrid, double epochgrid,  
                      double red, double green, double blue,
                      char *fontfile, double fontscale, double linewidth)
{
   int    i, side, offscl, use, used_lat, convert;
   int    ilon_label, ishr;
   int    ns, nl;
   int    fontsize;
   double dotmax;
   double lon, lat;
   double lonprev, latprev;
   double lonmin, lonmax;
   double latmin, latmax;
   double lbllat, lbllon;
   double reflat, reflon;
   double lon_range, lat_range;
   double lon_space, lat_space;
   double xpix, ypix;
   double x1, y1, z1;
   double x2, y2, z2;
   double lon0, lat0;
   double lonn, latn;
   double midlon, midlat;

   int    ipole;

   int    nlon, nlat;
   int    nlab_lon, nlab_lat;

   double dtr;

   int meridian; 

   double xsize, ysize, imgsize;

   int    neglon;
   double coslatmin, coslatmax, coslat;
   double lonrange, tmpval;

   int ispace_lon = 2;
   int ispace_lat = 2;

   struct Pix corner[4];

   convert = 0;
   if(csysgrid != csysimg || epochgrid != epochimg)
      convert = 1;

   ishr = 1;
   if(csysgrid != EQUJ && csysgrid != EQUB)
      ishr = 0;

   dtr = atan(1.0)/45.;


   /* Get the image dimensions (and initialize corner pixel coords) */

   ns = wcs->nxpix;
   nl = wcs->nypix;

   fontsize = 11;
   if(ns < 600)
      fontsize = 9;
   if(ns < 400)
      fontsize = 8;

   fontsize = (int)(fontsize * fontscale);

   if(fontsize < 1)
      fontsize = 1;

   corner[0].x = -0.5;
   corner[0].y = -0.5;

   corner[1].x = -0.5;
   corner[1].y = (double)nl + 0.5;

   corner[2].x = (double)ns + 0.5;
   corner[2].y = (double)nl + 0.5;

   corner[3].x = (double)ns + 0.5;
   corner[3].y = -0.5;


   /* Find the x,y pixel-based size */

   xsize = ns * wcs->cdelt[0]*dtr;
   ysize = nl * wcs->cdelt[1]*dtr;

   imgsize = sqrt(xsize*xsize + ysize*ysize);

   if(gdebug)
   {
      printf("mViewer_makeGrid> Image size = %dx%d (%-gx%-g)\n", ns, nl, xsize, ysize);
      printf("mViewer_makeGrid> Image diagonal = %-g\n", imgsize);
      fflush(stdout);
   }


   /* Find the min/max lat/lon for the corners */

   for(i=0; i<4; ++i)
   {
      pix2wcs(wcs, corner[i].x, corner[i].y, &lon, &lat);

      if(convert)
      {
         convertCoordinates(csysimg,  epochimg,  lon, lat,
                            csysgrid, epochgrid, &reflon, &reflat, 0.0);
         lon = reflon;
         lat = reflat;
      }

      if(i == 0)
      {
         lonmin = lon;
         lonmax = lon;
         latmin = lat;
         latmax = lat;
      }

      else
      {
         if(lon < lonmin) lonmin = lon;
         if(lon > lonmax) lonmax = lon;
         if(lat < latmin) latmin = lat;
         if(lat > latmax) latmax = lat;
      }
   }

   if(gdebug)
   {
      printf("mViewer_makeGrid> Corner check Lon: %8.4f -> %8.4f (%-g)\n", lonmin, lonmax, lonmax-lonmin);
      printf("mViewer_makeGrid> Corner check Lat: %8.4f -> %8.4f (%-g)\n", latmin, latmax, latmax-latmin);
      fflush(stdout);
   }


   /* Now we have a tricky case to take care of.  We may have a longitude  */
   /* range of near 360 degrees because of crossing the meridian.  In this */
   /* case we need to change the value near 360 to be a negative number    */
   /* and convert values in subsequent checking the same way.              */

   /* We know the rough scale of the image from naxis*cdelt.  We can scale */
   /* this to a longitude range based on the larger of cos(latmin) and     */
   /* cos(latmax).  If this is still substantially smaller than the lon    */
   /* min/max spread, we will do the conversions.                          */

   neglon = 0;

   coslatmin = cos(latmin*dtr);
   coslatmax = cos(latmax*dtr);

   coslat = coslatmin;

   if(coslatmax < coslat)
      coslat = coslatmax;

   lonrange = 360.;
  
   if(coslat > 0.)
      lonrange = imgsize / coslat;

   if(gdebug)
   {
      printf("mViewer_makeGrid> cos(lat): %-g  lonrange: %-g\n", coslat, lonrange);
      fflush(stdout);
   }

   if(lonrange < 180. && (lonmax - lonmin) > 180.)
   {
      neglon = 1;

      tmpval = lonmax;
      lonmax = lonmin;
      lonmin = tmpval - 360.;

      if(gdebug)
      {
         printf("mViewer_makeGrid> Scale check  Lon: %8.4f -> %8.4f\n", lonmin, lonmax);
         fflush(stdout);
      }
   }


   /* Walk around the image border, finding the exact range of lon/lat */
   /* We also need to detect big jumps in the position and switch to   */
   /* "all-sky" mode.                                                  */

   /* First point (for reference) */

   xpix = -0.5;
   ypix = -0.5;

   side = 0;
   i    = 0;

   pix2wcs(wcs, xpix, ypix, &lonprev, &latprev);

   if(convert)
   {
      convertCoordinates(csysimg,  epochimg,  lonprev, latprev,
                         csysgrid, epochgrid, &reflon, &reflat, 0.0);

      lonprev = reflon;
      latprev = reflat;
   }

   if(neglon && lonprev > 180.)
      lonprev -= 360.;


   /* convert to sysgrid */

   x1 = cos(latprev*dtr) * cos(lonprev*dtr);
   y1 = cos(latprev*dtr) * sin(lonprev*dtr);
   z1 = sin(latprev*dtr);


   /* Scale equal to 10 pixels worth of distance on the sky */
   
   dotmax = cos(10.*wcs->cdelt[0]*dtr);

   if(fabs(wcs->cdelt[1]) > fabs(wcs->cdelt[0]))
      dotmax = cos(10.*wcs->cdelt[1]*dtr);


   meridian = 0;
   
   while(1)
   {
      /* Get pixel coordinates on the sky */
      
      pix2wcs(wcs, xpix, ypix, &lon, &lat);

      if(convert)
      {
         convertCoordinates(csysimg,  epochimg,  lon, lat,
                            csysgrid, epochgrid, &reflon, &reflat, 0.0);

         lon = reflon;
         lat = reflat;
      }

      if(neglon && lon > 180.)
         lon -= 360.;
      
      x2 = cos(lat*dtr) * cos(lon*dtr);
      y2 = cos(lat*dtr) * sin(lon*dtr);
      z2 = sin(lat*dtr);


      /* Check for big jumps */

      if(fabs(x1*x2 + y1*y2 + z1*z2) < dotmax)
      {
         lonmin =   0.;
         lonmax = 360.;
         latmin = -90.;
         latmax =  90.;

         if(gdebug)
         {
            printf("\nmViewer_makeGrid> pixel jump (side %d, pixel %d) is %-g (compared to CDELT value of %-g)\n\n",
               side, i, acos(fabs(x1*x2 + y1*y2 + z1*z2)) / dtr, acos(dotmax)/dtr);
            fflush(stdout);
         }

         break;
      }

      
      /* Check for meridian crossing */

      if(fabs(lon - lonprev) > 180.)
         meridian = 1;


      /* Shift forward to the next point */

      x1 = x2;
      y1 = y2;
      z1 = z2;

      lonprev = lon;
         
      
      /* Update min/max if appropriate */

      if(lon < lonmin) lonmin = lon;
      if(lon > lonmax) lonmax = lon;
      if(lat < latmin) latmin = lat;
      if(lat > latmax) latmax = lat;

      ++i;


      /* Switch sides as we go around */

      if(side == 0)       /* Up */
      {
         ypix += 1.0;

         if(ypix > nl+0.5)
         {
            ypix -= 1.0;
            side = 1;
            i = 0;
         }
      }

      else if(side == 1)  /* Across */
      {
         xpix += 1.0;

         if(xpix > ns+0.5)
         {
            xpix -= 1.0;
            side = 2;
            i = 0;
         }
      }

      else if(side == 2)  /* Down */
      {
         ypix -= 1.0;

         if(ypix < -0.5)
         {
            ypix += 1.0;
            side = 3;
            i = 0;
         }
      }

      else if(side == 3)  /* Back */
      {
         xpix -= 1.0;

         if(xpix < -0.5)
            break;
      }
   }

   if(gdebug)
   {
      printf("\n");
      printf("mViewer_makeGrid> Edge check   Lon: %8.4f -> %8.4f\n", lonmin, lonmax);
      printf("mViewer_makeGrid> Edge check   Lat: %8.4f -> %8.4f\n", latmin, latmax);
      printf("\n");
      printf("mViewer_makeGrid> meridian:         %d\n", meridian);
      printf("\n");
      fflush(stdout);
   }


   /* Now check to see if a pole is included in the image */

   ipole = NEITHER;

   reflon =  0.;
   reflat = 90.;

   if(convert)
      convertCoordinates(csysgrid, epochgrid,  0., 90.,
                         csysimg,  epochimg, &reflon, &reflat, 0.0);

   wcs2pix(wcs, reflon, reflat, &xpix, &ypix, &offscl);

   if(hpx) mViewer_hpxCheck(&offscl, &xpix, &ypix);

   if(!offscl && !mNaN(xpix) && !mNaN(ypix))
   {
      lonmin =   0.;
      lonmax = 360.;
      latmax =  90.;

      ipole = NORTH;
   }

   reflon =   0.;
   reflat = -90.;

   if(convert)
      convertCoordinates(csysgrid, epochgrid,  0., -90.,
                         csysimg,  epochimg, &reflon, &reflat, 0.0);

   wcs2pix(wcs, reflon, reflat, &xpix, &ypix, &offscl);

   if(hpx) mViewer_hpxCheck(&offscl, &xpix, &ypix);

   if(!offscl && !mNaN(xpix) && !mNaN(ypix))
   {
      lonmin =   0.;
      lonmax = 360.;
      latmin = -90.;

      if(ipole)
         ipole = BOTH;
      else
         ipole = SOUTH;
   }


   // Kludge for (mostly) Aitoff: We need to make the min lon slightly
   // greater than zero so the longitude line for zero doesn't get drawn
   // at the same location as 360.

   if(gdebug)
   {
      if(ipole == NEITHER)  printf("mViewer_makeGrid> Pole: NEITHER\n\n");
      if(ipole == NORTH  )  printf("mViewer_makeGrid> Pole: NORTH  \n\n");
      if(ipole == SOUTH  )  printf("mViewer_makeGrid> Pole: SOUTH  \n\n");
      if(ipole == BOTH   )  printf("mViewer_makeGrid> Pole: BOTH   \n\n");

      printf("mViewer_makeGrid> After pole check   Lon: %8.4f -> %8.4f\n", lonmin, lonmax);
      printf("mViewer_makeGrid>                    Lat: %8.4f -> %8.4f\n", latmin, latmax);
      printf("\n");
      fflush(stdout);
   }


   /* From the range, we can determine the proper grid spacing    */
   /* and then the locations of the first and the last grid lines */


   /* LON */

   lon_range =lonmax - lonmin;

   lon_space = grid_space[0];

   for(i=1; i<ngrid; ++i)
   {
      if(lon_range <= grid_range[i])
      {
         lon_space = grid_space[i];

         if(gdebug)
            printf("mViewer_makeGrid> lon grid index = %d\n", i);
            
         break;
      }
   }

   if(lonmin >= 0)
      lon0 = ((int)(lonmin/lon_space)+1.)*lon_space;
   else
      lon0 = (int)(lonmin/lon_space)*lon_space;

   lonn = (int)(lonmax/lon_space)*lon_space;

   nlon = floor((lonn-lon0)/lon_space+0.5) + 1;


   /* LAT */

   lat_range =latmax - latmin;

   lat_space = grid_space[0];

   for(i=1; i<ngrid; ++i)
   {
      if(lat_range <= grid_range[i])
      {
         lat_space = grid_space[i];

         if(gdebug)
            printf("mViewer_makeGrid> lat grid index = %d\n", i);

         break;
      }
   }

   if(latmin >= 0)
      lat0 = (int)(latmin/lat_space+1.)*lat_space;
   else
      lat0 = (int)(latmin/lat_space)*lat_space;

   if(lat0 < latmin) lat0 += lat_space;


   latn = (int)(latmax/lat_space)*lat_space;

   if(latn > latmax) latn -= lat_space;

   if(latmax ==  90.) latn =  90. - lat_space;
   if(latmin == -90.) lat0 = -90. + lat_space;

   nlat = floor((latn-lat0)/lat_space+0.5) + 1;


   if(gdebug)
   {
      printf("mViewer_makeGrid> lon_range = %8.4f\n", lon_range);
      printf("mViewer_makeGrid> lon_space = %8.4f\n", lon_space);
      printf("\n");
      printf("mViewer_makeGrid> lat_range = %8.4f\n", lat_range);
      printf("mViewer_makeGrid> lat_space = %8.4f\n", lat_space);
      printf("\n");
      fflush(stdout);
   
      printf("mViewer_makeGrid> Longitude lines from lon0 = %8.4f to lonn = %8.4f by %8.4f (%d lines)\n", lon0, lonn, lon_space, nlon);
      printf("mViewer_makeGrid> Latitude  lines from lat0 = %8.4f to latn = %8.4f by %8.4f (%d lines)\n", lat0, latn, lat_space, nlat);
      fflush(stdout);
   }

   midlon = lon0 + (((nlon-1)/2) + 0.5) * lon_space;
   midlat = lat0 +  ((nlat-1)/2) * lat_space;

   if(gdebug)
   {
      printf("mViewer_makeGrid> longitude lines labeled at latitude  %8.4f (midpoint lon is number %d)\n", midlat, nlat/2);
      printf("mViewer_makeGrid> latitude  lines labeled at longitude %8.4f (midpoint lat is number %d)\n", midlon, nlon/2);
      fflush(stdout);
   }


   /* Which longitude labels? */

   nlab_lon = nlon;

   ispace_lon = 1;

   if(gdebug)
   {
      printf("mViewer_makeGrid> nlon = %d,  nlab_lon = %d,  ispace_lon = %d\n", 
         nlon, nlab_lon, ispace_lon);
      fflush(stdout);
   }

   ilon_label = (nlat+1)/2;

   if ((ilon_label % 2 == 1) && (ilon_label > 1)) 
   {
      if (ipole == 1) 
         ilon_label++;
      else
         ilon_label--;
   }

   if (ilon_label <      0) ilon_label = 0;
   if (ilon_label > nlat-1) ilon_label = nlat-1;

   if(gdebug)
   {
      printf("mViewer_makeGrid> ilon_label = %d\n", ilon_label);
      fflush(stdout);
   }


   /* Which latitude labels */

   nlab_lat = nlat;

   ispace_lat = 1;

   if(gdebug)
   {
      printf("mViewer_makeGrid> nlat = %d,  nlab_lat = %d,  ispace_lat = %d\n", 
         nlat, nlab_lat, ispace_lat);
      fflush(stdout);
   }


   /* Generate the longitude labels */

   lbllat = midlat;

   use = 0;

   for(i=0; i<nlon; ++i)
   {
      ++use;

      lbllon = lon0 + i*lon_space;

      if(lbllon < 0.)
         lbllon += 360.;

      if(use == ispace_lon)
      {
         if(gdebug)
         {
            printf("mViewer_makeGrid> lon label \"%s\" at %.5f %.5f\n",
               mViewer_longitude_label(lbllon, ishr), lbllon, lbllat);
            fflush(stdout);
         }

         mViewer_coord_label(fontfile, fontsize, 
                     lbllon, lbllat, mViewer_longitude_label(lbllon, ishr),
                     csysimg, epochimg, csysgrid, epochgrid,
                     red, green, blue);

         use = 0;
      }
   }


   /* Determine whether we need to shift the lat labels by 1 lat line */

   used_lat = 0;

   use = 0;

   for(i=0; i<nlat; ++i)
   {
      ++use;

      lbllat =lat0 + i*lat_space;

      if(use == ispace_lat)
      {
         if(lbllat == midlat)
         {
            used_lat = 1;
            break;
         }

         use = 0;
      }
   }

   use = 0;
   if(used_lat)
      use = 1;


   /* Generate the latitude labels */

   lbllon = midlon;

   for(i=0; i<nlat; ++i)
   {
      ++use;

      lbllat =lat0 + i*lat_space;

      if(lbllat == midlat)
         continue;

      if(use >= ispace_lat)
      {
         if(gdebug)
         {
            printf("mViewer_makeGrid> lat label \"%s\" at %.5f %.5f\n",
               mViewer_latitude_label(lbllat), lbllon, lbllat);
            fflush(stdout);
         }

         mViewer_coord_label(fontfile, fontsize, 
                             lbllon, lbllat, mViewer_latitude_label(lbllat),
                             csysimg, epochimg, csysgrid, epochgrid,
                             red, green, blue);
   
         if(gdebug) {
            printf("mViewer_makeGrid> returned mViewer_coord_label\n");
            fflush(stdout);
         }


         use = 0;
      }
   }

   if(gdebug) {
      printf("mViewer_makeGrid> starting longitude lines\n");
      fflush(stdout);
   }


   /* Generate the longitude lines */

   for(lon=lon0; lon<=lonn+lon_space; lon+=lon_space)
   {
      if(gdebug) {
         printf("mViewer_makeGrid> lon= [%lf]: call mViewer_longitude_line\n", lon);
         fflush(stdout);
      }

      mViewer_longitude_line(lon, latmin, latmax, 
                             csysimg, epochimg, csysgrid, epochgrid,
                             red, green, blue, linewidth);
      
      if(gdebug) {
         printf("mViewer_makeGrid> returned mViewer_longitude_line\n");
         fflush(stdout);
      }

   }

   if(gdebug) {
      printf("mViewer_makeGrid> starting latitude lines\n");
      fflush(stdout);
   }


   /* Generate the latitude lines */

   for(lat=lat0; lat<=latn+lat_space; lat+=lat_space)
   {
      if(gdebug) {
         printf("mViewer_makeGrid> lat= [%lf]: call mViewer_latitude_line\n", lat);
         fflush(stdout);
      }

      mViewer_latitude_line(lat, lonmin, lonmax, 
                            csysimg, epochimg, csysgrid, epochgrid,
                            red, green, blue, linewidth);
      
      if(gdebug) {
         printf("mViewer_makeGrid> returned mViewer_latitude_line\n");
         fflush(stdout);
      }

   }
   
   if(gdebug) {
      printf("mViewer_makeGrid> end of mViewer_makeGrid()\n");
      fflush(stdout);
   }


   // Aitoff projection is used often enough that
   // we have put in special code to draw the bounding
   // ellipse around the area where the map is defined.

   if(wcs->prjcode == WCS_AIT)
      mViewer_draw_boundary(red, green, blue, linewidth);
}


/**************************************************************/
/*                                                            */
/* Generate a sexegesimal latitude label.  Remove unnecessary */
/* trailing "zeros" (e.g. "15d" instead of "15d 00m 00.0s")   */
/*                                                            */
/**************************************************************/

char *mViewer_latitude_label(double lat)
{
   int    isign, ideg, imin;
   double val;
   char   dstr[16];
   char   mstr[16];
   char   sstr[16];
   char  *ptr;

   static char label[32];

   strcpy(label, "");

   val = lat;

   isign = 0;
   if(val < 0.)
   {
      isign = 1;
      val = -val;
   }

   ideg = val;
   sprintf(dstr, "%d", ideg);
   val = val - ideg;

   val = val*60.;
   imin = val;
   sprintf(mstr, "%02d", imin);
   val = val - imin;

   val = val * 60.;
   sprintf(sstr, "%05.2f", val);

   if(strcmp(sstr, "60.00") == 0)
   {
      strcpy(sstr, "00.00");
      ++imin;
      sprintf(mstr, "%02d", imin);
   }

   if(strcmp(mstr, "60") == 0)
   {
      strcpy(mstr, "00");
      ++ideg;
      sprintf(dstr, "%d", ideg);
   }

   ptr = sstr + strlen(sstr) - 1;

   while(*ptr == '0')
   {
      *ptr = '\0';
      --ptr;
   }

   if(*ptr == '.')
   {
      *ptr = '\0';
      --ptr;
   }

   if(strcmp(sstr, "00") == 0)
   {
      sstr[0] = '\0';

      if(strcmp(mstr, "00") == 0)
         mstr[0] = '\0';
   }

   if(isign)
      strcat(label, "-");
   else
      strcat(label, "+");

   strcat(label, dstr);
   strcat(label, "d");

   if(strlen(mstr) > 0)
   {
      strcat(label, " ");
      strcat(label, mstr);
      strcat(label, "m");
   }
   else if(strlen(sstr) > 0)
      strcat(label, " 00m");


   if(strlen(sstr) > 0)
   {
      strcat(label, " ");
      strcat(label, sstr);
      strcat(label, "s");
   }

   return(label);
}



/***************************************************************/
/*                                                             */
/* Generate a sexegesimal longitude label.  Remove unnecessary */
/* trailing "zeros" (e.g. "5h" instead of "5h 00m 00.0s")      */
/*                                                             */
/***************************************************************/

char *mViewer_longitude_label(double lon, int ishr)
{
   int    ihr, imin;
   double val;
   char   hstr[16];
   char   mstr[16];
   char   sstr[16];
   char  *ptr;

   static char label[32];

   strcpy(label, "");

   val = lon;

   if(ishr)
      val = val / 15.;

   ihr = val;
   sprintf(hstr, "%d", ihr);
   val = val - ihr;

   val = val*60.;
   imin = val;
   sprintf(mstr, "%02d", imin);
   val = val - imin;

   val = val * 60.;
   sprintf(sstr, "%05.2f", val);

   if(strcmp(sstr, "60.00") == 0)
   {
      strcpy(sstr, "00.00");
      ++imin;
      sprintf(mstr, "%02d", imin);
   }

   if(strcmp(mstr, "60") == 0)
   {
      strcpy(sstr, "00");
      ++ihr;
      sprintf(hstr, "%d", ihr);
   }

   ptr = sstr + strlen(sstr) - 1;

   while(*ptr == '0')
   {
      *ptr = '\0';
      --ptr;
   }

   if(*ptr == '.')
   {
      *ptr = '\0';
      --ptr;
   }

   if(strcmp(sstr, "00") == 0)
   {
      sstr[0] = '\0';

      if(strcmp(mstr, "00") == 0)
         mstr[0] = '\0';
   }

   strcat(label, hstr);

   if(ishr)
      strcat(label, "h");
   else
      strcat(label, "d");

   if(strlen(mstr) > 0)
   {
      strcat(label, " ");
      strcat(label, mstr);
      strcat(label, "m");
   }
   else if(strlen(sstr) > 0)
      strcat(label, " 00m");


   if(strlen(sstr) > 0)
   {
      strcat(label, " ");
      strcat(label, sstr);
      strcat(label, "s");
   }

   return(label);
}



/*****************************************************/
/*                                                   */
/* Draw a great circle segment between two locations */
/*                                                   */
/*****************************************************/

void mViewer_great_circle(struct WorldCoor *wcs, int flipY,
                          int csysimg,  double epochimg, 
                          int csys, double epoch,  
                          double lon0,  double lat0, double lon1, double lat1,
                          double red,   double green, double blue, double linewidth)
{
   int     i, n, convert, offscl, ncurve;
   
   double *xcurve, *ycurve;

   double  cos_lat0, sin_lat0, cos_lat1, sin_lat1, sin_dl;
   double  x0, y0, z0;
   double  x, y, z;

   double  lon, lat;
   double  reflon, reflat;
   double  xpix, ypix;
   double  cos_theta_max, sin_theta_max;
   double  sin_phi, cos_phi;
   double  sina, cosa;

   double  theta_max, sz, arc;

   double  dtr;

   dtr = atan(1.)/45.;
   
   convert = 0;
   if(csys != csysimg || epoch != epochimg)
      convert = 1;

   cos_lat0 = cos(lat0*dtr);
   sin_lat0 = sin(lat0*dtr);
   cos_lat1 = cos(lat1*dtr);
   sin_lat1 = sin(lat1*dtr);

   sin_dl = sin((lon1-lon0)*dtr);

   x0 = cos(lon0*dtr)*cos_lat0;
   y0 = sin(lon0*dtr)*cos_lat0;
   z0 = sin_lat0;

   x  = cos(lon1*dtr)*cos_lat1;
   y  = sin(lon1*dtr)*cos_lat1;
   z  = sin_lat1;

   cos_theta_max = x0*x + y0*y + z0*z;

   if(cos_theta_max > 1.)
      cos_theta_max = 1.;

   if(cos_theta_max < -1.)
      cos_theta_max = -1.;

   sin_theta_max = sqrt(1.-cos_theta_max*cos_theta_max);

   sin_phi = (sin_dl*cos_lat1)/sin_theta_max;
   cos_phi = (sin_lat1-cos_theta_max*sin_lat0) / (sin_theta_max*cos_lat0);

   theta_max = acos(cos_theta_max)/dtr;

   sz = fabs(wcs->cdelt[1] * 0.5);

   if (theta_max < sz)
      n = 2;
   else 
      n = (int)(theta_max/sz) + 1;

   if(n <= 0)
      return;
   
   xcurve = (double *)malloc(n * sizeof(double));
   ycurve = (double *)malloc(n * sizeof(double));

   ncurve = 0;

   for (i=0; i<n; ++i) 
   {
      if (i == n-1)
         arc = theta_max;
      else
         arc = i * sz;

      cosa = cos(arc*dtr);
      sina = sin(arc*dtr);

      reflat = asin(cos_phi*sina*cos_lat0 + cosa*sin_lat0)/dtr;

      reflon = lon0 + atan2(sina*sin_phi, cosa*cos_lat0 - sin_lat0*cos_phi*sina)/dtr;

      if(convert)
      {
         convertCoordinates(csys,  epoch,  reflon, reflat,
            csysimg, epochimg, &lon, &lat, 0.0);
         
         reflon = lon;
         lat = reflat = lat;
      }

      offscl = 0;
      wcs2pix(wcs, reflon, reflat, &xpix, &ypix, &offscl);

      if(hpx) mViewer_hpxCheck(&offscl, &xpix, &ypix);

      if(!offscl && !mNaN(xpix) && !mNaN(ypix))
      {
         if(!flipY || wcs->imflip)
         {
            xcurve[ncurve] = xpix;
            ycurve[ncurve] = wcs->nypix - ypix;
         }
         else
         {
            xcurve[ncurve] = xpix;
            ycurve[ncurve] = ypix;
         }

         ++ncurve;
      }
   }

   mViewer_curve(xcurve, ycurve, ncurve, red, green, blue, linewidth);

   free(xcurve);
   free(ycurve);
}



/*****************************************************/
/*                                                   */
/* Draw symbols, including a compass rose.           */
/*                                                   */
/*****************************************************/

void mViewer_symbol(struct WorldCoor *wcs, int flipY,
                    int csysimg,  double epochimg,
                    int csyssym, double epochsym,   
                    double inlon,  double inlat, int inpix,
                    double radius, int symnpnt, int symmax, int symtype, double symang,
                    double red,   double green, double blue, double linewidth)
{
   int    i, k;
   double cosc, colat, sina, dlon, vang, dvang, vangmax, lat, lon;
   double rad, type, lonprev, latprev;
   double xpix, ypix;
   double clon, clat;
   int    naxis1, naxis2;

   double x1, y1;
   double x2, y2;

   double lon1, lat1;
   double lon2, lat2;

   double dtr;

   dtr = atan(1.)/45.;

   clon = inlon;
   clat = inlat;

   xpix = inlon;
   ypix = inlat;

   if(inpix)
   {
      naxis1 = wcs->nxpix;
      naxis2 = wcs->nypix;

      while(xpix < 0.)     xpix += naxis1;
      while(xpix > naxis1) xpix -= naxis1;

      while(ypix < 0.)     ypix += naxis2;
      while(ypix > naxis2) ypix -= naxis2;

      pix2wcs(wcs, xpix, ypix, &clon, &clat);
   }

   lon = clon;
   lat = clat;

   type = symtype % 4;


   /* POLYGON */

   if(type == 0)
   {
      dvang = 360. / symnpnt;

      if(symmax == 0)
         vangmax = 180.+symang+0.001;
      else
         vangmax = -180.+dvang*symmax+symang+0.001;

      for(vang=-180.+symang; vang<=vangmax; vang += dvang)
      {
         cosc = cos(radius*dtr) * cos((90. - clat)*dtr) - sin(radius*dtr) * sin((90. - clat)*dtr) * cos(vang*dtr);

         colat = acos(cosc) / dtr;

         sina = sin(radius*dtr) * sin(vang*dtr) / sin(colat*dtr);

         dlon = asin(sina)/dtr;

         lat = 90. - colat;

         lon = clon + dlon;

         if(vang > -180.+symang)
            mViewer_great_circle(wcs, flipY, csysimg, epochimg, csyssym, epochsym, lonprev, latprev, lon, lat, red, green, blue, linewidth);

         lonprev = lon;
         latprev = lat;
      }
   }


   /* STARRED */

   else if(type == 1)
   {
      dvang = 360. / symnpnt / 2.;

      k = 0;

      if(symmax == 0)
         vangmax = 180.+symang+0.001;
      else
         vangmax = -180.+2.*dvang*symmax+symang+0.001;

      for(vang=-180.+symang; vang<=vangmax; vang += dvang)
      {
         if(k)
            rad = radius / 2.;
         else
            rad = radius;

         cosc = cos(rad*dtr) * cos((90. - clat)*dtr) - sin(rad*dtr) * sin((90. - clat)*dtr) * cos(vang*dtr);

         colat = acos(cosc) / dtr;

         sina = sin(rad*dtr) * sin(vang*dtr) / sin(colat*dtr);

         dlon = asin(sina)/dtr;

         lat = 90. - colat;

         lon = clon + dlon;

         if(vang > -180.+symang)
            mViewer_great_circle(wcs, flipY, csysimg, epochimg, csyssym, epochsym, lonprev, latprev, lon, lat, red, green, blue, linewidth);

         lonprev = lon;
         latprev = lat;

         k = (k+1) % 2;
      }
   }


   /* SKELETAL */

   else if(type == 2)
   {
      dvang = 360. / symnpnt;

      if(symmax == 0)
         vangmax = 180.+symang+0.001;
      else
         vangmax = -180.+dvang*symmax+symang+0.001;

      for(vang=-180.+symang; vang<=vangmax; vang += dvang)
      {
         cosc = cos(radius*dtr) * cos((90. - clat)*dtr) - sin(radius*dtr) * sin((90. - clat)*dtr) * cos(vang*dtr);

         colat = acos(cosc) / dtr;

         sina = sin(radius*dtr) * sin(vang*dtr) / sin(colat*dtr);

         dlon = asin(sina)/dtr;

         lat = 90. - colat;

         lon = clon + dlon;

         if(vang > -180.+symang)
            mViewer_great_circle(wcs, flipY, csysimg, epochimg, csyssym, epochsym, clon, clat, lon, lat, red, green, blue, linewidth);
      }
   }


   /* COMPASS ROSE */

   else if(type == 3)
   {
      for(i=0; i<9; ++i)
      {
         y1 = -compass_segments[i][0];
         x1 = -compass_segments[i][1];

         vang = atan2(y1, x1) / dtr;

         rad  = radius * sqrt(x1*x1 + y1*y1)/12.;

         cosc = cos(rad*dtr) * cos((90. - clat)*dtr) - sin(rad*dtr) * sin((90. - clat)*dtr) * cos(vang*dtr);

         colat = acos(cosc) / dtr;

         sina = sin(rad*dtr) * sin(vang*dtr) / sin(colat*dtr);

         dlon = asin(sina)/dtr;

         lat1 = 90. - colat;

         lon1 = clon + dlon;


         y2 = -compass_segments[i][2];
         x2 = -compass_segments[i][3];


         vang = atan2(y2, x2) / dtr;

         rad  = radius * sqrt(x2*x2 + y2*y2)/12.;

         cosc = cos(rad*dtr) * cos((90. - clat)*dtr) - sin(rad*dtr) * sin((90. - clat)*dtr) * cos(vang*dtr);

         colat = acos(cosc) / dtr;

         sina = sin(rad*dtr) * sin(vang*dtr) / sin(colat*dtr);

         dlon = asin(sina)/dtr;

         lat2 = 90. - colat;

         lon2 = clon + dlon;

         mViewer_great_circle(wcs, flipY, csysimg, epochimg, csyssym, epochsym, lon1, lat1, lon2, lat2, red, green, blue, linewidth);
      }
   }
}



/*****************************************************/
/*                                                   */
/* A drawing defined in a file.                      */
/*                                                   */
/*****************************************************/

void mViewer_drawing(char *filename, int flipY, struct WorldCoor *wcs, int csysimg,  double epochimg, char *fontfile)
{
   int    i, n, csys, offscl;
   int    cmdc;
   char  *cmdv[256];

   FILE  *fdraw;

   static double epoch = 2000.;

   double alpha = 1.;

   static double xprev = 0.;
   static double yprev = 0.;

   static double xpos, ypos;
   static double xpix, ypix;

   static double red   = 255.;
   static double green = 255.;
   static double blue  = 255.;
   
   static double linewidth = 3.;
   static double fontsize  = 1.;
   static double fontscale = 1.;

   static double lon = 0.;
   static double lat = 0.;

   static double *xpoly = (double *)NULL;
   static double *ypoly = (double *)NULL;

   static int    ref = JULIAN;

   static int    npoly;

   static int    ix = 0;
   static int    iy = 0;

   static char   line     [MAXSTR];
   static char   subline  [MAXSTR];
   static char   xstr     [MAXSTR];
   static char   ystr     [MAXSTR];
   static char   labeltext[MAXSTR];

   static char   *end;


   fdraw = fopen(filename, "r");

   if(fdraw == (FILE *)NULL)
      return;

   while(1)
   {
      if(fgets(line, MAXSTR, fdraw) == (char *)NULL)
         break;

      if(line[strlen(line) - 1] == '\n')
         line[strlen(line) - 1] =  '\0';

      if(gdebug)
      {
         printf("mViewer_drawing> read: [%s]\n", line);
         fflush(stdout);
      }

      if(line[0] == '#')
         continue;

      if(strlen(line) > 6)
         strcpy(subline, line+6);
      else
         strcpy(subline, "");

      cmdc = parsecmd(line, cmdv);

      if(cmdc <= 0)
         continue;

      if(gdebug)
      {
         printf("mViewer_drawing> cmdc: %d, cmdv[0]: [%s], subline: [%s]\n", cmdc, cmdv[0], subline);
         fflush(stdout);
      }

      
      // ALPHA command

      if(strcasecmp(cmdv[0], "alpha") == 0)
      {
         if(cmdc < 2)
            break;

         alpha  = atof(cmdv[1]);

         if(gdebug)
         {
            printf("mViewer_drawing> DRAW> ALPHA %-g\n", alpha);
            fflush(stdout);
         }
      }

      
      // COLOR command

      if(strcasecmp(cmdv[0], "color") == 0)
      {
         if(cmdc < 4)
            break;

         blue  = atof(cmdv[1]);
         green = atof(cmdv[2]);
         red   = atof(cmdv[3]);

         if(gdebug)
         {
            printf("mViewer_drawing> DRAW> COLOR %-g %-g %-g\n", blue, green, red);
            fflush(stdout);
         }
      }

      
      // LINEWIDTH command
      
      if(strcasecmp(cmdv[0], "linewidth") == 0
      || strcasecmp(cmdv[0], "width")     == 0)
      {
         if(cmdc < 2)
            break;

         linewidth = atof(cmdv[1]);

         if(gdebug)
         {
            printf("mViewer_drawing> DRAW> LINEWIDTH %-g\n", linewidth);
            fflush(stdout);
         }
      }


      // MOVE command (pixel coords)
      
      if(strcasecmp(cmdv[0], "move") == 0)
      {
         if(cmdc < 3)
            break;

         xprev = atof(cmdv[1]);
         yprev = atof(cmdv[2]);

         if(gdebug)
         {
            printf("mViewer_drawing> DRAW> MOVE %-g %-g\n", xprev, yprev);
            fflush(stdout);
         }
      }


      // DRAW command (pixel coords)
      
      if(strcasecmp(cmdv[0], "draw") == 0)
      {
         if(cmdc < 3)
            break;

         xpix = atof(cmdv[1]);
         ypix = atof(cmdv[2]);

         mViewer_smooth_line(xprev, yprev, xpix, ypix, red, green, blue, linewidth);

         xprev = xpix;
         yprev = ypix;

         if(gdebug)
         {
            printf("mViewer_drawing> DRAW> DRAW %-g %-g\n", xprev, yprev);
            fflush(stdout);
         }

         mViewer_addOverlay(alpha);
      }


      // CSYS command
      
      if(strcasecmp(cmdv[0], "csys") == 0)
      {
         if(cmdc < 2)
            break;

         ref = JULIAN;

         csys  = EQUJ;
         epoch = -999.;

         if(cmdc > 2)
         {
            if(cmdv[2][0] == 'j' || cmdv[2][0] == 'J')
            {
               ref = JULIAN;
               epoch = atof(cmdv[2]+1);
            }

            else if(cmdv[2][0] == 'b' || cmdv[2][0] == 'B')
            {
               ref = BESSELIAN;
               epoch = atof(cmdv[2]+1);
            }
         }

         if(strncasecmp(cmdv[1], "eq", 2) == 0)
         {
            if(ref == BESSELIAN)
               csys = EQUB;
            else
               csys = EQUJ;
         }

         else if(strncasecmp(cmdv[1], "ec", 2) == 0)
         {
            if(ref == BESSELIAN)
               csys = ECLB;
            else
               csys = ECLJ;
         }

         else if(strncasecmp(cmdv[1], "ga", 2) == 0)
            csys = GAL;

         ++i;

         if(epoch == -999.)
            epoch = 2000.;

         if(gdebug)
         {
            printf("mViewer_drawing> DRAW> CSYS: csys=%d epoch=%-g\n", csys, epoch);
            fflush(stdout);
         }
      }


      // COORD command
      
      if(strcasecmp(cmdv[0], "coord") == 0)
      {
         if(cmdc < 3)
            break;

         lon = atof(cmdv[1]);
         lat = atof(cmdv[2]);

         if(gdebug)
         {
            printf("mViewer_drawing> DRAW> COORD: %-g %-g\n", lon, lat);
            fflush(stdout);
         }
      }


      // FONTSCALE command
      
      if(strcasecmp(cmdv[0], "fontscale") == 0)
      {
         if(cmdc < 2)
            break;

         fontscale = atof(cmdv[1]);

         if(gdebug)
         {
            printf("mViewer_drawing> DRAW> FONTSCALE %-g\n", fontscale);
            fflush(stdout);
         }
      }


      // LABEL command
      
      if(strcasecmp(cmdv[0], "label") == 0)
      {
         if(cmdc < 2)
            break;

         strcpy(labeltext, subline);

         convertCoordinates(csys, epoch, lon, lat,
                             csysimg,  epochimg, &xpos, &ypos, 0.0);

         wcs2pix(wcs, xpos, ypos, &xpix, &ypix, &offscl);

         if(hpx) mViewer_hpxCheck(&offscl, &xpix, &ypix);

         ix = xpix;
         iy = ypix;

         fontsize = (int)(14. * fontscale);

         if(fontsize < 1)
            fontsize = 1;

         mViewer_draw_label(fontfile, fontsize, ix, iy, labeltext, red, green, blue);

         if(gdebug)
         {
            printf("mViewer_drawing> DRAW> LABEL  \"%s\" at pixel %d %d\n", labeltext, ix, iy);
            fflush(stdout);
         }

         mViewer_addOverlay(alpha);
      }


      // POLYLINE command (great circle segments)
      //
      if(strncasecmp(cmdv[0], "polyline", 4) == 0)
      {
         if(cmdc < 2)
            break;

         npoly = atoi(cmdv[1]);

         if(xpoly != (double *)NULL)
            free(xpoly);

         if(ypoly != (double *)NULL)
            free(ypoly);

         xpoly = (double *)malloc(npoly * sizeof(double));
         ypoly = (double *)malloc(npoly * sizeof(double));

         n = 0;

         for(i=0; i<npoly; ++i)
         {
            if(fgets(line, MAXSTR, fdraw) == (char *)NULL)
               break;

            sscanf(line, "%s %s", xstr, ystr);

            xpoly[i] = strtod(xstr, &end);

            if(end < xstr + strlen(xstr))
               break;

            ypoly[i] = strtod(ystr, &end);

            if(end < ystr + strlen(ystr))
               break;

            ++n;
         }

         for(i=0; i<n-1; ++i)
         {
            mViewer_great_circle(wcs, flipY, csysimg,  epochimg, csys, epoch, 
                                 xpoly[i], ypoly[i], xpoly[i+1], ypoly[i+1],
                                 red, green, blue, linewidth);
            if(gdebug)
            {
               printf("mViewer_drawing> DRAW> POLYLINE %d: %-g %-g -> %-g %-g\n", i, xpoly[i], ypoly[i], xpoly[i+1], ypoly[i+1]);
               fflush(stdout);
            }
         }

         mViewer_addOverlay(alpha);
      }
   }

   fclose(fdraw);
}
