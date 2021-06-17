/* Module: mHiPSTiles.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        03Jul19  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fitsio.h"
#include "wcs.h"
#include "coord.h"

#define MAXHIPS 256


void mHiPSTiles_printFitsError(int status);

int  mHiPSTiles_HiPSID(int pixlev, double x, double y);

void mHiPSTiles_splitIndex(long index, int level, int *x, int *y);

void mHiPSTiles_HiPSHdr(int level, int tile, fitsfile *fitshdr);


struct WorldCoor *wcs;

int debug;


//  base tile:   0  1  2  3  4  5  6  7  8  9 10 11 12->6

int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3, 4};
int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2, 4};


/*************************************************************************/
/*                                                                       */
/*  mHiPSTiles                                                           */
/*                                                                       */
/*  This program splits an HPX plate up into HiPS tiles. It is specific  */
/*  to the process of making HiPS sets and depends on the plates having  */
/*  been constructed correctly.                                          */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   char      plateFile [1024];
   char      hipsDir   [1024];
   char      ctype1     [256];
   char      ctype2     [256];
   char      path      [1024];

   char      **hipsFile;

   fitsfile *plate, **hips;

   int       i, j, k, l, m, it, offscl, istat, nhips, ntot, maxhips;
   int       hpxPix, hpxLevel, tileLevel, fullscale, nullcnt;

   int       ibegin, iend;
   int       jbegin, jend;

   int       iTileBegin, iTileEnd;
   int       jTileBegin, jTileEnd;

   int       padLeft, padRight;
   int       padTop,  padBottom;

   int       jrow, icenter, jcenter, tileID, subsetDir;

   int       bitpix = DOUBLE_IMG;
   long      naxis = 2;
   long      naxes[2];

   char     *header;

   double    naxis1;
   double    naxis2;

   double    crval1;
   double    crval2;

   double    crpix1;
   double    crpix2;

   double    cdelt1;
   double    cdelt2;

   long      fpixel[4], fpixelo[4], nelements, nelementso;

   double    outdata[512];
   double   *indata;

   struct stat type;

   FILE *fhdr;

   union
   {
      double d;
      char   c[8];
   }
   value;

   double dnan;


   int status = 0;

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;


   maxhips = MAXHIPS;

   hips = (fitsfile **)malloc(maxhips * sizeof(fitsfile *));


   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSTiles [-d level] plate.fits hipsDir\"]\n");
      exit(1);
   }

   for(i=0; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
         debug = atoi(argv[i+1]);
   }
      
   if(debug)
   {
      argv += 2;
      argc -= 2;
   }

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSTiles [-d level] plate.fits hipsDir\"]\n");
      exit(1);
   }

   strcpy(plateFile, argv[1]);
   strcpy(hipsDir,   argv[2]);

   if(hipsDir[strlen(hipsDir)-1] != '/')
      strcat(hipsDir, "/");

   if(debug)
   {
      printf("DEBUG> (main)> plateFile: [%s]\n", plateFile);
      fflush(stdout);

      printf("DEBUG> (main)> hipsDir  : [%s]\n", hipsDir);
      fflush(stdout);
   }


   /*************************************************/
   /* Make a NaN value to use checking blank pixels */
   /*************************************************/

   for(i=0; i<8; ++i)
      value.c[i] = 255;

   dnan = value.d;


   /*****************************************/
   /* Check to see if HiPS directory exists */
   /*****************************************/

   istat = stat(hipsDir, &type);

   if(istat < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot access %s\"]\n", hipsDir);
      exit(1);
   }

   if (S_ISDIR(type.st_mode) != 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s is not a directory.\"]\n", hipsDir);
      exit(1);
   }


   /****************************************/
   /* Open the (unsubsetted) original file */
   /* to get the WCS info                  */
   /****************************************/


   status = 0;
   if(fits_open_file(&plate, plateFile, READONLY, &status))
      mHiPSTiles_printFitsError(status);

   status = 0;
   if(fits_get_image_wcs_keys(plate, &header, &status))
      mHiPSTiles_printFitsError(status);

   wcs = wcsinit(header);

   strcpy(ctype1, wcs->ctype[0]);
   strcpy(ctype2, wcs->ctype[1]);

   naxis1 = wcs->nxpix;
   naxis2 = wcs->nypix;

   crval1 = wcs->xref;
   crval2 = wcs->yref;

   crpix1 = wcs->xrefpix;
   crpix2 = wcs->yrefpix;

   cdelt1 = wcs->xinc;
   cdelt2 = wcs->yinc;

   if(debug)
   {
      printf("\nDEBUG> (main)> Input HiPS plate:\n\n");

      printf("DEBUG> (main)> ctype1     = [%s]\n", ctype1);
      printf("DEBUG> (main)> ctype2     = [%s]\n", ctype2);
      printf("DEBUG> (main)> naxis1     =  %-g\n", naxis1);
      printf("DEBUG> (main)> naxis2     =  %-g\n", naxis2);
      printf("DEBUG> (main)> crval1     =  %-g\n", crval1);
      printf("DEBUG> (main)> crval2     =  %-g\n", crval2);
      printf("DEBUG> (main)> crpix1     =  %.1f\n", crpix1);
      printf("DEBUG> (main)> crpix2     =  %.1f\n", crpix2);
      printf("DEBUG> (main)> cdelt1     =  %-g\n", cdelt1);
      printf("DEBUG> (main)> cdelt2     =  %-g\n", cdelt2);
      fflush(stdout);
   }

   nelements = naxis1;

   indata = (double *)malloc(naxis1 * sizeof(double));
  

   /*******************************************/
   /* Determine the order from the pixel size */
   /*******************************************/

   if(fabs((fabs(cdelt1) - fabs(cdelt2))/fabs(cdelt1)) > 1.e-6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Pixel scales don't match.\"]\n");
      exit(1);
   }

   hpxPix = 90.0 / fabs(cdelt1) / sqrt(2.0) + 0.5;
 
   hpxLevel = log10((double)hpxPix)/log10(2.) + 0.5;

   tileLevel = hpxLevel - 9;
 
   hpxPix = pow(2., (double)hpxLevel) + 0.5;
     
   fullscale = 5 * hpxPix;

   if(debug)
   {
      printf("\nDEBUG> (main)> hpxLevel  =  %d\n", hpxLevel);
      printf("\nDEBUG> (main)> tileLevel =  %d\n", tileLevel);
      printf("DEBUG> (main)> fullscale =  %d\n", fullscale);
      fflush(stdout);
   }


   /**************************************************/
   /* Find the range of pixels that can be used for  */
   /* HiPS tiles.  Since we usually pad the "plates" */
   /* by 256 pixels, we would expect to start that   */
   /* many pixels in from the bottom and left side.  */
   /*                                                */
   /* However, some for various reasons, some plates */
   /* might not have padding, so we have to watch    */
   /* out for that.                                  */
   /*                                                */
   /**************************************************/

   ibegin = fullscale/2. - (crpix1 - 0.5) + 0.5;
   jbegin = fullscale/2. - (crpix2 - 0.5) + 0.5;

   // Note: The first 0.5 is the offset of FITS coords (index 1) to
   // pixel coords (index 0); the beginning of the image is FITS coord
   // 0.5 so that the center of the first pixel will be at coordinate 1.
   // CRPIX is relative to this location. 

   // The second 0.5 is for round-off.  Without it, the calculation 
   // should be exactly an integer but with roundoff we run the risk
   // of being just a little off and having it round down.  This way
   // it rounds to the right value.

   iend = ibegin + naxis1;
   jend = jbegin + naxis2;

   // This coordinate is off the image, so we will run from ibegin to
   // less than iend.

   if(debug)
   {
      printf("\nDEBUG> (main)> ibegin    =  %d\n", ibegin);
      printf("DEBUG> (main)> iend      =  %d\n", iend);
      printf("\nDEBUG> (main)> jbegin    =  %d\n", jbegin);
      printf("DEBUG> (main)> jend      =  %d\n", jend);
      fflush(stdout);
   }

   if((ibegin%512) == 0)
      iTileBegin = ibegin / 512;
   else
      iTileBegin = ibegin / 512. + 1;

   if((jbegin%512) == 0)
      jTileBegin = jbegin / 512;
   else
      jTileBegin = jbegin / 512. + 1;

   iTileEnd = iend / 512.;
   jTileEnd = jend / 512.;

   padLeft   = iTileBegin * 512 - ibegin;
   padBottom = jTileBegin * 512 - jbegin;

   padRight  = iend - iTileEnd * 512;
   padTop    = jend - jTileEnd * 512;

   if(debug)
   {
      printf("\nDEBUG> (main)> iTileBegin    =  %d\n", iTileBegin);
      printf("DEBUG> (main)> iTileEnd      =  %d\n", iTileEnd);
      printf("\nDEBUG> (main)> jTileBegin    =  %d\n", jTileBegin);
      printf("DEBUG> (main)> jTileEnd      =  %d\n", jTileEnd);

      printf("\n");
      printf("DEBUG> (main)> padLeft   = %d\n", padLeft);
      printf("DEBUG> (main)> padBottom = %d\n", padBottom);
      printf("DEBUG> (main)> padRight  = %d\n", padRight);
      printf("DEBUG> (main)> padTop    = %d\n", padTop);
      fflush(stdout);
   }


   /************************/
   /* Copy the data subset */
   /************************/

   jrow = 0;

   fpixel[1] = padBottom+1;

   fpixelo[0] = 1;
   fpixelo[1] = 1;

   ntot = 0;

   if(debug > 1)
   {
      printf("\n");
      fflush(stdout);
   }

   for(j=jbegin+padBottom; j<jend-padTop; ++j)
   {
      if(jrow == 0)
      {
         jcenter = j + 256;
         
         nhips = 0;

         it = 0;

         if(debug)
         {
            printf("\n");
            fflush(stdout);
         }

         for(icenter=ibegin+padLeft+256; icenter<iend-padRight+256; icenter+=512)
         {
            tileID = mHiPSTiles_HiPSID(hpxLevel, (double)icenter, (double)jcenter);

            if(tileID < 0)
               continue;

            subsetDir = tileID / 10000 * 10000;


            // Make the HiPS "order" directory

            sprintf(path, "%sNorder%d", hipsDir, tileLevel);

            istat = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

            if(istat < 0 && errno != EEXIST)
            {
               printf("[struct stat=\"ERROR\", msg=\"Problem creating output HiPS directory %s\"]\n", path);
               fflush(stdout);
               exit(0);
            }


            // And the HiPS "subset" directory for this ID

            sprintf(path, "%sNorder%d/Dir%d", hipsDir, tileLevel, subsetDir);

            istat = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

            if(istat < 0 && errno != EEXIST)
            {
               printf("[struct stat=\"ERROR\", msg=\"Problem creating output HiPS subset directory %s\"]\n", path);
               fflush(stdout);
               exit(0);
            }


            // Finally, the HiPS FITS file

            sprintf(path, "%sNorder%d/Dir%d/Npix%d.fits", hipsDir, tileLevel, subsetDir, tileID);

            unlink(path);

            status = 0;
            if(fits_create_file(&hips[nhips], path, &status))
               mHiPSTiles_printFitsError(status);

            naxes[0] = 512;
            naxes[1] = 512;

            status = 0;
            if (fits_create_img(hips[nhips], bitpix, naxis, naxes, &status))
               mHiPSTiles_printFitsError(status);

            
            // Set the header keywords

            mHiPSTiles_HiPSHdr(hpxLevel, tileID, hips[nhips]);


            ++nhips;

            if(nhips > maxhips)
            {
               maxhips += MAXHIPS;

               hips = (fitsfile **)realloc(hips, maxhips * sizeof(fitsfile *));
            }

            if(debug)
            {
               printf("DEBUG> (main)> %3d: icenter=%d, jcenter=%d  -->  %s (nhips=%d)\n", it, icenter, jcenter, path, nhips-1);
               fflush(stdout);
            }

            ++it;
         }
      }

      // Copy data

      if(debug > 1)
      {
         printf("READ>  fpixel  = %6ld %6ld, nelements  = %6ld\n", fpixel[0], fpixel[1], nelements);
         fflush(stdout);
      }

      status = 0;
      if(fits_read_pix(plate, TDOUBLE, fpixel, nelements, &dnan, indata, &nullcnt, &status))
         mHiPSTiles_printFitsError(status);

      nelementso = 512;

      for(l=0; l<nhips; ++l)
      {
         for(m=0; m<512; ++m)
            outdata[m] = indata[padLeft + 512*l + m];

         if(debug > 1)
         {
            printf("WRITE> fpixelo = %6ld %6ld, nelementso = %6ld\n", fpixelo[0], fpixelo[1], nelementso);
            fflush(stdout);
         }

         status = 0;
         if (fits_write_pix(hips[l], TDOUBLE, fpixelo, nelementso,
                            (void *)(outdata), &status))
            mHiPSTiles_printFitsError(status);
      }

      ++fpixelo[1];
      ++fpixel [1];

      ++jrow;

      if(jrow == 512)
      {
         jrow = 0;

         fpixelo[1] = 1;

         for(k=0; k<nhips; ++k)
         {
            status = 0;
            if(fits_close_file(hips[k], &status))
               mHiPSTiles_printFitsError(status);

            ++ntot;
         }
      }
   }
   

   /*******************/
   /* Close the files */
   /*******************/

   status = 0;
   if(fits_close_file(plate, &status))
      mHiPSTiles_printFitsError(status);

   printf("[struct stat=\"OK\", module=\"mHiPSTiles\", ntile=%d]\n", ntot);
   fflush(stdout);
   exit(0);
}



