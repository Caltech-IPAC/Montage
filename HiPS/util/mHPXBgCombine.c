/* Module: mMPXFindFragments.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <dirent.h>

#include <fitsio.h>
#include <mtbl.h>

#include <montage.h>

#define MAXSTR  256
#define MAXCNT  128

#define OVERLAP  0
#define ADJACENT 1
#define GAP      2
#define WRAP     3

#define NORTH    0
#define SOUTH    1

#define CW       0
#define CCW      1


int mHPXFindFragments_imCompare(const void *a, const void *b);


/* This is a list of the plates we want to combine */

char **plates;

int nplates;


/* This structure contains the basic geometry alignment          */
/* information for images.  Since all images are aligned to the  */
/* same location on the sky (and the same projection, etc.,      */
/* aligning them in cartesian pixel space only requires knowing  */
/* their size and reference pixel coordinates                    */

typedef struct
{
   int    cntr;
   int    plate_cntr;
   char   plate[1024];
   char   fname[1024];
   int    naxis1;
   int    naxis2;
   double crpix1;
   double crpix2;
}
ImgInfo;

ImgInfo *imgs;

static int nimages, maximages;


/* This structure contains the information describing the */
/* plane to be subtracted from each image to "correct" it */
/* to its neighbors.                                      */

static struct FitInfo
{
   int    plus;
   int    minus;
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
   double boxx;
   double boxy;
   double boxwidth;
   double boxheight;
   double boxangle;
   int    use;
   int    type;
   int    xref;
   int    yref;
   int    xrefm;
   int    yrefm;
   double trans[2][2];
}
*fits, *fragments;

static int nfits, maxfits;


static struct Set
{
   int cntr;
   char plate[1024];
   char fname[1024];
}
set[32];

int nset, setcount, dupcount;

struct timeval start;
struct timeval end;


int debug = 0;



