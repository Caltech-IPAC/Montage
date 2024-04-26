/*
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

#define _GNU_SOURCE

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


// Which parameters to fit (level/slope)

#define LEVEL 0
#define SLOPE 1
#define BOTH  2


// Pattern of fitting (which fit parameters for which iterations)

#define ALL        0
#define LEVEL_ONLY 1
#define FLIP       2
#define TOGGLE     3


// Physical relationship between overlapping images

#define OVERLAP  0
#define ADJACENT 1
#define GAP      2
#define WRAP     3


#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;}

int mBgModel_corrCompare(const void *a, const void *b);


/* This structure contains the basic geometry alignment          */
/* information for images.  Since all images are aligned to the  */
/* same location on the sky (and the same projection, etc.,      */
/* aligning them in cartesian pixel space only requires knowing  */
/* their size and reference pixel coordinates                    */

static struct ImgInfo
{
   int     cntr;
   char    fname[1024];
   int     naxis1;
   int     naxis2;
   double  crpix1;
   double  crpix2;
   int     locked;
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
   double boxx;
   double boxy;
   double boxwidth;
   double boxheight;
   double boxangle;
   int    useFit;
   int    type;
   int    level_only;

   struct CorrInfo *pluscorr;
   struct CorrInfo *minuscorr;

   double xref;
   double yref;

   double xrefm;
   double yrefm;