/********************************************************************/
/*                                                                  */
/*  mHiPSTiles_HiPSID                                               */
/*                                                                  */
/*  Given HPX pixel coordinates (and level), determine the tile     */
/*  ID, possibly at a different level.                              */
/*                                                                  */
/********************************************************************/

int mHiPSTiles_HiPSID(int pixlev, double x, double y)
{
   long long baseTile;
   long long nside, nsideout, index, id;
   long long xi, yi, ibase, jbase, base;

   double xintile, yintile;

   int i, ix, iy;

   // Command-line arguments


   // Size of a base tile at the pixel level

   nside    = (long long) pow(2., (double)pixlev);
   nsideout = (long long) pow(2., (double)(pixlev-9));

   if(debug > 1)
   {
      printf("\nDEBUG> (HiPSID)> nside     = %lld\n", nside);
      printf("DEBUG> (HiPSID)> nsideout  = %lld\n\n", nsideout);
      printf("DEBUG> (HiPSID)> Pixel coord:   X = %9.1f, Y = %9.1f (level %ld)\n", x, y, pixlev); 
      fflush(stdout);
   }


   // Next we need to determine the base tile.
   // We start by dividing the coordinates by the face size.
   // Then we can just look it up using the two offset
   // lists. 

   ibase = x / nside;
   jbase = y / nside;

   if(debug > 1)
   {
      printf("\nDEBUG> (HiPSID)> ibase     = %lld\n", ibase);
      printf("DEBUG> (HiPSID)> jbase     = %lld\n", jbase);
      fflush(stdout);
   }

   baseTile = -1;

   for(i=0; i<13; ++i)
   {
      if(ibase == xoffset[i])
      {
         if(jbase == yoffset[i])
         {
            baseTile = i;
            break;
         }
      }
   }

   if(baseTile == 12)
      baseTile = 6;

   if(debug > 1)
   {
      printf("DEBUG> (HiPSID)> Base tile = %d\n\n", baseTile);
      fflush(stdout);
   }

   if(baseTile < 0)
      return -1;
 

   // We need the index offset within the base tile

   xintile = x - ibase * nside;
   yintile = y - jbase * nside;

   if(debug > 1)
   {
      printf("DEBUG> (HiPSID)> Relative tile: X = %9.5f, Y = %9.5f (level %ld)\n", xintile, yintile, pixlev); 
      fflush(stdout);
   }


   // But make that in output tile units

   xintile = xintile * nsideout / nside;
   yintile = yintile * nsideout / nside;

   if(debug > 1)
   {
      printf("DEBUG> (HiPSID)> Relative tile: X = %9.5f, Y = %9.5f (level %ld)\n", xintile, yintile, pixlev-9); 
      fflush(stdout);
   }


   // The starting point for the index is the 
   // lowest index for the base tile

   index = baseTile * nsideout * nsideout;

   if(debug > 1)
   {
      printf("\nDEBUG> (HiPSID)> Tile starting index = %lld (level %ld)\n\n", index, pixlev-9);
      fflush(stdout);
   }


   // The Z-ordering scheme used to arrange the 
   // smaller tiles inside the base tile relates the
   // coordinates inside the tile to the index offset
   // by and interleaving of bits.

   // X is measured from the right, so we need to
   // complement it.

   xintile = nsideout - xintile;

   if(debug > 1)
   {
      printf("DEBUG> (HiPSID)> Relative tile: X = %9.5f, Y = %9.5f (of %d)\n", xintile, yintile, nsideout);
      fflush(stdout);
   }

   id = 0;

   xi = xintile;
   yi = yintile;

   if(debug > 1)
   {
      printf("DEBUG> (HiPSID)> Integer tile:  X = %9d, Y = %9d (of %d)\n", xi, yi, nsideout);
      fflush(stdout);
   }

   for(i=63; i>=0; --i)
   {
      iy = (yi >> i) & 1;
      ix = (xi >> i) & 1;

      id = 4*id + 2*iy + ix;
   }

   if(debug > 1)
   {
      printf("\nDEBUG> (HiPSID)> Tile index   = %ld (i.e., index inside base tile)\n", id);
      printf("DEBUG> (HiPSID)> Tile index   = %o (octal)\n\n", id);
      fflush(stdout);
   }

   index += id;
      
   return(index);
} 



