#include <stdio.h>
#include <stdlib.h>
#include <fitshead.h>
#include <fitsio.h>
#include <wcs.h>
#include <montage.h>


#define STRLEN 1024

#define TOP        0
#define BOTTOM     1

#define HORIZONTAL 0
#define VERTICAL   1

struct Plane
{
   double A;
   double B;
   double C;
};

void compare(char *plus, char *minus, int side);

struct Plane *transform(int dir,
                        double A, double B, double C, 
                        double x,  double y,
                        double xp, double yp);

int debug = 1;


/*********************************************************************/
/*                                                                   */
/* Typical Montage mosaics cover a small portion of the sky and      */
/* background matching between images can be done just by looking    */
/* at the image-to-image overlaps.  With an all-sky projection like  */
/* HPX (HEALPix), we have the added issue of wrapping the matching   */
/* around the sphere (usually at the -180/180 split at the left and  */
/* right edges of the projection and near the poles). In the case of */
/* HPX, which splits the sky open at four specific longitudes from   */
/* the North pole down to about 41.8 latitude (and similarly for the */
/* South), we need to match backgrounds across the resultant gaps.   */
/*                                                                   */
/* Since our HiPS processing subdivides the sky into a regular array */
/* of "tiles", this reduces down to determining the offsets between  */
/* a predictable set of pairs of tiles and adding this information   */
/* to the set of difference "fits" fed into the existing mBgModel    */
/* routine (along with the more normal image differences from above. */
/*                                                                   */
/* Since these specific tile pairs don't overlay in pixel space,     */
/* we need to handle this slightly differently from normal matching  */
/* (which relies on differencing the image overlaps).  Instead, we   */
/* fit a plane to each of the tile backgrounds, transform that plane */
/* (rotate 90 degrees and offset in pixel space) and compare it to   */
/* a plane fit to the other.  This last transform needs to be        */
/* included as a special case for these pairs in the background      */
/* modeling as well.                                                 */
/*                                                                   */
/*********************************************************************/

int main(int argc, char **argv)
{
    int fx, fy, Fx, Fy;
    int x, y, X, Y;
    int ntile;

    char plus [STRLEN];
    char minus[STRLEN];

    ntile = atoi(argv[1]);

    ntile = ntile/5;


   // In the following, fx is the horizontal "face" index (from 0 to 4)
   // for the "plus" tile image we are comparing and fy the vertical 
   // face index.  Fx and Fy are for the "minus" image.  In all this, we
   // have chosen to step through horizontal strips of tiles and compare 
   // them to the corresponding vertical strip on the other side of a gap.

   // x and y are plus tile image offsets within a face (typically, ntile=16
   // of them at order 9).  X and Y are the corresponding minus offsets.


    // North pole
    
    for(fx=0; fx<=3; ++fx)
    {
       fy = fx + 1;
       Fx = (fx+1)%4;
       Fy = Fx + 1;

       y = ntile - 1;
       X = 0;
       Y = ntile - 1;

       for(x=0; x<ntile; ++x)
       {
          sprintf(plus, "tile_%02d_%02d.fits", 
             ntile*fx+x, ntile*fy+y);

          sprintf(minus, "tile_%02d_%02d.fits", 
             ntile*Fx+X, ntile*Fy+Y);

          compare(plus, minus, TOP);

          --Y;
       }
    }


    // South pole
    
    for(Fy=0; Fy<=3; ++Fy)
    {
       Fx = Fy + 1;
       fy = (Fy+1)%4;
       fx = fy + 1;

       y = 0;
       X = ntile - 1;
       Y = ntile - 1;

       for(x=0; x<ntile; ++x)
       {
          sprintf(plus, "tile_%02d_%02d.fits", 
             ntile*fx+x, ntile*fy+y);

          sprintf(minus, "tile_%02d_%02d.fits", 
             ntile*Fx+X, ntile*Fy+Y);

          compare(plus, minus, BOTTOM);
          
          --Y;
       }
    }
}