   int    have_transform;
   double trans[2][2];
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
/*                                                                       */
/*   char  *gapdir         Special case for HiPS maps: fits to the       */
/*                         offsets for plates on opposite sides of       */
/*                         gaps in the projections near the poles        */
/*                                                                       */
/*   int    mode           Four possible background matching modes:      */
/*                                                                       */
/*                         (0) level fitting followed by fitting slope   */
/*                             and level;                                */
/*                                                                       */
/*                         (1) level fitting only;                       */
/*                                                                       */
/*                         (2) toggling between level and slope (N of    */
/*                             each, repeated);                          */
/*                                                                       */
/*                         (3) a full set of level fits followed by a    */
/*                             full set of slope fits                    */
/*                                                                       */
/* The last mode is the most stable and the first the most unstable but  */
/* also (if it works) the one with the smallest residual differences.    */
/* Pure level fitting is often perfectly adequate.                       */
/*                                                                       */
/*   int    useall         Use all the input differences (by default     */
/*                         we exclude very small overlap areas)          */
/*   int    niteration     Number of iterations to run                   */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mBgModelReturn *mBgModel(char *imgfile, char *fitfile, char *corrtbl, char *gapdir, int mode, int useall, int niter, int debug)
{
   int     i, j, k, found, index, stat, iimg=0;
   int     ntoggle, toggle, nancnt;
   int     ncols, iteration, istatus;
   int     maxlevel, refimage, niteration;
   double  averms, avearea;
   double  imin, imax, jmin, jmax;
   double  A, B, C, C0;
   double  Am, Bm, Cm;
   double  xref, yref;
   double  xrefm, yrefm;
   FILE   *fout;
   FILE   *fgap;

   char    tmpstr [1024];
   char    fname  [1024];
   char    line   [1024];
   char    gaptbl [1024];
   char    wraptbl[1024];
   char    append [1024];
   char   *ptr;

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
   int     ilevel_only;
   int     icntr;
   int     ifname;
   int     inl;
   int     ins;
   int     ilocked;

   int     plus_cntr;
   int     minus_cntr;
   char    plus_file [1024];
   char    minus_file[1024];
   double  crpix1, crpix2;
   int     xmin, xmax;
   int     ymin, ymax;
   double  xcenter, ycenter;
   int     npixel;
   double  rms;
   double  boxwidth, boxheight, boxang;
   int     have_transform;
   double  transform[3][3];

   int     fittype;

   int     nrms;
   char    rms_str[128];
   char    level_str[128];

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

   double  areaLimit = 0.001;
   int     fitDelete = 0;


   struct mBgModelReturn *returnStruct;


/*******************************************************************************

Overview of Background Matching in Montage
------------------------------------------

The background modelling is such a core part of Montage (second only to the
basic reprojection) that it is important that we fully describe the general
algorithm in detail.  This algorithm has recently been extended to include some
unusual image-to-image comparisons in support of global background matching and
these will be described as well.

We start with a set of overlapping sky images.  Our approach is based on the
assumption that these images represent some fixed sky plus an additive (planar)
background per image.  This situation happens often enough in astronomy that a
modelling process based on this assumption finds broad applicability.

So what do we have and what do we want?  What we want is a set of correction
parameters.  For us that will be Ax + By + C coefficients defining planes to be
subtracted from each image to bring them all in line with each other.  For N
images, that is 3N parameters.  What we have are the image, or more
specifically, the differences between the images for a collection of overlap
regions.  In principle, we could perform an iterative  non-linear least-squares
fit to all 3N parameters using all the overlap pixel differences for all the
differences.  We have use cases where this would involve the better part of
10^12 such pixel differences (and probably thousands of iterations) and so 
would take way to long to complete.

Two simplifying approximations make the problem solvable in a manageable time.
The first is that the sky is repeatable enough that differences between images
covering the same piece of sky end up being planar (except for noise).  That is,
to first order any sky structure subtracts  out and so difference images fit by 
a plane can be approximated just by that plane.

Where this first approximation involves the data values of the points we want to
fit, the second approximation involves the area over which we sum.  For regular
surveys, the image-to-image overlaps tend to be rather rectangular. We will
assume that even where they aren't we can get away with summing over a
rectangular bounding box around the region (i.e. around each difference image).
Since the first approximation makes the data uniform (of the form Ax+By+C), this
reduces the sums down to a collection of sum(x), sum(x^2), sum(xy) etc. all of
which have analytical ssolutions.  In summation, we end up doing a good
approximation to a full least-squares solution for the 3N parameters with very
little real calcalculation and no large summations.

The process is otherwise what you would expect for the iterative part of a
non-linear least-squares: Use the least squares fitting to determine a delta to
the parameters; adjust things accordingly; and use this the re-fit.  Iterate
until convergence (in our case the process is so fast that we just use a very
large number of iterations rather than trying to accurately determine
convergence criteria.

Here's how this works in practice.  We start off with a list of images and a set
of "fits" to the all the possible image overlaps.  These fits were pre-generated
(and this does take a fair amount of time) by creating a difference images and
fitting a plane to each one.  This plane fitting ignores large excursions (in
case the point sources don't in fact subtract out, to get rid of observing
artifacts like radiation hits, etc.)  The fitting also draws a bounding box
around the non-blank pixels which we can use for the boxes described above.  So
for each overlap "fit" we have the "plus" and "minus" image reference for the
difference, the fit plane and region parameters.  We then create a set of
"correction" structures, one for each image, to keep track of the evolving
correction plane for each image.  The corrections are initialized to a zero
slope/offset and the fits to the planes fit to image differences but both are
going to evolve through the iterations.

It might sound odd at first but the fits are actually duplicated (with reversed
plane parameters).  The reason for this is primarily that the fits as the are
adjusted through the iteration are changed by the amounts associated with the
primary (i.e. plus) image with which they are associated but then have to also
be corrected by the other (minus) image amount.  The bookeepping of this is
easiest if we do this in two steps, remembering the last delta offset associated
with structure as we apply it and then going through the paired fit to get the
value for its twin.  They both end up in the same place but without mental
gymnastics and with simple localization of the information.  This also
makes the addition of "special" pseudo-difference fits easily possible, as
we shall describe later.

So to summary, we loop over the images in the form of their corrections
structures.  For each correction, we do a fast approximation of a least-squares
fit to the set of all differences between the image and all its overlapping
neighbors.  This difference (actually half of it to prevent oscillations) is
applied to the cumulative correction (which starts as zero) and then to all of
the differences.  The whole set (sometimes as big as the whole sky) is then tied
together by finding the paired corrections for the other images and applying
that to the fits as well.  The result is a set of corrections and the updated
fits where everything has been brought closer to a global minimum in the
differences.  After iterating enough, we are in a state where a change to any
image would make a globally worse overall fit.


---


Special Overlaps and Differences for Global Matching
----------------------------------------------------

In the standard background matching described above, the paired fits are mirror
images of each other:  the "plus" and "minus" images are reversed and the fit
planes are simple negatives of each other.  Here we are going to describe a
general technique for adding constraints that help solve unusual problems.  We
will use the first of these cases we have implemented as an example but the
approach is generalizable.  

In processing the all-sky data for HEALPix-based HPX projections on large-scale
parallel processing on parallel cluster or cloud computing, we run into three
situations where the data doesn't give us the neat simple overlaps already
described.  The projection itself is responsible for two of these.  First, HPX
is one of those projections where the sky is split from the pole down to a
reference latitude, leaving blank area in the projected area that don't
correspond to location on the sphere.  This is used on the Earth, for example in
the Interrupted Goode Homolosine projection where the splits are positioned in
oceans and the continents are kept intact (except for Antarctia), or vice versa.
This leaves us with input images that are split across these gaps and with the
desire in general to have backgrounds match across them.

The second situation created by the projection is global "wrap-around".  HPX
actually duplicates part of the sky across the +180/-180 longitude line.  More
generally we have issue with any all-sky projection that we want the background
to be consistent across this wrap-around.  Any all-sky projection created edges
across which we want the sky to remain smooth.

The third situation get added if we try to subdivide the processing to take
advantage of parallization. The easiest way to do this is the break things up
into discrete regions, generally a rectangular grid imposed on the projection
space.  As with the other two situations, this leaves us with images that get
chopped into two or more pieces at the boundaries.  

The details are different but the general approach is to create pair of "fits"
that reflect how the pieces of these artificially divided image (which we will
treat now as separate images for bookeepping) affect each other.  Mainly,
this has to do with how the slope/offset of the plane defining a fit for
one of the regions should be applied to the other in the last step in
our iterations.  This is partiularly complex for the polar gaps above
since the orientation of these planes switches by 90 degrees going
across the gaps.  But the general approach is the same:  given a plane
fit to one image, how does that translate to the corresponding image
where that image, while adjacent on the sky, is widely separated in
projection XY coordinates.

---

"Adjacent" Subimages (across a plate boundary)

This situation is somewhat artificial but also the easiest to deal with.  The
two images are actual parts of the same original and they are still in the same
XY space as each other. So a plane fit to one part can be used for the other and
the regions of the two are side by side.  So not transforming of either is
required.  We can even use the bounding box of the original full image as the
region for both.


"Wrap-Around" Duplicate Images

Here the same original image on the sky will appear it two location in the
projection; one in the lower left of the all-sky near the +180/-180 longitude
line and the other in the upper right near the other instance of this line.  the
same slope/offset applied to one be applied to the other, but with a caveat.  We
are defining our planes as Ax+By+C with x and y defined as the global pixel
coordinates whereas the two planes here are the "same" relative to matching
relative locations in the two instances of the image.  So the plane from one has
to be transformed accordingly to the space of the other and each must similarly
use the region covered by that copy.


"Gap" Subimages 

This is difficult to visualize without referring to all-sky example of the
projection but is reasonably clear if you do.  One of the two subimages will be
against a horizontal line defining the gap and the other will at the same offset
along a similar vertical edge on the other side of the gap.  The two pieces are
rotated 90 degrees relative to each other so the slopes are transposed.  The
offsets need to match at corresponding points along the gap edges.  We can
define all this when we create the pair of fits (same for the other two cases
above) and this does not change with the iterating, so it is just a matter
of applying the right tranforms to the planes associated with each iteration.
 
*******************************************************************************/



   /* Simultaneous equation stuff */

   float **a;
   int     n;
   float **b;
   int     m;

   dtr = atan(1) / 45.;


   // Debug reference image (change here manually)

   refimage = 114;
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

   icntr       = tcol("cntr");
   ifname      = tcol("fname");
   inl         = tcol("nl");
   ins         = tcol("ns");
   icrpix1     = tcol("crpix1");
   icrpix2     = tcol("crpix2");
   ilocked     = tcol("locked");

   if(icntr < 0)
      icntr = tcol("id");

   if(ins < 0)
      ins = tcol("naxis1");

   if(inl < 0)
      inl = tcol("naxis2");

   if(ifname < 0)
      ifname = tcol("file");

   strcpy(append, "");

   if(ifname < 0)
   {
      ifname = tcol("plate");
      strcpy(append, ".fits");
   }

   if(mode == LEVEL_ONLY)
   {
      if(icntr   < 0
      || ifname  < 0)
      {
         tclose();

         sprintf(returnStruct->msg, "Need columns: cntr and fname in image info file (LEVEL_ONLY mode).");
         return returnStruct;
      }
   }
   else
   {
      if(icntr   < 0
      || ifname  < 0
      || inl     < 0
      || ins     < 0
      || icrpix1 < 0
      || icrpix2 < 0)
      {
         tclose();

         sprintf(returnStruct->msg, "Need columns: cntr, fname, nl, ns, crpix1 and crpix2 in image info file.");
         return returnStruct;
      }
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

      imgs[nimages].naxis1     = 0;
      imgs[nimages].naxis2     = 0;
      imgs[nimages].crpix1     = 0.;
      imgs[nimages].crpix2     = 0.;

      imgs[nimages].cntr = atoi(tval(icntr));

      strcpy(imgs[nimages].fname, tval(ifname));

      if(strlen(append) > 0)
         strcat(imgs[nimages].fname, append);

      if(ins     >= 0) imgs[nimages].naxis1 = atoi(tval(ins));
      if(inl     >= 0) imgs[nimages].naxis2 = atoi(tval(inl));
      if(icrpix1 >= 0) imgs[nimages].crpix1 = atof(tval(icrpix1));
      if(icrpix2 >= 0) imgs[nimages].crpix2 = atof(tval(icrpix2));

      imgs[nimages].locked = 0;

      if(ilocked  >= 0) imgs[nimages].locked  = atoi(tval(ilocked));

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

   if(debug >= 2)
   {
      printf("\nImages:\n\n");

      for(i=0; i<nimages; ++i)
         printf("%4d: [%s](%d)\n", i, imgs[i].fname, imgs[i].cntr);

      printf("\n");
   }

   avearea = avearea / nimages;



   /**************************************/ 
   /* Open the difference fit table file */
   /* (the normal ADJACENT images).      */
   /**************************************/ 

   ncols = topen(fitfile);

   if(ncols <= 0)
   {
      sprintf(returnStruct->msg, "Invalid background fit parameters file: %s", fitfile);
      return returnStruct;
   }

   iplus       = tcol("plus");
   iminus      = tcol("minus");
   ia          = tcol("a");
   ib          = tcol("b");
   ic          = tcol("c");
   icrpix1     = tcol("crpix1");
   icrpix2     = tcol("crpix2");
   ixmin       = tcol("xmin");
   ixmax       = tcol("xmax");
   iymin       = tcol("ymin");
   iymax       = tcol("ymax");
   ixcenter    = tcol("xcenter");
   iycenter    = tcol("ycenter");
   inpix       = tcol("npixel");
   irms        = tcol("rms");
   iboxx       = tcol("boxx");
   iboxy       = tcol("boxy");
   iboxwidth   = tcol("boxwidth");
   iboxheight  = tcol("boxheight");
   iboxangle   = tcol("boxang");
   ilevel_only = tcol("level_only");

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
      tclose();
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

      fits[nfits].level_only = 0;

      if(ilevel_only >= 0)
      {
         strcpy(level_str, tval(ilevel_only));

         if(strcasecmp(level_str, "true") == 0
         || strcasecmp(level_str, "1")    == 0)
            fits[nfits].level_only = 1;
      }

      strcpy(rms_str, tval(irms));

      if(strcasestr(rms_str, "inf")
      || strcasestr(rms_str, "nan"))
         fits[nfits].rms = 1.e99;

      boxx = atof(tval(iboxx));
      boxy = atof(tval(iboxy));

      fits[nfits].boxx = boxx;
      fits[nfits].boxy = boxy;

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

      fits[nfits].useFit =  1;

      fits[nfits].type = ADJACENT;

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
      /* For the special 'level_only' entries,   */
      /* the reverse connection is already in    */
      /* input fits.tbl file.                    */

      if(fits[nfits-1].level_only == 0)
      {
         fits[nfits].plus       =  atoi(tval(iminus));
         fits[nfits].minus      =  atoi(tval(iplus));
         fits[nfits].a          = -atof(tval(ia));
         fits[nfits].b          = -atof(tval(ib));
         fits[nfits].c          = -atof(tval(ic));
         fits[nfits].xmin       =  atoi(tval(ixmin));
         fits[nfits].xmax       =  atoi(tval(ixmax));
         fits[nfits].ymin       =  atoi(tval(iymin));
         fits[nfits].ymax       =  atoi(tval(iymax));
         fits[nfits].xcenter    =  atof(tval(ixcenter));
         fits[nfits].ycenter    =  atof(tval(iycenter));
         fits[nfits].npix       =  atof(tval(inpix));
         fits[nfits].rms        =  atof(tval(irms));
         fits[nfits].Xmin       =  fits[nfits-1].Xmin;
         fits[nfits].Xmax       =  fits[nfits-1].Xmax;
         fits[nfits].Ymin       =  fits[nfits-1].Ymin;
         fits[nfits].Ymax       =  fits[nfits-1].Ymax;
         fits[nfits].boxangle   =  fits[nfits-1].boxangle;
         fits[nfits].level_only =  fits[nfits-1].level_only;

         fits[nfits].useFit = 1;

         fits[nfits].type = ADJACENT;

         ++nfits;
      }

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

   averms = averms / nrms;


   /****************************************************************************************/
   /* For special projections, we sometimes have other pair-ups of plates besides          */
   /* the overlaps.  For instance, the HPX projection is mostly Cylindrical Equal Area,    */
   /* except at high latitudes where it "splits open" from the pole down some ~40 degrees. */
   /* Plates on either side of this gap are not only separated by a considerable number    */
   /* of (blank) pixels the matching edges are at right angles to each other.              */
   /*                                                                                      */
   /* Similarly, the far left and right of the projection is a "wrap-around" (-180/+180),  */
   /* contiguous on the real sky.                                                          */
   /*                                                                                      */
   /* The way we analyze these matches, there is one "fit" file for each of them in a      */
   /* "gap" directory and a summary table ("gap.tbl") listing them.                        */
   /****************************************************************************************/

   if(gapdir != (char *)NULL && strlen(gapdir) > 0)
   {
      strcpy(gaptbl, gapdir);
      strcat(gaptbl, "/gap.tbl");

      ncols = topen(gaptbl);

      if(ncols < 1)
      {
         strcpy(returnStruct->msg, "Cannot open gap.tbl file in gap directory.");
         return returnStruct;
      }

      ifname = tcol("file");

      if(ifname < 0)
      {
         strcpy(returnStruct->msg, "gap.tbl file in gap directory does not have 'file' column.");
         return returnStruct;
      }

      while(1)
      {
         stat = tread();

         if(stat < 0)
            break;

         strcpy(fname, tval(ifname));

         if(fname[0] != '/');

         strcpy(tmpstr, gapdir);
         strcat(tmpstr, "/");
         strcat(tmpstr, fname);

         fgap = fopen(tmpstr, "r");

         if(fgap == (FILE *)NULL)
         {
            tclose();

            sprintf(returnStruct->msg, "Cannot open gap file.");
            return returnStruct;
         }

         // Read through the diff file for this special pair.
         // These file, for now, are rigorously structured.

         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%s",          plus_file);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%s",          minus_file);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf %lf", &A, &B, &C);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf",         &C0);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &crpix1, &crpix2);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%d %d",       &xmin, &xmax);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%d %d",       &ymin, &ymax);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &xcenter, &ycenter);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%d %lf",      &npixel, &rms);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &boxx, &boxy);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf %lf", &boxwidth, &boxheight, &boxang);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%s",          tmpstr);
         
         have_transform = 0;
         if(strcmp(tmpstr, "true") == 0)
            have_transform = 1;

         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &transform[0][0], &transform[0][1]);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &transform[1][0], &transform[1][1]);
         fgets(line, 1024, fgap);

         plus_cntr = -1;

         ptr = plus_file;
         for(i=0; i<strlen(plus_file); ++i)
            if(plus_file[i] == '/')
               ptr = plus_file + i + 1;

         for(i=0; i<nimages; ++i)
         {
            if(strcmp(ptr, imgs[i].fname) == 0)
            {
               plus_cntr = imgs[i].cntr;
               break;
            }
         }
         

         minus_cntr = -1;

         ptr = minus_file;
         for(i=0; i<strlen(minus_file); ++i)
            if(minus_file[i] == '/')
               ptr = minus_file + i + 1;

         for(i=0; i<nimages; ++i)
         {
            if(strcmp(ptr, imgs[i].fname) == 0)
            {
               minus_cntr = imgs[i].cntr;
               break;
            }
         }

         width  = boxwidth;
         height = boxheight;
         angle  = boxang * dtr;

         X0 =  boxx * cos(angle) + boxy * sin(angle);
         Y0 = -boxx * sin(angle) + boxy * cos(angle);
         
         fits[nfits].plus       = plus_cntr;
         fits[nfits].minus      = minus_cntr;
         fits[nfits].a          = A;
         fits[nfits].b          = B;
         fits[nfits].c          = C;
         fits[nfits].xmin       = xmin;
         fits[nfits].xmax       = xmax;
         fits[nfits].ymin       = ymin;
         fits[nfits].ymax       = ymax;
         fits[nfits].xcenter    = xcenter;
         fits[nfits].ycenter    = ycenter;
         fits[nfits].npix       = npixel;
         fits[nfits].rms        = rms;
         fits[nfits].Xmin       = X0 - width /2.;
         fits[nfits].Xmax       = X0 + width /2.;
         fits[nfits].Ymin       = Y0 - height/2.;
         fits[nfits].Ymax       = Y0 + height/2.;
         fits[nfits].boxangle   = angle/dtr;

         fits[nfits].level_only = 0;

         fits[nfits].have_transform = have_transform;

         for(i=0; i<2; ++i)
            for(j=0; j<2; ++j)
               fits[nfits].trans[j][i] = transform[j][i];

         fits[nfits].useFit  = 1;
         
         fits[nfits].type = GAP;
         
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

         
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%s",          plus_file);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%s",          minus_file);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf %lf", &A, &B, &C);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf",         &C0);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &crpix1, &crpix2);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%d %d",       &xmin, &xmax);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%d %d",       &ymin, &ymax);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &xcenter, &ycenter);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%d %lf",      &npixel, &rms);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &boxx, &boxy);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf %lf", &boxwidth, &boxheight, &boxang);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%s",          tmpstr);
         
         have_transform = 0;
         if(strcmp(tmpstr, "true") == 0)
            have_transform = 1;

         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &transform[0][0], &transform[0][1]);
         fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &transform[1][0], &transform[1][1]);
         fgets(line, 1024, fgap);

         plus_cntr = -1;

         ptr = plus_file;
         for(i=0; i<strlen(plus_file); ++i)
            if(plus_file[i] == '/')
               ptr = plus_file + i + 1;

         for(i=0; i<nimages; ++i)
         {
            if(strcmp(ptr, imgs[i].fname) == 0)
            {
               plus_cntr = imgs[i].cntr;
               break;
            }
         }
         

         minus_cntr = -1;

         ptr = minus_file;
         for(i=0; i<strlen(minus_file); ++i)
            if(minus_file[i] == '/')
               ptr = minus_file + i + 1;

         for(i=0; i<nimages; ++i)
         {
            if(strcmp(ptr, imgs[i].fname) == 0)
            {
               minus_cntr = imgs[i].cntr;
               break;
            }
         }

         width  = boxwidth;
         height = boxheight;
         angle  = boxang * dtr;

         X0 =  boxx * cos(angle) + boxy * sin(angle);
         Y0 = -boxx * sin(angle) + boxy * cos(angle);
         
         fits[nfits].plus       = plus_cntr;
         fits[nfits].minus      = minus_cntr;
         fits[nfits].a          = A;
         fits[nfits].b          = B;
         fits[nfits].c          = C;
         fits[nfits].xmin       = xmin;
         fits[nfits].xmax       = xmax;
         fits[nfits].ymin       = ymin;
         fits[nfits].ymax       = ymax;
         fits[nfits].xcenter    = xcenter;
         fits[nfits].ycenter    = ycenter;
         fits[nfits].npix       = npixel;
         fits[nfits].rms        = rms;
         fits[nfits].Xmin       = X0 - width /2.;
         fits[nfits].Xmax       = X0 + width /2.;
         fits[nfits].Ymin       = Y0 - height/2.;
         fits[nfits].Ymax       = Y0 + height/2.;
         fits[nfits].boxangle   = angle/dtr;

         fits[nfits].level_only = 0;

         fits[nfits].have_transform = have_transform;

         for(i=0; i<2; ++i)
            for(j=0; j<2; ++j)
               fits[nfits].trans[j][i] = transform[j][i];
         
         fits[nfits].useFit  = 1;
         
         fits[nfits].type = GAP;

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

      }

      tclose();
   }


   /********************************************/
   /* And, if we have them, wraparound matches */
   /********************************************/

   if(gapdir != (char *)NULL && strlen(gapdir) > 0)
   {
      strcpy(wraptbl, gapdir);
      strcat(wraptbl, "/wrap.tbl");

      ncols = topen(wraptbl);

      if(ncols < 1)
      {
         strcpy(returnStruct->msg, "Cannot open wraps.tbl file in gap directory.");
         return returnStruct;
      }

      iplus = tcol("plus");

      if(iplus < 0 || iminus < 0)
      {
         strcpy(returnStruct->msg, "wrap.tbl file in gap directory does not have 'plus' and 'minus' columns.");
         return returnStruct;
      }

      while(1)
      {
         stat = tread();

         if(stat < 0)
            break;

         strcpy(plus_file,  tval(iplus));
         strcpy(minus_file, tval(iminus));


         have_transform = 0;


         plus_cntr = -1;

         ptr = plus_file;
         for(i=0; i<strlen(plus_file); ++i)
            if(plus_file[i] == '/')
               ptr = plus_file + i + 1;

         for(i=0; i<nimages; ++i)
         {
            if(strcmp(plus_file, imgs[i].fname) == 0)
            {
               iimg = i;
               plus_cntr = imgs[i].cntr;
               break;
            }
         }
         

         minus_cntr = -1;

         ptr = minus_file;
         for(i=0; i<strlen(minus_file); ++i)
            if(minus_file[i] == '/')
               ptr = minus_file + i + 1;

         for(i=0; i<nimages; ++i)
         {
            if(strcmp(minus_file, imgs[i].fname) == 0)
            {
               minus_cntr = imgs[i].cntr;
               break;
            }
         }

         fits[nfits].plus       = plus_cntr;
         fits[nfits].minus      = minus_cntr;
         fits[nfits].a          = 0.;
         fits[nfits].b          = 0.;
         fits[nfits].c          = 0.;
         fits[nfits].xmin       = imgs[iimg].crpix1 - imgs[iimg].naxis1/2;
         fits[nfits].xmax       = imgs[iimg].crpix1 + imgs[iimg].naxis1/2;
         fits[nfits].ymin       = imgs[iimg].crpix2 - imgs[iimg].naxis2/2;
         fits[nfits].ymax       = imgs[iimg].crpix2 + imgs[iimg].naxis2/2;
         fits[nfits].xcenter    = imgs[iimg].crpix1; 
         fits[nfits].ycenter    = imgs[iimg].crpix2;
         fits[nfits].npix       = imgs[iimg].naxis1 * imgs[iimg].naxis2;
         fits[nfits].rms        = 1.e-10;
         fits[nfits].Xmin       = fits[nfits].xmin;
         fits[nfits].Xmax       = fits[nfits].xmax;
         fits[nfits].Ymin       = fits[nfits].ymin;
         fits[nfits].Ymax       = fits[nfits].ymax;
         fits[nfits].boxangle   = 0.;

         fits[nfits].level_only = 0;

         fits[nfits].have_transform = have_transform;

         fits[nfits].useFit  = 1;
         
         fits[nfits].type = WRAP;
         
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

         fits[nfits].plus       = minus_cntr;
         fits[nfits].minus      = plus_cntr;
         fits[nfits].a          = 0.;
         fits[nfits].b          = 0.;
         fits[nfits].c          = 0.;
         fits[nfits].xmin       = imgs[iimg].crpix1 - imgs[iimg].naxis1/2;
         fits[nfits].xmax       = imgs[iimg].crpix1 + imgs[iimg].naxis1/2;
         fits[nfits].ymin       = imgs[iimg].crpix2 - imgs[iimg].naxis2/2;
         fits[nfits].ymax       = imgs[iimg].crpix2 + imgs[iimg].naxis2/2;
         fits[nfits].xcenter    = imgs[iimg].crpix1; 
         fits[nfits].ycenter    = imgs[iimg].crpix2;
         fits[nfits].npix       = imgs[iimg].naxis1 * imgs[iimg].naxis2;
         fits[nfits].rms        = 1.e-10;
         fits[nfits].Xmin       = fits[nfits].xmin;
         fits[nfits].Xmax       = fits[nfits].xmax;
         fits[nfits].Ymin       = fits[nfits].ymin;
         fits[nfits].Ymax       = fits[nfits].ymax;
         fits[nfits].boxangle   = 0.;

         fits[nfits].level_only = 0;

         fits[nfits].have_transform = have_transform;

         fits[nfits].useFit  = 1;
         
         fits[nfits].type = WRAP;
         
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

      }

      tclose();
   }


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

   if(debug >= 2)
   {
      printf("\nfits:\n");

      for(i=0; i<nfits; ++i)
         printf("%4d: %d - %d (%d - %d)\n", 
            i, fits[i].plus, fits[i].minus, fits[i].plusind, fits[i].minusind);

      printf("\n");
   }


   /***************************************************/
   /* Sanity check: Are all imgs represented in fits? */
   /***************************************************/

   if(debug)
   {
      for(i=0; i<nimages; ++i)
      {
         found = 0;

         for(j=0; j<nfits; ++j)
         {
            if(fits[j].plus  == i) found = 1;
            if(fits[j].minus == i) found = 1;
         }

         if(!found)
            printf("WARNING: img[%3d] %s (cntr = %3d) not covered by fits.\n",
               i, imgs[i].fname, imgs[i].cntr);
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
            fits[j].pluscorr = &corrs[i];
            break;
         }
      }
      
      for(i=0; i<ncorrs; ++i)
      {
         if(fits[j].minus == corrs[i].id)
         {
            fits[j].minuscorr = &corrs[i];
            break;
         }
      }
      
      if(debug >= 3)
      {
         if(j == 0)
            printf("\n");

         printf("fits[%3d]: (pluscorr=%3d  minuscorr=%3d) ", 
            j, fits[j].pluscorr->id, fits[j].minuscorr->id);

         printf(" %12.5e ",  fits[j].a);
         printf(" %12.5e ",  fits[j].b);
         printf(" %12.5e ",  fits[j].c);
         printf(" (%12.5e)\n", fits[j].rms);

         fflush(stdout);
      }
   }


   for(k=0; k<nfits; ++k)
      fits[k].useFit = 1;

   if(!useall)
   {
      /***********************************************/
      /* Turn off the fits which represent those     */
      /* overlaps which are smaller than 2% of the   */
      /* average image.                              */
      /***********************************************/

      for(k=0; k<nfits; ++k)
      {
         if(fits[k].rms >= 1.e99)
         {
            fits[k].useFit = 0;

            continue;
         }

         if(fits[k].npix < areaLimit * avearea)
         {
            if(debug >= 2)
               printf("not using fit %d [%d|%d] (area too small: %d/%-g\n",
                  k, fits[k].plus, fits[k].minus, fits[k].npix, avearea);

            fits[k].useFit = 0;

            ++fitDelete;

            continue;
         }
      }

      if(debug >= 1)
      {
         printf("Removed %d 'small' fits.", fitDelete);
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


   /************************************************************************************************************/
   /*                                                                                                          */
   /* The iterative process of adjusting the individual image backgrounds is the core of this code but         */
   /* is also where the code is most sensitive to details.  It is possible to fit both the overall background  */
   /* level for each input image and the gradient (and X and Y slopes) or just level or just slope.  For most  */
   /* image sets where both level and gradient can vary, trying to fit both once can be a bit unstable and has */
   /* trouble converging.                                                                                      */
   /*                                                                                                          */
   /* Various fitting patterns have proven to be better for different datasets and have been given IDs here.   */
   /* If we find many more we will probably switch to a pattern-based input rather than a set of IDs.  At the  */
   /* moment we support the following:                                                                         */
   /*                                                                                                          */
   /*    ID 0:  Fit both level and gradient with each iteration.  Since this has proven unstable often         */
   /*    enough, we preceed it with some level-only fits.  (ALL)                                               */
   /*                                                                                                          */
   /*    ID 1:  Fit the level only.  This has proven to be adequate for a lot of data and should be tried      */
   /*    first as it is extremely stable.  (LEVEL_ONLY)                                                        */
   /*                                                                                                          */
   /*    ID 2:  Flip between sets of fitting the level and sets of fitting the gradient by itself.  Each       */
   /*    'set' is 1000 iterations (if defaults are used).  So long as the gradient variations aren't too       */
   /*    extreme, this has proven pretty stable and is currently the default.  (FLIP)                          */
   /*                                                                                                          */
   /*    ID 3:  First fit the level only for a large number of iterations, then toggle to the gradient only    */
   /*    for an equally large number of iterations.  This might ultimately prove to be the best approach       */
   /*    since it doesn't suffer from the kind of feedback that gives rise to instability.  (TOGGLE)           */
   /*                                                                                                          */
   /************************************************************************************************************/

   fittype = LEVEL;

   iteration = 0;

   while(1)
   {
      /*************************************************/
      /* Original logic: Either level fitting or level */
      /* fitting followed by fitting slope and level.  */
      /*************************************************/

      // Level-only is extremely stable but can result
      // in a bit of a saw-tooth look 
      
      if(mode == LEVEL_ONLY)
         fittype = LEVEL;


      // Fitting BOTH level and slope together is very often
      // unstable (a feedback where a level shift drives a slope
      // change and vice versa).  So much so that we always
      // do at least a few level-only iteractions up front first.

      if(mode == ALL)
      {
         if(iteration < maxlevel)
            fittype = LEVEL;
         else
            fittype = BOTH;
      }


      // An alternate approach:  toggle between level
      // fitting and slope fitting.  There appears to be
      // some inherent instability in fitting both level and
      // slope, most apparent if fitting them BOTH, as above
      // and less so if FLIPping back and forth.  TOGGLE from
      // just level to just slope once doesn't provide any 
      // opportunity for the feedback that feeds this 
      // instability, so is the most stable (but potentially
      // a poorer fit).

      if(mode == FLIP || mode == TOGGLE)
      {
         ntoggle = 1000;
         if(niteration < 10000)
            ntoggle = niteration / 10.;

         if(mode == TOGGLE)
            ntoggle = (int)((niteration + 1.)/2.);

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
              if(fittype == LEVEL)    printf(" (LEVEL   ): ");
         else if(fittype == SLOPE)    printf(" (SLOPE   ): ");
         else if(fittype == BOTH)     printf(" (BOTH    ): ");
         else                         printf(" (ERROR   ): ");
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

            if(corrs[i].neighbors[j]->useFit == 0)
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
            
            index = corrs[i].neighbors[j]->pluscorr->id;

            A = corrs[i].neighbors[j]->a;
            B = corrs[i].neighbors[j]->b;
            C = corrs[i].neighbors[j]->c;

            if(corrs[i].neighbors[j]->level_only == 1)
            {
               // In the case where the relationship between i and j is "level_only" 
               // (basically to support special processing of HPX all-sky background
               // matching, we want to apply a 'C' value that is the offset at the center
               // of the 'overlap', with no slopes.  Really we just want to nudge the
               // special cases, which really are the same image across a 'gap', in the
               // all-sky wrap-around replication region, or oven just duplicated at a
               // plate boundary, back to having the same level.  Slope (and most of the
               // level info even) are controlled by real neighbors.

               C = A * corrs[i].neighbors[j]->boxx
                 + B * corrs[i].neighbors[j]->boxy
                 + C;

               A = 0.;
               B = 0.;
            }
            
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

         if(imgs[corrs[i].id].locked)
         {
            corrs[i].acorrection = 0.;
            corrs[i].bcorrection = 0.;
            corrs[i].ccorrection = 0.;
         }
         else
         {
            corrs[i].acorrection = b[0][0] / 2.;
            corrs[i].bcorrection = b[1][0] / 2.;
            corrs[i].ccorrection = b[2][0] / 2.;
         }

         if(debug >= 2 || corrs[i].id == refimage)
         {
            printf("Background corrections (Correction %d (%4d) / Iteration %d) ", 
               i, corrs[i].id, iteration+1);

                 if(fittype == LEVEL)    printf(" (LEVEL   ): ");
            else if(fittype == SLOPE)    printf(" (SLOPE   ): ");
            else if(fittype == BOTH)     printf(" (BOTH    ): ");
            else                         printf(" (ERROR   ): ");

            if(istatus)
               printf("\n***** Singular Matrix ***** \n\n");

            printf("  A = %12.5e\n",   corrs[i].acorrection);
            printf("  B = %12.5e\n",   corrs[i].bcorrection);
            printf("  C = %12.5e\n\n", corrs[i].ccorrection);

            fflush(stdout);
         }


         /* Apply the corrections increments to the image */

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
            else
               printf("\n");

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
            acorrection = fits[i].pluscorr->acorrection;
            bcorrection = fits[i].pluscorr->bcorrection;
            ccorrection = fits[i].pluscorr->ccorrection;

            if(fits[i].useFit)
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

      for(k=0; k<nfits; ++k)
      {
         // To avoid instabilities, don't allow corrections that are more
         // than 10 times the RMS correction.
         
         if(badslope && (fittype == SLOPE || fittype == BOTH))
         {
            if(fabs(fits[k].pluscorr->acorrection) > 4.*sigmaa)
            {
               printf("Ignoring fit %d\n", i);
               fflush(stdout);
               continue;
            }

            if(fabs(fits[k].pluscorr->bcorrection) > 4.*sigmab)
            {
               printf("Ignoring fit %d\n", i);
               fflush(stdout);
               continue;
            }
         }

         fits[k].a -= fits[k].pluscorr->acorrection;
         fits[k].b -= fits[k].pluscorr->bcorrection;
         fits[k].c -= fits[k].pluscorr->ccorrection;


         // For our "special" overlaps, the "minus" correction may need to be transformed,
         // since the plane there was calculated for a different region of the space but 
         // needs to be treated as local to the "plus" location.

         if(fits[k].type == ADJACENT)
         {
            // These don't need special treatment but 
            // we'll give it a section just to call out
            // its existence

            fits[k].a += fits[k].minuscorr->acorrection;
            fits[k].b += fits[k].minuscorr->bcorrection;
            fits[k].c += fits[k].minuscorr->ccorrection;
         }

         else if(fits[k].type == WRAP)
         {
            // Here the slopes don't change, there is
            // just a reference pixel shift.  In truth,
            // getting actual (xref, yref) locations 
            // doesn't really matter as it is the difference
            // in x and the difference in y that are used.
            // So here we use the difference in the original
            // image crpix values.
            
            A = fits[k].minuscorr->acorrection;
            B = fits[k].minuscorr->bcorrection;
            C = fits[k].minuscorr->ccorrection;

            xref = imgs[fits[k].plus].crpix1;
            yref = imgs[fits[k].plus].crpix2;

            xrefm = imgs[fits[k].minus].crpix1;
            yrefm = imgs[fits[k].minus].crpix2;

            C = A * (xref-xrefm) + B * (yref - yrefm) + C;

            fits[k].a += A;
            fits[k].b += B;
            fits[k].c += C;
         }

         else if(fits[k].type == GAP)
         {
            // Going across a gap, the slopes get transposed through
            // rotation.  This was taken care of in the program that 
            // created the fits structure (by looking at whether a specific 
            // transform was on the left or right side of the all-sky and
            // whether the 'plus' image was above or below the 'minus' in 
            // terms of Y coordiate.

            A = fits[k].minuscorr->acorrection;
            B = fits[k].minuscorr->bcorrection;
            C = fits[k].minuscorr->ccorrection;

            Am = fits[k].trans[0][0] * A + fits[k].trans[0][1] * B;
            Bm = fits[k].trans[1][0] * A + fits[k].trans[1][1] * B;

            Cm = C;

            fits[k].a += Am;
            fits[k].b += Bm;
            fits[k].c += Cm;
         }

         else  // Normal overlap
         {
            fits[k].a += fits[k].minuscorr->acorrection;
            fits[k].b += fits[k].minuscorr->bcorrection;
            fits[k].c += fits[k].minuscorr->ccorrection;
         }


         if(debug >= 2 || fits[k].pluscorr->id == refimage || fits[k].pluscorr->id == refimage)
         {
            if(i == 0)
               printf("\n");

            printf("Corrected fit (fit %5d / Iteration %5d / %4d vs %4d) ", 
               k, iteration+1, fits[k].pluscorr->id, fits[k].minuscorr->id);

                 if(fittype == LEVEL)    printf(" (LEVEL   ): ");
            else if(fittype == SLOPE)    printf(" (SLOPE   ): ");
            else if(fittype == BOTH)     printf(" (BOTH    ): ");
            else                         printf(" (ERROR   ): ");

            printf(" %12.5e ",  fits[k].a);
            printf(" %12.5e ",  fits[k].b);
            printf(" %12.5e\n", fits[k].c);

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

   int    first;
   double cmin, cmax;

   cmin = 0.;
   cmax = 0.;

   first = 1;

   fputs("|   id   |      a       |      b       |      c       |\n", fout);

   nancnt = 0;

   for(i=0; i<ncorrs; ++i)
   {
      if(mNaN(corrs[i].a)) ++nancnt;
      if(mNaN(corrs[i].b)) ++nancnt;
      if(mNaN(corrs[i].c)) ++nancnt;

      if(first && !mNaN(corrs[i].c))
      {
         cmin = corrs[i].c;
         cmax = corrs[i].c;
         
         first = 0;
      }

      if(!mNaN(corrs[i].c))
      {
         if(corrs[i].c < cmin)
            cmin = corrs[i].c;

         if(corrs[i].c > cmax)
            cmax = corrs[i].c;
      }

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

   sprintf(montage_msgstr, "nnan=%d, cmin=%-g, cmax=%-g",
      nancnt, cmin, cmax);

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
