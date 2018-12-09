/* Module: mSubset.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
2.2      John Good        01Feb08  There was some confusion with mixed coord systems
2.1      John Good        24Jun07  Added correction for CAR projection error
2.0      John Good        20Jan05  Remove all reference to file and cntr;
                                   they weren't actually needed.  Add
                                   check for CD and image corner columns.
1.9      John Good        13Oct04  added 'file' as an alternate to 'fname'
1.8      John Good        18Sep04  Removed extra '|' from table header
1.7      John Good        25Nov03  Added extern optarg references
1.6      John Good        06Oct03  Added NAXIS1,2 as alternatives to ns,nl
1.5      John Good        25Aug03  Added status file processing
1.4      John Good        27Jun03  Added a few comments for clarity
1.3      John Good        08Apr03  Also remove <CR> from template lines
1.2      John Good        22Mar03  Renamed wcsCheck to checkWCS
                                   in one location for consistency
                                   and replaced it in aother with
                                   checkHdr
1.1      John Good        13Mar03  Added WCS header check and
                                   modified command-line processing
                                   to use getopt() library.  Check for
                                   missing/invalid image.tbl
1.0      John Good        29Jan03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#include <mtbl.h>
#include <fitsio.h>
#include <wcs.h>
#include <coord.h>

#include <mSubset.h>
#include <montage.h>

#define MAXSTR 256
#define MAXIMG 256

#define CORNERS 0
#define CDELT   1
#define CD      2


/* Basic image WCS information    */
/* (from the FITS header and as   */
/* returned from the WCS library) */

static struct ImgInfo
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
   double            cd11, cd12, cd21, cd22;
}
input;


/* Structure used to store relevant */
/* information about a FITS file    */

static struct
{
   fitsfile         *fptr;
   long              naxes[2];
   struct WorldCoor *wcs;
   int               sys;
   double            crpix1;
   double            crpix2;
   double            cdelt1;
   double            cdelt2;
   double            epoch;
}
output;


static int nimages;

static double xcorrection;
static double ycorrection;

static int mSubset_debug;


