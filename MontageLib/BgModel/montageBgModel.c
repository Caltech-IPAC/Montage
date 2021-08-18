/* Module: mBgModel.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
3.1      John Good        29Aug15  Make output id column wider; some people have a lot 
                                   of images
3.0      John Good        17Nov14  Cleanup to avoid compiler warnings, in proparation
                                   for new development cycle.
2.7      John Good        09Dec12  Fix realloc bug (using wrong index variable)
2.6      John Good        28Apr08  Add flag to turn off rejection 
                                   (e.g. for all-sky CAR)
2.5      John Good        31Mar08  Make the rejection limits parameters
2.4      John Good        05Sep06  Increase default iterations to 10000
2.3      John Good        15Jun04  Don't stop if we hit a bad matrix 
                                   inversion, just don't generate any
                                   corrections for that image.
2.2      John Good        27Aug04  Added "[-s statusfile]" to Usage statement
2.1      John Good        28Jul04  Shouldn't have had a lower RMS cutoff;
                                   this removes overlaps that are actually
                                   just part of a larger tiled image.
2.0      John Good        20Apr04  Changed pixel "sums" to use integral form
                                   and allow for rotated overlap regions
1.9      John Good        09Mar04  Added "level-only" flag
1.8      John Good        07Mar04  Added checks for whether to use background 
                                   fit. Now must be at least 2% of average image
                                   area, have RMS no more than 2.0 times the
                                   average, and have linear dimensions of at
                                   least 25% that of one of the corresponding
                                   images.
1.7      John Good        25Nov03  Added extern optarg references
1.6      John Good        06Oct03  Added NAXIS1,2 as alternatives to ns,nl
1.5      John Good        25Aug03  Added status file processing
1.4      John Good        28May03  Changed fittype handling to allow arbitrarily
                                   large number of iterations
1.3      John Good        24Mar03  Fixed max count in error in message 
                                   statement for -i 
1.2      John Good        22Mar03  Fixed error in message statement for -i 
                                   option, and corrected error where -i value 
                                   was being overwritten, and turned on debug
                                   if refimage given
1.1      John Good        14Mar03  Modified command-line processing
                                   to use getopt() library.  Checks validity of
                                   niteration.  Checks for missing/invalid
                                   images.tbl or fits.tbl.
1.0      John Good        29Jan03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#include <mtbl.h>

#include <mBgModel.h>
#include <montage.h>

#define MAXSTR  256
#define MAXCNT  128

#define ALL        0
#define LEVEL_ONLY 1
#define ALTERNATE  2

#define LEVEL 0
#define SLOPE 1
#define BOTH  2

#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;}

int mBgModel_corrCompare(const void *a, const void *b);


/* This structure contains the basic geometry alignment          */
/* information for images.  Since all images are aligned to the  */
/* same location on the sky (and the same projection, etc.,      */
/* aligning them in cartesian pixel space only requires knowing  */
/* their size and reference pixel coordinates                    */

static struct ImgInfo
{
   int               cntr;
   char              fname[1024];
   int               naxis1;
   int               naxis2;
   double            crpix1;
   double            crpix2;
}
*imgs;

static int nimages, maximages;


/* This structure contains the information describing the */
/* plane to be subtracted from each image to "correct" it */
/* to its neighbors.                                      */

static struct FitInfo
{
   int    plus;
   int    minus;
   int    plusind;
   int    minusind;
   double a;
   double b;
   double c;
   double crpix1;
   double crpix2;
   int    xmin;
   int    xmax;
   int    ymin;
   int    ymax;
   double xcenter;
   double ycenter;
   int    npix;
   double rms;
   double Xmin;
   double Xmax;
   double Ymin;
   double Ymax;
   double boxangle;
   int    use;

   struct CorrInfo *plusimg;
   struct CorrInfo *minusimg;
}
*fits;

static int nfits, maxfits;


/* This structure contains the incremental           */
/* correction values to be applied to each image.    */
/* It is used to update the FitInfo structure above. */


struct CorrInfo
{
   int    id;

   double a;
   double b;
   double c;

   double acorrection;
   double bcorrection;
   double ccorrection;

   struct FitInfo **neighbors;

   int nneighbors;
   int maxneighbors;
}
*corrs;

static int ncorrs, maxcorrs;


static char montage_msgstr[1024];
static char montage_json  [1024];


