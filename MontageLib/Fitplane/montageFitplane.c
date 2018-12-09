/* Module: mFitplane.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
2.7      John Good        08Sep15  fits_read_pix() incorrect null value
2.6      John Good        15May08  Implement special bounding boxes for small areas
2.5      John Good        29Mar08  Add 'level only' fitting
2.4      John Good        21Sep04  Changed exit(1) after WARNING to exit(0)
2.3      John Good        27Aug04  Checking for negative value of border
2.2      John Good        03Aug04  Change "bad fit" response to WARNING
2.1      John Good        09May04  Check for naxis1,2 equal to one 
2.0      John Good        20Apr04  Find bounding box for pixels and
                                   write info to output
1.8      John Good        04Mar04  Added pixel count to the output
1.7      John Good        25Nov03  Added extern optarg references
1.6      John Good        10Nov03  Added code to skip blank pixels
1.5      John Good        15Sep03  Updated fits_read_pix() call
1.4      John Good        25Aug03  Added status file processing
1.3      John Good        13May03  Fixed ERROR structure output formatting
                                   for matrix inversion error condition.
1.2      John Good        09May03  Closed the FITS file at the end before
                                   printing out return message.
1.1      John Good        14Mar03  Modified command-line processing
                                   to use getopt() library.  Checks for valid
                                   FITS file.
1.0      John Good        29Jan03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>

#include <pixbounds.h>
#include <fitsio.h>

#include <mFitplane.h>
#include <montage.h>

#define MAXSTR  256

#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;}


static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mFitplane                                                            */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mFitplane, is used in conjuction with mDiff and         */
/*  mBgModel to determine how overlapping images relate to each          */
/*  other.  It is assumed that difference images have matching structure */
/*  information and that what is left when you difference them is just   */
/*  the relative offsets, slopes, etc.  By fitting the difference image, */
/*  we obtain the 'correction' that needs to be applied to one or the    */
/*  other (or in part to both) to bring them together.                   */
/*                                                                       */
/*   char  *input_file     FITS file for plane fitting                   */
/*   int    levelOnly      Only fit for level difference not a full      */
/*                         plane with slopes                             */
/*   int    border         Exclude a pixel border from the fitting       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mFitplaneReturn *mFitplane(char *input_file, int levelOnly, int border, int debug)
{
   fitsfile *fptr;
   int       i, j, nfound;
   int       nullcnt;
   long      naxes[2];
   double    crpix[2];
   long      fpixel[4], nelements;
   double  **data;
   double    pixel_value;
   double    xpos, ypos;
   int       status = 0;

   double    sumxx, sumyy, sumxy, sumx, sumy;

   double    sumxz, sumyz, sumz, sumn;
   double    xmin, xmax, ymin, ymax;
   double    xcenter, ycenter;
   double    rms, dz, fit, sumzz;

   int       iteration, niteration;

   int       mini, maxi;
   int       nbound;
   double   *xbound;
   double   *ybound;

   double    boxx, boxy, boxwidth, boxheight, boxang;
   double    minx, maxx, miny, maxy;


   /* Simultaneous equation stuff */

   double **a;
   int      n;
   double **b;
   int      m;

   struct mFitplaneReturn *returnStruct;


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
   

   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mFitplaneReturn *)malloc(sizeof(struct mFitplaneReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /******************/
   /* Open the image */
   /******************/

   if(fits_open_file(&fptr, input_file, READONLY, &status))
   {
      sprintf(returnStruct->msg, "Image file %s missing or invalid FITS\"]\n", input_file);
      return returnStruct;
   }

   if(fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
   {
      mFitplane_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(fits_read_keys_dbl(fptr, "CRPIX", 1, 2, crpix, &nfound, &status))
   {
      mFitplane_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug >= 1)
   {
      printf("\nFile: [%s]\n\n", input_file);
      printf("   %ld x %ld pixels\n", naxes[0], naxes[1]);
      printf("   crpix: (%-g,%-g)\n\n", crpix[0], crpix[1]);
      fflush(stdout);
   }

   if(!levelOnly && (naxes[0] < 2 || naxes[1] < 2))
   {
      mFitplane_nrerror("Too few pixels to fit");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   
   /**********************************************/
   /* Allocate memory for the input image pixels */
   /**********************************************/

   data = (double **)malloc(naxes[1] * sizeof(double *));

   for(j=0; j<naxes[1]; ++j)
      data[j] = (double *)malloc(naxes[0] * sizeof(double));

   if(debug >= 1)
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

   nbound = 0;

   xbound = (double *)malloc(2 * naxes[1] * sizeof(double));
   ybound = (double *)malloc(2 * naxes[1] * sizeof(double));

   for (j=0; j<naxes[1]; ++j)
   {
      if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, &nan,
                       data[j], &nullcnt, &status))
      {
         mFitplane_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      ++fpixel[1];

      mini = naxes[0];
      maxi = 0;

      for(i=0; i<naxes[0]; ++i)
      {
         if(!mNaN(data[j][i]))
         {
            if(i < mini) mini = i;
            if(i > maxi) maxi = i;
         }
      }

      if(mini < maxi)
      {
         xbound[nbound] = mini - crpix[0];
         ybound[nbound] =    j - crpix[1];
         ++nbound;

         xbound[nbound] = maxi - crpix[0];
         ybound[nbound] =    j - crpix[1];
         ++nbound;
      }
   }

   if(debug >= 1)
   {
      printf("%d pixels in bounding set\n", nbound);
      fflush(stdout);
   }


   /******************************/
   /* Find the bounding box info */
   /******************************/

   boxx = 0.;
   boxy = 0.;

   boxwidth  = 0.;
   boxheight = 0.;

   boxang = 0.;

   if(nbound >= 3)
   {
      cgeomInit(xbound, ybound, nbound);

      if(debug >= 1)
      {
         printf("\nCenter:    (%-g, %-g)\n",
         cgeomGetXcen(), 
         cgeomGetYcen());

         printf("Size:      %-g x %-g\n",
         cgeomGetWidth(), 
         cgeomGetHeight());

         printf("Rotation:  %-g\n\n",
         cgeomGetAngle()); 
      }

      boxx      = cgeomGetXcen();
      boxy      = cgeomGetYcen();
      boxwidth  = cgeomGetWidth();
      boxheight = cgeomGetHeight();
      boxang    = -1. * cgeomGetAngle();
   }
   else if(nbound > 0)
   {
      boxx = xbound[0];
      boxy = ybound[0];

      minx = xbound[0] - 0.5;
      maxx = xbound[0] + 0.5;
      miny = ybound[0] - 0.5;
      maxy = ybound[0] + 0.5;

      for(i=1; i<nbound; ++i)
      {
         boxx += xbound[i];
         boxy += ybound[i];

         if(xbound[i]-0.5 < minx) minx = xbound[i]-0.5;
         if(xbound[i]+0.5 > maxx) maxx = xbound[i]+0.5;
         if(ybound[i]-0.5 < miny) miny = ybound[i]-0.5;
         if(ybound[i]+0.5 > maxy) maxy = ybound[i]+0.5;
      }

      boxx = boxx/nbound;
      boxy = boxy/nbound;

      boxwidth  = maxx - minx;
      boxheight = maxy - miny;

      boxang = 0.;
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

      for (j=border; j<naxes[1]-border; ++j)
      {
         ypos = j - crpix[1];

         for (i=border; i<naxes[0]-border; ++i)
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

      xcenter = sumx / sumn;
      ycenter = sumy / sumn;


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

         if(debug >= 2)
         {
            printf("\n");
            printf("%12.5e %12.5e %12.5e     %12.5e \n", a[0][0], a[0][1], a[0][2], b[0][0]);
            printf("%12.5e %12.5e %12.5e     %12.5e \n", a[1][0], a[1][1], a[1][2], b[1][0]);
            printf("%12.5e %12.5e %12.5e     %12.5e \n", a[2][0], a[2][1], a[2][2], b[2][0]);
            printf("\n");
         }


         /* Solve */

         if(mFitplane_gaussj(a, n, b, m))
         {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
         }
      }

      if(debug >= 2)
      {
         printf("\n");
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

      for (j=border; j<naxes[1]-border; ++j)
      {
         ypos = j - crpix[1];

         for (i=border; i<naxes[0]-border; ++i)
         {
            xpos = i - crpix[0];

            if(mNaN(data[j][i]))
               continue;

            pixel_value = data[j][i];

            fit = b[0][0]*xpos +  b[1][0]*ypos +  b[2][0];

            dz = fabs(pixel_value - fit);

            if(dz > 2*rms)
               continue;

            sumzz += dz * dz;

            ++sumn;
         }
      }

      rms = sqrt(sumzz/sumn);

      if(debug >= 1)
         printf("iteration %d: rms=%-g\n", iteration, rms);
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

   sprintf(montage_msgstr, "a=%-g, b=%-g, c=%-g, crpix1=%-g, crpix2=%-g, xmin=%-g, xmax=%-g, ymin=%-g, ymax=%-g, xcenter=%-g, ycenter=%-g, npixel=%-g, rms=%-g, boxx=%-g, boxy=%-g, boxwidth=%-g, boxheight=%-g, boxang=%-g", 
      b[0][0], b[1][0], b[2][0], crpix[0], crpix[1], xmin, xmax, 
      ymin, ymax, xcenter, ycenter, sumn, rms,
      boxx, boxy, boxwidth, boxheight, boxang);

   sprintf(montage_json, "{\"a\":%-g, \"b\":%-g, \"c\":%-g, \"crpix1\":%-g, \"crpix2\":%-g, \"xmin\":%-g, \"xmax\":%-g, \"ymin\":%-g, \"ymax\":%-g, \"xcenter\":%-g, \"ycenter\":%-g, \"npixel\":%-g, \"rms\":%-g, \"boxx\":%-g, \"boxy\":%-g, \"boxwidth\":%-g, \"boxheight\":%-g, \"boxang\":%-g}", 
      b[0][0], b[1][0], b[2][0], crpix[0], crpix[1], xmin, xmax, 
      ymin, ymax, xcenter, ycenter, sumn, rms,
      boxx, boxy, boxwidth, boxheight, boxang);

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   returnStruct->a         = b[0][0];
   returnStruct->b         = b[1][0];
   returnStruct->c         = b[2][0];
   returnStruct->crpix1    = crpix[0];
   returnStruct->crpix2    = crpix[1];
   returnStruct->xmin      = xmin;
   returnStruct->xmax      = xmax;
   returnStruct->ymin      = ymin;
   returnStruct->ymax      = ymax;
   returnStruct->xcenter   = xcenter;
   returnStruct->ycenter   = ycenter;
   returnStruct->npixel    = sumn;
   returnStruct->rms       = rms;
   returnStruct->boxx      = boxx;
   returnStruct->boxy      = boxy;
   returnStruct->boxwidth  = boxwidth;
   returnStruct->boxheight = boxheight;
   returnStruct->boxang    = boxang;

   return returnStruct;
}


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mFitplane_printFitsError(int status)
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

int mFitplane_gaussj(double **a, int n, double **b, int m)
{
   int   *indxc, *indxr, *ipiv;
   int    i, icol, irow, j, k, l, ll;
   double big, dum, pivinv, temp;

   indxc = mFitplane_ivector(n);
   indxr = mFitplane_ivector(n);
   ipiv  = mFitplane_ivector(n);

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
                  mFitplane_nrerror("Singular Matrix-1");
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
         mFitplane_nrerror("Singular Matrix-2");
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

   mFitplane_free_ivector(ipiv);
   mFitplane_free_ivector(indxr);
   mFitplane_free_ivector(indxc);

   return 0;
}


/* Prints out an error message */

void mFitplane_nrerror(char *error_text)
{
   strcpy(montage_msgstr, error_text);
}


/* Allocates memory for an array of integers */

int *mFitplane_ivector(int nh)
{
   int *v;

   v=(int *)malloc((size_t) (nh*sizeof(int)));

   // if (!v) 
   //    mFitplane_nrerror("allocation failure in ivector()");

   return v;
}


/* Frees memory allocated by ivector */

void mFitplane_free_ivector(int *v)
{
   free((char *) v);
}