// In the main program we loop over all the possible pairs of images that
// need this "special" HPX background comparison (i.e. to stitch up the
// splits near the poles induced by the HPX projection scheme).  The third
// parameter indicates the pole involved or equivalently whether we want
// to compare the top of the plus image to the left side of the minus image
// or the bottom of the plus image to the right side of the minus image.

void compare(char *plus, char *minus, int side)
{
   struct mFitplaneReturn *fitplane;
   struct Plane *trans;
   
   double Aplus,  Bplus,  Cplus;
   double Aminus, Bminus, Cminus;
   double Atrans, Btrans, Ctrans;
   double Adiff,  Bdiff,  Cdiff;
   double valplus, valminus;

   double xplus,  yplus;
   double xminus, yminus;

   fitsfile *fptr;

   char *header;

   struct WorldCoor *wcs;

   int status;

   char outfile[STRLEN];


   if(debug)
   {
      printf("DEBUG> compare [%s] to [%s]\n", plus, minus);
      fflush(stdout);
   }


   // Get the image center coordinates from the FIT images
   
   // "plus" image
   
   status = 0;

   if(fits_open_file(&fptr, plus, READONLY,&status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open FITS file: [%s]\"]\n", plus);
      fflush(stdout);
      exit(0);
   }

   status = 0;

   if(fits_get_image_wcs_keys(fptr, &header, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"No WCS in FITS file: [%s]\"]\n", plus);
      fflush(stdout);
      exit(0);
   }

   wcs = wcsinit(header);

   free(header);

   if(wcs == (struct WorldCoor *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Bad WCS in FITS file: [%s]\"]\n", plus);
      fflush(stdout);
      exit(0);
   }

   if(side == TOP)
   {
      xplus = -wcs->xrefpix;
      yplus = -wcs->yrefpix + wcs->nypix;
   }
   else
   {
      xplus = -wcs->xrefpix;
      yplus = -wcs->yrefpix;
   }

   free(wcs);
   

   // "minus" image
   
   status = 0;

   if(fits_open_file(&fptr, minus, READONLY,&status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open FITS file: [%s]\"]\n", minus);
      fflush(stdout);
      exit(0);
   }

   status = 0;

   if(fits_get_image_wcs_keys(fptr, &header, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"No WCS in FITS file: [%s]\"]\n", minus);
      fflush(stdout);
      exit(0);
   }

   wcs = wcsinit(header);

   free(header);

   if(wcs == (struct WorldCoor *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Bad WCS in FITS file: [%s]\"]\n", minus);
      fflush(stdout);
      exit(0);
   }

   if(side == TOP)
   {
      xminus = -wcs->xrefpix;
      yminus = -wcs->yrefpix + wcs->nypix;
   }
   else
   {
      xminus = -wcs->xrefpix + wcs->nxpix;
      yminus = -wcs->yrefpix + wcs->nypix;
   }

   free(wcs);

   
   // For the normal Montage differencing, we subtract the minus image
   // from the plus image.  Since these images don't overlap, we can't
   // do that here.  So instead, we fit planes to the two regions (removing
   // "sources"), transform the plane equation for the minus image to the
   // XY space of the plus, and subtract the two planes for our "difference".
   
   fitplane = mFitplane(plus, 0, 0, 0);

   Aplus = fitplane->a;
   Bplus = fitplane->b;
   Cplus = fitplane->c;

   if(debug)
   {
      printf("DEBUG> Fit plane 'plus':       plane: [%9.2e %9.2e %9.2e] for 'plus' at  (%10.1f,%10.1f)\n",
         Aplus, Bplus, Cplus, xplus, yplus);
      fflush(stdout);
   }

   trans = transform(HORIZONTAL, Aplus, Bplus, Cplus, xplus, yplus, xminus, yminus);

   Atrans = trans->A;
   Btrans = trans->B;
   Ctrans = trans->C;
   
   if(debug)
   {
      printf("DEBUG> Transformed 'plus' plane:      [%9.2e %9.2e %9.2e] at 'minus'     (%10.1f,%10.1f)\n",
         Atrans, Btrans, Ctrans, xminus, yminus);
      fflush(stdout);
   }

   fitplane = mFitplane(minus, 0, 0, 0);

   Aminus = fitplane->a;
   Bminus = fitplane->b;
   Cminus = fitplane->c;

   if(debug)
   {
      printf("DEBUG> Fit plane 'minus':             [%9.2e %9.2e %9.2e]\n",
         Aminus, Bminus, Cminus);
      fflush(stdout);
   }

   Adiff = Atrans - Aminus;
   Bdiff = Btrans - Bminus;
   Cdiff = Ctrans - Cminus;

   if(debug)
   {
      printf("DEBUG> The difference is:             [%9.2e %9.2e %9.2e]\n",
         Adiff, Bdiff, Cdiff);
      fflush(stdout);
   }

   Aminus += Adiff/2.;
   Bminus += Bdiff/2.;
   Cminus += Cdiff/2.;

   if(debug)
   {
      printf("DEBUG> Adding half to 'minus':        [%9.2e %9.2e %9.2e]\n",
         Aminus, Bminus, Cminus);
      fflush(stdout);
   }

   trans = transform(VERTICAL, Adiff, Bdiff, Cdiff, xminus, yminus, xplus, yplus);

   Adiff = trans->A;
   Bdiff = trans->B;
   Cdiff = trans->C;

   if(debug)
   {
      printf("DEBUG> Transforming diff back:        [%9.2e %9.2e %9.2e]\n",
         Adiff, Bdiff, Cdiff);
      fflush(stdout);
   }

   Aplus -= Adiff/2.;
   Bplus -= Bdiff/2.;
   Cplus -= Cdiff/2.;

   if(debug)
   {
      printf("DEBUG> Subtracting half from 'plus':  [%9.2e %9.2e %9.2e]\n",
         Aplus, Bplus, Cplus);
      fflush(stdout);
   }

   
   // As a check, we calculate and compare the values the two planes (after
   // correction) give at their respective (x,y) reference points.


   if(debug)
   {
      valplus  = Aplus  * xplus  + Bplus  * yplus  + Cplus;
      valminus = Aminus * xminus + Bminus * yminus + Cminus;

      printf("DEBUG> Reference point values: %9.2e vs. %9.2e\n", 
         valplus, valminus);
      fflush(stdout);
   }


   // And finally, make the corrected images
   
   sprintf(outfile, "diffed_%s", plus);

   mBackground(plus, outfile, Aplus, Bplus, Cplus, 1, 0);

   sprintf(outfile, "diffed_%s", minus);

   mBackground(minus, outfile, Aminus, Bminus, Cminus, 1, 0);

   return;
}


/********************************************************************/
/*                                                                  */
/* This routine transforms a plane defined for a tile along the     */
/* "horizontal" edge of one of the polar splits in HPX projection   */
/* to the corresponding "vertical" edge of the matching tile on the */
/* other side of the split (or vice versa if "dir" is 1).           */
/*                                                                  */
/********************************************************************/

struct Plane *transform(int dir,
                        double A, double B, double C, 
                        double x0,  double y0,
                        double x0p, double y0p)
{
   static struct Plane plane;

   double Ap, Bp, Cp;

   if(dir == HORIZONTAL)
   {
      Ap =  B;
      Bp = -A;
      Cp =  (A*x0 + B*y0 + C) - (Ap*x0p + Bp*y0p);
   }
   else              // Here we are really going from Ap etc. to A
   {                 // so we reverse the meaning of the arguments
      Ap = -B;
      Bp =  A;
      Cp =  (A*x0 + B*y0 + C) - (Ap*x0p + Bp*y0p);
   }


   // Load into return structure
   //
   plane.A = Ap;
   plane.B = Bp;
   plane.C = Cp;

   return &plane;
}
