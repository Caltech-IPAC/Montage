/* Module: mSubHdr.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        11Nov19  Baseline code
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <wcs.h>
#include <coord.h>

#include <mSubHdr.h>
#include <montage.h>

#define STRLEN 4096

#define DEGREES 0
#define PIXELS  1

static double pixscale;

static int mSubHdr_debug;

static struct
{
   fitsfile         *fptr;
   long              naxes[2];
   struct WorldCoor *wcs;
   int               sys;
   double            epoch;
}
   input;


static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mSubHdr                                                              */
/*                                                                       */
/*  This program subsets an image header around a location of interest   */
/*  and creates a new output header which can be used to make a mosaic   */
/*  of just that region.                                                 */
/*  The location is defined by the RA,Dec (J2000) of the new center and  */
/*  the XY size in degrees of the area (X and Y) in the direction of     */
/*  the image axes, not Equatorial coordinates.                          */
/*                                                                       */
/*   char  *infile         Input FITS header file                        */
/*   char  *outfile        SubHdr output FITS header file                */
/*                                                                       */
/*   double ra             RA of cutout center (or start X pixel in      */
/*                         PIX mode                                      */
/*   double dec            Dec of cutout center (or start Y pixel in     */
/*                         PIX mode                                      */
/*                                                                       */
/*   char  *xsize          X size in degrees (SKY mode) or pixels        */
/*                         (PIX mode). Can have a 'p' or 'd' suffix      */
/*                         to override default units.                    */
/*   char  *ysize          Y size in degrees.  Same rules as X size.     */
/*                                                                       */
/*   int    mode           Processing mode. The two main modes are       */
/*                         0 (SKY) and 1 (PIX), corresponding to cutouts */
/*                         in sky coordinate or pixel space.             */
/*                                                                       */
/*   int    nowcs          Indicates that the image has no WCS info      */
/*                         (only makes sense in PIX mode)                */
/*                                                                       */
/*   int    shift          Try to shift the image if the window goes off */
/*                         an edge.                                      */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mSubHdrReturn *mSubHdr(char *infile, char *outfile, double ra, double dec, 
                              char *xsizein, char  *ysizein, int mode, int nowcs, int shift, int debugin)
{
   int       offscl;
   int       pixMode, lines;

   long      naxis1, naxis2;
   long      ibegin, iend;
   long      jbegin, jend;
   int       sys;
   double    epoch;
   double    lon, lat;
   double    xpix, ypix;
   double    xoff, yoff;
   double    crpix1, crpix2;
   double    xsize, ysize;
   int       xunits, yunits;

   char     *keyword;
   char     *value;
   char     *end;

   char      line   [STRLEN];
   char      refline[STRLEN];

   int       len;

   FILE     *fin;
   FILE     *fout;

   struct mSubHdrReturn *returnStruct;


   mSubHdr_debug = debugin;

   pixMode = 0;
   if(mode == PIX)
      pixMode = 1;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mSubHdrReturn *)malloc(sizeof(struct mSubHdrReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /*************************************************/
   /* Process the header template to get the image  */
   /* size, coordinate system and projection        */
   /*************************************************/

   if(mSubHdr_readTemplate(infile) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mSubHdr_debug >= 1)
   {
      printf("\nImage info\n");
      printf("input.naxes[0] =  %ld\n", input.naxes[0]);
      printf("input.naxes[1] =  %ld\n", input.naxes[1]);
      printf("input.sys      =  %d\n",  input.sys);
      printf("input.epoch    =  %-g\n", input.epoch);
      printf("input proj     =  %s\n",  input.wcs->ptype);
      printf("input crval[0] =  %-g\n", input.wcs->crval[0]);
      printf("input crval[1] =  %-g\n", input.wcs->crval[1]);
      printf("input crpix[0] =  %-g\n", input.wcs->crpix[0]);
      printf("input crpix[1] =  %-g\n", input.wcs->crpix[1]);
      printf("input cdelt[0] =  %-g\n", input.wcs->cdelt[0]);
      printf("input cdelt[1] =  %-g\n", input.wcs->cdelt[1]);

      fflush(stdout);
   }


   /* Extract the coordinate system and epoch info */

   if(input.wcs->syswcs == WCS_J2000)
   {
      sys   = EQUJ;
      epoch = 2000.;

      if(input.wcs->equinox == 1950.)
         epoch = 1950;
   }
   else if(input.wcs->syswcs == WCS_B1950)
   {
      sys   = EQUB;
      epoch = 1950.;

      if(input.wcs->equinox == 2000.)
         epoch = 2000;
   }
   else if(input.wcs->syswcs == WCS_GALACTIC)
   {
      sys   = GAL;
      epoch = 2000.;
   }
   else if(input.wcs->syswcs == WCS_ECLIPTIC)
   {
      sys   = ECLJ;
      epoch = 2000.;

      if(input.wcs->equinox == 1950.)
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
   
   if(mSubHdr_debug)
   {
      printf("\n");
      printf("coordinate system = %d\n", sys);
      printf("epoch             = %-g\n", epoch);
      fflush(stdout);
   }
  

   /*************************************/
   /* Process the X and Y size strings. */
   /*************************************/

   xsize = atof(xsizein);

   xunits = DEGREES;
   if(xsizein[strlen(xsizein)-1] == 'p') 
      xunits = PIXELS;
   else if(xsizein[strlen(xsizein)-1] == 'd')
      xunits = PIXELS;
   else if(pixMode)
      xunits = DEGREES;

   if(pixMode && xunits == DEGREES)
      xsize = xsize / fabs(input.wcs->cdelt[0]);
   else if(!pixMode && xunits == PIXELS)
      xsize = xsize * fabs(input.wcs->cdelt[0]);


   ysize = atof(ysizein);

   yunits = DEGREES;
   if(ysizein[strlen(ysizein)-1] == 'p') 
      yunits = PIXELS;
   else if(ysizein[strlen(ysizein)-1] == 'd')
      yunits = PIXELS;
   else if(pixMode)
      yunits = DEGREES;

   if(pixMode && yunits == DEGREES)
      ysize = ysize / fabs(input.wcs->cdelt[1]);
   else if(!pixMode && yunits == PIXELS)
      ysize = ysize * fabs(input.wcs->cdelt[1]);

   if(mSubHdr_debug)
   {
      printf("\n");
      printf("Parsed sizes:\n");
      printf("xsize = %-g\n", xsize);
      printf("ysize = %-g\n", ysize);
      fflush(stdout);
   }


   /******************************************/
   /* If we are working in pixel mode, we    */
   /* already have the info needed to subset */
   /* the image.                             */
   /*                                        */
   /* Otherwise, we need to convert the      */
   /* coordinates to pixel space.            */
   /******************************************/

   if(pixMode)
   {
      if(mSubHdr_debug) {
         printf("\nPixel mode\n");
         fflush(stdout);
      }
 
      naxis1 = (long)(xsize + 0.5);
      naxis2 = (long)(ysize + 0.5);
      
      ibegin = (int)ra;
      iend   = (int)(ra + xsize + 0.5);

      jbegin = (int)dec;
      jend   = (int)(dec + ysize + 0.5);

      if(mSubHdr_debug)
      {
         printf("'ra'    = %-g\n", ra);
         printf("'dec'   = %-g\n", dec);
         printf("xsize   = %-g\n", xsize);
         printf("ysize   = %-g\n", ysize);
         printf("naxis1  = %ld\n", naxis1);
         printf("naxis2  = %ld\n", naxis2);
         printf("ibegin  = %ld\n", ibegin);
         printf("iend    = %ld\n", iend);
         printf("jbegin  = %ld\n", jbegin);
         printf("jend    = %ld\n", jend);
         fflush(stdout);
      }
   }
   
   else
   {
      /**********************************/
      /* Find the pixel location of the */
      /* sky coordinate specified       */
      /**********************************/

      if(mSubHdr_debug)
      {
         printf("\nAngle mode\n");
         fflush(stdout);
      }

      convertCoordinates(EQUJ, 2000., ra, dec, sys, epoch, &lon, &lat, 0.);

      offscl = 0;

      wcs2pix(input.wcs, lon, lat, &xpix, &ypix, &offscl);

      if(mSubHdr_debug)
      {
         printf("   ra   = %-g\n", ra);
         printf("   dec  = %-g\n", dec);
         printf("-> lon  = %-g\n", lon);
         printf("   lat  = %-g\n", lat);
         printf("-> xpix = %-g\n", xpix);
         printf("   ypix = %-g\n", ypix);
         fflush(stdout);
      }


      /************************************/
      /* Find the range of pixels to keep */
      /************************************/

      xoff = fabs(xsize/2./input.wcs->cdelt[0]);
      yoff = fabs(ysize/2./input.wcs->cdelt[1]);

      naxis1 = (long)(2.*xoff + 0.5);
      naxis2 = (long)(2.*yoff + 0.5);

      ibegin = xpix - xoff;
      iend   = ibegin + floor(2.*xoff + 1.0);

      jbegin = ypix - yoff;
      jend   = jbegin + floor(2.*yoff + 1.0);

      if(mSubHdr_debug)
      {
         printf("\n");
         printf("   cdelt1 = %-g\n", input.wcs->cdelt[0]);
         printf("   cdelt2 = %-g\n", input.wcs->cdelt[1]);
         printf("   xsize  = %-g\n", xsize);
         printf("   ysize  = %-g\n", ysize);
         printf("   naxis1 = %ld\n", naxis1);
         printf("   naxis2 = %ld\n", naxis2);
         printf("   xoff   = %-g\n", xoff);
         printf("   yoff   = %-g\n", yoff);
         printf("   ibegin = %ld\n", ibegin);
         printf("   iend   = %ld\n", iend);
         printf("   jbegin = %ld\n", jbegin);
         printf("   jend   = %ld\n", jend);
         fflush(stdout);
      }

      if(ibegin > iend
      || jbegin > jend)
      {
         sprintf(returnStruct->msg, "No pixels match area.");
         return returnStruct;
      }
   }

   if(shift)
   {
      if(ibegin < 0 && iend > input.naxes[0]-1)
         ibegin = -(ibegin + input.naxes[0]-1)/2;
      else if(ibegin < 0)
         ibegin = 0;
      else if(iend > input.naxes[0]-1)
         ibegin -= iend - (input.naxes[0]- 1);
         
      if(jbegin < 0 && jend > input.naxes[1]-1)
         jbegin = -(jbegin + input.naxes[1]-1)/2;
      else if(jbegin < 0)
         jbegin = 0;
      else if(jend > input.naxes[1]-1)
         jbegin -= jend - (input.naxes[1]- 1);
   }
      
   crpix1 = input.wcs->crpix[0] - ibegin;
   crpix2 = input.wcs->crpix[1] - jbegin;

   if(mSubHdr_debug)
   {
      printf("\n");
      printf("crpix1  = %-g\n", crpix1);
      printf("crpix2  = %-g\n", crpix2);
   }


   /***********************************************************/
   /* Copy the lines of the input header file to the output,  */
   /* replacing those that have changed (NAXIS1, NAXIS2, etc. */
   /***********************************************************/

   fout = fopen(outfile, "w+");

   if(fout == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Cannot open output header file.");
      return returnStruct;
   }

   fin = fopen(infile, "r");

   lines = 0;

   while(1)
   {
      if(fgets(line, STRLEN, fin) == (char *)NULL)
         break;

      ++lines;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';

      strcpy(refline, line);

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

      if(mSubHdr_debug >= 1)
      {
         printf("keyword [%s] = value [%s]\n", keyword, value);
         fflush(stdout);
      }

      if(strcmp(keyword, "NAXIS1") == 0)
         fprintf(fout, "NAXIS1  = %ld\n", naxis1);

      else if(strcmp(keyword, "NAXIS2") == 0)
         fprintf(fout, "NAXIS2  = %ld\n", naxis2);

      else if(strcmp(keyword, "CRPIX1") == 0)
         fprintf(fout, "CRPIX1  = %.2f\n", crpix1);

      else if(strcmp(keyword, "CRPIX2") == 0)
         fprintf(fout, "CRPIX2  = %.2f\n", crpix2);

      else
         fprintf(fout, "%s\n", refline);

      fflush(fout);
   }

   fclose(fout);


   /**************/
   /* And return */
   /**************/

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "lines=\"%d\"",       lines);
   sprintf(returnStruct->json, "{\"lines\":\"%d\"}", lines);

   returnStruct->lines = lines;

   return returnStruct;
}



/**************************************************/
/*                                                */
/*  Read the input header template file.          */
/*  Specifically extract the image size info.     */
/*  Also, create a single-string version of the   */
/*  header data and use it to initialize the      */
/*  input WCS transform.                          */
/*                                                */
/**************************************************/

int mSubHdr_readTemplate(char *filename)
{
   int       i;

   FILE     *fp;

   char      line[STRLEN];

   char     *header[2];

   int       sys;
   double    epoch;

   header[0] = malloc(32768);
   header[1] = (char *)NULL;


   /********************************************************/
   /* Open the template file, read and parse all the lines */
   /********************************************************/

   fp = fopen(filename, "r");

   if(fp == (FILE *)NULL)
   {
      sprintf(montage_msgstr, "Template file [%s] not found.", filename);
      return 1;
   }

   while(1)
   {
      if(fgets(line, STRLEN, fp) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';

      if(mSubHdr_debug >= 2)
      {
         printf("Template line: [%s]\n", line);
         fflush(stdout);
      }

      for(i=strlen(line); i<80; ++i)
         line[i] = ' ';
      
      line[80] = '\0';

      strcat(header[0], line);

      mSubHdr_parseLine(line);
   }

   fclose(fp);

   if(mSubHdr_debug >= 2)
   {
      printf("\nheader ----------------------------------------\n");
      printf("%s\n", header[0]);
      printf("-----------------------------------------------\n\n");
   }


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   input.wcs = wcsinit(header[0]);

   if(input.wcs == (struct WorldCoor *)NULL)
   {
      strcpy(montage_msgstr, "Output wcsinit() failed.");
      return 1;
   }

   pixscale = fabs(input.wcs->xinc);
   if(fabs(input.wcs->yinc) < pixscale)
      pixscale = fabs(input.wcs->xinc);


   /*************************************/
   /*  Set up the coordinate transform  */
   /*************************************/

   if(input.wcs->syswcs == WCS_J2000)
   {
      sys   = EQUJ;
      epoch = 2000.;

      if(input.wcs->equinox == 1950.)
         epoch = 1950;
   }
   else if(input.wcs->syswcs == WCS_B1950)
   {
      sys   = EQUB;
      epoch = 1950.;

      if(input.wcs->equinox == 2000.)
         epoch = 2000;
   }
   else if(input.wcs->syswcs == WCS_GALACTIC)
   {
      sys   = GAL;
      epoch = 2000.;
   }
   else if(input.wcs->syswcs == WCS_ECLIPTIC)
   {
      sys   = ECLJ;
      epoch = 2000.;

      if(input.wcs->equinox == 1950.)
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

   input.sys   = sys;
   input.epoch = epoch;

   free(header[0]);

   return 0;
}



/**************************************************/
/*                                                */
/*  Parse header lines from the template,         */
/*  looking for NAXIS1 and NAXIS2                 */
/*                                                */
/**************************************************/

int mSubHdr_parseLine(char *line)
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

   if(mSubHdr_debug >= 2)
   {
      printf("keyword [%s] = value [%s]\n", keyword, value);
      fflush(stdout);
   }

   if(strcmp(keyword, "NAXIS1") == 0)
      input.naxes[0] = atoi(value);

   if(strcmp(keyword, "NAXIS2") == 0)
      input.naxes[1] = atoi(value);

   return 0;
}


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mSubHdr_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}