/*************************************************************************/
/*                                                                       */
/*  mHiPSTiles_HiPSHdr                                                   */
/*                                                                       */
/*  Given a HiPS tile level and tile ID within that level, create a      */
/*  proper HPX FITS header, to be used for cutouts or mosaicking.        */
/*                                                                       */
/*************************************************************************/

void mHiPSTiles_HiPSHdr(int pixlev, int tile, fitsfile *fitshdr)
{
   long level, baseTile;
   long nside, nsidePix, fullscale, index;
   int  i, x, y, status;

   double cdelt, cos45;

   double crpix1, crpix2;
   double pc1_1, pc1_2, pc2_1, pc2_2;
   double cdelt1, cdelt2;
   double crval1, crval2;
   int    pv2_1, pv2_2;

   char   ctype1[16], ctype2[16];


   // Command-line arguments

   level = pixlev - 9;

   if(debug > 1)
   {
      printf("\nDEBUG> (HiPSHdr)> pixel level  = %ld (input)\n", pixlev);
      printf("\nDEBUG> (HiPSHdr)> tile level   = %ld\n", level);
      printf("DEBUG> (HiPSHdr)> tile         = %ld (input)\n", tile);
      fflush(stdout);
   }


   // Size of a base tile, in both output tile units
   // and output pixel units

   nside    = pow(2., (double)level);
   nsidePix = pow(2., (double)pixlev);

   if(debug > 1)
   {
      printf("\nDEBUG> (HiPSHdr)> nside(tile)  = %ld\n", nside);
      printf("DEBUG> (HiPSHdr)> nside(pixel) = %ld\n", nsidePix);
      fflush(stdout);
   }


   // The simplest way to identify the base tile
   // or output tile is in is by seeing how many
   // nside*nside sets it contains.  Since all our
   // numbering starts at zero, this simple division
   // will show us what base tile we are in.

   baseTile = (int)tile/(nside*nside);

   if(debug > 1)
   {
      printf("\nDEBUG> (HiPSHdr)> Base tile    = %d (offset %d %d)\n", 
         baseTile, xoffset[baseTile], yoffset[baseTile]);
      fflush(stdout);
   }


   // Subtracting the integer number of preceding
   // base tiles shows us the tile index we are at
   // inside the current base tile.

   index = tile - baseTile * nside * nside;

   if(debug > 1)
   {
      printf("DEBUG> (HiPSHdr)> Tile index   = %ld (i.e., index inside base tile)\n", index);
      fflush(stdout);
   }


   // The Z-ordering scheme used to arrange the 
   // smaller tiles inside the base tile provides
   // a way to turn a relative index into X and Y
   // tile offsets.  We have a function that can
   // do this.

   // X is measured from the right, so we need to
   // complement it.

   mHiPSTiles_splitIndex(index, level, &x, &y);

   x = nside - 1 - x;

   if(debug > 1)
   {
      printf("\nDEBUG> (HiPSHdr)> Relative tile: X = %7d, Y = %7d (of %d) [X was originally %d]\n", x, y, nside, nside - 1 - x);
      fflush(stdout);
   }


   // Now add in the base tile offsets

   x = x + xoffset[baseTile]*nside;
   y = y + yoffset[baseTile]*nside;

   if(debug > 1)
   {
      printf("DEBUG> (HiPSHdr)> Absolute:      X = %7d, Y = %7d (level %ld)\n", x, y, level);
      fflush(stdout);
   }


   // And scale to pixels

   x = x * nsidePix/nside;
   y = y * nsidePix/nside; 

   if(debug > 1)
   {
      printf("DEBUG> (HiPSHdr)> Pixels:        X = %7d, Y = %7d (level %ld)\n", x, y, pixlev);
      fflush(stdout);
   }


   // The whole region is a 5x5 grid of nsidePix boxes, so

   fullscale = 5 * nsidePix;


   if(debug > 1)
   {
      printf("\nDEBUG> (HiPSHdr)> Full scale       = %ld pixels\n", fullscale);
      printf("\nDEBUG> (HiPSHdr)> Scaled coords  X = %.2f%%, Y = %.2f%%\n", 100.*x/fullscale, 100.*y/fullscale);
      printf("\nDEBUG> (HiPSHdr)> CRPIX            = %.1f, Y = %.1f\n\n", (fullscale + 1.)/2. - x, (fullscale + 1.)/2. - y);
      fflush(stdout);
   }

   cos45 = sqrt(2.0)/2.0;

   cdelt = -90.0 / nsidePix / sqrt(2.0);

   crpix1 = (fullscale + 1.)/2. - x;
   crpix2 = (fullscale + 1.)/2. - y;

   pc1_1 = cos45;
   pc1_2 = cos45;
   pc2_1 = cos45;
   pc2_2 = cos45;

   cdelt1 =  cdelt;
   cdelt2 = -cdelt;

   strcpy(ctype1, "GLON-HPX");
   strcpy(ctype2, "GLAT-HPX");

   crval1 = 0.0;
   crval2 = 0.0;

   pv2_1 = 4;
   pv2_2 = 3;

   status = 0;
   if(fits_write_key_dbl(fitshdr, "CRPIX1", crpix1, -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_dbl(fitshdr, "CRPIX2", crpix2, -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_dbl(fitshdr, "PC1_1",  pc1_1,  -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_dbl(fitshdr, "PC1_2",  pc1_2,  -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_dbl(fitshdr, "PC2_1",  pc2_1,  -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_dbl(fitshdr, "PC2_2",  pc2_2,  -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_dbl(fitshdr, "CDELT1", cdelt1, -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_dbl(fitshdr, "CDELT2", cdelt2, -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_str(fitshdr, "CTYPE1", ctype1,      (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_str(fitshdr, "CTYPE2", ctype2,      (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_dbl(fitshdr, "CRVAL1", crval1, -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_dbl(fitshdr, "CRVAL2", crval2, -14, (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_lng(fitshdr, "PV2_1",  pv2_1,       (char *)NULL, &status)) mHiPSTiles_printFitsError(status);
   if(fits_write_key_lng(fitshdr, "PV2_2",  pv2_2,       (char *)NULL, &status)) mHiPSTiles_printFitsError(status);

   return;
}


/***************************************************/
/*                                                 */
/* mHiPSTiles_splitIndex()                         */
/*                                                 */
/* Cell indices are Z-order binary constructs.     */
/* The x and y pixel offsets are constructed by    */
/* extracting the pattern made by every other bit  */
/* to make new binary numbers.                     */
/*                                                 */
/***************************************************/

void mHiPSTiles_splitIndex(long index, int level, int *x, int *y)
{
   int i;
   long val;

   val = index;

   *x = 0;
   *y = 0;

   for(i=0; i<level; ++i)
   {
      *x = *x + (((val >> (2*i))   & 1) << i);
      *y = *y + (((val >> (2*i+1)) & 1) << i);
   }

   return;
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mHiPSTiles_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   printf("[struct stat=\"ERROR\", msg=\"CFITSIO: %s\"]\n", status_str);
   exit(1);
}
