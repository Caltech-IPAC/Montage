/* Module: mBgCombine.c
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


int mHPXBgCombine_imCompare(const void *a, const void *b);


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


int debug = 0;



/*-***********************************************************************/
/*                                                                       */
/*  mHPXBgCombine                                                        */
/*                                                                       */
/*  For large, high-resolution HiPS maps, we do the background           */
/*  difference fitting subsetted by tiles.  So before running the        */
/*  background modelling we need to combine the results of that          */
/*  processing into a single reprojected image list and a a single       */
/*  difference FITS table.                                               */
/*                                                                       */
/*  This tiled approach will have a subset of images at the tile         */
/*  boundaries split between two (sometimes more) tiles and the way we   */
/*  tie the whole sky together is by creating zero-offset differences    */
/*  between these image pieces.                                          */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int     i, j, k, ch, index, stat, imgoffset;
   int     ntoggle, toggle, count;
   int     ncols, iteration, istatus, status;
   int     maxlevel, refimage, niteration;
   double  imin, imax, jmin, jmax;
   double  A, B, C;
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

   char   *ptr;

   fitsfile *ffits;

   struct WorldCoor *wcs;

   char *header;


   getcwd(cwd, 1024);

   header = (char *)malloc(80000*sizeof(char));


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   // Command-line arguments

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXBgCombine [-d] projectdir platelist.tbl images.tbl fits.tbl\"]\n");
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
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXBgCombine [-d] projectdir platelist.tbl images.tbl fits.tbl\"]\n");
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

         imgs[nimages].cntr   = atoi(tval(icntr)) + imgoffset;
         imgs[nimages].naxis1 = atoi(tval(ins));
         imgs[nimages].naxis2 = atoi(tval(inl));
         imgs[nimages].crpix1 = atof(tval(icrpix1));
         imgs[nimages].crpix2 = atof(tval(icrpix2));

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

   fprintf(fimg, "|  cntr  | naxis1 | naxis2 |     crpix1     |     crpix2     |    plate   |  %46s  |\n", "fname");

   for(i=0; i<nimages; ++i)
   {
      fprintf(fimg, " %8d %8d %8d %16.5f %16.5f %12s %50s \n",
         imgs[i].cntr,
         imgs[i].naxis1,
         imgs[i].naxis2,
         imgs[i].crpix1,
         imgs[i].crpix2,
         imgs[i].plate,
         imgs[i].fname);

      fflush(stdout);
   }

   fflush(fimg);
   fclose(fimg);



      fprintf(ffit, "|   plus  |  minus  |         a      |        b       |        c       |    crpix1    |    crpix2    |   xmin   |   xmax   |   ymin   |   ymax   |   xcenter   |   ycenter   |    npixel   |      rms       |      boxx      |      boxy      |    boxwidth    |   boxheight    |     boxang     |single|\n");

   for(i=0; i<nfits; ++i)
   {
      fprintf(ffit, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13d %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f        \n",
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
         fits[i].boxangle);
      
      fflush(ffit);
   }


   /*******************************************************************************************/
   /* We won't close the fits file yet because we are going to add pseudo-overlap lines for   */
   /* images that were artificially split in half by tile boundaries.  We will also add lines */
   /* for images that appear in both the lower left and the upper right of the all-sky HPX    */
   /* map, thus closing the -180/+180 wrap-around.                                            */
   /*******************************************************************************************/

   // To identify the split/duplicated images, we first sort the imgs structure array on the
   // file name column.
   
   qsort(imgs, nimages, sizeof(ImgInfo), mHPXBgCombine_imCompare);


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

      printf("\n");

      if(nset >= 3)
         printf("BIG SET\n\n");


      // For each member of a set, we are going to need the "fitplane" parameters for
      // that fragment.  These will be used to create new "overlap" entries in the 
      // "fits" array just as the normal entries are made by defining a region in the
      // middle of the fragment and treating that as if it were the overlap region from
      // an image difference (and doing the same thing with all the corresponding
      // fragments).  These patches will of course be set to have no difference
      // (a,b,c all zero) since the really come from the same image.  In mBgModel, we
      // will have to make sure we only apply the 'c' (constant offset) parameter to
      // updating these specific differences because the locations can be quite different
      // and including slopes would be quite messy.
      //
      for(j=0; j<nset; ++j)
      {
         printf("XXX> %5d %2d: %7d [%s][%s]\n", setcount, j, set[j].cntr, set[j].plate, set[j].fname);

         sprintf(filename, "%s/%s/projected/%s", projectdir, set[j].plate, set[j].fname);

         status = 0;

         if(fits_open_file(&ffits, filename, READONLY, &status))
         {
            printf("[struct stat=\"ERROR\", msg=\"Failed open FITS file [%s].\"]\n", filename);
            fflush(stdout);
            exit(1);
         }

         if(fits_get_image_wcs_keys(ffits, &header, &status))
         {
            fits_close_file(ffits, &status);

            printf("[struct stat=\"ERROR\", msg=\"Can't find WCS in FITS file [%s].\"]\n", filename);
            fflush(stdout);
            exit(1);
         }

         if(fits_close_file(ffits, &status))
         {
            printf("[struct stat=\"ERROR\", msg=\"Failed closing FITS file [%s].\"]\n", filename);
            fflush(stdout);
            exit(1);
         }

         wcs = wcsinit(header);

         free(header);

         crpix1 = wcs->crpix[0];
         crpix2 = wcs->crpix[1];

         naxis1 = (int)(wcs->nxpix + 0.5);
         naxis2 = (int)(wcs->nxpix + 0.5);

         delta1 = naxis1 / 8.;
         delta2 = naxis2 / 8.;

         xcenter = -crpix1 + naxis1 / 2.;
         ycenter = -crpix2 + naxis2 / 2.;

         fragments[j].plus      = -1;
         fragments[j].minus     = set[j].cntr;

         fragments[j].a         = 0.;
         fragments[j].b         = 0.;
         fragments[j].c         = 0.;

         fragments[j].crpix1    = -(xcenter - delta1);
         fragments[j].crpix2    = -(ycenter - delta2);

         fragments[j].xcenter   = xcenter;
         fragments[j].ycenter   = xcenter;

         fragments[j].xmin      = xcenter - delta1;
         fragments[j].xmax      = xcenter + delta1;
         fragments[j].ymin      = ycenter - delta2;
         fragments[j].ymax      = ycenter + delta2;

         fragments[j].npix      = naxis1 * naxis2;
         fragments[j].rms       = 0.;

         fragments[j].boxx      = xcenter;
         fragments[j].boxy      = ycenter;
         fragments[j].boxwidth  = (double)naxis1;
         fragments[j].boxheight = (double)naxis2;
         fragments[j].boxangle  = 0.;

         wcsfree(wcs);
      }

      // For each set pair [nset*(nset-1) of them] we create a 'fits' entry 
      // where the 'minus' ID is for the value in the fragment and the 'plus'
      // ID is from the other other pair element.
      //
      for(j=0; j<nset; ++j)
      {
         for(k=j+1; k<nset; ++k)
         {
            fprintf(ffit, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13d %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f  true  \n",
               set[k].cntr,
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
               fragments[j].boxangle);
            
            fprintf(ffit, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13d %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f  true  \n",
               set[j].cntr,
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
               fragments[k].boxangle);
            
            fflush(ffit);
         }
      }

      i += nset-1;

      nset = 0;
   }

   fflush(ffit);
   fclose(ffit);

   printf("[struct=\"OK\", nimages=%d, nfits=%d, setcount=%d, dupcount=%d]\n",
      nimages, nfits, setcount, dupcount);
   fflush(stdout);
   exit(0);
}

int mHPXBgCombine_imCompare(const void *a, const void *b)
{
   ImgInfo *imgA = (ImgInfo *)a;
   ImgInfo *imgB = (ImgInfo *)b;

   return strcmp(imgA->fname, imgB->fname);
}
