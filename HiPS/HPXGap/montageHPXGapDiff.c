/* Module: mHPXGapDiff.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        23Feb21  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#include <fitsio.h>

#include <mHPXGap.h>

#define MAXSTR  256

#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;}

char mHPXGapDiff_edges[4][32] = {"top", "left", "right", "bottom"};

static time_t currtime, start;

static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mHPXGapDiff                                                          */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mHPXGapDiff, is used in conjuction with mDiff and       */
/*  mBgModel to determine how overlapping images relate to each          */
/*  other.  It is assumed that difference images have matching structure */
/*  information and that what is left when you difference them is just   */
/*  the relative offsets, slopes, etc.  By fitting the difference image, */
/*  we obtain the 'correction' that needs to be applied to one or the    */
/*  other (or in part to both) to bring them together.                   */
/*                                                                       */
/*   char  *plus_path    'Plus' image in the difference.                 */
/*   int    plus_edge     Edge of the image to background fit.           */
/*   char  *minus_path   'Plus' image in the difference.                 */
/*   int    minus_edge    Edge of the image to background fit.           */
/*   char  *gap_dir       Output directory for diff info.                */
/*   int    levelOnly     Only fit for level difference not a full       */
/*                        plane with slopes.                             */
/*   int    width         The width from the edge to include.            */
/*   int    debug         Debugging output level.                        */
/*                                                                       */
/*************************************************************************/