/*-***********************************************************************/
/*                                                                       */
/*  mHPXFindFragments                                                    */
/*                                                                       */
/*  For large, high-resolution HiPS maps, we do the background           */
/*  difference fitting subsetted by plates.  Before running the          */
/*  background modelling we need to combine the results of that          */
/*  processing into a single reprojected image list and a a single       */
/*  difference FITS table.                                               */
/*                                                                       */
/*  This tiled approach will have a subset of images at the tile         */
/*  boundaries split between two (sometimes more) tiles and the way we   */
/*  tie the whole sky together is by creating zero-offset differences    */
/*  between these image pieces.  Similarly, there are other "pseudo-     */
/*  differences" to tie together the sky across splits that occur in     */
/*  the projection associated with the poles and at the +180/-180        */
/*  longitude sky wrap-around.                                           */
/*                                                                       */
/*  A big chunk of this processing can be done very quickly but there is */
/*  bit in the middle that takes a long time, so we have had to split    */
/*  the processing into three parts, with the middle bit (which can be   */
/*  done in parallel by plate) parallelized and run on the cluster.      */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int     i, j, k, l, m, ch, index, stat, imgoffset;
   int     ntoggle, toggle, count, nind, fitscntr;
   int     ncols, iteration, istatus, status, nullcnt;
   int     maxlevel, refimage, niteration;
   int     side, rotation;
   double  imin, imax, jmin, jmax;
   double  A, B, C;
   double *data;
   FILE   *fimg, *ffit;

   int     iplate;
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
   int     icntr;
   int     ifname;
   int     inl;
   int     ins;

   double  a;
   double  b;
   double  c;
   double  crpix1;
   double  crpix2;
   int     xmin;
   int     xmax;
   int     ymin;
   int     ymax;
   double  xcenter;
   double  ycenter;
   double  npixel;
   double  rms;
   double  boxx;
   double  boxy;
   double  boxwidth;
   double  boxheight;
   double  boxangle;

   double  delta1, delta2;
   int     naxis1, naxis2;

   char    cwd       [1024];
   char    line      [1024];
   char    projectdir[1024];
   char    platelist [1024];
   char    imgfile   [1024];
   char    fitfile   [1024];
   char    outimg    [1024];
   char    outfit    [1024];
   char    tmpdir    [1024];
   char    filename  [1024];

   long    fpixel[4], nelements;

   long    naxis[2];
   double  crpix[2];
   int     nfound;

   char   *ptr;

   fitsfile *ffits;

   struct WorldCoor *wcs;

   char *header;

   struct mFitplaneReturn *fitplane;


   getcwd(cwd, 1024);

   header = (char *)malloc(80000*sizeof(char));


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
      value.c[i] = (char)255;

   nan = value.d;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   // Command-line arguments

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXFindFragments [-d] projectdir platelist.tbl images.tbl fits.tbl\"]\n");
      fflush(stdout);
      exit(1);
   }

   debug = 0;

   while ((ch = getopt(argc, argv, "d")) != EOF)
   {
      switch (ch)
      {
         case 'd':
            debug = 1;
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXFindFragments [-d] projectdir platelist.tbl images.tbl fits.tbl\"]\n");
            fflush(stdout);
            exit(1);
      }
   }

   strcpy(projectdir, argv[optind]);
   strcpy(platelist,  argv[optind + 1]);
   strcpy(outimg,     argv[optind + 2]);
   strcpy(outfit,     argv[optind + 3]);


   if(projectdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, projectdir);

      strcpy(projectdir, tmpdir);
   }

   if(platelist[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platelist);

      strcpy(platelist, tmpdir);
   }

   if(outimg[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, outimg);

      strcpy(outimg, tmpdir);
   }

   if(outfit[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, outfit);

      strcpy(outfit, tmpdir);
   }

   if(debug)
   {
      printf("projectdir = %s\n", projectdir);
      printf("platelist  = %s\n", platelist);
      printf("outfit    = %s\n", outfit);
      fflush(stdout);
   }

   fimg = fopen(outimg, "w+");

   if(fimg == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Failed to open output images file [%s].\"]\n", outimg);
      fflush(stdout);
      exit(1);
   }

   ffit = fopen(outfit, "w+");

   if(ffit == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Failed to open output fits file [%s].]\n", outfit);
      fflush(stdout);
      exit(1);
   }

   fitscntr = 0;


   /**************************/ 
   /* Read in the plate list */
   /**************************/ 

   ncols = topen(platelist);

   if(ncols < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open plate list file [%s].\"]\n", platelist);
      fflush(stdout);
      exit(1);
   }

   nind = atoi(tfindkey("nplate"));

   nplates = tlen();

   iplate = tcol("plate");

   if(iplate < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need plate column in plate list file [%s].\"]\n", platelist);
      fflush(stdout);
      exit(1);
   }

   plates = (char **)malloc(nplates * sizeof(char *));

   for(i=0; i<nplates; ++i)
      plates[i] = (char *)malloc(MAXSTR * sizeof(MAXSTR));

   count = 0;

   for(i=0; i<nplates; ++i)
   {
      if(tread() < 0)
         break;

      strcpy(plates[i], tval(iplate));

      if(debug)
      {
         printf("DEBUG> plates[%d] = [%s]\n", i, plates[i]);
         fflush(stdout);
      }

      ++count;
   }

   tclose();

   if(count < nplates)
   {
      printf("[struct stat=\"ERROR\", msg=\"Should be %d plates; only found %d.\"]\n", nplates, count);
      fflush(stdout);
      exit(1);
   }


   /***********************************************************************************/
   /* Now we loop over the image lists and image difference lists for all the plates, */
   /* combining both into single lists.  Image 'cntr' and difference "plus'/'minus'   */
   /* get updated to work in the combined lists.                                      */
   /***********************************************************************************/

   nimages   =      0;
   maximages = MAXCNT;

   if(debug >= 2)
   {
      printf("Allocating imgs to %d (size %lu) [11]\n", maximages, maximages * sizeof(ImgInfo));
      fflush(stdout);
   }

   imgs = (ImgInfo *)malloc(maximages * sizeof(ImgInfo));

   if(imgs == (ImgInfo *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"malloc() failed (ImgInfo).\"]\n");
      fflush(stdout);
      exit(1);
   }


   nfits   =      0;
   maxfits = MAXCNT;

   if(debug >= 2)
   {
      printf("Allocating fits to %d (size %lu) [12]\n", maxfits, maxfits * sizeof(struct FitInfo));
      fflush(stdout);
   }

   fits = (struct FitInfo *)malloc(maxfits * sizeof(struct FitInfo));

   if(fits == (struct FitInfo *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"malloc() failed (FitInfo).\"]\n");
      fflush(stdout);
      exit(1);
   }


   imgoffset = 0;

   for(k=0; k<nplates; ++k)
   {
      /*********************************************/ 
      /* Open the image header metadata table file */
      /*********************************************/ 

      sprintf(imgfile, "%s/%s/pimages.tbl", projectdir, plates[k]);

      if(debug)
      {
         printf("DEBUG> [%s] imgoffset = %d\n", imgfile, imgoffset);
         fflush(stdout);
      }

      ncols = topen(imgfile);

      if(ncols <= 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Invalid image metadata file: [%s]\"]\n", imgfile);
         fflush(stdout);
         exit(1);
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
         printf("[struct stat=\"ERROR\", msg=\"Need columns: cntr fname nl ns crpix1 crpix2 in image info file [%s].\"]\n", imgfile);
         fflush(stdout);
         exit(1);
      }



      /******************************/ 
      /* Read the image information */ 
      /******************************/ 

      while(1)
      {
         stat = tread();

         if(stat < 0)
            break;

         imgs[nimages].cntr       = atoi(tval(icntr)) + imgoffset;
         imgs[nimages].plate_cntr = atoi(tval(icntr));
         imgs[nimages].naxis1     = atoi(tval(ins));
         imgs[nimages].naxis2     = atoi(tval(inl));
         imgs[nimages].crpix1     = atof(tval(icrpix1));
         imgs[nimages].crpix2     = atof(tval(icrpix2));

         strcpy(imgs[nimages].fname, tval(ifname));
         strcpy(imgs[nimages].plate, plates[k]);

         ++nimages;

         if(nimages >= maximages)
         {
            maximages += MAXCNT;

            if(debug >= 2)
            {
               printf("Reallocating imgs to %d (size %lu) [14]\n", maximages, maximages * sizeof(ImgInfo));
               fflush(stdout);
            }

            imgs = (ImgInfo *)realloc(imgs, 
                                         maximages * sizeof(ImgInfo));

            if(imgs == (ImgInfo *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"realloc() failed (ImgInfo) [1].\"]\n");
               fflush(stdout);
               exit(1);
            }
         }
      }

      tclose();


      /**************************************/ 
      /* Open the difference fit table file */
      /**************************************/ 

      sprintf(fitfile, "%s/%s/fits.tbl", projectdir, plates[k]);
      
      ncols = topen(fitfile);

      if(ncols <= 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Invalid background fit parameters file: [%s]\"]\n", fitfile);
         fflush(stdout);
         exit(1);
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
         printf("[struct stat=\"ERROR\", msg=\"Need columns: plus minus a b c crpix1 crpix2 xmin xmax ymin ymax xcenter ycenter npixel rms boxx boxy boxwidth boxheight boxang.\"]\n");
         fflush(stdout);
         exit(1);
      }



      /*****************/ 
      /* Read the fits */ 
      /*****************/ 

      while(1)
      {
         stat = tread();

         if(stat < 0)
            break;

         fits[nfits].plus      = atoi(tval(iplus))  + imgoffset;
         fits[nfits].minus     = atoi(tval(iminus)) + imgoffset;
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
         fits[nfits].npix      = atoi(tval(inpix));
         fits[nfits].rms       = atof(tval(irms));
         fits[nfits].boxx       = atof(tval(iboxx));
         fits[nfits].boxy       = atof(tval(iboxy));
         fits[nfits].boxwidth   = atof(tval(iboxwidth));
         fits[nfits].boxheight  = atof(tval(iboxheight));
         fits[nfits].boxangle   = atof(tval(iboxangle));

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
               printf("[struct stat=\"ERROR\", msg=\"realloc() failed (FitInfo) [%lu] [2]\"]\n", maxfits * sizeof(struct FitInfo));
               fflush(stdout);
               exit(1);

            }
         }
      }

      tclose();

      if(debug)
      {
         printf("DEBUG> nfits -> %d\n", nfits);
         fflush(stdout);
      }

      imgoffset = nimages + 1;
   }

   if(debug)
   {
      printf("DEBUG> Total images: %d, fits: %d\n", nimages, nfits);
      fflush(stdout);
   }


   /*****************************************************************************/
   /* Print back out the combined image lists and the combined difference fits. */
   /*****************************************************************************/

   fprintf(fimg, "|  cntr  | naxis1 | naxis2 |     crpix1     |     crpix2     |    plate   |plate_cntr|  %100s  |\n", "fname");

   for(i=0; i<nimages; ++i)
   {
      fprintf(fimg, " %8d %8d %8d %16.5f %16.5f %12s %10d %50s \n",
         imgs[i].cntr,
         imgs[i].naxis1,
         imgs[i].naxis2,
         imgs[i].crpix1,
         imgs[i].crpix2,
         imgs[i].plate,
         imgs[i].plate_cntr,
         imgs[i].fname);

      fflush(stdout);
   }

   fflush(fimg);
   fclose(fimg);


   fprintf(ffit, "|%9s|%9s|%16s|%16s|%16s|%14s|%14s|%10s|%10s|%10s|%10s|%13s|%13s|%13s|%16s|%16s|%16s|%16s|%16s|%16s|%8s|%12s|%12s|%12s|%12s|%7s|%7s|%7s|%7s|\n",
      "plus",
      "minus",
      "a",
      "b",
      "c",
      "crpix1",
      "crpix2",
      "xmin",
      "xmax",
      "ymin",
      "ymax",
      "xcenter",
      "ycenter",
      "npixel",
      "rms",
      "boxx",
      "boxy",
      "boxwidth",
      "boxheight",
      "boxang",
      "type",
      "xref",
      "yref",
      "xrefm",
      "yrefm",
      "trans00",
      "trans01",
      "trans10",
      "trans11");

   for(i=0; i<nfits; ++i)
   {
      fprintf(ffit, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13d %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f %8d %12d %12d %12d %12d %7.4f %7.4f %7.4f %7.4f \n",
         fits[i].plus,
         fits[i].minus,
         fits[i].a,
         fits[i].b,
         fits[i].c,
         fits[i].crpix1,
         fits[i].crpix2,
         fits[i].xmin,
         fits[i].xmax,
         fits[i].ymin,
         fits[i].ymax,
         fits[i].xcenter,
         fits[i].ycenter,
         fits[i].npix,
         fits[i].rms,
         fits[i].boxx,
         fits[i].boxy,
         fits[i].boxwidth,
         fits[i].boxheight,
         fits[i].boxangle,
         OVERLAP,
         0,
         0,
         0,
         0,
         1.0,
         0.0,
         0.0,
         1.0);
      
      fflush(ffit);

      // Note: We are consciously not closing ffit yet.
   }


   /*******************************************************************************************/
   /* At this point, we have created a complete list of images and of the image difference    */
   /* fits.  We now need to determine the set of "pseudo-overlap" and generate the scripts    */
   /* that will create the data we need to create them (to be run in parallel on a cloud      */
   /* or cluster.  A third program will then use that data to append these overlaps to the    */
   /* file we just created.                                                                   */
   /*                                                                                         */
   /* These pseudo-overlaps fall into three categories, all based on images that were split   */
   /* by plate boundaries. The first are images covering two adjacent plates.  There are also */
   /* images that appear in both the lower left and the upper right of the all-sky HPX        */
   /* map because of the -180/+180 wrap-around and images that were split over gaps in        */
   /* the projection.  For HPX, these are caused by the way the projection deals with the     */
   /* poles.  For projections like AITOFF (which we don't handle), these are caused by the    */
   /* curved +180/-180 outside edge.                                                          */
   /*******************************************************************************************/

   // To identify the split/duplicated images, we first sort the imgs structure array on the
   // file name column.  Split/duplicate images will have either multiple pieces with the
   // same file name in different plates or full duplicates in different plates.
   
   qsort(imgs, nimages, sizeof(ImgInfo), mHPXFindFragments_imCompare);


   /*********************************************************************/
   /* Print the combined image list in processing (alphabetical) order. */
   /*********************************************************************/

   strcat(outimg, ".sorted");

   fimg = fopen(outimg, "w+");

   if(fimg == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Failed to open output images file [%s].\"]\n", outimg);
      fflush(stdout);
      exit(1);
   }

   fprintf(fimg, "|  cntr  | naxis1 | naxis2 |     crpix1     |     crpix2     |    plate   |plate_cntr|  %100s  |\n", "fname");

   for(i=0; i<nimages; ++i)
   {
      fprintf(fimg, " %8d %8d %8d %16.5f %16.5f %12s %10d %50s \n",
         imgs[i].cntr,
         imgs[i].naxis1,
         imgs[i].naxis2,
         imgs[i].crpix1,
         imgs[i].crpix2,
         imgs[i].plate,
         imgs[i].plate_cntr,
         imgs[i].fname);

      fflush(stdout);
   }

   fflush(fimg);
   fclose(fimg);


   // For each image, collect it and any following image with the same name
   // The theoretical maximum for these "fragments" is, I believe, eight.
   // That would happen when an image falls right on the corner where four
   // tiles meet and for a part of the sky replicated in the "lower left"
   // and "upper right".  We'll double this to sixteen to be safe.

   setcount = 0;
   dupcount = 0;

   fragments = (struct FitInfo *)malloc(16 * sizeof(struct FitInfo));

   if(fragments == (struct FitInfo *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"malloc() failed (fragment FitInfo).\"]\n");
      fflush(stdout);
      exit(1);
   }

   for(i=0; i<nimages; ++i)
   {
      nset = 0;

      set[nset].cntr = imgs[i].cntr;
      
      strcpy(set[nset].plate, imgs[i].plate);
      strcpy(set[nset].fname, imgs[i].fname);
      
      ++nset;

      for(j=i+1; j<nimages; ++j)
      {
         if(strcmp(imgs[j].fname, imgs[i].fname) != 0)
            break;

         set[nset].cntr = imgs[j].cntr;
         
         strcpy(set[nset].plate, imgs[j].plate);
         strcpy(set[nset].fname, imgs[j].fname);
            
         ++nset;
      }

      // If we didn't find anything else in the group, move on
      
      if(nset == 1)
      {
         nset = 0;
         continue;
      }

      setcount += 1;
      dupcount += nset;


      // Just as we had for the basic overlaps, we are going to need a bunch of geometric
      // information for each of these image fragments.  So we apply the same processing
      // to these images that we did to the differences.  The actual plane fit for this 
      // pseudo-difference is easy:  its the same image so the difference plane is zero.
      
      for(j=0; j<nset; ++j)
      {
         sprintf(filename, "%s/%s/projected/%s", projectdir, set[j].plate, set[j].fname);

         gettimeofday(&start, NULL);
         fitplane = mFitplane(filename, 1, 0, 0., 0);
         gettimeofday(&end, NULL);

         fragments[j].use = 1;

         if(fitplane->status)
            fragments[j].use = 0;


         fragments[j].plus      = set[j].cntr;
         fragments[j].minus     = -1;
         fragments[j].a         = 0.;
         fragments[j].b         = 0.;
         fragments[j].c         = 0.;
         fragments[j].crpix1    = fitplane->crpix1;
         fragments[j].crpix2    = fitplane->crpix2;
         fragments[j].xmin      = fitplane->xmin;
         fragments[j].xmax      = fitplane->xmax;
         fragments[j].ymin      = fitplane->ymin;
         fragments[j].ymax      = fitplane->ymax; 
         fragments[j].xcenter   = fitplane->xcenter;
         fragments[j].ycenter   = fitplane->xcenter;
         fragments[j].npix      = fitplane->npixel;
         fragments[j].rms       = 0.;
         fragments[j].boxx      = fitplane->boxx;
         fragments[j].boxy      = fitplane->boxy;
         fragments[j].boxwidth  = fitplane->boxwidth;
         fragments[j].boxheight = fitplane->boxheight;
         fragments[j].boxangle  = fitplane->boxang;
      }


      // For each of the N*(N-1) pairs within the set, we create  create a pair
      // of 'fits' entries.  For HPX projection, we have identified three types
      // of these pairs and two of these types require additional information
      // for use by the mBGModel module.  For the case where the fragments are
      // 'ADJACENT' we don't need anything else, they can be handled the same
      // as the standard 'OVERLAP' entries.
      //
      // For 'WRAP' fragments, we need to take into account the large XY offset
      // through the xref, etc. parameters.  We will set xref,yref to the 
      // current image boxx,boxy and the remote xrefm,yrefm location to the
      // boxx,boxy of that fragment.  Since these two images are identical other
      // than location, these should refer to the same pixel both images, which
      // is all we need.
      //
      // 'GAP' fragments are the most difficult.  There is a shift in location
      // and the coordinate system rotates and both of these are complicated to
      // get right.
      //
      // Note:  Below, we have the fragment structures do double duty as we
      // construct fits entries for output, but just those parts that change 
      // from fit pair to fit pair.  The actual 'fragment' information stays 
      // fixed at the values set above.
      
      int jindx, jindy, kindx, kindy;

      for(j=0; j<nset; ++j)
      {
         jindx = (int)atoi(set[j].plate + 6);
         jindy = (int)atoi(set[j].plate + 9);

         for(k=j+1; k<nset; ++k)
         {
            gettimeofday(&start, NULL);

            kindx = (int)atoi(set[k].plate + 6);
            kindy = (int)atoi(set[k].plate + 9);

            fragments[j].minus = fragments[k].plus;
            fragments[k].minus = fragments[j].plus;


            // ADJACENT
            
            if(abs(jindx-kindx) <= 1 && abs(jindy-kindy) <= 1)
            {
               fragments[j].type = ADJACENT;

               fragments[j].xref  = 0;
               fragments[j].yref  = 0;
               fragments[j].xrefm = 0;
               fragments[j].yrefm = 0;

               fragments[j].trans[0][0] = 1.;
               fragments[j].trans[0][1] = 0.;
               fragments[j].trans[1][0] = 0.;
               fragments[j].trans[1][1] = 1.;


               fragments[k].type = ADJACENT;

               fragments[k].xref  = 0;
               fragments[k].yref  = 0;
               fragments[k].xrefm = 0;
               fragments[k].yrefm = 0;

               fragments[k].trans[0][0] = 1.;
               fragments[k].trans[0][1] = 0.;
               fragments[k].trans[1][0] = 0.;
               fragments[k].trans[1][1] = 1.;
            }


            // GAP
            
            else if(abs(jindx-kindx) <= nind/5 && abs(jindy-kindy) <= nind/5)
            {
               // Determining whether the coordinate system rotation
               // (which is always 90 degrees) is clockwise or counterclockwise
               // can be parameterized by whether the gap is in the north
               // (i.e., to the left of the HPX diagonal) or south (i.e., to 
               // the right) and whether the "plus" image is at higher Y values
               // in general.
               
               fragments[j].type = GAP;
               fragments[k].type = GAP;

               if(fragments[j].boxx < fragments[j].boxy)    // Northern half of the map
               {
                  side = NORTH;

                  if(fragments[j].boxy < fragments[j].boxy) // So minus is up
                  {                                         // and to the right of plus
                     rotation = CW;

                     fragments[j].trans[0][0] =  0.;
                     fragments[j].trans[0][1] = -1.;
                     fragments[j].trans[1][0] =  1.;
                     fragments[j].trans[1][1] =  0.;

                     fragments[k].trans[0][0] =  0.;
                     fragments[k].trans[0][1] =  1.;
                     fragments[k].trans[1][0] = -1.;
                     fragments[k].trans[1][1] =  0.;
                  }
                  else                                     // Minus is down and to the left
                  {
                     rotation = CCW;

                     fragments[j].trans[0][0] =  0.;
                     fragments[j].trans[0][1] =  1.;
                     fragments[j].trans[1][0] = -1.;
                     fragments[j].trans[1][1] =  0.;

                     fragments[k].trans[0][0] =  0.;
                     fragments[k].trans[0][1] = -1.;
                     fragments[k].trans[1][0] =  1.;
                     fragments[k].trans[1][1] =  0.;
                  }
               }

               else                                         // Southern half of the map
               {
                  side = SOUTH;

                  if(fragments[j].boxy > fragments[j].boxy) // So minus is down 
                  {                                         // and to the left of plus
                     rotation = CW;

                     fragments[j].trans[0][0] =  0.;
                     fragments[j].trans[0][1] =  1.;
                     fragments[j].trans[1][0] = -1.;
                     fragments[j].trans[1][1] =  0.;

                     fragments[k].trans[0][0] =  0.;
                     fragments[k].trans[0][1] = -1.;
                     fragments[k].trans[1][0] =  1.;
                     fragments[k].trans[1][1] =  0.;
                  }
                  else                                     // Minus is up and to the right
                  {
                     rotation = CCW;

                     fragments[j].trans[0][0] =  0.;
                     fragments[j].trans[0][1] = -1.;
                     fragments[j].trans[1][0] =  1.;
                     fragments[j].trans[1][1] =  0.;

                     fragments[k].trans[0][0] =  0.;
                     fragments[k].trans[0][1] =  1.;
                     fragments[k].trans[1][0] = -1.;
                     fragments[k].trans[1][1] =  0.;
                  }
               }

             
               // Finding matching reference pixels is even tricker.  We just 
               // need the find one matching pixel coordinate on either side of
               // the gap, so if when scan across the top of the image on the
               // horizontal arm of the gap to the first non-null pixel and then
               // along the side of the matching side along the vertical arm of
               // the other image, the pixels we find should match (or at least
               // be adjacent in the image).  Using the correct left/top and
               // right/bottom edges and first/last pixel logic takes a little
               // finesse.


               sprintf(filename, "%s/%s/projected/%s", projectdir, set[j].plate, set[j].fname);

               status = 0;

               if(fits_open_file(&ffits, filename, READONLY, &status))
               {
                  printf("[struct stat=\"ERROR\", msg=\"Failed to open FITS file [%s].\"]\n", filename);
                  fflush(stdout);
                  exit(1);
               }

               if(fits_read_keys_lng(ffits, "NAXIS", 1, 2, naxis, &nfound, &status))
               {
                  printf("[struct stat=\"ERROR\", msg=\"Failed to find parameters NAXIS1, NAXIS2 in [%s].\"]\n", filename);
                  fflush(stdout);
                  exit(1);
               }

               if(fits_read_keys_dbl(ffits, "CRPIX", 1, 2, crpix, &nfound, &status))
               {
                  printf("[struct stat=\"ERROR\", msg=\"Failed to find parameters CRPIX1, CRPIX2 in [%s].\"]\n", filename);
                  fflush(stdout);
                  exit(1);
               }


               fpixel[0] = 1;
               fpixel[1] = 1;
               fpixel[2] = 1;
               fpixel[3] = 1;

               nelements = naxis[0];

               data = (double *) malloc(nelements * sizeof(double));


               // If NORTH, we scan right across the top of the image

               if(side == NORTH)
               {
                  fpixel[1] = naxis[1];

                  status = 0;

                  if(fits_read_pix(ffits, TDOUBLE, fpixel, nelements, &nan, data, &nullcnt, &status))
                  {
                        printf("XXX> First image j=%d fits[j].plus=%d NORTH %dx%d fpixel = %d %d %d %d, nelements = %d, status = %d\n",
                              j, fits[j].plus,
                           (int)naxis[0],
                           (int)naxis[1],
                           fpixel[0], fpixel[1], fpixel[2], fpixel[3], 
                           (int)nelements, status);
                     fflush(stdout);
                     printf("[struct stat=\"ERROR\", msg=\"Error reading FITS file [%s].\"]\n", filename);
                     fflush(stdout);
                     exit(1);
                  }

                  for(l=nelements-1; l>=0; --l)
                  {
                     if(!mNaN(data[l]))
                     {
                        fragments[j].xref = l + fragments[j].crpix1;
                        fragments[j].yref = naxis[1] + fragments[j].crpix2;

                        break;
                     }
                  }
               }
               
               
               // Otherwise, if SOUTH, right across the bottom.

               else
               {
                  status = 0;

                  if(fits_read_pix(ffits, TDOUBLE, fpixel, nelements, &nan, data, &nullcnt, &status))
                  {
                        printf("XXX> First image SOUTH %dx%d fpixel = %d %d %d %d, nelements = %d, status = %d\n",
                           (int)naxis[0],
                           (int)naxis[1],
                           fpixel[0], fpixel[1], fpixel[2], fpixel[3], 
                           (int)nelements, status);
                     printf("XXX> First image SOUTH fpixel = %d %d %d %d, nelements = %d, status = %d\n", fpixel[0], fpixel[1], fpixel[2], fpixel[3], (int)nelements, status);
                     fflush(stdout);
                     printf("[struct stat=\"ERROR\", msg=\"Error reading FITS file [%s].\"]\n", filename);
                     fflush(stdout);
                     exit(1);
                  }

                  for(l=nelements-1; l>=0; --l)
                  {
                     if(!mNaN(data[l]))
                     {
                        fragments[j].xref = l + fragments[j].crpix1;
                        fragments[j].yref = fragments[j].crpix2;

                        break;
                     }
                  }
               }

               free(data);

               status = 0;

               if(fits_close_file(ffits, &status))
               {
                  printf("[struct stat=\"ERROR\", msg=\"Failed to close FITS file [%s].\"]\n", filename);
                  fflush(stdout);
                  exit(1);
               }

               wcsfree(wcs);
               

               // Then for the other image

               sprintf(filename, "%s/%s/projected/%s", projectdir, set[k].plate, set[k].fname);

               status = 0;

               if(fits_open_file(&ffits, filename, READONLY, &status))
               {
                  printf("[struct stat=\"ERROR\", msg=\"Failed to open FITS file [%s].\"]\n", filename);
                  fflush(stdout);
                  exit(1);
               }

               if(fits_read_keys_lng(ffits, "NAXIS", 1, 2, naxis, &nfound, &status))
               {
                  printf("[struct stat=\"ERROR\", msg=\"Failed to find parameters NAXIS1, NAXIS2 in [%s].\"]\n", filename);
                  fflush(stdout);
                  exit(1);
               }

               if(fits_read_keys_dbl(ffits, "CRPIX", 1, 2, crpix, &nfound, &status))
               {
                  printf("[struct stat=\"ERROR\", msg=\"Failed to find parameters CRPIX1, CRPIX2 in [%s].\"]\n", filename);
                  fflush(stdout);
                  exit(1);
               }
               fpixel[0] = 1;
               fpixel[1] = 1;
               fpixel[2] = 1;
               fpixel[3] = 1;

               nelements = naxis[0];

               data = (double *) malloc(nelements * sizeof(double));


               // If NORTH, we scan up along the left of the image

               if(side == NORTH)
               {
                  for(m=0; m<naxis[1]; ++m)
                  {
                     fpixel[1] = m+1;

                     status = 0;

                     if(fits_read_pix(ffits, TDOUBLE, fpixel, nelements, &nan, data, &nullcnt, &status))
                     {
                        printf("XXX> Second image NORTH %dx%d fpixel = %d %d %d %d, nelements = %d, status = %d\n",
                           (int)naxis[0],
                           (int)naxis[1],
                           fpixel[0], fpixel[1], fpixel[2], fpixel[3], 
                           (int)nelements, status);
                        fflush(stdout);
                        printf("[struct stat=\"ERROR\", msg=\"Error reading FITS file [%s].\"]\n", filename);
                        fflush(stdout);
                        exit(1);
                     }

                     if(!mNaN(data[0]))
                     {
                        fragments[j].xrefm = naxis[0] + fragments[k].crpix1;
                        fragments[j].yrefm = m + fragments[k].crpix2;

                        break;
                     }
                  }
               }


               // Otherwise, if SOUTH, we scan up the right of the image

               else
               {
                  for(m=0; m<naxis[1]; ++m)
                  {
                     fpixel[1] = m+1;

                     status = 0;

                     if(fits_read_pix(ffits, TDOUBLE, fpixel, nelements, &nan, data, &nullcnt, &status))
                     {
                        printf("XXX> Second image SOUTH %dx%d fpixel = %d %d %d %d, nelements = %d, status = %d\n",
                           (int)naxis[0],
                           (int)naxis[1],
                           fpixel[0], fpixel[1], fpixel[2], fpixel[3], 
                           (int)nelements, status);
                        fflush(stdout);
                        printf("[struct stat=\"ERROR\", msg=\"Error reading FITS file [%s].\"]\n", filename);
                        fflush(stdout);
                        exit(1);
                     }

                     if(!mNaN(data[nelements-1]))
                     {
                        fragments[j].xrefm = fragments[k].crpix1;
                        fragments[j].yrefm = m + fragments[k].crpix2;

                        break;
                     }
                  }
               }

               fragments[k].xref = fragments[j].xrefm;
               fragments[k].yref = fragments[j].yrefm;

               fragments[k].xrefm = fragments[j].xref;
               fragments[k].yrefm = fragments[j].yref;

               free(data);

               status = 0;

               if(fits_close_file(ffits, &status))
               {
                  printf("[struct stat=\"ERROR\", msg=\"Failed to close FITS file [%s].\"]\n", filename);
                  fflush(stdout);
                  exit(1);
               }
               
               wcsfree(wcs);
            }


            // WRAP
            
            else
            {
               fragments[j].type = WRAP;

               fragments[j].xref  = fragments[j].boxx;
               fragments[j].yref  = fragments[j].boxy;
               fragments[j].xrefm = fragments[k].boxx;
               fragments[j].yrefm = fragments[k].boxy;

               fragments[j].trans[0][0] = 1.;
               fragments[j].trans[0][1] = 0.;
               fragments[j].trans[1][0] = 0.;
               fragments[j].trans[1][1] = 1.;


               fragments[k].type = WRAP;

               fragments[k].xref  = fragments[k].boxx;
               fragments[k].yref  = fragments[k].boxy;
               fragments[k].xrefm = fragments[j].boxx;
               fragments[k].yrefm = fragments[j].boxy;

               fragments[k].trans[0][0] = 1.;
               fragments[k].trans[0][1] = 0.;
               fragments[k].trans[1][0] = 0.;
               fragments[k].trans[1][1] = 1.;
            }


            // Finally, write out the pair of fits
            
            fprintf(ffit, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13d %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f %8d %12d %12d %12d %12d %7.4f %7.4f %7.4f %7.4f \n",
               fragments[j].plus,
               fragments[j].minus,
               fragments[j].a,
               fragments[j].b,
               fragments[j].c,
               fragments[j].crpix1,
               fragments[j].crpix2,
               fragments[j].xmin,
               fragments[j].xmax,
               fragments[j].ymin,
               fragments[j].ymax,
               fragments[j].xcenter,
               fragments[j].ycenter,
               fragments[j].npix,
               fragments[j].rms,
               fragments[j].boxx,
               fragments[j].boxy,
               fragments[j].boxwidth,
               fragments[j].boxheight,
               fragments[j].boxangle,
               fragments[j].type,
               fragments[j].xref,
               fragments[j].yref,
               fragments[j].xrefm,
               fragments[j].yrefm,
               fragments[j].trans[0][0],
               fragments[j].trans[0][1],
               fragments[j].trans[1][0],
               fragments[j].trans[1][1]);
            
            fprintf(ffit, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13d %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f %8d %12d %12d %12d %12d %7.4f %7.4f %7.4f %7.4f \n",
               fragments[k].plus,
               fragments[k].minus,
               fragments[k].a,
               fragments[k].b,
               fragments[k].c,
               fragments[k].crpix1,
               fragments[k].crpix2,
               fragments[k].xmin,
               fragments[k].xmax,
               fragments[k].ymin,
               fragments[k].ymax,
               fragments[k].xcenter,
               fragments[k].ycenter,
               fragments[k].npix,
               fragments[k].rms,
               fragments[k].boxx,
               fragments[k].boxy,
               fragments[k].boxwidth,
               fragments[k].boxheight,
               fragments[k].boxangle,
               fragments[k].type,
               fragments[k].xref,
               fragments[k].yref,
               fragments[k].xrefm,
               fragments[k].yrefm,
               fragments[k].trans[0][0],
               fragments[k].trans[0][1],
               fragments[k].trans[1][0],
               fragments[k].trans[1][1]);
            
            fflush(ffit);

            gettimeofday(&end, NULL);
            printf("XXX> Pair fragments (type %d): %.6f\n", fragments[j].type, 
               (end.tv_sec-start.tv_sec) + (end.tv_usec-start.tv_usec)/1000000.);
            fflush(stdout);
         }
      }

      i += nset-1;

      nset = 0;
   }

   fclose(ffit);

   printf("[struct=\"OK\", nimages=%d, nfits=%d, setcount=%d, dupcount=%d]\n",
      nimages, nfits, setcount, dupcount);
   fflush(stdout);
   exit(0);
}

int mHPXFindFragments_imCompare(const void *a, const void *b)
{
   ImgInfo *imgA = (ImgInfo *)a;
   ImgInfo *imgB = (ImgInfo *)b;

   return strcmp(imgA->fname, imgB->fname);
}
