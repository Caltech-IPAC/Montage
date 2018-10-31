/* Module: mMakeHdr.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
3.1      John Good        22Sep15  Add full cube info to template header
3.0      John Good        29Jul15  Check for cube columns and do a little analysis
2.7      John Good        27Sep12  Add max pixel size capability
2.6      John Good        20Aug07  Add 'table of tables' capability
2.5      John Good        07Aug07  Added check for "allsky" (large area) data
2.4      John Good        09Jul06  Exit properly when a bad WCS encountered
2.3      John Good        22Feb06  Removed extra (repeated) header lines
2.2      John Good        17Nov05  Added flag (-p) to externally set pixel scale
2.1      John Good        30Nov04  Forgot to check for pixel scale when
                                   using four corners (or lat,lon).
2.0      John Good        16Aug04  Added code to alternately check:
                                   four corners (equatorial) or four corners
                                   (arbitrary with system in header) or
                                   just a set of ra, dec or just a set of
                                   lon, lat (with system in header)
1.9      John Good        10Aug04  Added four corners of region to output
1.8      John Good        09Jan04  Fixed realloc() bug for 'lats'
1.7      John Good        25Nov03  Added extern optarg references
1.6      John Good        01Oct03  Add check for naxis1, naxis2 columns
                                   in addition to ns, nl
1.5      John Good        25Aug03  Added status file processing
1.4      John Good        27Jun03  Added a few comments for clarity
1.3      John Good        09Apr03  Removed unused variable offscl
1.2      John Good        22Mar03  Renamed wcsCheck to checkWCS
                                   for consistency.  Checking system
                                   and equinox strings on command line
                                   for validity
1.1      John Good        13Mar03  Added WCS header check and
                                   modified command-line processing
                                   to use getopt() library.  Check for 
                                   missing/invalid images.tbl.  Check
                                   for valid equinox and propogate to
                                   output header.
1.0      John Good        29Jan03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include <mtbl.h>
#include <fitsio.h>
#include <coord.h>
#include <wcs.h>
#include <boundaries.h>

#include <mMakeHdr.h>
#include <montage.h>

#define MAXSTR   4096
#define MAXFILES   16
#define MAXCOORD 4096

#define UNKNOWN     0
#define FOURCORNERS 1
#define WCS         2
#define LONLAT      3


struct WorldCoor *outwcs;


/* Basic image WCS information    */
/* (from the FITS header and as   */
/* returned from the WCS library) */

struct ImgInfo
{
   struct WorldCoor *wcs;
   int               sys;
   int               equinox;
   double            epoch;
   char              ctype1[16];
   char              ctype2[16];
   int               naxis1;
   int               naxis2;
   double            crpix1;
   double            crpix2;
   double            crval1;
   double            crval2;
   double            cdelt1;
   double            cdelt2;
   double            crota2;
}
   input;

int mMakeHdr_debug;