struct mHPXGapDiffReturn *mHPXGapDiff(char *plus_path, int plus_edge, char *minus_path, int minus_edge,
                                      char *gap_dir, int levelOnly, int width, int debug)
{
   char      tmpstr    [1024];
   char      basename  [1024];

   char     *ptr, *first;

   double    a, b, c;
   double    ap, bp, cp;
   double    adiff, bdiff, cdiff;

   double    x0, y0;
   double    x0p, y0p;

   FILE     *diff_file;
   FILE     *view_script;

   struct mHPXGapDiffReturn *returnStruct;

   time(&currtime);
   start = currtime;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mHPXGapDiffReturn *)malloc(sizeof(struct mHPXGapDiffReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /* Strip down plus_path name to get the "base" name */

   first = (char *)NULL;

   ptr = plus_path;

   while(ptr < plus_path + strlen(plus_path))
   {
      if(*ptr == '/')
         first = ptr+1;

      ++ptr;
   }

   if(first == (char *)NULL)
      first = plus_path;

   strcpy(basename, first);

   ptr = basename + strlen(basename);

   while(*ptr != '.' && ptr > basename)
      --ptr;

   if(strcasecmp(ptr, ".fits") == 0
   || strcasecmp(ptr, ".fts" ) == 0
   || strcasecmp(ptr, ".fit" ) == 0)
      *ptr = '\0';

   if(debug)
   {
      printf("basename = [%s]\n", basename);
      fflush(stdout);
   }


   /* Open the diff table for this difference */

   if(strlen(gap_dir) == 0)
      sprintf(tmpstr, "./gap");
   else
      sprintf(tmpstr, "%s/gap", gap_dir);

   if(mkdir(tmpstr, 0755) < 0 && errno != EEXIST)
   {
      sprintf(returnStruct->msg, "Cannot create gap directory [%s]", tmpstr);
      return returnStruct;
   }


   if(strlen(gap_dir) == 0)
      sprintf(tmpstr, "./gap/%s.diff", basename);
   else
      sprintf(tmpstr, "%s/gap/%s.diff", gap_dir, basename);

   if(debug)
   {
      printf("diff_file = [%s]\n", tmpstr);
      fflush(stdout);
   }

   diff_file = fopen(tmpstr, "w+");

   if(diff_file == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Cannot create diff file [%s]", tmpstr);
      return returnStruct;
   }


   if(strlen(gap_dir) == 0)
      sprintf(tmpstr, "./gap/%s.sh", basename);
   else
      sprintf(tmpstr, "%s/gap/%s.sh", gap_dir, basename);

   if(debug)
   {
      printf("view_script = [%s]\n", tmpstr);
      fflush(stdout);
   }

   view_script = fopen(tmpstr, "w+");

   if(view_script == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Cannot create diff file [%s]", tmpstr);
      return returnStruct;
   }

   chmod(tmpstr, 0755);


   // Compute fit for the 'plus' image
   
   mHPXGapDiff_fitImage(plus_path, levelOnly, plus_edge, width, returnStruct, 0, debug);

   if(returnStruct->status == 1)
   {
      if(debug)
      {
          printf("\nPLUS ERROR: \"%s\"]\n", returnStruct->msg);
          fflush(stdout);
          return returnStruct;
      }
   }
   
   
   // Compute fit for the 'minus' image
   
   mHPXGapDiff_fitImage(minus_path, levelOnly, minus_edge, width, returnStruct, 1, debug);

   if(returnStruct->status == 1)
   {
      if(debug)
      {
          printf("\nMINUS ERROR: \"%s\"]\n", returnStruct->msg);
          fflush(stdout);
          return returnStruct;
      }
   }


   // Find the reference pixels coordinates
   
   if(plus_edge == TOP)
   {
      x0 = -returnStruct->crpix1[0];
      y0 = -returnStruct->crpix2[0] + returnStruct->naxis2[0]; 

      x0p = -returnStruct->crpix1[1];
      y0p = -returnStruct->crpix2[1] + returnStruct->naxis2[1]; 
   }

   else if(plus_edge == BOTTOM)
   {
      x0 = -returnStruct->crpix1[0];
      y0 = -returnStruct->crpix2[0]; 

      x0p = -returnStruct->crpix1[1] + returnStruct->naxis1[1];
      y0p = -returnStruct->crpix2[1] + returnStruct->naxis2[1]; 
   }

   else
   {
      strcpy(returnStruct->msg, "Only TOP (to LEFT) and BOTTOM (to RIGHT) gap comparisons are implemented for HPX projection.");
      return returnStruct;
   }

   // -----------------------------------------------------

   // Transform the 'plus' parameters to 'minus' space

   a = returnStruct->a[0];
   b = returnStruct->b[0];
   c = returnStruct->c[0];
   
   ap =  b;
   bp = -a;

   cp = (a*x0 + b*y0 + c) - (ap*x0p + bp*y0p);

   adiff = returnStruct->a[1] - ap;
   bdiff = returnStruct->b[1] - bp;
   cdiff = returnStruct->c[1] - cp;

   if(debug)
   {
      printf("\nPLUS:            a = %14.9f,  b = %14.9f,  c = %14.9f\n", a, b, c);
      printf("PLUS -> MINUS:  ap = %14.9f, bp = %14.9f, cp = %14.9f\n", ap, bp, cp);
      fflush(stdout);
   }

   b =  ap;
   a = -bp;

   c = (ap*x0p + bp*y0p + cp) - (a*x0 + b*y0);

   if(debug)
   {
      printf("PLUS <- MINUS:   a = %14.9f,  b = %14.9f,  c = %14.9f\n\n", a, b, c);
      printf("MINUS diff:      a = %14.9f,  b = %14.9f,  c = %14.9f\n\n", adiff, bdiff, cdiff);
      fflush(stdout);
   }

   returnStruct->transform[0][0][0] =   0.;   returnStruct->transform[0][0][1] =   1.;   returnStruct->transform[0][0][2] = 0.;
   returnStruct->transform[0][1][0] =  -1.;   returnStruct->transform[0][1][1] =   0.;   returnStruct->transform[0][1][2] = 0.;
   returnStruct->transform[0][2][0] = x0+y0p; returnStruct->transform[0][2][1] = y0-x0p; returnStruct->transform[0][2][2] = 1.;


   fprintf(view_script, "#!/bin/sh\n\nmBackground -n %s minus_corrected.fits %-g %-g %-g\n",
         minus_path, adiff/2., bdiff/2., cdiff/2.);

   fprintf(diff_file, "plus_file                 : %s\n",             plus_path);
   fprintf(diff_file, "minus_file                : %s\n",             minus_path);
   fprintf(diff_file, "A,B,C                     : %.5e %.5e %.5e\n", adiff, bdiff, cdiff);
   fprintf(diff_file, "crpix                     : %.2f %.2f\n",      returnStruct->crpix1[0],   returnStruct->crpix2[0]);
   fprintf(diff_file, "xmin,xmax                 : %d %d\n",          returnStruct->xmin[0],     returnStruct->xmax[0]);
   fprintf(diff_file, "ymin,ymax                 : %d %d\n",          returnStruct->ymin[0],     returnStruct->ymax[0]); 
   fprintf(diff_file, "xcenter,ycenter           : %.2f %.2f\n",      returnStruct->xcenter[0],  returnStruct->ycenter[0]);
   fprintf(diff_file, "npixel,rms                : %d %.5e\n",        returnStruct->npixel[0],   returnStruct->rms[0]);
   fprintf(diff_file, "boxx,boxy                 : %.2f %.2f \n",     returnStruct->boxx[0],     returnStruct->boxy[0]);
   fprintf(diff_file, "boxwidth,boxheight,boxang : %.2f %.2f %.2f\n", returnStruct->boxwidth[0], returnStruct->boxheight[0], returnStruct->boxang[0]);
   fprintf(diff_file, "have_transform            : true\n");
   fprintf(diff_file, "transform                 : %12.5e %12.5e %12.5e\n", returnStruct->transform[0][0][0], returnStruct->transform[0][0][1], returnStruct->transform[0][0][2]);
   fprintf(diff_file, "                            %12.5e %12.5e %12.5e\n", returnStruct->transform[0][1][0], returnStruct->transform[0][1][1], returnStruct->transform[0][1][2]);
   fprintf(diff_file, "                            %12.5e %12.5e %12.5e\n", returnStruct->transform[0][2][0], returnStruct->transform[0][2][1], returnStruct->transform[0][2][2]);
   fprintf(diff_file, "\n");

   fflush(diff_file);

   // -----------------------------------------------------

   // Transform the 'minus' parameters to 'plus' space
   
   ap = returnStruct->a[1];
   bp = returnStruct->b[1];
   cp = returnStruct->c[1];
   
   b =  ap;
   a = -bp;

   c = (ap*x0p + bp*y0p + cp) - (a*x0 + b*y0);

   adiff = returnStruct->a[0] - a;
   bdiff = returnStruct->b[0] - b;
   cdiff = returnStruct->c[0] - c;

   if(debug)
   {
      printf("MINUS:          ap = %14.9f, bp = %14.9f, cp = %14.9f\n", ap, bp, cp);
      printf("MINUS -> PLUS:   a = %14.9f, b  = %14.9f,  c = %14.9f\n", a, b, c);
      fflush(stdout);
   }
   
   ap =  b;
   bp = -a;

   cp = (a*x0 + b*y0 + c) - (ap*x0p + bp*y0p);

   if(debug)
   {
         printf("MINUS <- PLUS:  ap = %14.9f, bp = %14.9f, cp = %14.9f\n\n", ap, bp, cp);
         printf("PLUS diff:       a = %14.9f,  b = %14.9f,  c = %14.9f\n\n", adiff, bdiff, cdiff);
      fflush(stdout);
   }

   returnStruct->transform[1][0][0] =   0.;   returnStruct->transform[1][0][1] =  -1.;   returnStruct->transform[1][0][2] = 0.;
   returnStruct->transform[1][1][0] =   1.;   returnStruct->transform[1][1][1] =   0.;   returnStruct->transform[1][1][2] = 0.;
   returnStruct->transform[1][2][0] = x0p-y0; returnStruct->transform[1][2][1] = y0p+x0; returnStruct->transform[1][2][2] = 1.;


   fprintf(view_script, "mBackground -n %s plus_corrected.fits %-g %-g %-g\n\n",
         plus_path, adiff/2., bdiff/2., cdiff/2.);

   fprintf(diff_file, "plus_file                 : %s\n",             minus_path);
   fprintf(diff_file, "minus_file                : %s\n",             plus_path);
   fprintf(diff_file, "A,B,C                     : %.5e %.5e %.5e\n", adiff, bdiff, cdiff);
   fprintf(diff_file, "crpix                     : %.2f %.2f\n",      returnStruct->crpix1[1],   returnStruct->crpix2[1]);
   fprintf(diff_file, "xmin,xmax                 : %d %d\n",          returnStruct->xmin[1],     returnStruct->xmax[1]);
   fprintf(diff_file, "ymin,ymax                 : %d %d\n",          returnStruct->ymin[1],     returnStruct->ymax[1]); 
   fprintf(diff_file, "xcenter,ycenter           : %.2f %.2f\n",      returnStruct->xcenter[1],  returnStruct->ycenter[1]);
   fprintf(diff_file, "npixel,rms                : %d %.5e\n",        returnStruct->npixel[1],   returnStruct->rms[1]);
   fprintf(diff_file, "boxx,boxy                 : %.2f %.2f \n",     returnStruct->boxx[1],     returnStruct->boxy[1]);
   fprintf(diff_file, "boxwidth,boxheight,boxang : %.2f %.2f %.2f\n", returnStruct->boxwidth[1], returnStruct->boxheight[1], returnStruct->boxang[1]);
   fprintf(diff_file, "have_transform            : true\n");
   fprintf(diff_file, "transform                 : %12.5e %12.5e %12.5e\n", returnStruct->transform[1][0][0], returnStruct->transform[1][0][1], returnStruct->transform[1][0][2]);
   fprintf(diff_file, "                            %12.5e %12.5e %12.5e\n", returnStruct->transform[1][1][0], returnStruct->transform[1][1][1], returnStruct->transform[1][1][2]);
   fprintf(diff_file, "                            %12.5e %12.5e %12.5e\n", returnStruct->transform[1][2][0], returnStruct->transform[1][2][1], returnStruct->transform[1][2][2]);
   fprintf(diff_file, "\n");


   fflush(diff_file);

   // -----------------------------------------------------

   fprintf(view_script, "mHistogram -file plus_corrected.fits -2s max gaussian-log -out plus.hist\n\n");

   fprintf(view_script, "mViewer -ct 1 -gray  plus_corrected.fits -histfile plus.hist -out  plus_corrected.png\n");
   fprintf(view_script, "mViewer -ct 1 -gray minus_corrected.fits -histfile plus.hist -out minus_corrected.png\n\n");

   fclose(view_script);
   fclose(diff_file);
   
   // -----------------------------------------------------

   time(&currtime);

   returnStruct->status = 0;
   
   sprintf(returnStruct->msg,  "time=%.1f",        (double)(currtime - start));
   sprintf(returnStruct->json, "{\"time\": %.1f}", (double)(currtime - start));

   return(returnStruct);
}



void mHPXGapDiff_fitImage(char *input_file, int levelOnly, int edge, int width, 
                          struct mHPXGapDiffReturn *returnStruct, int index, int debug)
{
   fitsfile *fptr;

   int       i, j, nfound, nullcnt;
   long      naxes[2];
   double    crpix[2];
   double    crpix1, crpix2;
   long      fpixel[4], nelements;
   double  **data;
   double    pixel_value;
   double    xpos, ypos;
   int       xstart, xend, ystart, yend;
   int       status = 0;

   double    sumxx, sumyy, sumxy, sumx, sumy;

   double    sumxz, sumyz, sumz, sumn;
   int       xmin, xmax, ymin, ymax;
   double    xcenter, ycenter;
   double    rms, dz, fit, sumzz;

   int       iteration, niteration;

   double    boxx, boxy, boxwidth, boxheight, boxang;


   /* Simultaneous equation stuff */

   double **a;
   int      n;
   double **b;
   int      m;


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


   /******************/
   /* Open the image */
   /******************/

   if(fits_open_file(&fptr, input_file, READONLY, &status))
   {
      sprintf(returnStruct->msg, "Image file %s missing or invalid FITS", input_file);

      return;
   }

   if(fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
   {
      mHPXGapDiff_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);

      return;
   }

   if(fits_read_keys_dbl(fptr, "CRPIX", 1, 2, crpix, &nfound, &status))
   {
      mHPXGapDiff_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);

      return;
   }

   if(debug)
   {
      printf("\nWCS from file [%s]:\n\n", input_file);
      printf("   %ld x %ld pixels\n", naxes[0], naxes[1]);
      printf("   crpix: (%-g,%-g)\n\n", crpix[0], crpix[1]);
      fflush(stdout);
   }

   if(!levelOnly && (naxes[0] < 2 || naxes[1] < 2))
   {
      mHPXGapDiff_nrerror("Too few pixels to fit");
      strcpy(returnStruct->msg, montage_msgstr);

      return;
   }

   
   /***********************************/
   /* Determine the pixel area to use */
   /***********************************/

   if(edge == TOP)
   {
      xstart = 0;
      xend   = naxes[0] - 1;
      ystart = naxes[1] - width;
      yend   = naxes[1] - 1;
   }

   else if(edge == LEFT)
   {
      xstart = 0;
      xend   = width - 1;
      ystart = 0;
      yend   = naxes[1] - 1;
   }

   else if(edge == RIGHT)
   {
      xstart = naxes[0] - width;
      xend   = naxes[0] - 1;
      ystart = 0;
      yend   = naxes[1] - 1;
   }

   else if(edge == BOTTOM)
   {
      xstart = 0;
      xend   = naxes[0] - 1;
      ystart = 0;
      yend   = width - 1;
   }

   if(debug)
   {
      printf("\nX pixel range: %d to %d\n", xstart, xend);
      printf("Y pixel range: %d to %d\n\n", ystart, yend);
      fflush(stdout);
   }



   
   /**********************************************/
   /* Allocate memory for the input image pixels */
   /**********************************************/

   data = (double **)malloc(naxes[1] * sizeof(double *));

   for(j=0; j<naxes[1]; ++j)
      data[j] = (double *)malloc(naxes[0] * sizeof(double));

   if(debug)
   {
      printf("%ld bytes allocated for image pixels\n\n",
         naxes[0] * naxes[1] * sizeof(double));
      fflush(stdout);
   }


   /***********************/
   /* Read the image data */
   /***********************/

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   nelements = naxes[0];

   for (j=0; j<naxes[1]; ++j)
   {
      if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, &nan,
                       data[j], &nullcnt, &status))
      {
         mHPXGapDiff_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);

         for(i=0; i<naxes[1]; ++i)
            free(data[i]);

         free(data);

         return;
      }

      ++fpixel[1];
   }

   if(debug)
   {
      printf("Data successfully read from file.\n");
      fflush(stdout);
   }


   /***************************************************************/
   /* Allocate matrix space (for solving least-squares equations) */
   /***************************************************************/

   n = 3;

   a = (double **)malloc(n*sizeof(double *));

   for(i=0; i<n; ++i)
   {
      a[i] = (double *)malloc(n*sizeof(double));

      for(j=0; j<n; ++j)
         a[i][j] = 0.;
   }


   /*************************/
   /* Allocate vector space */
   /*************************/

   m = 1;

   b = (double **)malloc(n*sizeof(double *));

   for(i=0; i<n; ++i)
   {
      b[i] = (double *)malloc(m*sizeof(double));

      for(j=0; j<m; ++j)
         b[i][j] = 0.;
   }


   /******************/
   /* Fit the pixels */
   /******************/

   rms        = 0.;
   niteration = 20;

   for(iteration=0; iteration<niteration; ++iteration)
   {
      if(debug)
      {
         printf("Starting iteration %d\n", iteration);
         fflush(stdout);
      }

      sumxx = 0.;
      sumyy = 0.;
      sumxy = 0.;
      sumx  = 0.;
      sumy  = 0.;
      sumxz = 0.;
      sumyz = 0.;
      sumz  = 0.;
      sumn  = 0.;

      xmin =  1000000;
      xmax = -1000000;
      ymin =  1000000;
      ymax = -1000000;

      if(debug)
      {
         printf("\nUsing x: %d to %d\n", xstart, xend-1);
         printf("Using y: %d to %d\n\n", ystart, yend-1);
         printf("CRPIX: (%-g,%-g)\n\n", crpix[0], crpix[1]);
         fflush(stdout);
      }

      for (j=ystart; j<yend; ++j)
      {
         ypos = j - crpix[1];

         for (i=xstart; i<xend; ++i)
         {
            xpos = i - crpix[0];
            
            pixel_value = data[j][i];

            if(mNaN(pixel_value))
               continue;

            if(rms > 0.)
            {
               fit = b[0][0]*xpos +  b[1][0]*ypos +  b[2][0];

               dz = fabs(pixel_value - fit);

               if(dz > 2*rms)
                  continue;
            }


            if(debug >= 3)
            {
               printf("%12.4e at (%7.2f, %7.2f) [%4d,%4d]\n", 
                  pixel_value, xpos, ypos, i, j);
               fflush(stdout);
            }


            /* Sums needed for least-squares */
            /* plane fitting                 */

            sumxx += xpos*xpos;
            sumyy += ypos*ypos;
            sumxy += xpos*ypos;
            sumx  += xpos;
            sumy  += ypos;
            sumxz += xpos*pixel_value;
            sumyz += ypos*pixel_value;
            sumz  += pixel_value;
            sumn  += 1.;


            /* Region info */

            if(xpos < xmin) xmin = xpos;
            if(xpos > xmax) xmax = xpos;
            if(ypos < ymin) ymin = ypos;
            if(ypos > ymax) ymax = ypos;
         }
      }


      /***************************************/
      /* Box parameters for "diff/fit" table */
      /***************************************/

      xcenter = (xmax+xmin)/2.;
      ycenter = (ymax+ymin)/2.;

      crpix1 = -xcenter + 0.5;
      crpix2 = -ycenter + 0.5;

      boxx = (xmax+xmin)/2.;
      boxy = (ymax+ymin)/2.;

      boxwidth  = abs(ymax-ymin);
      boxheight = abs(xmax-xmin);

      boxang = 90;


      /***********************************/
      /* Least-squares plane calculation */
      /***********************************/

      /*** Fill the matrix and vector  ****

           |a00  a01 a02| |A|   |b00|
           |a10  a11 a12|x|B| = |b01|
           |a20  a21 a22| |C|   |b02|

      *************************************/

      if(levelOnly)
      {
         b[0][0] = 0.;
         b[1][0] = 0.;
         b[2][0] = sumz / sumn;

         if(debug)
         {
            printf("\n");
            printf("Level offset calculated.\n");
            fflush(stdout);
         }
      }
      else
      {
         a[0][0] = sumxx;
         a[1][0] = sumxy;
         a[2][0] = sumx;

         a[0][1] = sumxy;
         a[1][1] = sumyy;
         a[2][1] = sumy;

         a[0][2] = sumx;
         a[1][2] = sumy;
         a[2][2] = sumn;

         b[0][0] = sumxz;
         b[1][0] = sumyz;
         b[2][0] = sumz;

         if(debug)
         {
            printf("\n");
            printf("Simultaneous equations:\n");
            printf("%12.5e %12.5e %12.5e     %12.5e \n", a[0][0], a[0][1], a[0][2], b[0][0]);
            printf("%12.5e %12.5e %12.5e     %12.5e \n", a[1][0], a[1][1], a[1][2], b[1][0]);
            printf("%12.5e %12.5e %12.5e     %12.5e \n", a[2][0], a[2][1], a[2][2], b[2][0]);
            fflush(stdout);
         }


         /* Solve */

         if(mHPXGapDiff_gaussj(a, n, b, m))
         {
            strcpy(returnStruct->msg, montage_msgstr);

            for(i=0; i<naxes[1]; ++i)
               free(data[i]);

            free(data);

            for(i=0; i<n; ++i)
               free(a[i]);

            free(a);

            for(i=0; i<m; ++i)
               free(b[i]);

            free(b);

            return;
         }
      }

      if(debug)
      {
         printf("\n");
         printf("Solution:\n");
         printf("a = %12.5e \n", b[0][0]);
         printf("b = %12.5e \n", b[1][0]);
         printf("c = %12.5e \n", b[2][0]);
         printf("\n");
      }


      /*******************/
      /* Find RMS to fit */
      /*******************/

      sumzz = 0.;
      sumn  = 0.;

      for (j=ystart; j<yend; ++j)
      {
         ypos = j - crpix[1];

         for (i=xstart; i<xend; ++i)
         {
            xpos = i - crpix[0];

            if(mNaN(data[j][i]))
               continue;

            pixel_value = data[j][i];

            fit = b[0][0]*xpos +  b[1][0]*ypos +  b[2][0];

            dz = fabs(pixel_value - fit);

            if(iteration > 0 && dz > 2*rms)
               continue;

            sumzz += dz * dz;

            ++sumn;
         }
      }

      rms = sqrt(sumzz/sumn);

      if(debug)
      {
         printf("iteration %d: rms=%-g (using %.0f values)\n", 
               iteration, rms, sumn);
         fflush(stdout);
      }
   }


   /*******************/
   /* Close things up */
   /*******************/

   fits_close_file(fptr, &status);


   /**************/
   /* Output fit */
   /**************/

   if(boxwidth == 0.)
   {
      boxx      = xmin;
      boxwidth  = 1.;
      boxang    = 0.;
   }

   if(boxheight == 0.)
   {
      boxy      = ymin;
      boxheight = 1.;
      boxang    = 0.;
   }


   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   strcpy(returnStruct->file[index], input_file);

   returnStruct->a[index]         = b[0][0];
   returnStruct->b[index]         = b[1][0];
   returnStruct->c[index]         = b[2][0];
   returnStruct->naxis1[index]    = naxes[0];
   returnStruct->naxis2[index]    = naxes[1];
   returnStruct->crpix1[index]    = crpix1;
   returnStruct->crpix2[index]    = crpix2;
   returnStruct->xmin[index]      = xmin;
   returnStruct->xmax[index]      = xmax;
   returnStruct->ymin[index]      = ymin;
   returnStruct->ymax[index]      = ymax;
   returnStruct->xcenter[index]   = xcenter;
   returnStruct->ycenter[index]   = ycenter;
   returnStruct->npixel[index]    = sumn;
   returnStruct->rms[index]       = rms;
   returnStruct->boxx[index]      = boxx;
   returnStruct->boxy[index]      = boxy;
   returnStruct->boxwidth[index]  = boxwidth;
   returnStruct->boxheight[index] = boxheight;
   returnStruct->boxang[index]    = boxang;


   for(i=0; i<naxes[1]; ++i)
      free(data[i]);

   free(data);

   for(i=0; i<n; ++i)
      free(a[i]);

   free(a);

   for(i=0; i<m; ++i)
      free(b[i]);

   free(b);

   return;
}


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mHPXGapDiff_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}