static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mSubset                                                              */
/*                                                                       */
/*  Given a list of image metadata and a region definition (in the form  */
/*  of a 'header' file), determine which subset of the list to keep by   */
/*  checking to see if the image overlaps with the area defined by the   */
/*  header.                                                              */
/*                                                                       */
/*   char  *tblfile        Image metadata table                          */
/*   char  *template       FITS header file used to define the desired   */
/*                         output region                                 */
/*                                                                       */
/*   char  *subtbl         Subset of the metadata table that overlap     */
/*                         with the template                             */
/*                                                                       */
/*   int    fastMode       Overlap check just involves great circles     */
/*                         connecting the image/template corners         */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mSubsetReturn *mSubset(char *tblfile, char *template, char *subtbl, int fastmode, int debugin)
{
   int    i, j, jnext, stat, ncols, overlap, nmatches;
   int    mode, iinc, jinc, interior, clockwise;
   double lon, lat;
   double oxpix, oypix;
   double xpos, ypos;
   double dtr;
   int    offscl;

   char   header  [1600];
   char   temp    [80];

   char  *checkHdr;
   char  *checkWCS;

   FILE  *fout;

   int    ictype1;
   int    ictype2;
   int    iequinox;
   int    inl;
   int    ins;
   int    icrval1;
   int    icrval2;
   int    icrpix1;
   int    icrpix2;
   int    icdelt1;
   int    icdelt2;
   int    icrota2;
   int    icd11;
   int    icd12;
   int    icd21;
   int    icd22;
   int    ira[4], idec[4];

   double image_corner_ra[4], image_corner_dec[4];
   double region_corner_ra[4], region_corner_dec[4];

   Vec    image_corner[4];
   Vec    region_corner[4];

   Vec    image_normal[4];

   Vec    normal1, normal2, direction;

   struct mSubsetReturn *returnStruct;


   dtr = atan(1.) / 45.;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mSubsetReturn *)malloc(sizeof(struct mSubsetReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mSubset_debug = debugin;

   checkHdr = montage_checkHdr(template, 1, 0);

   if(checkHdr)
   {
      strcpy(returnStruct->msg, checkHdr);
      return returnStruct;
   }

   fout = fopen(subtbl, "w+");

   if(fout == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Failed to open output %s", subtbl);
      return returnStruct;
   }


   /*************************************************/ 
   /* Process the output header template to get the */ 
   /* image size, coordinate system and projection  */ 
   /*************************************************/ 

   if(mSubset_readTemplate(template) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mSubset_debug >= 2)
   {
      printf("\noutput.naxes[0]  =  %ld\n", output.naxes[0]);
      printf("output.naxes[1]  =  %ld\n",   output.naxes[1]);
      printf("output.sys       =  %d\n",    output.sys);
      printf("output.epoch     =  %-g\n",   output.epoch);
      printf("output proj      =  %s\n",    output.wcs->ptype);

      fflush(stdout);
   }


   /*********************************************/ 
   /* Open the image header metadata table file */
   /*********************************************/ 

   ncols = topen(tblfile);

   if(ncols <= 0)
   {
      sprintf(returnStruct->msg, "Invalid image metadata file: %s", tblfile);
      return returnStruct;
   }

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

   icd11    = tcol("cd1_1");
   icd12    = tcol("cd1_2");
   icd21    = tcol("cd2_1");
   icd22    = tcol("cd2_2");

   ira[0]   = tcol("ra1");
   idec[0]  = tcol("dec1");
   ira[1]   = tcol("ra2");
   idec[1]  = tcol("dec2");
   ira[2]   = tcol("ra3");
   idec[2]  = tcol("dec3");
   ira[3]   = tcol("ra4");
   idec[3]  = tcol("dec4");

   if(ins < 0)
      ins = tcol("naxis1");

   if(inl < 0)
      inl = tcol("naxis2");


   /* Check mode (we can only use corners if in "fastmode" */

   if(fastmode
   && ira[0] >= 0 && idec[0] >= 0
   && ira[1] >= 0 && idec[1] >= 0
   && ira[2] >= 0 && idec[2] >= 0
   && ira[3] >= 0 && idec[3] >= 0)
      mode = CORNERS;

   else if(icd11 >= 0 && icd12 >= 0  && icd21 >= 0  && icd12 >= 0)
      mode = CD;

   else if(icdelt1 >= 0 && icdelt2 >= 0  && icrota2 >= 0)
      mode = CDELT;

   else
   {
      sprintf(returnStruct->msg, "Not enough information to determine coverages (corners, CDELTs or CD matrix)");
      return returnStruct;
   }


   /* If using CD/CDELT info, need more columns */

   if(mode != CORNERS)
   {
      if(ictype1 < 0
      || ictype2 < 0
      || inl     < 0
      || ins     < 0
      || icrval1 < 0
      || icrval2 < 0
      || icrpix1 < 0
      || icrpix2 < 0)
      {
         sprintf(returnStruct->msg, "Need columns: ctype1 ctype2 naxis1 naxis2 crval1 crval2 crpix1 crpix2 (equinox optional)");
         return returnStruct;
      }
   }

   fprintf(fout, "%s\n", tbl_hdr_string);
   fflush(fout);


   /***********************************/ 
   /* Read the projection information */ 
   /***********************************/ 

   nimages   =      0;
   nmatches  =      0;

   while(1)
   {
      stat = tread();

      if(mSubset_debug >= 1)
      {
         printf("\nCHECKING rec %d\n", nimages);
         fflush(stdout);
      }

      if(stat < 0)
         break;

      ++nimages;

      if(mode == CORNERS)
      {
         /* Check the image corners to see if any of them is in */
         /* the region of interest                              */

         offscl = 0;

         for(i=0; i<4; ++i)
         {
            image_corner_ra[i]  = atof(tval(ira[i]));
            image_corner_dec[i] = atof(tval(idec[i]));

            lon = image_corner_ra[i];
            lat = image_corner_dec[i];

            offscl = 0;

            wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

            mSubset_fixxy(&oxpix, &oypix, &offscl);

            if(mSubset_debug >= 4)
            {
               printf("I%d: (ra, dec) = (%-g,%-g)\n", i, image_corner_ra[i], image_corner_dec[i]);
               printf("(lon,lat)     = (%-g,%-g)\n", lon, lat);
               printf("(oxpix,oypix) = (%-g,%-g) %d\n", oxpix, oypix, offscl);
            }

            if(!offscl)
            {
               ++nmatches;

               fprintf(fout, "%s\n", tbl_rec_string);
               fflush(fout);
               overlap = 1;
               break;
            }
         }

         if(!offscl)
            continue;


         /* Check region of interest corners to see if any of */
         /* them is in the image (this can miss some funny    */
         /* overlaps; use slow mode to be sure)               */


         pix2wcs(output.wcs, -0.5, -0.5, &lon, &lat);
         convertCoordinates (output.sys, output.epoch, lon, lat,
                             EQUJ, 2000., &region_corner_ra[0], &region_corner_dec[0], 0.);


         pix2wcs(output.wcs, output.wcs->nxpix+0.5, -0.5, &lon, &lat);
         convertCoordinates (output.sys, output.epoch, lon, lat,
                             EQUJ, 2000., &region_corner_ra[1], &region_corner_dec[1], 0.);


         pix2wcs(output.wcs, output.wcs->nxpix+0.5, output.wcs->nypix+0.5, &lon, &lat);
         convertCoordinates (output.sys, output.epoch, lon, lat,
                             EQUJ, 2000., &region_corner_ra[2], &region_corner_dec[2], 0.);


         pix2wcs(output.wcs, -0.5, output.wcs->nypix+0.5, &lon, &lat);
         convertCoordinates (output.sys, output.epoch, lon, lat,
                             EQUJ, 2000., &region_corner_ra[3], &region_corner_dec[3], 0.);

         for(i=0; i<4; ++i)
         {
            region_corner[i].x = cos(region_corner_ra[i]*dtr) * cos(region_corner_dec[i]*dtr);
            region_corner[i].y = sin(region_corner_ra[i]*dtr) * cos(region_corner_dec[i]*dtr);
            region_corner[i].z = sin(region_corner_dec[i]*dtr);
         }

         for(i=0; i<4; ++i)
         {
            image_corner[i].x = cos(image_corner_ra[i]*dtr) * cos(image_corner_dec[i]*dtr);
            image_corner[i].y = sin(image_corner_ra[i]*dtr) * cos(image_corner_dec[i]*dtr);
            image_corner[i].z = sin(image_corner_dec[i]*dtr);
         }


         /* Reverse if counterclockwise on the sky */

         mSubset_Cross(&image_corner[0], &image_corner[1], &normal1);
         mSubset_Cross(&image_corner[1], &image_corner[2], &normal2);
         mSubset_Cross(&normal1, &normal2, &direction);

         clockwise = 0;
         if(mSubset_Dot(&direction, &image_corner[1]) > 0.)
            clockwise = 1;

         if(!clockwise)
         {
            mSubset_swap(&image_corner_ra [0], &image_corner_ra [3]);
            mSubset_swap(&image_corner_dec[0], &image_corner_dec[3]);

            mSubset_swap(&image_corner[0].x, &image_corner[3].x);
            mSubset_swap(&image_corner[0].y, &image_corner[3].y);
            mSubset_swap(&image_corner[0].z, &image_corner[3].z);

            mSubset_swap(&image_corner_ra [1], &image_corner_ra [2]);
            mSubset_swap(&image_corner_dec[1], &image_corner_dec[2]);

            mSubset_swap(&image_corner[1].x, &image_corner[2].x);
            mSubset_swap(&image_corner[1].y, &image_corner[2].y);
            mSubset_swap(&image_corner[1].z, &image_corner[2].z);
         }


         /* Check to see if any of the region corners is inside the image */

         for(i=0; i<4; ++i)
         {
            interior = 1;

            for(j=0; j<4; ++j)
            {
               jnext = (j+1)%4;

               mSubset_Cross(&image_corner[j], &image_corner[jnext], &image_normal[j]);

               if(mSubset_debug >= 4)
               {
                  printf("Comparing image corner %d with region side %d: %-g\n", i, j,
                     mSubset_Dot(&image_normal[j], &region_corner[i]));
                  fflush(stdout);
               }

               if(mSubset_Dot(&image_normal[j], &region_corner[i]) < 0)
               {
                  interior = 0;
                  break;
               }
            }

            if(interior)
            {
               ++nmatches;

               fprintf(fout, "%s\n", tbl_rec_string);
               fflush(fout);
               overlap = 1;
               break;
            }
         }
      }
      else
      {
         strcpy(input.ctype1, tval(ictype1));
         strcpy(input.ctype2, tval(ictype2));

         input.naxis1    = atoi(tval(ins));
         input.naxis2    = atoi(tval(inl));
         input.crpix1    = atof(tval(icrpix1));
         input.crpix2    = atof(tval(icrpix2));
         input.crval1    = atof(tval(icrval1));
         input.crval2    = atof(tval(icrval2));

         if(mode == CDELT)
         {
            input.cdelt1    = atof(tval(icdelt1));
            input.cdelt2    = atof(tval(icdelt2));
            input.crota2    = atof(tval(icrota2));
         }
         else
         {
            input.cd11      = atof(tval(icd11));
            input.cd12      = atof(tval(icd12));
            input.cd21      = atof(tval(icd21));
            input.cd22      = atof(tval(icd22));
         }

         input.equinox   = 2000;

         strcpy(header, "");
         sprintf(temp, "SIMPLE  = T"                    ); mSubset_stradd(header, temp);
         sprintf(temp, "BITPIX  = -64"                  ); mSubset_stradd(header, temp);
         sprintf(temp, "NAXIS   = 2"                    ); mSubset_stradd(header, temp);
         sprintf(temp, "NAXIS1  = %d",     input.naxis1 ); mSubset_stradd(header, temp);
         sprintf(temp, "NAXIS2  = %d",     input.naxis2 ); mSubset_stradd(header, temp);
         sprintf(temp, "CTYPE1  = '%s'",   input.ctype1 ); mSubset_stradd(header, temp);
         sprintf(temp, "CTYPE2  = '%s'",   input.ctype2 ); mSubset_stradd(header, temp);
         sprintf(temp, "CRVAL1  = %11.6f", input.crval1 ); mSubset_stradd(header, temp);
         sprintf(temp, "CRVAL2  = %11.6f", input.crval2 ); mSubset_stradd(header, temp);
         sprintf(temp, "CRPIX1  = %11.6f", input.crpix1 ); mSubset_stradd(header, temp);
         sprintf(temp, "CRPIX2  = %11.6f", input.crpix2 ); mSubset_stradd(header, temp);

         if(mode == CDELT)
         {
            sprintf(temp, "CDELT1  = %14.9f", input.cdelt1 ); mSubset_stradd(header, temp);
            sprintf(temp, "CDELT2  = %14.9f", input.cdelt2 ); mSubset_stradd(header, temp);
            sprintf(temp, "CROTA2  = %11.6f", input.crota2 ); mSubset_stradd(header, temp);
         }
         else
         {
            sprintf(temp, "CD1_1   = %11.6f", input.cd11   ); mSubset_stradd(header, temp);
            sprintf(temp, "CD1_2   = %11.6f", input.cd12   ); mSubset_stradd(header, temp);
            sprintf(temp, "CD2_1   = %11.6f", input.cd21   ); mSubset_stradd(header, temp);
            sprintf(temp, "CD2_2   = %11.6f", input.cd22   ); mSubset_stradd(header, temp);
         }

         sprintf(temp, "EQUINOX = %d",     input.equinox); mSubset_stradd(header, temp);
         sprintf(temp, "END"                            ); mSubset_stradd(header, temp);
         
         if(mSubset_debug >= 1)
         {
            printf("Image %d:\n", nimages);
            fflush(stdout);
         }

         if(mSubset_debug >= 3)
         {
            printf("%s\n", header);
            fflush(stdout);
         }

         if(iequinox >= 0)
            input.equinox = atoi(tval(iequinox));

         input.wcs = wcsinit(header);

         checkWCS = montage_checkWCS(input.wcs);

         if(checkWCS)
         {
            strcpy(returnStruct->msg, checkWCS);
            return returnStruct;
         }

         if(input.wcs == (struct WorldCoor *)NULL)
         {
            sprintf(returnStruct->msg, "Bad WCS for image %d", nimages);
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


         /* Compare it to the template.                  */
         /* Go around the outside of the input image,    */
         /* finding the range of output pixel locations  */

         jinc = 1;
         iinc = 1;

         if(fastmode)
         {
            jinc = input.naxis2;
            iinc = input.naxis1;
         }

         /* Left and right */

         overlap = 0;

         for (j=0; j<input.naxis2+1; j+=jinc)
         {
            pix2wcs(input.wcs, 0.5, j+0.5, &xpos, &ypos);

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               output.sys, output.epoch, &lon, &lat, 0.0);
            
            offscl = input.wcs->offscl;

            if(!offscl)
               wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

            mSubset_fixxy(&oxpix, &oypix, &offscl);

            if(mSubset_debug >= 4)
            {
               i = 0;

               printf("\nImage (i,j)   = (%-g,%-g)\n", i+0.5, j+0.5);
               printf("(xpos,ypos)   = (%-g,%-g)\n", xpos, ypos);
               printf("(lon,lat)     = (%-g,%-g)\n", lon, lat);
               printf("(oxpix,oypix) = (%-g,%-g)\n", oxpix, oypix);
            }

            if(!offscl)
            {
               ++nmatches;

               fprintf(fout, "%s\n", tbl_rec_string);
               fflush(fout);
               overlap = 1;
               break;
            }

            pix2wcs(input.wcs, input.naxis1+0.5, j+0.5, &xpos, &ypos);

            convertCoordinates(input.sys, input.epoch, xpos, ypos,
                               output.sys, output.epoch, &lon, &lat, 0.0);
            
            offscl = input.wcs->offscl;

            if(!offscl)
               wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

            mSubset_fixxy(&oxpix, &oypix, &offscl);

            if(mSubset_debug >= 4)
            {
               i = input.naxis1;

               printf("\n(i,j)         = (%-g,%-g)\n", i+0.5, j+0.5);
               printf("(xpos,ypos)   = (%-g,%-g)\n", xpos, ypos);
               printf("(lon,lat)     = (%-g,%-g)\n", lon, lat);
               printf("(oxpix,oypix) = (%-g,%-g)\n", oxpix, oypix);
            }

            if(!offscl)
            {
               ++nmatches;
               fprintf(fout, "%s\n", tbl_rec_string);
               fflush(fout);
               overlap = 1;
               break;
            }
         }

         if(overlap)
            continue;


         /* Top and bottom */

         if(!fastmode)
         {
            for (i=0; i<input.naxis1+1; i+=iinc)
            {
               pix2wcs(input.wcs, i+0.5, 0.5, &xpos, &ypos);

               convertCoordinates(input.sys, input.epoch, xpos, ypos,
                                  output.sys, output.epoch, &lon, &lat, 0.0);
               
               offscl = input.wcs->offscl;

               if(!offscl)
                  wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

               mSubset_fixxy(&oxpix, &oypix, &offscl);

               if(mSubset_debug >= 3)
               {
                  j = 0;

                  printf("\n(i,j)         = (%-g,%-g)\n", i+0.5, j+0.5);
                  printf("(xpos,ypos)   = (%-g,%-g)\n", xpos, ypos);
                  printf("(lon,lat)     = (%-g,%-g)\n", lon, lat);
                  printf("(oxpix,oypix) = (%-g,%-g)\n", oxpix, oypix);
               }

               if(!offscl)
               {
                  ++nmatches;
                  fprintf(fout, "%s\n", tbl_rec_string);
                  fflush(fout);
                  overlap = 1;
                  break;
               }

               pix2wcs(input.wcs, i+0.5, input.naxis2+0.5, &xpos, &ypos);

               convertCoordinates(input.sys, input.epoch, xpos, ypos,
                                  output.sys, output.epoch, &lon, &lat, 0.0);
               
               offscl = input.wcs->offscl;

               if(!offscl)
                  wcs2pix(output.wcs, lon, lat, &oxpix, &oypix, &offscl);

               mSubset_fixxy(&oxpix, &oypix, &offscl);

               if(mSubset_debug >= 3)
               {
                  j = input.naxis2;

                  printf("\n(i,j)         = (%-g,%-g)\n", i+0.5, j+0.5);
                  printf("(xpos,ypos)   = (%-g,%-g)\n", xpos, ypos);
                  printf("(lon,lat)     = (%-g,%-g)\n", lon, lat);
                  printf("(oxpix,oypix) = (%-g,%-g)\n", oxpix, oypix);
               }

               if(!offscl)
               {
                  ++nmatches;
                  fprintf(fout, "%s\n", tbl_rec_string);
                  fflush(fout);
                  overlap = 1;
                  break;
               }
            }
         }

         if(overlap)
            continue;


         /* Check region of interest corners to see if any of */
         /* them is in the image                              */


         pix2wcs(input.wcs, 0.5, 0.5, &xpos, &ypos);

         convertCoordinates (input.sys, input.epoch, xpos, ypos,
                             EQUJ, 2000., &image_corner_ra[0], &image_corner_dec[0], 0.0);

         pix2wcs(input.wcs, input.naxis1+0.5, 0.5, &xpos, &ypos);

         convertCoordinates (input.sys, input.epoch, xpos, ypos,
                             EQUJ, 2000., &image_corner_ra[1], &image_corner_dec[1], 0.0);

         pix2wcs(input.wcs, input.naxis1+0.5, input.naxis2+0.5, &xpos, &ypos);

         convertCoordinates (input.sys, input.epoch, xpos, ypos,
                             EQUJ, 2000., &image_corner_ra[2], &image_corner_dec[2], 0.0);

         pix2wcs(input.wcs, 0.5, input.naxis2+0.5, &xpos, &ypos);

         convertCoordinates (input.sys, input.epoch, xpos, ypos,
                             EQUJ, 2000., &image_corner_ra[3], &image_corner_dec[3], 0.0);


         pix2wcs(output.wcs, -0.5, -0.5, &lon, &lat);
         convertCoordinates (output.sys, output.epoch, lon, lat,
                             EQUJ, 2000., &region_corner_ra[0], &region_corner_dec[0], 0.);

         pix2wcs(output.wcs, output.wcs->nxpix+0.5, -0.5, &lon, &lat);
         convertCoordinates (output.sys, output.epoch, lon, lat,
                             EQUJ, 2000., &region_corner_ra[1], &region_corner_dec[1], 0.);


         pix2wcs(output.wcs, output.wcs->nxpix+0.5, output.wcs->nypix+0.5, &lon, &lat);
         convertCoordinates (output.sys, output.epoch, lon, lat,
                             EQUJ, 2000., &region_corner_ra[2], &region_corner_dec[2], 0.);


         pix2wcs(output.wcs, -0.5, output.wcs->nypix+0.5, &lon, &lat);
         convertCoordinates (output.sys, output.epoch, lon, lat,
                             EQUJ, 2000., &region_corner_ra[3], &region_corner_dec[3], 0.);


         for(i=0; i<4; ++i)
         {
            region_corner[i].x = cos(region_corner_ra[i]*dtr) * cos(region_corner_dec[i]*dtr);
            region_corner[i].y = sin(region_corner_ra[i]*dtr) * cos(region_corner_dec[i]*dtr);
            region_corner[i].z = sin(region_corner_dec[i]*dtr);
         }

         for(i=0; i<4; ++i)
         {
            image_corner[i].x = cos(image_corner_ra[i]*dtr) * cos(image_corner_dec[i]*dtr);
            image_corner[i].y = sin(image_corner_ra[i]*dtr) * cos(image_corner_dec[i]*dtr);
            image_corner[i].z = sin(image_corner_dec[i]*dtr);
         }


         /* Reverse if counterclockwise on the sky */

         mSubset_Cross(&image_corner[0], &image_corner[1], &normal1);
         mSubset_Cross(&image_corner[1], &image_corner[2], &normal2);
         mSubset_Cross(&normal1, &normal2, &direction);

         clockwise = 0;
         if(mSubset_Dot(&direction, &image_corner[1]) > 0.)
            clockwise = 1;

         if(!clockwise)
         {
            mSubset_swap(&image_corner_ra [0], &image_corner_ra [3]);
            mSubset_swap(&image_corner_dec[0], &image_corner_dec[3]);

            mSubset_swap(&image_corner[0].x, &image_corner[3].x);
            mSubset_swap(&image_corner[0].y, &image_corner[3].y);
            mSubset_swap(&image_corner[0].z, &image_corner[3].z);

            mSubset_swap(&image_corner_ra [1], &image_corner_ra [2]);
            mSubset_swap(&image_corner_dec[1], &image_corner_dec[2]);

            mSubset_swap(&image_corner[1].x, &image_corner[2].x);
            mSubset_swap(&image_corner[1].y, &image_corner[2].y);
            mSubset_swap(&image_corner[1].z, &image_corner[2].z);
         }


         /* Check to see if any of the region corners is inside the image */

         for(i=0; i<4; ++i)
         {
            interior = 1;

            for(j=0; j<4; ++j)
            {
               jnext = (j+1)%4;

               mSubset_Cross(&image_corner[j], &image_corner[jnext], &image_normal[j]);

               if(mSubset_debug >= 4)
               {
                  printf("Comparing image corner %d with region side %d: %-g\n", i, j,
                     mSubset_Dot(&image_normal[j], &region_corner[i]));
                  fflush(stdout);
               }

               if(mSubset_Dot(&image_normal[j], &region_corner[i]) < 0.)
               {
                  interior = 0;
                  break;
               }
            }

            if(interior)
            {
               ++nmatches;

               fprintf(fout, "%s\n", tbl_rec_string);
               fflush(fout);
               overlap = 1;
               break;
            }
         }
      }
   }

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d, nmatches=%d",           nimages, nmatches);
   sprintf(returnStruct->json, "{\"count\":%d, \"nmatches\":%d}", nimages, nmatches);

   returnStruct->count    = nimages;
   returnStruct->nmatches = nmatches;

   return returnStruct;
}


/**************************************************/
/*  Projections like CAR sometimes add an extra   */
/*  360 degrees worth of pixels to the return     */
/*  and call it off-scale.                        */
/**************************************************/

void mSubset_fixxy(double *x, double *y, int *offscl)
{
   *x = *x - xcorrection;
   *y = *y - ycorrection;

   if(*x < 0.
   || *x > output.wcs->nxpix+1.
   || *y < 0.
   || *y > output.wcs->nypix+1.)
      *offscl = 1;

   return;
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

int mSubset_readTemplate(char *filename)
{
   FILE  *fp;

   char   line[MAXSTR];

   char   header[32768];

   int    sys;
   double epoch;

   double ix, iy;
   double xpos, ypos;
   double x, y;

   int    offscl;


   /********************************************************/
   /* Open the template file, read and parse all the lines */
   /********************************************************/

   fp = fopen(filename, "r");

   if(fp == (FILE *)NULL)
   {
      mSubset_printError("Template file not found.");
      return 1;
   }

   strcpy(header, "");

   while(1)
   {
      if(fgets(line, MAXSTR, fp) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';
      
      if(line[strlen(line)-1] == '\r')
         line[strlen(line)-1]  = '\0';

      if(mSubset_debug >= 3)
      {
         printf("Template line: [%s]\n", line);
         fflush(stdout);
      }

      mSubset_stradd(header, line);

      mSubset_parseLine(line);
   }

   fclose(fp);


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   if(mSubset_debug >= 3)
   {
      printf("Output Header to wcsinit():\n%s\n", header);
      fflush(stdout);
   }

   output.wcs = wcsinit(header);

   if(output.wcs == (struct WorldCoor *)NULL)
   {
      sprintf(montage_msgstr, "Output wcsinit() failed.");
      return 1;
   }


   /* Kludge to get around bug in WCS library:   */
   /* 360 degrees sometimes added to pixel coord */

   ix = 0.5;
   iy = 0.5;

   pix2wcs(output.wcs, ix, iy, &xpos, &ypos);

   wcs2pix(output.wcs, xpos, ypos, &x, &y, &offscl);

   xcorrection = x-ix;
   ycorrection = y-iy;


   /*************************************/
   /*  Set up the coordinate transform  */
   /*************************************/

   if(output.wcs->syswcs == WCS_J2000)
   {
      sys   = EQUJ;
      epoch = 2000.;

      if(output.wcs->equinox == 1950.)
         epoch = 1950;
   }
   else if(output.wcs->syswcs == WCS_B1950)
   {
      sys   = EQUB;
      epoch = 1950.;

      if(output.wcs->equinox == 2000.)
         epoch = 2000;
   }
   else if(output.wcs->syswcs == WCS_GALACTIC)
   {
      sys   = GAL;
      epoch = 2000.;
   }
   else if(output.wcs->syswcs == WCS_ECLIPTIC)
   {
      sys   = ECLJ;
      epoch = 2000.;

      if(output.wcs->equinox == 1950.)
      {
         sys   = ECLB;
         epoch = 1950.;
      }
   }
   else       
   {
      sys   = EQUJ;
      epoch = 2000.;
   }

   output.sys   = sys;
   output.epoch = epoch;

   return 0;
}



/**********************************************/
/*                                            */
/*  Parse header lines from the template,     */
/*  looking for NAXIS1, NAXIS2, CRPIX1 CRPIX2 */
/*  CDELT1 and CDELT2                         */
/*                                            */
/**********************************************/

int mSubset_parseLine(char *line)
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

   if(mSubset_debug >= 2)
   {
      printf("keyword [%s] = value [%s]\n", keyword, value);
      fflush(stdout);
   }

   if(strcmp(keyword, "NAXIS1") == 0)
      output.naxes[0] = atoi(value);

   if(strcmp(keyword, "NAXIS2") == 0)
      output.naxes[1] = atoi(value);

   if(strcmp(keyword, "CRPIX1") == 0)
      output.crpix1 = atof(value);

   if(strcmp(keyword, "CRPIX2") == 0)
      output.crpix2 = atof(value);

   if(strcmp(keyword, "CDELT1") == 0)
      output.cdelt1 = atof(value);

   if(strcmp(keyword, "CDELT2") == 0)
      output.cdelt2 = atof(value);

   return 0;
}



/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void mSubset_printError(char *msg)
{
   sprintf(montage_msgstr, "%s", msg);
}


/* stradd adds the string "card" to a header line, and */
/* pads the header out to 80 characters.               */

int mSubset_stradd(char *header, char *card)
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


/***************************************************/
/*                                                 */
/* Cross()                                         */
/*                                                 */
/* Vector cross product.                           */
/*                                                 */
/***************************************************/

int mSubset_Cross(Vec *v1, Vec *v2, Vec *v3)
{
   v3->x =  v1->y*v2->z - v2->y*v1->z;
   v3->y = -v1->x*v2->z + v2->x*v1->z;
   v3->z =  v1->x*v2->y - v2->x*v1->y;

   if(v3->x == 0.
   && v3->y == 0.
   && v3->z == 0.)
      return 0;

   return 1;
}


/***************************************************/
/*                                                 */
/* Dot()                                           */
/*                                                 */
/* Vector dot product.                             */
/*                                                 */
/***************************************************/

double mSubset_Dot(Vec *a, Vec *b)
{
   double sum = 0.0;

   sum = a->x * b->x
       + a->y * b->y
       + a->z * b->z;

   return sum;
}





/***************************************************/
/*                                                 */
/* swap()                                          */
/*                                                 */
/* Switches the values of two memory locations     */
/*                                                 */
/***************************************************/

int mSubset_swap(double *x, double *y)
{
   double tmp;

   tmp = *x;
   *x  = *y;
   *y  = tmp;

   return(0);
}