static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mMakeHdr                                                             */
/*                                                                       */
/*  Create the best header 'bounding' a table or set of tables (each     */
/*  with image metadata or point sources).                               */
/*                                                                       */
/*   char  *tblfile        Input image metadata table or source table    */
/*                         or table of tables                            */
/*                                                                       */
/*   char  *template       Output image header template                  */
/*                                                                       */
/*   char  *csys           Coordinate system (e.g. 'EquJ', 'Galactic').  */
/*                         Fairly forgiving                              */
/*                                                                       */
/*   double equinox        Coordinate system equinox (e.g. 2000.0)       */
/*                                                                       */
/*   double pixelScale     Pixel scale in degrees                        */
/*                                                                       */
/*   int    northAligned   Defaults to minimum bounding box around       */
/*                         input images.  This forces template to be     */
/*                         north-aligned                                 */
/*                                                                       */
/*   double pad            Optional extra padding around output template */
/*                                                                       */
/*   int    isPercentage   Pad is in pixels by default.  This changes    */
/*                         that to a percentage of the image size        */
/*                                                                       */
/*   int    maxPixel       Setting the pixel scale can result in really  */
/*                         big images.  This forces a maximum number     */
/*                         of pixels in NAXIS1, NAXIS2                   */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mMakeHdrReturn *mMakeHdr(char *tblfile, char *template, char *csysin, double equinox, double pixelScale, 
                                int northAligned, double pad, int isPercentage, int maxPixel, int debugin)
{
   int     i, stat, ncols, nimages, ntables, maxfiles;
   int     naxis1, naxis2, datatype, nCube;
   int     have3d, have4d;

   int     sys, ifiles;
   double  val;

   int     itable;

   int     inaxis;

   int     ictype1;
   int     ictype2;
   int     iequinox;
   int     inl;
   int     ins;
   int     icrval1;
   int     icrval2;
   int     icrpix1;
   int     icrpix2;
   int     icdelt1;
   int     icdelt2;
   int     icrota2;

   int     ilon1, ilat1;
   int     ilon2, ilat2;
   int     ilon3, ilat3;
   int     ilon4, ilat4;
   
   int     inaxis3, inaxis4;
   int     icrval3, icrval4;
   int     icrpix3, icrpix4;
   int     icdelt3, icdelt4;

   char    epochStr [MAXSTR];
   char    csysStr  [MAXSTR];
   char    csys     [MAXSTR];
   char    msg      [MAXSTR];
   char    tfile    [MAXSTR];

   char   *checkWCS;

   double  xpos, ypos;
   double  lon, lat;

   double  x, y, z;
   double  xmin, ymin, zmin;
   double  xmax, ymax, zmax;

   int     naxis,  colNaxis;

   int     naxis3, colNaxis3;
   double  crval3, colCrval3;
   double  crpix3, colCrpix3;
   double  cdelt3, colCdelt3;

   int     naxis4, colNaxis4;
   double  crval4, colCrval4;
   double  crpix4, colCrpix4;
   double  cdelt4, colCdelt4;

   double  minpix,    maxpix;
   double  colMinpix, colMaxpix;

   double  minCdelt = 360.;

   char    header[1600];
   char    temp[80];

   char   *keyval;
   char    refsys;

   double *lons, *lats;
   int     maxcoords, ncoords;

   double  lonc[4], latc[4];

   struct bndInfo *box = (struct bndInfo *)NULL;

   FILE   *fout;

   char  **fnames;

   double  dtr;

   struct mMakeHdrReturn *returnStruct;


   dtr = atan(1.)/45.;

   maxfiles = MAXFILES;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mMakeHdrReturn *)malloc(sizeof(struct mMakeHdrReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /**************************/
   /* Initialize data arrays */
   /**************************/

   fnames = (char **)malloc(maxfiles * sizeof(char *));
   
   for(i=0; i<maxfiles; ++i)
      fnames[i] = (char *)malloc(MAXSTR * sizeof(char));

   lons = (double *)malloc(MAXCOORD * sizeof(double));
   lats = (double *)malloc(MAXCOORD * sizeof(double));

   if(lats == (double *)NULL)
   {
      sprintf(returnStruct->msg, "Memory allocation failure.");
      return returnStruct;
   }

   maxcoords = MAXCOORD;
   ncoords   = 0;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mMakeHdr_debug = debugin;

   strcpy(csys, csysin);

        if(strcasecmp(csys, "eq")            == 0) sys = EQUJ;
   else if(strcasecmp(csys, "equ")           == 0) sys = EQUJ;
   else if(strcasecmp(csys, "equatorial")    == 0) sys = EQUJ;
   else if(strcasecmp(csys, "equj")          == 0) sys = EQUJ;
   else if(strcasecmp(csys, "equb")          == 0) sys = EQUB;
   else if(strcasecmp(csys, "ec")            == 0) sys = ECLJ;
   else if(strcasecmp(csys, "ecl")           == 0) sys = ECLJ;
   else if(strcasecmp(csys, "ecliptic")      == 0) sys = ECLJ;
   else if(strcasecmp(csys, "eclj")          == 0) sys = ECLJ;
   else if(strcasecmp(csys, "eclb")          == 0) sys = ECLB;
   else if(strcasecmp(csys, "ga")            == 0) sys = GAL;
   else if(strcasecmp(csys, "gal")           == 0) sys = GAL;
   else if(strcasecmp(csys, "galactic")      == 0) sys = GAL;
   else if(strcasecmp(csys, "sgal")          == 0) sys = SGAL;
   else if(strcasecmp(csys, "supergalactic") == 0) sys = SGAL;
   else
   {
      sprintf(returnStruct->msg, "Invalid system string.  Must be EQUJ|EQUB|ECLJ|ECLB|GAL|SGAL");
      return returnStruct;
   }

   if(equinox == 0. && (sys == EQUB || sys == ECLB))
      equinox = 1950.;

   fout = fopen(template, "w+");

   if(fout == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Can't open output header file.");
      return returnStruct;
   }


   /****************************/ 
   /* Open the list table file */
   /* This may be a list of    */
   /* tables, in which case    */
   /* read them in.            */
   /****************************/ 

   ntables = 0;

   ncols = topen(tblfile);

   if(ncols <= 0)
   {
      sprintf(returnStruct->msg, "Invalid table file: %s", tblfile);
      return returnStruct;
   }

   itable  = tcol("table");

   if(itable < 0)
   {
      strcpy(fnames[0], tblfile);

      ntables = 1;
   }

   else
   {
      while(1)
      {
         stat = tread();

         if(stat < 0)
            break;

         strcpy(fnames[ntables], tval(itable));

         ++ntables;

         if(ntables >= maxfiles)
         {
            maxfiles += MAXFILES;

            fnames = (char **)malloc(maxfiles * sizeof(char *));
            
            for(i=maxfiles-MAXFILES; i<maxfiles; ++i)
               fnames[i] = (char *)malloc(MAXSTR * sizeof(char));
         }
      }
   }

   tclose();


   /*********************************************/ 
   /* Loop over the set of image metadata files */
   /*********************************************/ 

   nimages = 0;
   nCube   = 0;

   naxis   = 2;

   naxis3  = 0;
   crval3  = 0.;
   crpix3  = 0.;

   naxis4  = 0;
   crval4  = 0.;
   crpix4  = 0.;

   have3d  = 1;
   have4d  = 1;

   for(ifiles=0; ifiles<ntables; ++ifiles)
   {
      strcpy(tfile, fnames[ifiles]);

      if(mMakeHdr_debug >= 1)
      {
         printf("Table file %d: [%s]\n", ifiles, tfile);
         fflush(stdout);
      }

      /**********************************/ 
      /* Open the image list table file */
      /**********************************/ 
      ncols = topen(tfile);

      if(ncols <= 0)
      {
         for(ifiles=0; ifiles<ntables; ++ifiles)
            free(fnames[ifiles]);

         free(fnames);

         sprintf(returnStruct->msg, "Invalid image metadata file: %s", tfile);
         return returnStruct;
      }

      inaxis = tcol("naxis");


      /* Check for "cube" parameters */

      inaxis3 = tcol("naxis3");
      icrval3 = tcol("crval3");
      icrpix3 = tcol("crpix3");
      icdelt3 = tcol("cdelt3");

      inaxis4 = tcol("naxis4");
      icrval4 = tcol("crval4");
      icrpix4 = tcol("crpix4");
      icdelt4 = tcol("cdelt4");

      

      /* First check to see if we have four equatorial corners */

      icdelt1 = tcol("cdelt1");
      icdelt2 = tcol("cdelt2");
      
      datatype = UNKNOWN;

      ilon1 = tcol("ra1");
      ilon2 = tcol("ra2");
      ilon3 = tcol("ra3");
      ilon4 = tcol("ra4");

      ilat1 = tcol("dec1");
      ilat2 = tcol("dec2");
      ilat3 = tcol("dec3");
      ilat4 = tcol("dec4");

      if(ilon1 >= 0
      && ilon2 >= 0
      && ilon3 >= 0
      && ilon4 >= 0
      && ilat1 >= 0
      && ilat2 >= 0
      && ilat3 >= 0
      && ilat4 >= 0)
         datatype = FOURCORNERS;


      /* Then check for generic lon, lat corners */

      if(datatype == UNKNOWN)
      {
         ilon1 = tcol("lon1");
         ilon2 = tcol("lon2");
         ilon3 = tcol("lon3");
         ilon4 = tcol("lon4");

         ilat1 = tcol("lat1");
         ilat2 = tcol("lat2");
         ilat3 = tcol("lat3");
         ilat4 = tcol("lat4");

         if(ilon1 >= 0
         && ilon2 >= 0
         && ilon3 >= 0
         && ilon4 >= 0
         && ilat1 >= 0
         && ilat2 >= 0
         && ilat3 >= 0
         && ilat4 >= 0)
            datatype = FOURCORNERS;
      }


      /* Then check for full WCS */

      if(datatype == UNKNOWN)
      {
         ictype1  = tcol("ctype1");
         ictype2  = tcol("ctype2");
         iequinox = tcol("equinox");
         inl      = tcol("nl");
         ins      = tcol("ns");
         icrval1  = tcol("crval1");
         icrval2  = tcol("crval2");
         icrpix1  = tcol("crpix1");
         icrpix2  = tcol("crpix2");
         icdelt1  = tcol("cdelt1");
         icdelt2  = tcol("cdelt2");
         icrota2  = tcol("crota2");

         if(ins < 0)
            ins = tcol("naxis1");

         if(inl < 0)
            inl = tcol("naxis2");

         if(ictype1 >= 0
         && ictype2 >= 0
         && inl     >= 0
         && ins     >= 0
         && icrval1 >= 0
         && icrval2 >= 0
         && icrpix1 >= 0
         && icrpix2 >= 0
         && icdelt1 >= 0
         && icdelt2 >= 0
         && icrota2 >= 0)
            datatype = WCS;
      }


      /* And finally settle for just (ra,dec) or (lon,lat) columns */

      if(datatype == UNKNOWN)
      {
         ilon1 = tcol("ra");
         ilat1 = tcol("dec");

         if(ilon1 >= 0
         && ilat1 >= 0)
            datatype = LONLAT;
      }

      if(datatype == UNKNOWN)
      {
         ilon1 = tcol("lon");
         ilat1 = tcol("lat");

         if(ilon1 >= 0
         && ilat1 >= 0)
            datatype = LONLAT;
      }

      if(datatype == UNKNOWN)
      {
         ilon1 = tcol("crval1");
         ilat1 = tcol("crval2");

         if(ilon1 >= 0
         && ilat1 >= 0)
            datatype = LONLAT;
      }


      /* If we found none of the above, give up */

      if(datatype == UNKNOWN)
      {
         for(ifiles=0; ifiles<ntables; ++ifiles)
            free(fnames[ifiles]);

         free(fnames);

         sprintf(returnStruct->msg, "Need columns: ctype1 ctype2 nl ns crval1 crval2 crpix1 crpix2 cdelt1 cdelt2 crota2 (equinox optional).  Four corners (equatorial) will be used if they exist or even just a single set of coordinates");
         return returnStruct;
      }



      /**************************************************/ 
      /* Try to determine coordinate system and equinox */
      /**************************************************/ 

      input.sys   = EQUJ;
      input.epoch = 2000.;


      /* Equinox */

      keyval = tfindkey("EQUINOX");

      if(keyval != (char *)NULL)
        strcpy(epochStr, keyval);  

      keyval = tfindkey("EPOCH");

      if(keyval != (char *)NULL)
        strcpy(epochStr, keyval);  

      keyval = tfindkey("equinox");

      if(keyval != (char *)NULL)
        strcpy(epochStr, keyval);  

      keyval = tfindkey("epoch");

      if(keyval != (char *)NULL)
        strcpy(epochStr, keyval);  


      /* Coordinate system */

      keyval = tfindkey("CSYS");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      keyval = tfindkey("SYSTEM");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      keyval = tfindkey("SYS");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      keyval = tfindkey("COORD");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      keyval = tfindkey("COORDSYS");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      keyval = tfindkey("csys");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      keyval = tfindkey("system");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      keyval = tfindkey("sys");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      keyval = tfindkey("coord");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      keyval = tfindkey("coordsys");

      if(keyval != (char *)NULL)
        strcpy(csysStr, keyval);

      for(i=0; i<strlen(csysStr); ++i)
         csysStr[i] = tolower(csysStr[i]);
      
      if(epochStr[0] == 'j'
      || epochStr[0] == 'J')
      {
         refsys = 'j';
         input.epoch = atof(epochStr+1);
      }
      else if(epochStr[0] == 'b'
           || epochStr[0] == 'B')
      {
         refsys = 'b';
         input.epoch = atof(epochStr+1);
      }
      else
      {
         refsys = 'j';
         input.epoch = atof(epochStr);

         if(input.epoch == 0.)
            input.epoch = 2000.;
      }

      if(csysStr[strlen(csysStr)-1] == 'j')
         refsys = 'j';
      else if(csysStr[strlen(csysStr)-1] == 'j')
         refsys = 'b';

      if(strncmp(csysStr, "eq", 2) == 0 && refsys == 'j') input.sys = EQUJ;
      if(strncmp(csysStr, "ec", 2) == 0 && refsys == 'j') input.sys = ECLJ;
      if(strncmp(csysStr, "eq", 2) == 0 && refsys == 'b') input.sys = EQUB;
      if(strncmp(csysStr, "ec", 2) == 0 && refsys == 'b') input.sys = ECLB;
      
      if(strncmp(csysStr, "ga", 2) == 0) input.sys = GAL;
      if(strncmp(csysStr, "sg", 2) == 0) input.sys = SGAL;
      if(strncmp(csysStr, "su", 2) == 0) input.sys = SGAL;



      /**************************************************/ 
      /* Read the records and collect the image corners */
      /**************************************************/ 

      if(inaxis3 < 0
      || icrval3 < 0
      || icrpix3 < 0
      || icdelt3 < 0)
         have3d = 0;

      if(inaxis4 < 0
      || icrval4 < 0
      || icrpix4 < 0
      || icdelt4 < 0)
         have4d = 0;


      while(1)
      {
         stat = tread();

         if(stat < 0)
            break;

         ++nimages;

         if(inaxis >= 0)
         {
            if(tnull(inaxis))
               naxis = -1;           // Empty naxis value means we can't do cube
            else
            {
               colNaxis = atoi(tval(inaxis));

               if(nimages == 1)
                  naxis = colNaxis;  // First naxis value becomes our reference
               else
               {
                  if(colNaxis == 2)  
                     naxis = -1;              // An naxis value of 2 means not a cube
                  else if(colNaxis != naxis)
                     naxis = -1;              // If we don't match reference, we can't do cube
               }

               if(colNaxis > 2)
               {
                  ++nCube;

                  if(naxis > 2 && have3d)     // Now we check for cube parameter consistency
                  {
                     if(tnull(inaxis3)
                     || tnull(icrval3)
                     || tnull(icrpix3)
                     || tnull(icdelt3))
                        naxis = -1;

                     else
                     {
                        colNaxis3 = atoi(tval(inaxis3));
                        colCrval3 = atof(tval(icrval3)); 
                        colCrpix3 = atof(tval(icrpix3)); 
                        colCdelt3 = atof(tval(icdelt3)); 
                     }
                  }

                  if(naxis > 3 && have4d)
                  {
                     if(tnull(inaxis4)
                     || tnull(icrval4)
                     || tnull(icrpix4)
                     || tnull(icdelt4))
                        naxis = -1;

                     else
                     {
                        colNaxis4 = atoi(tval(inaxis4));
                        colCrval4 = atof(tval(icrval4)); 
                        colCrpix4 = atof(tval(icrpix4)); 
                        colCdelt4 = atof(tval(icdelt4)); 
                     }
                  }

                  if(nimages == 1)
                  {
                     naxis3 = colNaxis3;      // Taking the first values as reference
                     crval3 = colCrval3;
                     crpix3 = colCrpix3;
                     cdelt3 = colCdelt3;

                     naxis4 = colNaxis4;
                     crval4 = colCrval4;
                     crpix4 = colCrpix4;
                     cdelt4 = colCdelt4;
                  }

                  if(naxis > 2 && !have3d)    // If it says there are 3 dimensions but
                     naxis = -1;              // we don't have the columns, give up

                  if(have3d)
                  {
                     if(colCrval3 != crval3)  // CRVAL and CDELT must agree
                        naxis = -1;

                     if(colCdelt3 != cdelt3)
                        naxis = -1;

                     minpix = -crpix3;           // We use the columns CRPIX3 and NAXIS3
                     maxpix = -crpix3 + naxis3;  // to adjust the overall range (global
                                                 // CRPIX4 and NAXIS3)
                     colMinpix = -colCrpix3;
                     colMaxpix = -colCrpix3 + colNaxis3;

                     if(colMinpix < minpix) minpix = colMinpix;
                     if(colMaxpix > maxpix) maxpix = colMaxpix;

                     crpix3 = -minpix;
                     naxis3 = (int)(maxpix + crpix3 + 0.00001);
                  }

                  if(naxis > 3 && !have4d)    // Ditto for the fourth dimension
                     naxis = -1;

                  if(naxis > 3 && have4d)
                  {
                     if(colCrval4 != crval4)
                        naxis = -1;

                     if(colCdelt4 != cdelt4)
                        naxis = -1;

                     minpix = -crpix4;
                     maxpix = -crpix4 + naxis4;

                     colMinpix = -colCrpix4;
                     colMaxpix = -colCrpix4 + colNaxis4;

                     if(colMinpix < minpix) minpix = colMinpix;
                     if(colMaxpix > maxpix) maxpix = colMaxpix;

                     crpix4 = -minpix;
                     naxis4 = (int)(maxpix + crpix4 + 0.00001);
                  }
               }
            }
         }

         if(datatype == LONLAT)
         {
            xpos = atof(tval(ilon1));
            ypos = atof(tval(ilat1));

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               sys, equinox, &lon, &lat, 0.0);

            lons[ncoords] = lon;
            lats[ncoords] = lat;

            ++ncoords;

            if(icdelt1 >= 0)
            {
               val = fabs(atof(tval(icdelt1)));

               if(val < minCdelt)
                  minCdelt = val;
            }

            if(icdelt2 >= 0)
            {
               val = fabs(atof(tval(icdelt2)));

               if(val < minCdelt)
                  minCdelt = val;
            }
         }         
         if(datatype == FOURCORNERS)
         {
            xpos = atof(tval(ilon1));
            ypos = atof(tval(ilat1));

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               sys, equinox, &lon, &lat, 0.0);

            lons[ncoords] = lon;
            lats[ncoords] = lat;

            ++ncoords;
            
            xpos = atof(tval(ilon2));
            ypos = atof(tval(ilat2));

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               sys, equinox, &lon, &lat, 0.0);

            lons[ncoords] = lon;
            lats[ncoords] = lat;

            ++ncoords;
            
            xpos = atof(tval(ilon3));
            ypos = atof(tval(ilat3));

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               sys, equinox, &lon, &lat, 0.0);

            lons[ncoords] = lon;
            lats[ncoords] = lat;

            ++ncoords;
            
            xpos = atof(tval(ilon4));
            ypos = atof(tval(ilat4));

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               sys, equinox, &lon, &lat, 0.0);

            lons[ncoords] = lon;
            lats[ncoords] = lat;

            ++ncoords;

            if(icdelt1 >= 0)
            {
               val = fabs(atof(tval(icdelt1)));

               if(val < minCdelt)
                  minCdelt = val;
            }

            if(icdelt2 >= 0)
            {
               val = fabs(atof(tval(icdelt2)));

               if(val < minCdelt)
                  minCdelt = val;
            }
         }
         else if(datatype == WCS)
         {
            strcpy(input.ctype1, tval(ictype1));
            strcpy(input.ctype2, tval(ictype2));

            input.naxis1    = atoi(tval(ins));
            input.naxis2    = atoi(tval(inl));
            input.crpix1    = atof(tval(icrpix1));
            input.crpix2    = atof(tval(icrpix2));
            input.crval1    = atof(tval(icrval1));
            input.crval2    = atof(tval(icrval2));
            input.cdelt1    = atof(tval(icdelt1));
            input.cdelt2    = atof(tval(icdelt2));
            input.crota2    = atof(tval(icrota2));
            input.equinox   = 2000;

            if(iequinox >= 0)
               input.equinox = atoi(tval(iequinox));
            
            if(fabs(input.cdelt1) < minCdelt) minCdelt = fabs(input.cdelt1);
            if(fabs(input.cdelt2) < minCdelt) minCdelt = fabs(input.cdelt2);

            strcpy(header, "");
            sprintf(temp, "SIMPLE  = T"                    ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "BITPIX  = -64"                  ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "NAXIS   = 2"                    ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "NAXIS1  = %d",     input.naxis1 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "NAXIS2  = %d",     input.naxis2 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "CTYPE1  = '%s'",   input.ctype1 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "CTYPE2  = '%s'",   input.ctype2 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "CRVAL1  = %14.9f", input.crval1 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "CRVAL2  = %14.9f", input.crval2 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "CRPIX1  = %14.9f", input.crpix1 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "CRPIX2  = %14.9f", input.crpix2 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "CDELT1  = %14.9f", input.cdelt1 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "CDELT2  = %14.9f", input.cdelt2 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "CROTA2  = %14.9f", input.crota2 ); mMakeHdr_stradd(header, temp);
            sprintf(temp, "EQUINOX = %d",     input.equinox); mMakeHdr_stradd(header, temp);
            sprintf(temp, "END"                            ); mMakeHdr_stradd(header, temp);
            
            input.wcs = wcsinit(header);
                                   
            if(input.wcs == (struct WorldCoor *)NULL)
            {
               for(ifiles=0; ifiles<ntables; ++ifiles)
                  free(fnames[ifiles]);

               free(fnames);

               sprintf(returnStruct->msg, "Bad WCS for image %d", nimages);
               return returnStruct;
            }

            checkWCS = montage_checkWCS(input.wcs);

            if(checkWCS)
            {
               for(ifiles=0; ifiles<ntables; ++ifiles)
                  free(fnames[ifiles]);

               free(fnames);

               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }


            /* Get the coordinate system and epoch in a     */
            /* form compatible with the conversion library  */

            if(input.wcs->syswcs == WCS_J2000)
            {
               input.sys   = EQUJ;
               input.epoch = 2000.;

               if(input.wcs->equinox == 1950)
                  input.epoch = 1950.;
            }
            else if(input.wcs->syswcs == WCS_B1950)
            {
               input.sys   = EQUB;
               input.epoch = 1950.;

               if(input.wcs->equinox == 2000)
                  input.epoch = 2000;
            }
            else if(input.wcs->syswcs == WCS_GALACTIC)
            {
               input.sys   = GAL;
               input.epoch = 2000.;
            }
            else if(input.wcs->syswcs == WCS_ECLIPTIC)
            {
               input.sys   = ECLJ;
               input.epoch = 2000.;

               if(input.wcs->equinox == 1950)
               {
                  input.sys   = ECLB;
                  input.epoch = 1950.;
               }
            }
            else       
            {
               input.sys   = EQUJ;
               input.epoch = 2000.;
            }



            /* Collect the locations of the corners of the images */

            pix2wcs(input.wcs, 0.5, 0.5, &xpos, &ypos);

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               sys, equinox, &lon, &lat, 0.0);

            lons[ncoords] = lon;
            lats[ncoords] = lat;

            ++ncoords;


            pix2wcs(input.wcs, input.naxis1+0.5, 0.5, &xpos, &ypos);

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               sys, equinox, &lon, &lat, 0.0);

            lons[ncoords] = lon;
            lats[ncoords] = lat;

            ++ncoords;


            pix2wcs(input.wcs, input.naxis1+0.5, input.naxis2+0.5,
                    &xpos, &ypos);

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               sys, equinox, &lon, &lat, 0.0);

            lons[ncoords] = lon;
            lats[ncoords] = lat;

            ++ncoords;


            pix2wcs(input.wcs, 0.5, input.naxis2+0.5, &xpos, &ypos);

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               sys, equinox, &lon, &lat, 0.0);

            lons[ncoords] = lon;
            lats[ncoords] = lat;

            ++ncoords;
         }

         if(pixelScale > 0.)
            minCdelt = pixelScale;

         if(ncoords >= maxcoords)
         {
            maxcoords += MAXCOORD;

            lons = (double *)realloc(lons, maxcoords * sizeof(double));
            lats = (double *)realloc(lats, maxcoords * sizeof(double));

            if(lats == (double *)NULL)
            {
               fclose(fout);

               for(ifiles=0; ifiles<ntables; ++ifiles)
                  free(fnames[ifiles]);

               free(fnames);

               sprintf(returnStruct->msg, "Memory allocation failure.");
               return returnStruct;
            }
         }
      }

      tclose();
   }

   for(ifiles=0; ifiles<ntables; ++ifiles)
      free(fnames[ifiles]);

   free(fnames);


   /**********************************************/ 
   /* Check for very large region (e.g. all-sky) */
   /**********************************************/ 

   xmin =  1.;
   xmax = -1.;
   ymin =  1.;
   ymax = -1.;
   zmin =  1.;
   zmax = -1.;

   for(i=0; i<ncoords; ++i)
   {
      x = cos(lons[i] * dtr) * cos(lats[i] * dtr);
      y = sin(lons[i] * dtr) * cos(lats[i] * dtr);
      z = sin(lats[i] * dtr);

      if(x < xmin) xmin = x;
      if(x > xmax) xmax = x;
      if(y < ymin) ymin = y;
      if(y > ymax) ymax = y;
      if(z < zmin) zmin = z;
      if(z > zmax) zmax = z;
   }

   if(xmax - xmin > 1
   || ymax - ymin > 1
   || zmax - zmin > 1)
   {
      naxis1 = 360. / minCdelt;
      naxis2 = 180. / minCdelt;

      fprintf(fout, "SIMPLE  = T\n");
      fprintf(fout, "BITPIX  = -64\n");

      if(naxis == -1)
         fprintf(fout, "NAXIS   = 2\n");
      else
         fprintf(fout, "NAXIS   = %d\n", naxis);

      fprintf(fout, "NAXIS1  = %d\n", naxis1);
      fprintf(fout, "NAXIS2  = %d\n", naxis2);

      if(naxis > 2)
         fprintf(fout, "NAXIS3  = %d\n", naxis3);

      if(naxis > 3)
         fprintf(fout, "NAXIS4  = %d\n", naxis4);

      if(sys == EQUJ)
      {
         fprintf(fout, "CTYPE1  = 'RA---AIT'\n");
         fprintf(fout, "CTYPE2  = 'DEC--AIT'\n");
         fprintf(fout, "EQUINOX = %-g\n", equinox);
      }
      if(sys == EQUB)
      {
         fprintf(fout, "CTYPE1  = 'RA---AIT'\n");
         fprintf(fout, "CTYPE2  = 'DEC--AIT'\n");
         fprintf(fout, "EQUINOX = %-g\n", equinox);
      }
      if(sys == ECLJ)
      {
         fprintf(fout, "CTYPE1  = 'ELON-AIT'\n");
         fprintf(fout, "CTYPE2  = 'ELAT-AIT'\n");
         fprintf(fout, "EQUINOX = %-g\n", equinox);
      }
      if(sys == ECLB)
      {
         fprintf(fout, "CTYPE1  = 'ELON-AIT'\n");
         fprintf(fout, "CTYPE2  = 'ELAT-AIT'\n");
         fprintf(fout, "EQUINOX = %-g\n", equinox);
      }
      if(sys == GAL)
      {
         fprintf(fout, "CTYPE1  = 'GLON-AIT'\n");
         fprintf(fout, "CTYPE2  = 'GLAT-AIT'\n");
      }
      if(sys == SGAL)
      {
         fprintf(fout, "CTYPE1  = 'SLON-AIT'\n");
         fprintf(fout, "CTYPE2  = 'SLAT-AIT'\n");
      }
      
      fprintf(fout, "CRVAL1  = %14.9f\n", 0.);
      fprintf(fout, "CRVAL2  = %14.9f\n", 0.);

      if(naxis > 2)
         fprintf(fout, "CRVAL3  = %14.9f\n", crval3);

      if(naxis > 3)
         fprintf(fout, "CRVAL4  = %14.9f\n", crval4);

      fprintf(fout, "CRPIX1  = %14.4f\n", ((double)naxis1 + 1.)/2.);
      fprintf(fout, "CRPIX2  = %14.4f\n", ((double)naxis2 + 1.)/2.);

      if(naxis > 2)
         fprintf(fout, "CRPIX3  = %14.9f\n", crpix3);

      if(naxis > 3)
         fprintf(fout, "CRPIX4  = %14.9f\n", crpix4);

      fprintf(fout, "CDELT1  = %14.9f\n", -minCdelt);
      fprintf(fout, "CDELT2  = %14.9f\n", minCdelt);

      if(naxis > 2)
         fprintf(fout, "CDELT3  = %14.9f\n", cdelt3);

      if(naxis > 3)
         fprintf(fout, "CDELT4  = %14.9f\n", cdelt4);

      fprintf(fout, "CROTA2  = %14.9f\n", 0.);
      fprintf(fout, "END\n");
      fflush(fout);
      fclose(fout);

      strcpy(msg, "");

      if(naxis < 0)
         strcpy(msg, "  Cube columns exist but are either blank or inconsistent; outputting 2D only.");
      
      free(lons);
      free(lats);

      returnStruct->status = 0;

      sprintf(returnStruct->msg, "msg=\"Large area; defaulting to AITOFF projection.%s\", count=%d, ncube=%d, naxis1=%d, naxis2=%d", 
         msg, nimages, nCube, naxis1, naxis2);

      sprintf(returnStruct->json, "{\"msg\":\"Large area; defaulting to AITOFF projection.%s\", \"count\":%d, \"ncube\":%d, \"naxis1\":%d, \"naxis2\":%d}", 
         msg, nimages, nCube, naxis1, naxis2);

      strcpy(returnStruct->note, msg);

      returnStruct->count   = nimages;
      returnStruct->ncube   = nCube;
      returnStruct->naxis1  = naxis1;
      returnStruct->naxis2  = naxis2;
      returnStruct->clon    = 0.;
      returnStruct->clat    = 0.;
      returnStruct->lonsize = 360.;
      returnStruct->latsize = 180.;
      returnStruct->posang  = 0.;
      returnStruct->lon1    = 0.;
      returnStruct->lat1    = 0.;
      returnStruct->lon2    = 0.;
      returnStruct->lat2    = 0.;
      returnStruct->lon3    = 0.;
      returnStruct->lat3    = 0.;
      returnStruct->lon4    = 0.;
      returnStruct->lat4    = 0.;

      return returnStruct;
   }


   /************************************/ 
   /* Get the bounding box information */
   /************************************/ 

   if(northAligned)
      box = bndVerticalBoundingBox(ncoords, lons, lats);
   else
      box = bndBoundingBox(ncoords, lons, lats);

   free(lons);
   free(lats);

   if(box == (struct bndInfo *)NULL)
   {
      sprintf(montage_msgstr, "Error computing boundaries.");
      return returnStruct;
   }

   if(minCdelt <= 0. || minCdelt >= 360.)
      minCdelt = 1./3600.;

   if(maxPixel > 0)
   {
      if(box->latSize > box->lonSize)
         minCdelt = box->latSize / maxPixel;
      else
         minCdelt = box->lonSize / maxPixel;
   }

   if(isPercentage)
   {
      naxis1 = box->lonSize / minCdelt;
      naxis2 = box->latSize / minCdelt;

      if(naxis1 > naxis2)
         pad = pad / 100. * naxis1;
      else
         pad = pad / 100. * naxis2;
   }

   if(mMakeHdr_debug >= 1)
   {
      printf("pad = %-g (isPercentage = %d)\n", pad, isPercentage);
      fflush(stdout);
   }

   naxis1 = box->lonSize / minCdelt + 2 * pad;
   if(naxis1 * minCdelt < box->lonSize) naxis1 += 2;

   naxis2 = box->latSize / minCdelt + 2 * pad;
   if(naxis2 * minCdelt < box->lonSize) naxis2 += 2;

   fprintf(fout, "SIMPLE  = T\n");
   fprintf(fout, "BITPIX  = -64\n");

   if(naxis == -1)
      fprintf(fout, "NAXIS   = 2\n");
   else
      fprintf(fout, "NAXIS   = %d\n", naxis);

   fprintf(fout, "NAXIS1  = %d\n", naxis1);
   fprintf(fout, "NAXIS2  = %d\n", naxis2);

   if(naxis > 2)
      fprintf(fout, "NAXIS3  = %d\n", naxis3);

   if(naxis > 3)
      fprintf(fout, "NAXIS4  = %d\n", naxis4);

   if(sys == EQUJ)
   {
      fprintf(fout, "CTYPE1  = 'RA---TAN'\n");
      fprintf(fout, "CTYPE2  = 'DEC--TAN'\n");
      fprintf(fout, "EQUINOX = %-g\n", equinox);
   }
   if(sys == EQUB)
   {
      fprintf(fout, "CTYPE1  = 'RA---TAN'\n");
      fprintf(fout, "CTYPE2  = 'DEC--TAN'\n");
      fprintf(fout, "EQUINOX = %-g\n", equinox);
   }
   if(sys == ECLJ)
   {
      fprintf(fout, "CTYPE1  = 'ELON-TAN'\n");
      fprintf(fout, "CTYPE2  = 'ELAT-TAN'\n");
      fprintf(fout, "EQUINOX = %-g\n", equinox);
   }
   if(sys == ECLB)
   {
      fprintf(fout, "CTYPE1  = 'ELON-TAN'\n");
      fprintf(fout, "CTYPE2  = 'ELAT-TAN'\n");
      fprintf(fout, "EQUINOX = %-g\n", equinox);
   }
   if(sys == GAL)
   {
      fprintf(fout, "CTYPE1  = 'GLON-TAN'\n");
      fprintf(fout, "CTYPE2  = 'GLAT-TAN'\n");
   }
   if(sys == SGAL)
   {
      fprintf(fout, "CTYPE1  = 'SLON-TAN'\n");
      fprintf(fout, "CTYPE2  = 'SLAT-TAN'\n");
   }
   
   fprintf(fout, "CRVAL1  = %14.9f\n", box->centerLon);
   fprintf(fout, "CRVAL2  = %14.9f\n", box->centerLat);

   if(naxis > 2)
      fprintf(fout, "CRVAL3  = %14.9f\n", crval3);

   if(naxis > 3)
      fprintf(fout, "CRVAL4  = %14.9f\n", crval4);

   fprintf(fout, "CRPIX1  = %14.4f\n", ((double)naxis1 + 1.)/2.);
   fprintf(fout, "CRPIX2  = %14.4f\n", ((double)naxis2 + 1.)/2.);

   if(naxis > 2)
      fprintf(fout, "CRPIX3  = %14.9f\n", crpix3);

   if(naxis > 3)
      fprintf(fout, "CRPIX4  = %14.9f\n", crpix4);

   fprintf(fout, "CDELT1  = %14.9f\n", -minCdelt);
   fprintf(fout, "CDELT2  = %14.9f\n", minCdelt);

   if(naxis > 2)
      fprintf(fout, "CDELT3  = %14.9f\n", cdelt3);

   if(naxis > 3)
      fprintf(fout, "CDELT4  = %14.9f\n", cdelt4);

   fprintf(fout, "CROTA2  = %14.9f\n", box->posAngle);
   fprintf(fout, "END\n");
   fflush(fout);
   fclose(fout);
   

   /* Collect the locations of the corners of the images */

   if(mMakeHdr_readTemplate(template))
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   ncoords = 0;

   pix2wcs(outwcs, 0.5, 0.5, &lon, &lat);

   lonc[ncoords] = lon;
   latc[ncoords] = lat;

   ++ncoords;


   pix2wcs(outwcs, naxis1+0.5, 0.5, &lon, &lat);

   lonc[ncoords] = lon;
   latc[ncoords] = lat;

   ++ncoords;


   pix2wcs(outwcs, naxis1+0.5, naxis2+0.5, &lon, &lat);

   lonc[ncoords] = lon;
   latc[ncoords] = lat;

   ++ncoords;


   pix2wcs(outwcs, 0.5, naxis2+0.5, &lon, &lat);

   lonc[ncoords] = lon;
   latc[ncoords] = lat;

   ++ncoords;


   if(mMakeHdr_debug != 1)
   {
      strcpy(msg, "");

      if(naxis < 0)
         strcpy(msg, "Cube columns exist but are either blank or inconsistent. Outputting 2D only.");

      if(strlen(msg) == 0)
      {
         sprintf(montage_msgstr, "count=%d, ncube=%d, naxis1=%d, naxis2=%d, clon=%.6f, clat=%.6f, loncize=%.6f, latsize=%.6f, posang=%.6f, lon1=%.6f, lat1=%.6f, lon2=%.6f, lat2=%.6f, lon3=%.6f, lat3=%.6f, lon4=%.6f, lat4=%.6f",
            nimages, 
            nCube,
            naxis1, naxis2,
            box->centerLon, box->centerLat,
            minCdelt*naxis1, minCdelt*naxis2,
            box->posAngle,
            lonc[0], latc[0],
            lonc[1], latc[1],
            lonc[2], latc[2],
            lonc[3], latc[3]);

         sprintf(montage_json, "{\"count\"=%d, \"ncube\"=%d, \"naxis1\"=%d, \"naxis2\"=%d, \"clon\"=%.6f, \"clat\"=%.6f, \"lonsize\"=%.6f, \"latsize\"=%.6f, \"posang\"=%.6f, \"lon1\"=%.6f, \"lat1\"=%.6f, \"lon2\"=%.6f, \"lat2\"=%.6f, \"lon3\"=%.6f, \"lat3\"=%.6f, \"lon4\"=%.6f, \"lat4\"=%.6f}",
            nimages, 
            nCube,
            naxis1, naxis2,
            box->centerLon, box->centerLat,
            minCdelt*naxis1, minCdelt*naxis2,
            box->posAngle,
            lonc[0], latc[0],
            lonc[1], latc[1],
            lonc[2], latc[2],
            lonc[3], latc[3]);
      }
      else
      {
         sprintf(montage_msgstr, "msg=\"%s\", count=%d, ncube=%d, naxis1=%d, naxis2=%d, clon=%.6f, clat=%.6f, lonsize=%.6f, latsize=%.6f, posang=%.6f, lon1=%.6f, lat1=%.6f, lon2=%.6f, lat2=%.6f, lon3=%.6f, lat3=%.6f, lon4=%.6f, lat4=%.6f",
            msg,
            nimages, 
            nCube,
            naxis1, naxis2,
            box->centerLon, box->centerLat,
            minCdelt*naxis1, minCdelt*naxis2,
            box->posAngle,
            lonc[0], latc[0],
            lonc[1], latc[1],
            lonc[2], latc[2],
            lonc[3], latc[3]);

         sprintf(montage_json, "{msg=\"%s\", \"count\"=%d, \"ncube\"=%d, \"naxis1\"=%d, \"naxis2\"=%d, \"clon\"=%.6f, \"clat\"=%.6f, \"lonsize\"=%.6f, \"latsize\"=%.6f, \"posang\"=%.6f, \"lon1\"=%.6f, \"lat1\"=%.6f, \"lon2\"=%.6f, \"lat2\"=%.6f, \"lon3\"=%.6f, \"lat3\"=%.6f, \"lon4\"=%.6f, \"lat4\"=%.6f}",
            msg,
            nimages, 
            nCube,
            naxis1, naxis2,
            box->centerLon, box->centerLat,
            minCdelt*naxis1, minCdelt*naxis2,
            box->posAngle,
            lonc[0], latc[0],
            lonc[1], latc[1],
            lonc[2], latc[2],
            lonc[3], latc[3]);
      }
   }

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   strcpy(returnStruct->note, msg);

   returnStruct->count   = nimages;
   returnStruct->ncube   = nCube;
   returnStruct->naxis1  = naxis1;
   returnStruct->naxis2  = naxis2;
   returnStruct->clon    = box->centerLon;
   returnStruct->clat    = box->centerLat;
   returnStruct->lonsize = minCdelt*naxis1;
   returnStruct->latsize = minCdelt*naxis2;
   returnStruct->posang  = box->posAngle;
   returnStruct->lon1    = lonc[0];
   returnStruct->lat1    = latc[0];
   returnStruct->lon2    = lonc[1];
   returnStruct->lat2    = latc[1];
   returnStruct->lon3    = lonc[2];
   returnStruct->lat3    = latc[2];
   returnStruct->lon4    = lonc[3];
   returnStruct->lat4    = latc[3];

   return returnStruct;
}