/***********************************/
/*                                 */
/*  Performs gaussian fitting on   */
/*  backgrounds and returns the    */
/*  parameters A and B in a        */
/*  function of the form           */
/*  y = Ax + B                     */
/*                                 */
/***********************************/

int mHPXGapDiff_gaussj(double **a, int n, double **b, int m)
{
   int   *indxc, *indxr, *ipiv;
   int    i, icol, irow, j, k, l, ll;
   double big, dum, pivinv, temp;

   indxc = mHPXGapDiff_ivector(n);
   indxr = mHPXGapDiff_ivector(n);
   ipiv  = mHPXGapDiff_ivector(n);

   for (j=0; j<n; j++) 
      ipiv[j] = 0;

   for (i=0; i<n; i++) 
   {
      big=0.0;

      for (j=0; j<n; j++)
      {
         if (ipiv[j] != 1)
         {
            for (k=0; k<n; k++) 
            {
               if (ipiv[k] == 0) 
               {
                  if (fabs(a[j][k]) >= big) 
                  {
                     big = fabs(a[j][k]);

                     irow = j;
                     icol = k;
                  }
               }

               else if (ipiv[k] > 1) 
               {
                  mHPXGapDiff_nrerror("Singular Matrix-1");

                  mHPXGapDiff_free_ivector(ipiv);
                  mHPXGapDiff_free_ivector(indxr);
                  mHPXGapDiff_free_ivector(indxc);

                  return 1;
               }
            }
         }
      }

      ++(ipiv[icol]);

      if (irow != icol) 
      {
         for (l=0; l<n; l++) SWAP(a[irow][l], a[icol][l])
         for (l=0; l<m; l++) SWAP(b[irow][l], b[icol][l])
      }

      indxr[i] = irow;
      indxc[i] = icol;

      if (a[icol][icol] == 0.0)
      {
         mHPXGapDiff_nrerror("Singular Matrix-2");

         mHPXGapDiff_free_ivector(ipiv);
         mHPXGapDiff_free_ivector(indxr);
         mHPXGapDiff_free_ivector(indxc);

         return 1;
      }

      pivinv=1.0/a[icol][icol];

      a[icol][icol]=1.0;

      for (l=0; l<n; l++) a[icol][l] *= pivinv;
      for (l=0; l<m; l++) b[icol][l] *= pivinv;

      for (ll=0; ll<n; ll++)
      {
         if (ll != icol) 
         {
            dum=a[ll][icol];

            a[ll][icol]=0.0;

            for (l=0; l<n; l++) a[ll][l] -= a[icol][l]*dum;
            for (l=0; l<m; l++) b[ll][l] -= b[icol][l]*dum;
         }
      }
   }

   for (l=n-1;l>=0; l--) 
   {
      if (indxr[l] != indxc[l])
      {
         for (k=0; k<n; k++)
            SWAP(a[k][indxr[l]], a[k][indxc[l]]);
      }
   }

   mHPXGapDiff_free_ivector(ipiv);
   mHPXGapDiff_free_ivector(indxr);
   mHPXGapDiff_free_ivector(indxc);

   return 0;
}


/* Prints out an error message */

void mHPXGapDiff_nrerror(char *error_text)
{
   strcpy(montage_msgstr, error_text);
}


/* Allocates memory for an array of integers */

int *mHPXGapDiff_ivector(int nh)
{
   int *v;

   v=(int *)malloc((size_t) (nh*sizeof(int)));

   // if (!v) 
   //    mHPXGapDiff_nrerror("allocation failure in ivector()");

   return v;
}


/* Frees memory allocated by ivector */

void mHPXGapDiff_free_ivector(int *v)
{
   free((char *) v);
}