/*-***********************************************************************/
/*                                                                       */
/*  mBModel                                                              */
/*                                                                       */
/*  Given a set of image overlap difference fits (parameters on the      */
/*  planes fit to pairwise difference images between adjacent images)    */
/*  interatively determine the 'best' background adjustment for each     */
/*  image (assuming each image is changed to best match its neighbors    */
/*  with them remaining unchanged) uses these adjustments to modify      */
/*  each set of difference parameters, and iterate until the changes     */
/*  modifications become small enough.                                   */
/*                                                                       */
/*   char  *imgfile        Reprojected image metadata list               */
/*   char  *fitfile        Set of image overlap difference fits          */
/*   char  *corrtbl        Output table of corrections for images        */
/*                         in input list                                 */
/*   int    mode           Three possible background matching modes:     */
/*                         level fitting only; level fitting followed    */
/*                         by fitting slope and level; and alternating   */
/*                         between level and slope fitting.              */
/*   int    useall         Use all the input differences (by default     */
/*                         we exclude very small overlap areas)          */
/*   int    niteration     Number of iterations to run                   */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mBgModelReturn *mBgModel(char *imgfile, char *fitfile, char *corrtbl, int mode, int useall, int niter, int debug)
{
   int     i, j, k, index, stat, single;
   int     ntoggle, toggle, nancnt;
   int     ncols, iteration, istatus;
   int     maxlevel, refimage, niteration;
   double  averms, sigrms, avearea;
   double  imin, imax, jmin, jmax;
   double  A, B, C;
   FILE   *fout;

   int     iplus;
   int     iminus;
   int     ia;
   int     ib;
   int     ic;
   int     icrpix1;
   int     icrpix2;
   int     ixmin;
   int     ixmax;
   int     iymin;
   int     iymax;
   int     ixcenter;
   int     iycenter;
   int     inpix;
   int     irms;
   int     iboxx;
   int     iboxy;
   int     iboxwidth;
   int     iboxheight;
   int     iboxangle;
   int     isingle;
   int     icntr;
   int     ifname;
   int     inl;
   int     ins;

   int     fittype;

   int     nrms;
   char    rms_str[128];
   char    single_str[128];

   double  boxx, boxy;
   double  width, height;
   double  angle;
   double  X0, Y0;

   double  sumxx, sumyy, sumxy, sumx, sumy, sumn;
   double  dsumxx, dsumyy, dsumxy, dsumx, dsumy, dsumn;
   double  sumxz, sumyz, sumz;

   int     badslope;
   double  suma, sumb, sumc;
   double  suma2, sumb2, sumc2;
   double  avea, aveb, avec;
   double  sigmaa, sigmab, sigmac;
   double  maxa, maxb, maxc;
   int     iamax, ibmax, icmax;

   double  acorrection;
   double  bcorrection;
   double  ccorrection;

   double  dtr;
   double  Xmin, Xmax, Ymin, Ymax;
   double  theta, sinTheta, cosTheta;

   double  sigmaLimit  = 2.00;
   double  areaLimit   = 0.01;


   struct mBgModelReturn *returnStruct;


   /* Simultaneous equation stuff */

   float **a;
   int     n;
   float **b;
   int     m;

   dtr = atan(1) / 45.;


   // Debug reference image (change here manually)

   refimage = 272;
   refimage =  -1;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mBgModelReturn *)malloc(sizeof(struct mBgModelReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /***************************************************************/
   /* Allocate matrix space (for solving least-squares equations) */
   /***************************************************************/

   n = 3;

   a = (float **)malloc(n*sizeof(float *));

   for(i=0; i<n; ++i)
      a[i] = (float *)malloc(n*sizeof(float));


   /*************************/
   /* Allocate vector space */
   /*************************/

   m = 1;

   b = (float **)malloc(n*sizeof(float *));

   for(i=0; i<n; ++i)
      b[i] = (float *)malloc(m*sizeof(float));



   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   niteration = niter;

   if(niteration == 0)
      niteration = 10000;

   maxlevel = 2500;
   if(niteration < 5000)
      maxlevel = niteration / 2.;

   if(debug)
   {
      printf("niteration = %d\n", niteration);
      printf("mode       = %d\n", mode);
      printf("imgfile    = %s\n", imgfile);
      printf("fitfile    = %s\n", fitfile);
      printf("corrtbl    = %s\n", corrtbl);
      fflush(stdout);
   }

   fout = fopen(corrtbl, "w+");

   if(fout == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Failed to open output %s", corrtbl);
      return returnStruct;
   }


   /*********************************************/ 
   /* Open the image header metadata table file */
   /*********************************************/ 

   ncols = topen(imgfile);

   if(ncols <= 0)
   {
      sprintf(returnStruct->msg, "Invalid image metadata file: %s", imgfile);
      return returnStruct;
   }

   icntr    = tcol("cntr");
   ifname   = tcol("fname");
   inl      = tcol("nl");
   ins      = tcol("ns");
   icrpix1  = tcol("crpix1");
   icrpix2  = tcol("crpix2");

   if(ins < 0)
      ins = tcol("naxis1");

   if(inl < 0)
      inl = tcol("naxis2");

   if(icntr   < 0
   || ifname  < 0
   || inl     < 0
   || ins     < 0
   || icrpix1 < 0
   || icrpix2 < 0)
   {
      sprintf(returnStruct->msg, "Need columns: cntr fname nl ns crpix1 crpix2 in image info file");
      return returnStruct;
   }



   /******************************/ 
   /* Read the image information */ 
   /******************************/ 

   nimages   =      0;
   maximages = MAXCNT;

   if(debug >= 2)
   {
      printf("Allocating imgs to %d (size %lu) [11]\n", maximages, maximages * sizeof(struct ImgInfo));
      fflush(stdout);
   }

   imgs = (struct ImgInfo *)malloc(maximages * sizeof(struct ImgInfo));

   if(imgs == (struct ImgInfo *)NULL)
   {
      sprintf(returnStruct->msg, "malloc() failed (ImgInfo)");
      return returnStruct;
   }


   avearea = 0.0;

   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      imgs[nimages].cntr      = atoi(tval(icntr));
      imgs[nimages].naxis1    = atoi(tval(ins));
      imgs[nimages].naxis2    = atoi(tval(inl));
      imgs[nimages].crpix1    = atof(tval(icrpix1));
      imgs[nimages].crpix2    = atof(tval(icrpix2));

      strcpy(imgs[nimages].fname, tval(ifname));

      avearea += imgs[nimages].naxis1*imgs[nimages].naxis2;

      ++nimages;

      if(nimages >= maximages)
      {
         maximages += MAXCNT;

         if(debug >= 2)
         {
            printf("Reallocating imgs to %d (size %lu) [14]\n", maximages, maximages * sizeof(struct ImgInfo));
            fflush(stdout);
         }

         imgs = (struct ImgInfo *)realloc(imgs, 
                                      maximages * sizeof(struct ImgInfo));

         if(imgs == (struct ImgInfo *)NULL)
         {
            sprintf(returnStruct->msg, "realloc() failed (ImgInfo) [1]");
            return returnStruct;
         }
      }
   }

   avearea = avearea / nimages;



   /**************************************/ 
   /* Open the difference fit table file */
   /**************************************/ 

   ncols = topen(fitfile);

   if(ncols <= 0)
   {
      sprintf(returnStruct->msg, "Invalid background fit parameters file: %s", fitfile);
      return returnStruct;
   }

   iplus      = tcol("plus");
   iminus     = tcol("minus");
   ia         = tcol("a");
   ib         = tcol("b");
   ic         = tcol("c");
   icrpix1    = tcol("crpix1");
   icrpix2    = tcol("crpix2");
   ixmin      = tcol("xmin");
   ixmax      = tcol("xmax");
   iymin      = tcol("ymin");
   iymax      = tcol("ymax");
   ixcenter   = tcol("xcenter");
   iycenter   = tcol("ycenter");
   inpix      = tcol("npixel");
   irms       = tcol("rms");
   iboxx      = tcol("boxx");
   iboxy      = tcol("boxy");
   iboxwidth  = tcol("boxwidth");
   iboxheight = tcol("boxheight");
   iboxangle  = tcol("boxang");
   isingle    = tcol("single");

   if(iplus      < 0
   || iminus     < 0
   || ia         < 0
   || ib         < 0
   || ic         < 0
   || icrpix1    < 0
   || icrpix2    < 0
   || ixmin      < 0
   || ixmax      < 0
   || iymin      < 0
   || iymax      < 0
   || ixcenter   < 0
   || iycenter   < 0
   || inpix      < 0
   || irms       < 0
   || iboxx      < 0
   || iboxy      < 0
   || iboxwidth  < 0
   || iboxheight < 0
   || iboxangle  < 0)
   {
      sprintf(returnStruct->msg, "Need columns: plus minus a b c crpix1 crpix2 xmin xmax ymin ymax xcenter ycenter npixel rms boxx boxy boxwidth boxheight boxang");
      return returnStruct;
   }



   /*****************/ 
   /* Read the fits */ 
   /*****************/ 

   nfits   =      0;
   nrms    =      0;
   maxfits = MAXCNT;

   if(debug >= 2)
   {
      printf("Allocating fits to %d (size %lu) [12]\n", maxfits, maxfits * sizeof(struct FitInfo));
      fflush(stdout);
   }

   fits = (struct FitInfo *)malloc(maxfits * sizeof(struct FitInfo));

   if(fits == (struct FitInfo *)NULL)
   {
      sprintf(returnStruct->msg, "malloc() failed (FitInfo)");
      return returnStruct;
   }

   averms = 0.0;

   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      fits[nfits].plus      = atoi(tval(iplus));
      fits[nfits].minus     = atoi(tval(iminus));
      fits[nfits].a         = atof(tval(ia));
      fits[nfits].b         = atof(tval(ib));
      fits[nfits].c         = atof(tval(ic));
      fits[nfits].crpix1    = atof(tval(icrpix1));
      fits[nfits].crpix2    = atof(tval(icrpix2));
      fits[nfits].xmin      = atoi(tval(ixmin));
      fits[nfits].xmax      = atoi(tval(ixmax));
      fits[nfits].ymin      = atoi(tval(iymin));
      fits[nfits].ymax      = atoi(tval(iymax));
      fits[nfits].xcenter   = atof(tval(ixcenter));
      fits[nfits].ycenter   = atof(tval(iycenter));
      fits[nfits].npix      = atof(tval(inpix));
      fits[nfits].rms       = atof(tval(irms));

      single = 0;

      if(isingle >= 0)
      {
         strcpy(single_str, tval(isingle));

         if(strcasecmp(single_str, "true") == 0
         || strcasecmp(single_str, "1")    == 0)
            single = 1;
      }

      strcpy(rms_str, tval(irms));
      if(strcmp(rms_str, "Inf" ) == 0
      || strcmp(rms_str, "-Inf") == 0
      || strcmp(rms_str, "NaN" ) == 0)
         fits[nfits].rms = 1.e99;

      boxx   = atof(tval(iboxx));
      boxy   = atof(tval(iboxy));
      width  = atof(tval(iboxwidth ));
      height = atof(tval(iboxheight));
      angle  = atof(tval(iboxangle)) * dtr;

      X0 =  boxx * cos(angle) + boxy * sin(angle);
      Y0 = -boxx * sin(angle) + boxy * cos(angle);

      fits[nfits].Xmin = X0 - width /2.;
      fits[nfits].Xmax = X0 + width /2.;
      fits[nfits].Ymin = Y0 - height/2.;
      fits[nfits].Ymax = Y0 + height/2.;

      fits[nfits].boxangle  = angle/dtr;

      if(fits[nfits].rms < 1.e99)
      {
         averms += fits[nfits].rms;
         ++nrms;
      }

      fits[nfits].use =  1;

      ++nfits;

      if(nfits >= maxfits-2)
      {
         maxfits += MAXCNT;

         if(debug >= 2)
         {
            printf("Reallocating fits to %d (size %lu) [15]\n", maxfits, maxfits * sizeof(struct FitInfo));
            fflush(stdout);
         }

         fits = (struct FitInfo *)realloc(fits, 
                                     maxfits * sizeof(struct FitInfo));

         if(fits == (struct FitInfo *)NULL)
         {
            sprintf(returnStruct->msg, "realloc() failed (FitInfo) [%lu] [2]", maxfits * sizeof(struct FitInfo));
            return returnStruct;

         }
      }


      /* Use the same info for the complementary */
      /* comparison, with the fit reversed       */

      if(!single)
      {
         fits[nfits].plus     =  atoi(tval(iminus));
         fits[nfits].minus    =  atoi(tval(iplus));
         fits[nfits].a        = -atof(tval(ia));
         fits[nfits].b        = -atof(tval(ib));
         fits[nfits].c        = -atof(tval(ic));
         fits[nfits].xmin     =  atoi(tval(ixmin));
         fits[nfits].xmax     =  atoi(tval(ixmax));
         fits[nfits].ymin     =  atoi(tval(iymin));
         fits[nfits].ymax     =  atoi(tval(iymax));
         fits[nfits].xcenter  =  atof(tval(ixcenter));
         fits[nfits].ycenter  =  atof(tval(iycenter));
         fits[nfits].npix     =  atof(tval(inpix));
         fits[nfits].rms      =  atof(tval(irms));
         fits[nfits].Xmin     =  fits[nfits-1].Xmin;
         fits[nfits].Xmax     =  fits[nfits-1].Xmax;
         fits[nfits].Ymin     =  fits[nfits-1].Ymin;
         fits[nfits].Ymax     =  fits[nfits-1].Ymax;
         fits[nfits].boxangle =  fits[nfits-1].boxangle;

         fits[nfits].use = 1;

         ++nfits;

         if(nfits >= maxfits-2)
         {
            maxfits += MAXCNT;

            if(debug >= 2)
            {
               printf("Reallocating fits to %d (size %lu) [16]\n", maxfits, maxfits * sizeof(struct FitInfo));
               fflush(stdout);
            }

            fits = (struct FitInfo *)realloc(fits, 
                                        maxfits * sizeof(struct FitInfo));

            if(fits == (struct FitInfo *)NULL)
            {
               sprintf(returnStruct->msg, "realloc() failed (FitInfo) [%lu] [3]", maxfits * sizeof(struct FitInfo));
               return returnStruct;
            }
         }
      }
   }

   averms = averms / nrms;


   /*************************************************************************/
   /* Find the image indices for the images mentioned in the fits structure */
   /*************************************************************************/

   for(j=0; j<nfits; ++j)
   {
      for(i=0; i<nimages; ++i)
      {
         if(fits[j].plus == imgs[i].cntr)
            fits[j].plusind = i;

         if(fits[j].minus == imgs[i].cntr)
            fits[j].minusind = i;
      }
   }


   /********************************************/
   /* From the fit information, initialize the */
   /* image structures                         */
   /********************************************/

   ncorrs   =      0;
   maxcorrs = MAXCNT;

   if(debug >= 2)
   {
      printf("Allocating corrs to %d (size %lu) [13]\n", maxcorrs, maxcorrs * sizeof(struct CorrInfo));
      fflush(stdout);
   }

   corrs = (struct CorrInfo *)malloc(maxcorrs * sizeof(struct CorrInfo));

   if(corrs == (struct CorrInfo *)NULL)
   {
      sprintf(returnStruct->msg, "malloc() failed (CorrInfo)");
      return returnStruct;
   }

   for(i=0; i<maxcorrs; ++i)
   {
      corrs[i].id = -1;

      corrs[i].a = 0.;
      corrs[i].b = 0.;
      corrs[i].c = 0.;

      corrs[i].nneighbors = 0;
      corrs[i].maxneighbors = MAXCNT;

      if(debug >= 2)
      {
         printf("Allocating corrs[%d].neighbors to %d (size %lu) [20]\n", i, corrs[i].maxneighbors, 
           corrs[i].maxneighbors * sizeof(struct FitInfo *));
         fflush(stdout);
      }

      corrs[i].neighbors 
         = (struct FitInfo **)malloc(corrs[i].maxneighbors 
                                     * sizeof(struct FitInfo *));

      if(corrs[i].neighbors == (struct FitInfo **)NULL)
      {
         sprintf(returnStruct->msg, "malloc() failed (FitInfo *)");
         return returnStruct;
      }
   }

   for(k=0; k<nfits; ++k)
   {
      /* See if we already have a structure for this image */

      index = -1;

      for(j=0; j<ncorrs; ++j)
      {
         if(corrs[j].id < 0)
            break;
         
         if(corrs[j].id == fits[k].plus)
         {
            index = j;
            break;
         }
      }


      /* If not, get the next free one */

      if(index < 0)
      {
         if(ncorrs >= maxcorrs)
         {
            maxcorrs += MAXCNT;

            if(debug >= 2)
            {
               printf("Reallocating corrs to %d (size %lu) [17]\n", maxcorrs, maxcorrs * sizeof(struct CorrInfo));
               fflush(stdout);
            }

            corrs = (struct CorrInfo *)realloc(corrs, maxcorrs * sizeof(struct CorrInfo));

            if(corrs == (struct CorrInfo *)NULL)
            {
               sprintf(returnStruct->msg, "realloc() failed (CorrInfo) [4]");
               return returnStruct;
            }

            for(i=maxcorrs-MAXCNT; i<maxcorrs; ++i)
            {
               corrs[i].id = -1;

               corrs[i].a = 0.;
               corrs[i].b = 0.;
               corrs[i].c = 0.;

               corrs[i].nneighbors = 0;
               corrs[i].maxneighbors = MAXCNT;

               if(debug >= 2)
               {
                  printf("Allocating corrs[%d].maxneighbors to %d (size %lu) [18]\n", i, corrs[i].maxneighbors, 
                     corrs[i].maxneighbors * sizeof(struct FitInfo *));
                  fflush(stdout);
               }

               corrs[i].neighbors 
                  = (struct FitInfo **)malloc(corrs[i].maxneighbors 
                                              * sizeof(struct FitInfo *));

               if(corrs[i].neighbors == (struct FitInfo **)NULL)
               {
                  sprintf(returnStruct->msg, "malloc() failed (FitInfo *)");
                  return returnStruct;
               }
            }
         }

         index = ncorrs;

         corrs[index].id = fits[k].plus;

         if(debug >= 3)
         {
            printf("corrs[%d].id = %d\n", index, corrs[index].id);
            fflush(stdout);
         }

         ++ncorrs;
      }


      /* Add this reference */

      corrs[index].a = 0.;
      corrs[index].b = 0.;
      corrs[index].c = 0.;

      corrs[index].neighbors[corrs[index].nneighbors] = &fits[k];

      ++corrs[index].nneighbors;

      if(corrs[index].nneighbors >= corrs[index].maxneighbors)
      {
         corrs[index].maxneighbors += MAXCNT;

         if(debug >= 2)
         {
            printf("Reallocating corrs[%d].neighbors to %d (size %lu) [19]\n", index, corrs[index].maxneighbors, corrs[index].maxneighbors * sizeof(struct FitInfo *));
            fflush(stdout);
         }

         corrs[index].neighbors 
            = (struct FitInfo **)realloc(corrs[index].neighbors,
                                         corrs[index].maxneighbors 
                                         * sizeof(struct FitInfo *));

         if(corrs[index].neighbors == (struct FitInfo **)NULL)
         {
            sprintf(returnStruct->msg, "realloc() failed (FitInfo *) [5]");
            return returnStruct;
         }
      }
   }


   /*******************************************************/
   /* Find the back reference from the fits to the images */
   /*******************************************************/

   for(j=0; j<nfits; ++j)
   {
      for(i=0; i<ncorrs; ++i)
      {
         if(fits[j].plus == corrs[i].id)
         {
            fits[j].plusimg = &corrs[i];
            break;
         }
      }
      
      for(i=0; i<ncorrs; ++i)
      {
         if(fits[j].minus == corrs[i].id)
         {
            fits[j].minusimg = &corrs[i];
            break;
         }
      }
      
      if(debug >= 3)
      {
         if(j == 0)
            printf("\n");

         printf("fits[%3d]: (plusimg=%3d  minusimg=%3d) ", 
            j, fits[j].plusimg->id, fits[j].minusimg->id);

         printf(" %12.5e ",  fits[j].a);
         printf(" %12.5e ",  fits[j].b);
         printf(" %12.5e ",  fits[j].c);
         printf(" (%12.5e)\n", fits[j].rms);

         fflush(stdout);
      }
   }


   /***********************************************/
   /* Turn off the fits which represent those     */
   /* overlaps which are smaller than 2% of the   */
   /* average image area and those whose linear   */
   /* extent in at least one direction isn't      */
   /* at least half the size of the corresponding */
   /* images                                      */
   /***********************************************/

   for(k=0; k<nfits; ++k)
      fits[k].use = 1;

   if(!useall)
   {
      for(k=0; k<nfits; ++k)
      {

         if(fits[k].rms >= 1.e99)
         {
            fits[k].use = 0;

            continue;
         }

         if(fits[k].npix < areaLimit * avearea)
         {
            if(debug >= 2)
               printf("not using fit %d [%d|%d] (area too small: %d/%-g\n",
                  k, fits[k].plus, fits[k].minus, fits[k].npix, avearea);

            fits[k].use = 0;

            continue;
         }
      }

      if(debug >= 1)
      {
         printf("Removed any 'small' fits.");
         fflush(stdout);
      }
   }


   /***********************************************/
   /* We don't want to use noisy fits, so turn    */
   /* off those with an rms more than two sigma   */
   /* above the average                           */
   /***********************************************/

   if(!useall)
   {
      sumn   = 0.;
      sumx   = 0.;
      sumxx  = 0.;

      for(k=0; k<nfits; ++k)
      {
         if(fits[k].use)
         {
            sumn  += 1.;
            sumx  += fits[k].rms;
            sumxx += fits[k].rms * fits[k].rms;
         }
      }

      averms = sumx / sumn;
      sigrms = sqrt(sumxx/sumn - averms*averms);

      for(k=0; k<nfits; ++k)
      {
         if(fits[k].use)
         {
            if(fits[k].rms > averms + sigmaLimit * sigrms)
            {
               if(debug >= 2)
                  printf("not using fit %d [%d|%d] rms too large: %-g/%-g+%-g)\n",
                     k, fits[k].plus, fits[k].minus, fits[k].rms, averms, sigrms);

               fits[k].use = 0;

               continue;
            }
         }
      }

      if(debug >= 1)
      {
         printf("Removed fits with RMS greater than %-g + %-g x %-g (average RMS + sigmaLimit*sigrms)\n",
               averms, sigmaLimit, sigrms);
         fflush(stdout);
      }
   }


   /***************************************/
   /* Dump out the correction information */
   /***************************************/

   if(debug >= 3 || corrs[i].id == refimage)
   {
      for(i=0; i<ncorrs; ++i)
      {
         printf("\n-----\n\nCorrection %d (Image %d)\n\n", i, corrs[i].id);

         for(j=0; j<corrs[i].nneighbors; ++j)
         {
            printf("\n  neighbor %3d:\n", j+1);

            printf("            id: %d\n", corrs[i].neighbors[j]->minus);

            printf("       (A,B,C): (%-g,%-g,%-g)\n", 
               corrs[i].neighbors[j]->a, 
               corrs[i].neighbors[j]->b, 
               corrs[i].neighbors[j]->c);   

            printf("             x: %5d to %5d\n", 
               corrs[i].neighbors[j]->xmin, corrs[i].neighbors[j]->xmax);

            printf("             y: %5d to %5d\n",
               corrs[i].neighbors[j]->ymin, corrs[i].neighbors[j]->ymax);

            printf("        center: (%-g,%-g)\n", 
               corrs[i].neighbors[j]->xcenter, corrs[i].neighbors[j]->ycenter);
         }
      }
   }


   fittype = LEVEL;

   iteration = 0;

   while(1)
   {
      /*************************************************/
      /* Original logic: Either level fitting or level */
      /* fitting followed by fitting slope and level.  */
      /*************************************************/

      if(mode == LEVEL_ONLY)
         fittype = LEVEL;

      if(mode == ALL)
      {
         if(iteration < maxlevel)
            fittype = LEVEL;
         else
            fittype = BOTH;
      }


      /************************************************/
      /* An alternate approach:  toggle between level */
      /* fitting and slope fitting.                   */
      /************************************************/

      if(mode == ALTERNATE)
      {
         ntoggle = 1000;
         if(niteration < 10000)
            ntoggle = niteration / 10.;

         if(ntoggle < 1)
            ntoggle = 1;

         toggle = (iteration / ntoggle) % 2;

         if(toggle)
            fittype = SLOPE;
         else
            fittype = LEVEL;
      }

      if(debug >= 2 || refimage >= 0)
      {
         printf("\n\n============================================================================================================\n\n");
         printf("Iteration %d", iteration+1);
              if(fittype == LEVEL) printf(" (LEVEL):\n");
         else if(fittype == SLOPE) printf(" (SLOPE):\n");
         else if(fittype == BOTH)  printf(" (BOTH ):\n");
         else                      printf(" (ERROR):\n");
         fflush(stdout);
      }

      /*********************************************/
      /* For each image, calculate the "best fit"  */
      /* correction plane, based of the difference */
      /* data between an image and its neighbors   */
      /*********************************************/

      for(i=0; i<ncorrs; ++i)
      {
         sumn  = 0.;
         sumx  = 0.;
         sumy  = 0.;
         sumxx = 0.;
         sumxy = 0.;
         sumyy = 0.;
         sumxz = 0.;
         sumyz = 0.;
         sumz  = 0.;

         corrs[i].acorrection = 0.;
         corrs[i].bcorrection = 0.;
         corrs[i].ccorrection = 0.;

         for(j=0; j<corrs[i].nneighbors; ++j)
         {
            /* We have earlier "turned off" some of these because */
            /* the fit was bad (too few points or too noisy).     */
            /* If so, don't include them in the sums.             */

            if(corrs[i].neighbors[j]->use == 0)
               continue;


            /* What we do here is essentially a "least squares",   */
            /* though rather than go back to the difference files  */
            /* we instead use the parameterized value of the plane */
            /* fit to that data.                                   */

            imin = corrs[i].neighbors[j]->xmin;
            imax = corrs[i].neighbors[j]->xmax;
            jmin = corrs[i].neighbors[j]->ymin;
            jmax = corrs[i].neighbors[j]->ymax;

            theta = corrs[i].neighbors[j]->boxangle;

            Xmin = corrs[i].neighbors[j]->Xmin;
            Xmax = corrs[i].neighbors[j]->Xmax;
            Ymin = corrs[i].neighbors[j]->Ymin;
            Ymax = corrs[i].neighbors[j]->Ymax;

            if(debug >= 3 || corrs[i].id == refimage)
            {
               printf("\n--------------------------------------------------\n");
               printf("\nCorrection %d (%d) / Neighbor %d (%d)\n\nPixel Range:\n",
                  i, corrs[i].id, j, corrs[i].neighbors[j]->minus);
               printf("i:     %12.5e->%12.5e (%12.5e)\n", imin, imax, imax-imin+1);
               printf("j:     %12.5e->%12.5e (%12.5e)\n", jmin, jmax, jmax-jmin+1);
               printf("X:     %12.5e->%12.5e (%12.5e)\n", Xmin, Xmax, Xmax-Xmin+1);
               printf("Y:     %12.5e->%12.5e (%12.5e)\n", Ymin, Ymax, Ymax-Ymin+1);
               printf("angle: %-g\n", theta);
               printf("\n");

               fflush(stdout);
            }

            sinTheta = sin(theta*dtr);
            cosTheta = cos(theta*dtr);

            dsumn = (Xmax - Xmin) * (Ymax - Ymin);

            dsumx  = (Ymax - Ymin) * (Xmax*Xmax - Xmin*Xmin)/2. * cosTheta 
                   - (Xmax - Xmin) * (Ymax*Ymax - Ymin*Ymin)/2. * sinTheta;

            dsumy  = (Ymax - Ymin) * (Xmax*Xmax - Xmin*Xmin)/2. * sinTheta 
                   + (Xmax - Xmin) * (Ymax*Ymax - Ymin*Ymin)/2. * cosTheta;
            
            dsumxx = (Ymax - Ymin) * (Xmax*Xmax*Xmax - Xmin*Xmin*Xmin)/3. * cosTheta*cosTheta
                   - 2. * (Xmax*Xmax - Xmin*Xmin)/2. * (Ymax*Ymax - Ymin*Ymin)/2. * cosTheta*sinTheta
                   + (Xmax - Xmin) * (Ymax*Ymax*Ymax - Ymin*Ymin*Ymin)/3. * sinTheta*sinTheta;

            dsumyy = (Ymax - Ymin) * (Xmax*Xmax*Xmax - Xmin*Xmin*Xmin)/3. * sinTheta*sinTheta
                   + 2. * (Xmax*Xmax - Xmin*Xmin)/2. * (Ymax*Ymax - Ymin*Ymin)/2. * sinTheta*cosTheta
                   + (Xmax - Xmin) * (Ymax*Ymax*Ymax - Ymin*Ymin*Ymin)/3. * cosTheta*cosTheta;

            dsumxy = (Ymax - Ymin) * (Xmax*Xmax*Xmax - Xmin*Xmin*Xmin)/3. * cosTheta*sinTheta
                   + (Xmax*Xmax - Xmin*Xmin)/2. * (Ymax*Ymax - Ymin*Ymin)/2. * (cosTheta*cosTheta - sinTheta*sinTheta)
                   - (Xmax - Xmin) * (Ymax*Ymax*Ymax - Ymin*Ymin*Ymin)/3. * sinTheta*cosTheta;


            if(debug >= 3 || corrs[i].id == refimage)
            {
               printf("\nSums:\n");
               printf("dsumn   = %12.5e\n", dsumn);
               printf("dsumx   = %12.5e\n", dsumx);
               printf("dsumy   = %12.5e\n", dsumy);
               printf("dsumxx  = %12.5e\n", dsumxx);
               printf("dsumxy  = %12.5e\n", dsumxy);
               printf("dsumyy  = %12.5e\n", dsumyy);
               printf("\n");

               fflush(stdout);
            }

            sumn  += dsumn;
            sumx  += dsumx;
            sumy  += dsumy;
            sumxx += dsumxx;
            sumxy += dsumxy;
            sumyy += dsumyy;

            index = corrs[i].neighbors[j]->plusimg->id;

            A = corrs[i].neighbors[j]->a;
            B = corrs[i].neighbors[j]->b;
            C = corrs[i].neighbors[j]->c;

            sumz  += A * dsumx  + B * dsumy  + C * dsumn;
            sumxz += A * dsumxx + B * dsumxy + C * dsumx;
            sumyz += A * dsumxy + B * dsumyy + C * dsumy;
            
            if(debug >= 3 || corrs[i].id == refimage)
            {
               printf("\n");
               printf("sumn    = %12.5e\n", sumn);
               printf("sumx    = %12.5e\n", sumx);
               printf("sumy    = %12.5e\n", sumy);
               printf("sumxx   = %12.5e\n", sumxx);
               printf("sumxy   = %12.5e\n", sumxy);
               printf("sumyy   = %12.5e\n", sumyy);
               printf("A       = %12.5e\n", A);
               printf("B       = %12.5e\n", B);
               printf("C       = %12.5e\n", C);
               printf("sumz    = %12.5e\n", sumz);
               printf("sumxz   = %12.5e\n", sumxz);
               printf("sumyz   = %12.5e\n", sumyz);
               printf("\n");

               fflush(stdout);
            }
         }


         /* If we found no overlaps, don't */
         /* try to compute correction      */

         if(sumn == 0.)
            continue;



         /***********************************/
         /* Least-squares plane calculation */

         /*** Fill the matrix and vector  ****

              |a00  a01 a02| |A|   |b00|
              |a10  a11 a12|x|B| = |b01|
              |a20  a21 a22| |C|   |b02|

         *************************************/

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

         if(debug >= 3 || corrs[i].id == refimage)
         {
            printf("\n\n============================================================================================================\n\n");
            printf("Correction %d solution:\n", corrs[i].id);
            printf("Matrix:\n");
            printf("| %12.5e %12.5e %12.5e | |A|   |%12.5e|\n", a[0][0], a[0][1], a[0][2], b[0][0]);
            printf("| %12.5e %12.5e %12.5e |x|B| = |%12.5e|\n", a[1][0], a[1][1], a[1][2], b[1][0]);
            printf("| %12.5e %12.5e %12.5e | |C|   |%12.5e|\n", a[2][0], a[2][1], a[2][2], b[2][0]);
            printf("\n");

            fflush(stdout);
         }


         /* Solve */

         if(fittype == LEVEL)
         {
            b[0][0] = 0.;
            b[1][0] = 0.;
            b[2][0] = sumz/sumn;

            istatus = 0;
         }
         else if(fittype == SLOPE)
         {
            n = 2;
            istatus = mBgModel_gaussj(a, n, b, m);

            b[2][0] = 0.;
         }
         else if(fittype == BOTH)
         {
            n = 3;
            istatus = mBgModel_gaussj(a, n, b, m);
         }
         else
         {
            sprintf(returnStruct->msg, "Invalid fit type");
            return returnStruct;
         }


         /* Singular matrix, don't use corrections */

         if(istatus)
         {
            b[0][0] = 0.;
            b[1][0] = 0.;
            b[2][0] = 0.;
         }

         /* Save the corrections */

         if(debug >= 3)
         {
            printf("\nMatrix Solution:\n");

            printf(" |%12.5e|\n", b[0][0]);
            printf(" |%12.5e|\n", b[1][0]);
            printf(" |%12.5e|\n", b[2][0]);
            printf("\n");

            fflush(stdout);
         }

         corrs[i].acorrection = b[0][0] / 2.;
         corrs[i].bcorrection = b[1][0] / 2.;
         corrs[i].ccorrection = b[2][0] / 2.;

         if(debug >= 2 || corrs[i].id == refimage)
         {
            printf("Background corrections (Correction %d (%4d) / Iteration %d) ", 
               i, corrs[i].id, iteration+1);

                 if(fittype == LEVEL) printf(" (LEVEL):\n");
            else if(fittype == SLOPE) printf(" (SLOPE):\n");
            else if(fittype == BOTH)  printf(" (BOTH ):\n");
            else                      printf(" (ERROR):\n");

            if(istatus)
               printf("\n***** Singular Matrix ***** \n\n");

            printf("  A = %12.5e\n",   corrs[i].acorrection);
            printf("  B = %12.5e\n",   corrs[i].bcorrection);
            printf("  C = %12.5e\n\n", corrs[i].ccorrection);

            fflush(stdout);
         }
      }


      /***************************************/
      /* Apply the corrections to each image */
      /***************************************/

      for(i=0; i<ncorrs; ++i)
      {
         corrs[i].a += corrs[i].acorrection;
         corrs[i].b += corrs[i].bcorrection;
         corrs[i].c += corrs[i].ccorrection;

         if(debug >= 1 || corrs[i].id == refimage)
         {
            printf("Correction %4d (i.e., Image %4d) / Iteration %4d values:  ", 
               i, corrs[i].id, iteration+1);

            if(corrs[i].id == refimage)
               printf("\n");

            printf(" %12.5e ",  corrs[i].a);
            printf(" %12.5e ",  corrs[i].b);
            printf(" %12.5e ",  corrs[i].c);

            if(corrs[i].id == refimage)
               printf("\n\n");

            fflush(stdout);
         }
      }


      /***********************************/
      /* Calculate correction statistics */
      /***********************************/

      badslope = 0;

      if(badslope)
      {
         sumn   = 0.;
         suma   = 0.;
         suma2  = 0.;
         sumb   = 0.;
         sumb2  = 0.;
         sumc   = 0.;
         sumc2  = 0.;

         maxa = 0.;
         maxb = 0.;
         maxc = 0.;

         for(i=0; i<nfits; ++i)
         {
            acorrection = fits[i].plusimg->acorrection;
            bcorrection = fits[i].plusimg->bcorrection;
            ccorrection = fits[i].plusimg->ccorrection;

            if(fits[i].use)
            {
               sumn  += 1.;

               suma  += fabs(acorrection);
               suma2 += acorrection * acorrection;

               sumb  += fabs(bcorrection);
               sumb2 += bcorrection * bcorrection;

               sumc  += fabs(ccorrection);
               sumc2 += ccorrection * ccorrection;

               if(fabs(acorrection) > maxa)
               {
                  maxa = fabs(acorrection);
                  iamax = i;
               }

               if(fabs(bcorrection) > maxb)
               {
                  maxb = fabs(acorrection);
                  ibmax = i;
               }

               if(fabs(ccorrection) > maxc)
               {
                  maxc = fabs(ccorrection);
                  icmax = i;
               }
            }
         }

         avea   = suma / sumn;
         sigmaa = sqrt(suma2/sumn - avea*avea);

         aveb   = sumb / sumn;
         sigmab = sqrt(sumb2/sumn - aveb*aveb);

         avec   = sumc / sumn;
         sigmac = sqrt(sumc2/sumn - avec*avec);

         if(debug >= 1)
         {
            printf("Iteration %6d Overlaps Statistics\n", iteration);
            printf("   A: %-g +/- %-g, max: %-g (%s - %s overlap)\n",   avea, sigmaa, maxa, imgs[fits[iamax].plusind].fname, imgs[fits[iamax].minusind].fname);
            printf("   B: %-g +/- %-g, max: %-g (%s - %s overlap)\n",   aveb, sigmab, maxb, imgs[fits[ibmax].plusind].fname, imgs[fits[ibmax].minusind].fname);
            printf("   C: %-g +/- %-g, max: %-g (%s - %s overlap)\n\n", avec, sigmac, maxc, imgs[fits[icmax].plusind].fname, imgs[fits[icmax].minusind].fname);
            fflush(stdout);
         }
      }


      /*************************************/
      /* Apply the corrections to each fit */
      /*************************************/

      for(i=0; i<nfits; ++i)
      {
         // To avoid instabilities, don't allow corrections that are more
         // than 10 times the RMS correction.
         
         if(badslope && (fittype == SLOPE || fittype == BOTH))
         {
            if(fabs(fits[i].plusimg->acorrection) > 4.*sigmaa)
            {
               printf("Ignoring fit %d\n", i);
               fflush(stdout);
               continue;
            }

            if(fabs(fits[i].plusimg->bcorrection) > 4.*sigmab)
            {
               printf("Ignoring fit %d\n", i);
               fflush(stdout);
               continue;
            }
         }

         fits[i].a -= fits[i].plusimg->acorrection;
         fits[i].b -= fits[i].plusimg->bcorrection;
         fits[i].c -= fits[i].plusimg->ccorrection;

         fits[i].a += fits[i].minusimg->acorrection;
         fits[i].b += fits[i].minusimg->bcorrection;
         fits[i].c += fits[i].minusimg->ccorrection;


         if(debug >= 2 || fits[i].plusimg->id == refimage || fits[i].plusimg->id == refimage)
         {
            if(i == 0)
               printf("\n");

            printf("Corrected fit (fit %5d / Iteration %5d / %4d vs %4d) ", 
               i, iteration+1, fits[i].plusimg->id, fits[i].minusimg->id);

                 if(fittype == LEVEL) printf(" (LEVEL): ");
            else if(fittype == SLOPE) printf(" (SLOPE): ");
            else if(fittype == BOTH)  printf(" (BOTH ): ");
            else                      printf(" (ERROR): ");

            printf(" %12.5e ",  fits[i].a);
            printf(" %12.5e ",  fits[i].b);
            printf(" %12.5e\n", fits[i].c);

            fflush(stdout);
         }
      }


      ++iteration;

      if(iteration >= niteration)
         break;
   }


   /*****************************************/
   /* Put the corrections in image ID order */
   /*****************************************/

   qsort(corrs, ncorrs, sizeof(struct CorrInfo), mBgModel_corrCompare);



   /*********************************************/
   /* For each image, print out the final plane */
   /*********************************************/

   fputs("|   id   |      a       |      b       |      c       |\n", fout);

   nancnt = 0;

   for(i=0; i<ncorrs; ++i)
   {
      if(mNaN(corrs[i].a)) ++nancnt;
      if(mNaN(corrs[i].b)) ++nancnt;
      if(mNaN(corrs[i].c)) ++nancnt;

      fprintf(fout, " %8d  %13.5e  %13.5e  %13.5e\n", 
         corrs[i].id, corrs[i].a, corrs[i].b, corrs[i].c);

      if(corrs[i].id == refimage)
      {
         printf("\n");
         printf("Final:\n");
         printf("|   id   |      a       |      b       |      c       |\n");

         printf(" %8d  %13.5e  %13.5e  %13.5e\n", 
            corrs[i].id, corrs[i].a, corrs[i].b, corrs[i].c);
         fflush(stdout);
      }
   }
   
   fflush(fout);
   fclose(fout);

   sprintf(montage_msgstr, "nnan=%d", nancnt);
   sprintf(montage_json, "{\"nnan\":%d}", nancnt);

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   returnStruct->nnan = nancnt;

   return returnStruct;
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