int mMakeHdr_stradd(char *header, char *card)
{
   int i;

   int hlen = strlen(header);
   int clen = strlen(card);

   for(i=0; i<clen; ++i)
      header[hlen+i] = card[i];

   if(clen < 80)
      for(i=clen; i<80; ++i)
         header[hlen+i] = ' ';
   
   header[hlen+80] = '\0';

   return(strlen(header));
}



/**************************************************/
/*                                                */
/*  Read the output header template file.         */
/*  Specifically extract the image size info.     */
/*  Also, create a single-string version of the   */
/*  header data and use it to initialize the      */
/*  output WCS transform.                         */
/*                                                */
/**************************************************/

int mMakeHdr_readTemplate(char *filename)
{
   int       j;

   FILE     *fp;

   char      line[MAXSTR];

   char      header[80000];


   /********************************************************/
   /* Open the template file, read and parse all the lines */
   /********************************************************/

   fp = fopen(filename, "r");

   if(fp == (FILE *)NULL)
   {
      sprintf(montage_msgstr, "Template file not found.");
      return 1;
   }

   strcpy(header, "");

   for(j=0; j<1000; ++j)
   {
      if(fgets(line, MAXSTR, fp) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';
      
      if(line[strlen(line)-1] == '\r')
         line[strlen(line)-1]  = '\0';

      mMakeHdr_stradd(header, line);
   }

   fclose(fp);


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   outwcs = wcsinit(header);

   if(outwcs == (struct WorldCoor *)NULL)
   {
      sprintf(montage_msgstr, "Output wcsinit() failed.");
      return 1;
   }

   return 0;
}
