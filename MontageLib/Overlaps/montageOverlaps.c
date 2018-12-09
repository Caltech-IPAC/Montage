/* Module: mOverlaps.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
2.0      John Good        30Sep12  Added check for pre-existing four corners
                                   and center in metadata
1.10     John Good        24Jun07  Added CAR offset problem workaround    
1.9      John Good        05Sep06  Increased diff file name range 
                                   (up to "diff.999999.999998.fits")
1.8      John Good        23Mar05  Added code to handle CD matrix tables
1.7      John Good        18Sep03  Removed unnecessary -p argument
1.6      John Good        25Nov03  Added extern optarg references
1.5      John Good        05Oct03  Added faster approximate overlap check
                                   based on great-circle connecting lines
                                   between image corners (exact mode is 
                                   now -e flag)
1.4      John Good        05Oct03  Added NAXIS1,2 alternatives to ns, nl
1.3      John Good        25Aug03  Added status file processing
1.2      John Good        22Mar03  Renamed wcsCheck to checkWCS
                                   for consistency
1.1      John Good        14Mar03  Added fileName() processing,
                                   -p argument, and getopt()
                                   argument processing.  Check for 
                                   missing/invalid images.tbl
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
#include <wcs.h>
#include <coord.h>

#include <mOverlaps.h>
#include <montage.h>

#define MAXSTR 256
#define MAXIMG 256

#define COLINEAR_SEGMENTS 0
#define ENDPOINT_ONLY     1
#define NORMAL_INTERSECT  2
#define NO_INTERSECTION   3

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
   double            cd11;
   double            cd12;
   double            cd21;
   double            cd22;
   Vec               center;
   Vec               corner[4];
   Vec               normal[4];
   double            maxRadius;
   int               cntr;
   char              fname[MAXSTR];
   double            xcorrection;
   double            ycorrection;
}
*input;

static int nimages, maximages;


/*-***********************************************************************/
/*                                                                       */
/*  mOverlaps                                                            */
/*                                                                       */
/*  Given a list of images, determines which ones overlap.  This program */
/*  assumes that the images are relatively small (i.e. not all-sky) and  */
/*  determines if there is overlap by going around the outside of each   */
/*  to see if any of the edge pixels are inside the other.               */
/*                                                                       */
/*   char  *tblfile        Image metadata file                           */
/*   char  *difftbl        Output table of overlap areas                 */
/*                                                                       */
/*   int    quickmode      Use faster but fairly accurate overlap check  */
/*                         rather than full geometry calculation         */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mOverlapsReturn *mOverlaps(char *tblfile, char *difftbl, int quickmode, int debug)
{
   int    i, j, k, l, stat, ncols, overlap, nmatches;
   int    interior, inext, jnext, intersectionCode, mode;
   int    haveCorners;
   double lon, lat;
   double oxpix, oypix;
   double xpos, ypos;
   double x0, y0, z0;
   double x, y, z, dist, dtr;
   double ix, iy;
   int    offscl, namelen, index[4];

   char   fmt[MAXSTR];

   char  *checkWCS;

   FILE  *fout;

   char   header[1600];
   char   temp[80];

   int    icntr;
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
   int    ifname;
   int    icd11;
   int    icd12;
   int    icd21;
   int    icd22;

   int    ira, idec;
   int    ira1, idec1;
   int    ira2, idec2;
   int    ira3, idec3;
   int    ira4, idec4;

   Vec    firstIntersection;
   Vec    secondIntersection;

   struct mOverlapsReturn *returnStruct;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mOverlapsReturn *)malloc(sizeof(struct mOverlapsReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   dtr = atan(1.)/45.;

   fout = fopen(difftbl, "w+");

   if(fout == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Failed to open output %s", difftbl);
      return returnStruct;
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

   icntr    = tcol("cntr");
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
   ifname   = tcol("fname");

   icd11    = tcol("cd1_1");
   icd12    = tcol("cd1_2");
   icd21    = tcol("cd2_1");
   icd22    = tcol("cd2_2");

   ira      = tcol("ra");
   idec     = tcol("dec");
   ira1     = tcol("ra1");
   idec1    = tcol("dec1");
   ira2     = tcol("ra2");
   idec2    = tcol("dec2");
   ira3     = tcol("ra3");
   idec3    = tcol("dec3");
   ira4     = tcol("ra4");
   idec4    = tcol("dec4");

   if(ins < 0)
      ins = tcol("naxis1");

   if(inl < 0)
      inl = tcol("naxis2");

   if(ifname < 0)
      ifname = tcol("file");

   if(icd11 >= 0 && icd12 >= 0  && icd21 >= 0  && icd12 >= 0)
   {
      mode = CD;

      if(debug >= 1)
      {
         printf("CD matrix mode\n");
         fflush(stdout);
      }
   }

   else if(icdelt1 >= 0 && icdelt2 >= 0  && icrota2 >= 0)
   {
      mode = CDELT;

      if(debug >= 1)
      {
         printf("CDELT mode\n");
         fflush(stdout);
      }
   }

   else
   {
      sprintf(returnStruct->msg, "Not enough information to determine coverages (CDELTs or CD matrix)");
      return returnStruct;
   }

   haveCorners = 1;
   if(icntr < 0
   || ira   < 0
   || idec  < 0
   || ira1  < 0
   || idec1 < 0
   || ira2  < 0
   || idec2 < 0
   || ira3  < 0
   || idec3 < 0
   || ira4  < 0
   || idec4 < 0)
      haveCorners = 0;

   if(haveCorners)
      quickmode = 1;

   if(debug >= 1)
   {
      printf("haveCorners = %d\n", haveCorners);
      fflush(stdout);
   }

   if(icntr   < 0
   || ictype1 < 0
   || ictype2 < 0
   || inl     < 0
   || ins     < 0
   || icrval1 < 0
   || icrval2 < 0
   || icrpix1 < 0
   || icrpix2 < 0
   || ifname  < 0)
   {

      sprintf(returnStruct->msg, "Need columns: cntr ctype1 ctype2 nl ns crval1 crval2 crpix1 crpix2 cdelt1 cdelt2 crota2 fname (equinox optional)");
      return returnStruct;
   }



   /***********************************/ 
   /* Read the projection information */ 
   /***********************************/ 

   namelen   =      0;

   nimages   =      0;
   maximages = MAXIMG;

   input = (struct ImgInfo *)malloc(maximages * sizeof(struct ImgInfo));


   while(1)
   {
      if(debug >= 2)
      {
         printf("DEBUG> Reading image table record %d\r", nimages+1);
         fflush(stdout);
      }

      stat = tread();

      if(stat < 0)
         break;

      index[0] = 0;
      index[1] = 1;
      index[2] = 2;
      index[3] = 3;

      input[nimages].cntr = atoi(tval(icntr));

      strcpy(input[nimages].fname, mOverlaps_fileName(tval(ifname)));

      if(strlen(input[nimages].fname) > namelen)
         namelen = strlen(input[nimages].fname);

      if(!haveCorners)
      {
         strcpy(input[nimages].ctype1, tval(ictype1));
         strcpy(input[nimages].ctype2, tval(ictype2));

         input[nimages].naxis1    = atoi(tval(ins));
         input[nimages].naxis2    = atoi(tval(inl));
         input[nimages].crpix1    = atof(tval(icrpix1));
         input[nimages].crpix2    = atof(tval(icrpix2));
         input[nimages].crval1    = atof(tval(icrval1));
         input[nimages].crval2    = atof(tval(icrval2));

         if(mode == CDELT)
         {
            input[nimages].cdelt1 = atof(tval(icdelt1));
            input[nimages].cdelt2 = atof(tval(icdelt2));
            input[nimages].crota2 = atof(tval(icrota2));
         }
         else
         {
            input[nimages].cd11   = atof(tval(icd11));
            input[nimages].cd12   = atof(tval(icd12));
            input[nimages].cd21   = atof(tval(icd21));
            input[nimages].cd22   = atof(tval(icd22));
         }

         input[nimages].equinox   = 2000;
         input[nimages].maxRadius = 0.;

         strcpy(header, "");
         sprintf(temp, "SIMPLE  = T"                             ); mOverlaps_stradd(header, temp);
         sprintf(temp, "BITPIX  = -64"                           ); mOverlaps_stradd(header, temp);
         sprintf(temp, "NAXIS   = 2"                             ); mOverlaps_stradd(header, temp);
         sprintf(temp, "NAXIS1  = %d",     input[nimages].naxis1 ); mOverlaps_stradd(header, temp);
         sprintf(temp, "NAXIS2  = %d",     input[nimages].naxis2 ); mOverlaps_stradd(header, temp);
         sprintf(temp, "CTYPE1  = '%s'",   input[nimages].ctype1 ); mOverlaps_stradd(header, temp);
         sprintf(temp, "CTYPE2  = '%s'",   input[nimages].ctype2 ); mOverlaps_stradd(header, temp);
         sprintf(temp, "CRVAL1  = %11.6f", input[nimages].crval1 ); mOverlaps_stradd(header, temp);
         sprintf(temp, "CRVAL2  = %11.6f", input[nimages].crval2 ); mOverlaps_stradd(header, temp);
         sprintf(temp, "CRPIX1  = %11.6f", input[nimages].crpix1 ); mOverlaps_stradd(header, temp);
         sprintf(temp, "CRPIX2  = %11.6f", input[nimages].crpix2 ); mOverlaps_stradd(header, temp);

         if(mode == CDELT)
         {
            sprintf(temp, "CDELT1  = %14.9f", input[nimages].cdelt1 ); mOverlaps_stradd(header, temp);
            sprintf(temp, "CDELT2  = %14.9f", input[nimages].cdelt2 ); mOverlaps_stradd(header, temp);
            sprintf(temp, "CROTA2  = %11.6f", input[nimages].crota2 ); mOverlaps_stradd(header, temp);
         }
         else
         {
            sprintf(temp, "CD1_1   = %11.6f", input[nimages].cd11   ); mOverlaps_stradd(header, temp);
            sprintf(temp, "CD1_2   = %11.6f", input[nimages].cd12   ); mOverlaps_stradd(header, temp);
            sprintf(temp, "CD2_1   = %11.6f", input[nimages].cd21   ); mOverlaps_stradd(header, temp);
            sprintf(temp, "CD2_2   = %11.6f", input[nimages].cd22   ); mOverlaps_stradd(header, temp);
         }

         sprintf(temp, "CRVAL2  = %11.6f", input[nimages].crval2 ); mOverlaps_stradd(header, temp);
         sprintf(temp, "CRVAL1  = %11.6f", input[nimages].crval1 ); mOverlaps_stradd(header, temp);
         sprintf(temp, "EQUINOX = %d",     input[nimages].equinox); mOverlaps_stradd(header, temp);
         sprintf(temp, "END"                                     ); mOverlaps_stradd(header, temp);
         
         if(iequinox >= 0)
            input[nimages].equinox = atoi(tval(iequinox));

         input[nimages].wcs = wcsinit(header);

         checkWCS = montage_checkWCS(input[nimages].wcs);

         if(checkWCS)
         {
            free(input);
            strcpy(returnStruct->msg, checkWCS);
            return returnStruct;
         }
                                
         if(input[nimages].wcs == (struct WorldCoor *)NULL)
         {
            free(input);
            sprintf(returnStruct->msg, "Bad WCS for image %d", nimages);
            return returnStruct;
         }


         /* Kludge to get around bug in WCS library:   */
         /* 360 degrees sometimes added to pixel coord */

         ix = 0.5;
         iy = 0.5;

         pix2wcs(input[nimages].wcs, ix, iy, &xpos, &ypos);

         offscl = input[nimages].wcs->offscl;

         if(!offscl)
            wcs2pix(input[nimages].wcs, xpos, ypos, &x, &y, &offscl);

         input[nimages].xcorrection = x-ix;
         input[nimages].ycorrection = y-iy;


         /* We need to get the corners in "clockwise" order */

         if((input[nimages].wcs->xinc < 0 && input[nimages].wcs->yinc < 0)
         || (input[nimages].wcs->xinc > 0 && input[nimages].wcs->yinc > 0))
         {
            index[0] = 0;
            index[1] = 1;
            index[2] = 2;
            index[3] = 3;
         }
         else
         {
            index[0] = 3;
            index[1] = 2;
            index[2] = 1;
            index[3] = 0;
         }
      }


      /* Coordinates of center */

      if(haveCorners)
      {
         xpos = atof(tval(ira));
         ypos = atof(tval(idec));
      }
      else
         pix2wcs(input[nimages].wcs, input[nimages].naxis1/2., input[nimages].naxis2/2., 
            &xpos, &ypos);
      
      if(debug >= 2)
      {
         printf("%d center = %.6f %.6f\n", nimages, xpos, ypos);
         fflush(stdout);
      }
   
      x0 = cos(ypos*dtr) * cos(xpos*dtr);
      y0 = cos(ypos*dtr) * sin(xpos*dtr);
      z0 = sin(ypos*dtr);

      input[nimages].center.x = x0;
      input[nimages].center.y = y0;
      input[nimages].center.z = z0;

      
      /* Lower left */

      if(haveCorners)
      {
         xpos = atof(tval(ira1));
         ypos = atof(tval(idec1));
      }
      else
         pix2wcs(input[nimages].wcs, 0.5, 0.5, &xpos, &ypos);

      x = cos(ypos*dtr) * cos(xpos*dtr);
      y = cos(ypos*dtr) * sin(xpos*dtr);
      z = sin(ypos*dtr);

      input[nimages].corner[index[0]].x = x;
      input[nimages].corner[index[0]].y = y;
      input[nimages].corner[index[0]].z = z;

      dist = acos(x*x0 + y*y0 + z*z0) / dtr;

      if(dist > input[nimages].maxRadius) 
         input[nimages].maxRadius = dist;


      /* Lower right */

      if(haveCorners)
      {
         xpos = atof(tval(ira2));
         ypos = atof(tval(idec2));
      }
      else
         pix2wcs(input[nimages].wcs, input[nimages].naxis1+0.5, 0.5, &xpos, &ypos);

      x = cos(ypos*dtr) * cos(xpos*dtr);
      y = cos(ypos*dtr) * sin(xpos*dtr);
      z = sin(ypos*dtr);

      input[nimages].corner[index[1]].x = x;
      input[nimages].corner[index[1]].y = y;
      input[nimages].corner[index[1]].z = z;

      dist = acos(x*x0 + y*y0 + z*z0) / dtr;

      if(dist > input[nimages].maxRadius) 
         input[nimages].maxRadius = dist;

      
      /* Upper right */
      if(haveCorners)
      {
         xpos = atof(tval(ira3));
         ypos = atof(tval(idec3));
      }
      else
         pix2wcs(input[nimages].wcs, input[nimages].naxis1+0.5, 
                 input[nimages].naxis2+0.5, &xpos, &ypos);

      x = cos(ypos*dtr) * cos(xpos*dtr);
      y = cos(ypos*dtr) * sin(xpos*dtr);
      z = sin(ypos*dtr);

      input[nimages].corner[index[2]].x = x;
      input[nimages].corner[index[2]].y = y;
      input[nimages].corner[index[2]].z = z;

      dist = acos(x*x0 + y*y0 + z*z0) / dtr;

      if(dist > input[nimages].maxRadius) 
         input[nimages].maxRadius = dist;

      
      /* Upper left */

      if(haveCorners)
      {
         xpos = atof(tval(ira4));
         ypos = atof(tval(idec4));
      }
      else
         pix2wcs(input[nimages].wcs, 0.5, input[nimages].naxis2+0.5, &xpos, &ypos);

      x = cos(ypos*dtr) * cos(xpos*dtr);
      y = cos(ypos*dtr) * sin(xpos*dtr);
      z = sin(ypos*dtr);

      input[nimages].corner[index[3]].x = x;
      input[nimages].corner[index[3]].y = y;
      input[nimages].corner[index[3]].z = z;

      dist = acos(x*x0 + y*y0 + z*z0) / dtr;

      if(dist > input[nimages].maxRadius) 
         input[nimages].maxRadius = dist;


      /* Normals to the image "sides" */

      for(i=0; i<4; ++i)
      {
         inext = (i+1)%4;

         mOverlaps_Cross(&input[nimages].corner[i], &input[nimages].corner[inext], &input[nimages].normal[i]);
      }

      ++nimages;

      if(nimages >= maximages)
      {
         maximages += MAXIMG;
         input = (struct ImgInfo *)realloc(input, 
                                      maximages * sizeof(struct ImgInfo));
      }
   }

   if(debug >= 1)
   {
      printf("nimages = %d\n", nimages);
      fflush(stdout);
   }

   sprintf(fmt, "| cntr1 | cntr2 |%%%ds |%%%ds |         diff             |\n", namelen, namelen);
   fprintf(fout, fmt, "plus", "minus");

   sprintf(fmt, "| int   | int   |%%%ds |%%%ds |         char             |\n", namelen, namelen);
   fprintf(fout, fmt, "char", "char");

   fflush(fout);



   /************************************************/
   /* Get the coordinate system and epoch in a     */
   /* form compatible with the conversion library  */
   /************************************************/

   if(haveCorners)
   {
      for(i=0; i<nimages; ++i)
      {
         input[i].sys   = EQUJ;
         input[i].epoch = 2000.;
      }
   }

   else
   {
      for(i=0; i<nimages; ++i)
      {
         if(input[i].wcs->syswcs == WCS_J2000)
         {
            input[i].sys   = EQUJ;
            input[i].epoch = 2000.;

            if(input[i].wcs->equinox == 1950)
               input[i].epoch = 1950.;
         }
         else if(input[i].wcs->syswcs == WCS_B1950)
         {
            input[i].sys   = EQUB;
            input[i].epoch = 1950.;

            if(input[i].wcs->equinox == 2000)
               input[i].epoch = 2000;
         }
         else if(input[i].wcs->syswcs == WCS_GALACTIC)
         {
            input[i].sys   = GAL;
            input[i].epoch = 2000.;
         }
         else if(input[i].wcs->syswcs == WCS_ECLIPTIC)
         {
            input[i].sys   = ECLJ;
            input[i].epoch = 2000.;

            if(input[i].wcs->equinox == 1950)
            {
               input[i].sys   = ECLB;
               input[i].epoch = 1950.;
            }
         }
         else       
         {
            input[i].sys   = EQUJ;
            input[i].epoch = 2000.;
         }
      }
   }


   /************************************************/
   /* For each image, compare it to all the images */
   /* that come after it in the list.              */
   /************************************************/

   nmatches = 0;

   sprintf(fmt, "%%8d%%8d %%%ds  %%%ds  diff.%%06d.%%06d.fits\n", namelen, namelen);


   for(k=0; k<nimages; ++k)
   {
      for(l=k+1; l<nimages; ++l)
      {
         /* Check to see if the bounding radius circles */
         /* overlap (abandon if not)                    */

         dist = acos(mOverlaps_Dot(&input[k].center, &input[l].center)) / dtr;

         if(debug >= 1)
         {
            printf("\nComparing %d and %d (%s and %s) [(%-g,%-g,%-g) and (%-g,%-g,%-g)]\n",
               input[k].cntr, input[l].cntr,
               input[k].fname, input[l].fname,
               input[k].center.x,
               input[k].center.y,
               input[k].center.z,
               input[l].center.x,
               input[l].center.y,
               input[l].center.z);

            printf("  dist = %-g < %-g ? (%-g + %-g)\n", 
               dist, input[k].maxRadius + input[l].maxRadius,
               input[k].maxRadius,
               input[l].maxRadius);

            fflush(stdout);
         }

         if(dist > input[k].maxRadius + input[l].maxRadius)
            continue;


         /* Big Switch:  Either we are doing the comparison exactly */
         /* (checking the corners of each pixel) or we are just     */
         /* checking for overlapping great circle side segments     */

         if(quickmode)
         {
            /* Region inside image check */

            overlap = 0;

            for(i=0; i<4; ++i)
            {
               interior = 1;

               for(j=0; j<4; ++j)
               {
                  if(mOverlaps_Dot(&input[l].normal[j], &input[k].corner[i]) < 0)
                  {
                     interior = 0;
                     break;
                  }
               }

               if(interior)
               {
                  overlap = 1;

                  break;
               }
            }


            /* Image inside region check */

            if(!overlap)
            {
               for(i=0; i<4; ++i)
               {
                  interior = 1;

                  for(j=0; j<4; ++j)
                  {
                     if(mOverlaps_Dot(&input[k].normal[j], &input[l].corner[i]) < 0)
                     {
                        interior = 0;
                        break;
                     }
                  }

                  if(interior)
                  {
                     overlap = 1;

                     break;
                  }
               }
            }


            /* Overlapping segments check */

            if(!overlap)
            {
               for(j=0; j<4; ++j)
               {
                  jnext = (j+1)%4;

                  for(i=0; i<4; ++i)
                  {
                     inext = (i+1)%4;

                     intersectionCode = mOverlaps_SegSegIntersect(&input[l].normal[j], &input[k].normal[i], 
                                                                  &input[l].corner[j], &input[l].corner[jnext],
                                                                  &input[k].corner[i], &input[k].corner[inext], 
                                                                  &firstIntersection,  &secondIntersection);


                     if(intersectionCode == NORMAL_INTERSECT 
                     || intersectionCode == ENDPOINT_ONLY) 
                     {
                        overlap = 1;

                        break;
                     }
                  }

                  if(overlap)
                     break;
               }
            }


            /* If it passed any of the checks, copy the record to output */

            if(overlap)
            {
               ++nmatches;
               fprintf(fout, fmt, input[k].cntr, input[l].cntr,
               input[k].fname, input[l].fname, input[k].cntr, input[l].cntr);
               fflush(fout);
            }
            
            continue;
         }

         else
         {
            /* Go around the outside of the input image,    */
            /* finding the range of output pixel locations  */

            /* Left and right */

            overlap = 0;

            for (j=0; j<input[k].naxis2+1; ++j)
            {
               pix2wcs(input[k].wcs, 0.5, j+0.5, &xpos, &ypos);

               convertCoordinates(input[k].sys, input[k].epoch, xpos, ypos,
                                  input[l].sys, input[l].epoch, &lon, &lat, 0.0);
               
               offscl = input[k].wcs->offscl;

               if(!offscl)
                  wcs2pix(input[l].wcs, lon, lat, &oxpix, &oypix, &offscl);

               mOverlaps_fixxy(l, &oxpix, &oypix, &offscl);

               if(debug >= 1)
               {
                  i = 0;

                  printf("\n(i,j)         = (%-g,%-g)\n", i+0.5, j+0.5);
                  printf("(xpos,ypos)   = (%-g,%-g)\n", xpos, ypos);
                  printf("(lon,lat)     = (%-g,%-g)\n", lon, lat);
                  printf("(oxpix,oypix) = (%-g,%-g)\n", oxpix, oypix);
               }

               if(!offscl)
               {
                  ++nmatches;

                  fprintf(fout, fmt, input[k].cntr, input[l].cntr,
                     input[k].fname, input[l].fname, input[k].cntr, input[l].cntr);
                  fflush(fout);
                  overlap = 1;
                  break;
               }

               pix2wcs(input[k].wcs, input[k].naxis1+0.5, j+0.5, &xpos, &ypos);

               convertCoordinates(input[k].sys, input[k].epoch, xpos, ypos,
                                  input[l].sys, input[l].epoch, &lon, &lat, 0.0);
               
               offscl = input[k].wcs->offscl;

               if(!offscl)
                  wcs2pix(input[l].wcs, lon, lat, &oxpix, &oypix, &offscl);

               mOverlaps_fixxy(l, &oxpix, &oypix, &offscl);

               if(debug >= 1)
               {
                  i = input[k].naxis1;

                  printf("\n(i,j)         = (%-g,%-g)\n", i+0.5, j+0.5);
                  printf("(xpos,ypos)   = (%-g,%-g)\n", xpos, ypos);
                  printf("(lon,lat)     = (%-g,%-g)\n", lon, lat);
                  printf("(oxpix,oypix) = (%-g,%-g)\n", oxpix, oypix);
               }

               if(!offscl)
               {
                  ++nmatches;
                  fprintf(fout, fmt, input[k].cntr, input[l].cntr,
                     input[k].fname, input[l].fname, input[k].cntr, input[l].cntr);
                  fflush(fout);
                  overlap = 1;
                  break;
               }
            }

            if(overlap)
               continue;


            /* Top and bottom */

            for (i=0; i<input[k].naxis1+1; ++i)
            {
               pix2wcs(input[k].wcs, i+0.5, 0.5, &xpos, &ypos);

               convertCoordinates(input[k].sys, input[k].epoch, xpos, ypos,
                                  input[l].sys, input[l].epoch, &lon, &lat, 0.0);
               
               offscl = input[k].wcs->offscl;

               if(!offscl)
                  wcs2pix(input[l].wcs, lon, lat, &oxpix, &oypix, &offscl);

               mOverlaps_fixxy(l, &oxpix, &oypix, &offscl);

               if(debug >= 1)
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
                  fprintf(fout, fmt, input[k].cntr, input[l].cntr,
                     input[k].fname, input[l].fname, input[k].cntr, input[l].cntr);
                  fflush(fout);
                  overlap = 1;
                  break;
               }

               pix2wcs(input[k].wcs, i+0.5, input[k].naxis2+0.5, &xpos, &ypos);

               convertCoordinates(input[k].sys, input[k].epoch, xpos, ypos,
                                  input[l].sys, input[l].epoch, &lon, &lat, 0.0);
               
               offscl = input[k].wcs->offscl;

               if(!offscl)
                  wcs2pix(input[l].wcs, lon, lat, &oxpix, &oypix, &offscl);

               mOverlaps_fixxy(l, &oxpix, &oypix, &offscl);

               if(debug >= 1)
               {
                  j = input[k].naxis2;

                  printf("\n(i,j)         = (%-g,%-g)\n", i+0.5, j+0.5);
                  printf("(xpos,ypos)   = (%-g,%-g)\n", xpos, ypos);
                  printf("(lon,lat)     = (%-g,%-g)\n", lon, lat);
                  printf("(oxpix,oypix) = (%-g,%-g)\n", oxpix, oypix);
               }

               if(!offscl)
               {
                  ++nmatches;
                  fprintf(fout, fmt, input[k].cntr, input[l].cntr, 
                     input[k].fname, input[l].fname, input[k].cntr, input[l].cntr);
                  fflush(fout);
                  overlap = 1;
                  break;
               }
            }
         }
      }
   }

   free(input);

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d",       nmatches);
   sprintf(returnStruct->json, "{\"count\":%d}", nmatches);

   returnStruct->count = nmatches;

   return returnStruct;
}


int mOverlaps_stradd(char *header, char *card)
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


/*************************************************************************/
/*                                                                       */
/*  fileName                                                             */
/*                                                                       */
/*  This routine pulls out the file name (no path info) from a string    */
/*                                                                       */
/*************************************************************************/

char *mOverlaps_fileName(char *fname)
{
   int   i, len;


   /* Pull out the last part of the */
   /* string (the file name)        */

   len = strlen(fname);

   for(i=len-1; i>=0; --i)
   {
      if(fname[i] == '/')
    return(fname + i + 1);
   }

   return(fname);
}


/**************************************************/
/*  Projections like CAR sometimes add an extra   */
/*  360 degrees worth of pixels to the return     */
/*  and call it off-scale.                        */
/**************************************************/

void mOverlaps_fixxy(int l, double *x, double *y, int *offscl)
{
   *x = *x - input[l].xcorrection;
   *y = *y - input[l].ycorrection;

   if(*x < 0.
   || *x > input[l].wcs->nxpix+1.
   || *y < 0.
   || *y > input[l].wcs->nypix+1.)
      *offscl = 1;

   return;
}


/****************************************************************************/
/*                                                                          */
/* SegSegIntersect()                                                        */
/*                                                                          */
/* Finds the point of intersection p between two closed                     */
/* segments ab and cd.  Returns p and a char with the following meaning:    */
/*                                                                          */
/*   COLINEAR_SEGMENTS: The segments colinearly overlap, sharing a point.   */
/*                                                                          */
/*   ENDPOINT_ONLY:     An endpoint (vertex) of one segment is on the other */
/*                      segment, but COLINEAR_SEGMENTS doesn't hold.        */
/*                                                                          */
/*   NORMAL_INTERSECT:  The segments intersect properly (i.e., they share   */
/*                      a point and neither ENDPOINT_ONLY nor               */
/*                      COLINEAR_SEGMENTS holds).                           */
/*                                                                          */
/*   NO_INTERSECTION:   The segments do not intersect (i.e., they share     */
/*                      no points).                                         */
/*                                                                          */
/* Note that two colinear segments that share just one point, an endpoint   */
/* of each, returns COLINEAR_SEGMENTS rather than ENDPOINT_ONLY as one      */
/* might expect.                                                            */
/*                                                                          */
/****************************************************************************/

int mOverlaps_SegSegIntersect(Vec *pEdge, Vec *qEdge, 
                              Vec *p0, Vec *p1, Vec *q0, Vec *q1, 
                              Vec *intersect1, Vec *intersect2)
{
   double pDot,  qDot;  /* Dot product [cos(length)] of the edge vertices */
   double p0Dot, p1Dot; /* Dot product from vertices to intersection      */
   double q0Dot, q1Dot; /* Dot product from vertices to intersection      */
   int    len;


   /* Get the edge lengths (actually cos(length)) */

   pDot = mOverlaps_Dot(p0, p1);
   qDot = mOverlaps_Dot(q0, q1);


   /* Find the point of intersection */

   len = mOverlaps_Cross(pEdge, qEdge, intersect1);


   /* If the two edges are colinear, */ 
   /* check to see if they overlap   */

   if(len == 0)
   {
      if(mOverlaps_Between(q0, p0, p1)
      && mOverlaps_Between(q1, p0, p1))
      {
         intersect1 = q0;
         intersect2 = q1;
         return COLINEAR_SEGMENTS;
      }

      if(mOverlaps_Between(p0, q0, q1)
      && mOverlaps_Between(p1, q0, q1))
      {
         intersect1 = p0;
         intersect2 = p1;
         return COLINEAR_SEGMENTS;
      }

      if(mOverlaps_Between(q0, p0, p1)
      && mOverlaps_Between(p1, q0, q1))
      {
         intersect1 = q0;
         intersect2 = p1;
         return COLINEAR_SEGMENTS;
      }

      if(mOverlaps_Between(p0, q0, q1)
      && mOverlaps_Between(q1, p0, p1))
      {
         intersect1 = p0;
         intersect2 = q1;
         return COLINEAR_SEGMENTS;
      }

      if(mOverlaps_Between(q1, p0, p1)
      && mOverlaps_Between(p1, q0, q1))
      {
         intersect1 = p0;
         intersect2 = p1;
         return COLINEAR_SEGMENTS;
      }

      if(mOverlaps_Between(q0, p0, p1)
      && mOverlaps_Between(p0, q0, q1))
      {
         intersect1 = p0;
         intersect2 = q0;
         return COLINEAR_SEGMENTS;
      }

      return NO_INTERSECTION;
   }


   /* If this is the wrong one of the two */
   /* (other side of the sky) reverse it  */

   mOverlaps_Normalize(intersect1);

   if(mOverlaps_Dot(intersect1, p0) < 0.)
      mOverlaps_Reverse(intersect1);


   /* Point has to be inside both sides to be an intersection */

   p0Dot = mOverlaps_Dot(intersect1, p0);
   p1Dot = mOverlaps_Dot(intersect1, p1);
   q0Dot = mOverlaps_Dot(intersect1, q0);
   q1Dot = mOverlaps_Dot(intersect1, q1);

   if((p0Dot = mOverlaps_Dot(intersect1, p0)) <  pDot) return NO_INTERSECTION;
   if((p1Dot = mOverlaps_Dot(intersect1, p1)) <  pDot) return NO_INTERSECTION;
   if((q0Dot = mOverlaps_Dot(intersect1, q0)) <  qDot) return NO_INTERSECTION;
   if((q1Dot = mOverlaps_Dot(intersect1, q1)) <  qDot) return NO_INTERSECTION;


   /* Otherwise, if the intersection is at an endpoint */

   if(p0Dot == pDot) return ENDPOINT_ONLY;
   if(p1Dot == pDot) return ENDPOINT_ONLY;
   if(q0Dot == qDot) return ENDPOINT_ONLY;
   if(q1Dot == qDot) return ENDPOINT_ONLY;


   /* Otherwise, it is a normal intersection */

   return NORMAL_INTERSECT;
}



/***************************************************/
/*                                                 */
/* swap()                                          */
/*                                                 */
/* Switches the values of two memory locations     */
/*                                                 */
/***************************************************/

int mOverlaps_swap(double *x, double *y)
{
   double tmp;

   tmp = *x;
   *x  = *y;
   *y  = tmp;

   return(0);
}




/***************************************************/
/*                                                 */
/* Between()                                       */
/*                                                 */
/* Tests whether whether a point on an arc is      */
/* between two other points.                       */
/*                                                 */
/***************************************************/

int mOverlaps_Between(Vec *v, Vec *a, Vec *b)
{
   double abDot, avDot, bvDot;

   abDot = mOverlaps_Dot(a, b);
   avDot = mOverlaps_Dot(a, v);
   bvDot = mOverlaps_Dot(b, v);

   if(avDot > abDot
   && bvDot > abDot)
      return 1;
   else
      return 0;
}


/***************************************************/
/*                                                 */
/* Cross()                                         */
/*                                                 */
/* Vector cross product.                           */
/*                                                 */
/***************************************************/

int mOverlaps_Cross(Vec *v1, Vec *v2, Vec *v3)
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

double mOverlaps_Dot(Vec *a, Vec *b)
{
   double sum = 0.0;

   sum = a->x * b->x
       + a->y * b->y
       + a->z * b->z;

   return sum;
}


/***************************************************/
/*                                                 */
/* Normalize()                                     */
/*                                                 */
/* Normalize the vector                            */
/*                                                 */
/***************************************************/

double mOverlaps_Normalize(Vec *v)
{
   double len;

   len = 0.;

   len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);

   v->x = v->x / len;
   v->y = v->y / len;
   v->z = v->z / len;

   return len;
}


/***************************************************/
/*                                                 */
/* Reverse()                                       */
/*                                                 */
/* Reverse the vector                              */
/*                                                 */
/***************************************************/

void mOverlaps_Reverse(Vec *v)
{
   v->x = -v->x;
   v->y = -v->y;
   v->z = -v->z;
}