int mBgModel_gaussj(float **a, int n, float **b, int m)
{
   int  *indxc, *indxr, *ipiv;
   int   i, icol, irow, j, k, l, ll;
   float big, dum, pivinv, temp;

   irow = 0;
   icol = 0;

   indxc = mBgModel_ivector(n);
   indxr = mBgModel_ivector(n);
   ipiv  = mBgModel_ivector(n);

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
                  mBgModel_free_ivector(ipiv);
                  mBgModel_free_ivector(indxr);
                  mBgModel_free_ivector(indxc);

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
         mBgModel_free_ivector(ipiv);
         mBgModel_free_ivector(indxr);
         mBgModel_free_ivector(indxc);

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

   mBgModel_free_ivector(ipiv);
   mBgModel_free_ivector(indxr);
   mBgModel_free_ivector(indxc);

   return 0;
}


/* Allocates memory for an array of integers */

int *mBgModel_ivector(int nh)
{
   int *v;

   v=(int *)malloc((size_t) (nh*sizeof(int)));

   return v;
}


/* Frees memory allocated by ivector */

void mBgModel_free_ivector(int *v)
{
      free((char *) v);
}


/* Sort the corrections structures in image ID order */

int mBgModel_corrCompare(const void *a, const void *b)
{
   struct CorrInfo *corrA = (struct CorrInfo *)a;
   struct CorrInfo *corrB = (struct CorrInfo *)b;

   return corrA->id > corrB->id;
}
