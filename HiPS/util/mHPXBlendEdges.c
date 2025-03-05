#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <fitsio.h>
#include <svc.h>


#define MAXSTR 1024

#define CENTER 0
#define LEFT   1
#define RIGHT  2
#define TOP    3
#define BOTTOM 4
#define LL     5
#define UR     6
#define UL     7
#define LR     8

#define mNaN(x) (isnan(x) || !isfinite(x))


struct ImgInfo
{
   char      infile [MAXSTR];

   fitsfile *fptr;

   long      naxes  [2];
   double    crpix  [2];

   int       left;
   int       right;
   int       top;
   int       bottom;

   int       inxoffset;
   int       inxstart;
   int       outxstart;
   int       xlen;

   int       inyoffset;
   int       inystart;
   int       outystart;
   int       ylen;
};

struct ImgInfo info[9];


int xoffset[] = {0, -1,  1,  0,  0, -1,  1, -1,  1};
int yoffset[] = {0,  0,  0,  1, -1, -1,  1,  1, -1};

char location[9][16] = {"CENTER", "LEFT", "RIGHT", "TOP", "BOTTOM", "LL", "UR", "UL", "LR"};

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

void mHPXBlendEdges_printFitsError(int status);

int debug = 0;


int main(int argc, char **argv)
{
   int       c, i, j, k, nullcnt, nfound, tile, order;

   int       iplate, jplate, nplate, width, height, blend;

   int       bitpix = DOUBLE_IMG;

   long      fpixel [4];
   long      fpixelo[4];
   long      naxiso, naxeso[10];
   long      nelements;

   double    xmin0, xmax0;
   double    xmin,  xmax;

   double    values[4], vtmp;

   double    hfrac, vfrac, newval;

   double    ymin0, ymax0;
   double    ymin,  ymax;

   double  **obuffer;

   char      outfile[MAXSTR];
   char      path   [MAXSTR];
   char      base   [MAXSTR];

   fitsfile *outfptr;

   int       status = 0;

   double  **data[9];

   int       in[9];


   /*************************************************/
   /* Make a NaN value to use checking blank pixels */
   /*************************************************/

   union
   {
      double d8;
      char   c[8];
   }
   value;

   double dnan;
   float  fnan;

   for(i=0; i<8; ++i)
      value.c[i] = (char)255;

   dnan = value.d8;


   /************************/
   /* Process command-line */
   /************************/

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mBlendEdges [-d][-c(enter-only)][-p path] iplate jplate width outfile\"]\n");
      exit(0);
   }

   strcpy(path, "./");
   strcpy(base, "plate");

   blend = 1;

   while ((c = getopt(argc, argv, "d:cb:p:")) != EOF)
   {
      switch (c)
      {
         case 'd':
            debug = atoi(optarg);
            break;

         case 'b':
            strcpy(base, optarg);
            break;

         case 'c':
            blend = 0;
            break;

         case 'p':
            strcpy(path, optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mBlendEdges [-d][-c(enter-only)][-p path] iplate jplate width outfile\"]\n");
            fflush(stdout);
            exit(1);
      }
   }

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mBlendEdges [-d][-c(enter-only)][-p path] iplate jplate width outfile\"]\n");
      exit(0);
   }

   iplate = atoi(argv[optind]);
   jplate = atoi(argv[optind+1]);
   nplate = atoi(argv[optind+2]);
   width  = atoi(argv[optind+3]);

   if(debug)
   {
      printf("DEBUG> iplate = %d\n", iplate);
      printf("DEBUG> jplate = %d\n", jplate);
      printf("DEBUG> nplate = %d\n", nplate);
      printf("DEBUG> width  = %d\n", width);
      fflush(stdout);
   }

   strcpy(outfile, argv[optind+4]);

   height = width;

   if(path[strlen(path)-1] != '/')
      strcat(path, "/");


   /*********************************/
   /* Process the nine input images */
   /*********************************/
   
   xmin0 = (iplate - nplate/2.) * width;
   ymin0 = (jplate - nplate/2.) * height;
   
   xmax0 = xmin0 + width;
   ymax0 = ymin0 + height;
   
   for(i=0; i<9; ++i)
   {
      // What we really need is to determine what range of pixels in this image
      // overlap with what pixels in the nominal (non-padded) "center" image (even
      // if this is the center image).  The simplest way to do this is to characterize
      // both boxes in "crpix" units since we have those pretty much to hand anyway.

      sprintf(info[i].infile, "%s%s_%02d_%02d.fits", path, base, iplate+xoffset[i], jplate+yoffset[i]);

      status = 0;
      if(fits_open_file(&(info[i].fptr), info[i].infile, READONLY, &status))
      {
         info[i].fptr = (fitsfile *)NULL;
         continue;
      }

      status = 0;
      if(fits_read_keys_lng(info[i].fptr, "NAXIS", 1, 2, info[i].naxes, &nfound, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Can't find NAXIS keywords [%s].\"]\n", info[i].infile);
         fflush(stdout);
         exit(0);
      }

      status = 0;
      if(fits_read_keys_dbl(info[i].fptr, "CRPIX", 1, 2, info[i].crpix, &nfound, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Can't find CRPIX keywords [%s].\"]\n", info[i].infile);
         fflush(stdout);
         exit(0);
      }

      xmin = -(info[i].crpix[0]+0.5);
      ymin = -(info[i].crpix[1]+0.5);

      xmax = xmin + info[i].naxes[0];
      ymax = ymin + info[i].naxes[1];


      // Sort the four overlap box edge values.  The middle two values in the sorted
      // list will be the extent of overlap. The value "inxstart" is the offset of this
      // data's start (e.g. "LR") relative to the file we are reading from and 
      // the value "outxstart" is the same data relative to the "CENTER" array.

      values[0] = xmin;
      values[1] = xmax;
      values[2] = xmin0;
      values[3] = xmax0;

      for(j=2; j>=0; --j)
         for(k=0; k<=j; ++k)
            if(values[k] > values[k+1]) {vtmp = values[k]; values[k] = values[k+1]; values[k+1] = vtmp;}

      info[i].inxoffset = (int)(values[1]-values[0]);

      if(i == RIGHT || i == UR || i == LR) 
         info[i].inxoffset = 0;

      info[i].inxstart  = (int)(values[2]-xmin);
      info[i].outxstart = (int)(values[2]-xmin0);
      info[i].xlen      = (int)(values[2]-values[1]);

      values[0] = ymin;
      values[1] = ymax;
      values[2] = ymin0;
      values[3] = ymax0;

      for(j=2; j>=0; --j)
         for(k=0; k<=j; ++k)
            if(values[k] > values[k+1]) {vtmp = values[k]; values[k] = values[k+1]; values[k+1] = vtmp;}

      info[i].inyoffset = (int)(values[1]-values[0]);

      if(i == TOP || i == UR || i == UL) 
         info[i].inyoffset = 0;

      info[i].inystart  = (int)(values[2]-ymin);
      info[i].outystart = (int)(values[2]-ymin0);
      info[i].ylen      = (int)(values[2]-values[1]);

      info[i].left   =  (iplate-nplate/2+xoffset[i]  )*width  +  (info[i].crpix[0]+0.5);
      info[i].right  = -(iplate-nplate/2+xoffset[i]+1)*width  - ((info[i].crpix[0]+0.5)-info[i].naxes[0]);
      info[i].top    =  (jplate-nplate/2+yoffset[i]  )*height +  (info[i].crpix[1]+0.5);
      info[i].bottom = -(jplate-nplate/2+yoffset[i]+1)*height - ((info[i].crpix[1]+0.5)-info[i].naxes[1]);

      if(debug)
      {
         printf("\nDEBUG> IMAGE PARAMETERS -------------------------------------------------\n\n");

         printf("\nDEBUG> Image %d (%s):\n\n", i, location[i]);

         printf("DEBUG> file:   [%s]\n\n", info[i].infile);

         printf("DEBUG> NAXES: %ld x %ld\n",      info[i].naxes[0], info[i].naxes[1]);
         printf("DEBUG> CRPIX: %.1f, %.1f\n\n",   info[i].crpix[0], info[i].crpix[1]);

         printf("DEBUG> X offset:       %d\n",    info[i].inxoffset);
         printf("DEBUG> Y offset:       %d\n\n",  info[i].inyoffset);

         printf("DEBUG> left pad:       %d\n",    info[i].left);
         printf("DEBUG> right pad:      %d\n",    info[i].right);
         printf("DEBUG> top pad:        %d\n",    info[i].top);
         printf("DEBUG> bottom pad:     %d\n\n",  info[i].bottom);

         printf("DEBUG> X coord: image %6d -> center %6d (%6d pixels)\n",   info[i].inxstart, info[i].outxstart, info[i].xlen);
         printf("DEBUG> Y coord: image %6d -> center %6d (%6d pixels)\n\n", info[i].inystart, info[i].outystart, info[i].ylen);

         fflush(stdout);
      }


      // Allocate space and read in the part of the images we need
      
      if(info[i].xlen > 0 && info[i].ylen > 0)
      {
         data[i] = (double **)malloc(info[i].ylen * sizeof(double *));

         for(j=0; j<info[i].ylen; ++j)
         {
            data[i][j] = (double *)malloc(info[i].xlen * sizeof(double));

            fpixel[0] = info[i].inxoffset + 1;
            fpixel[1] = info[i].inyoffset + 1 + j;
            fpixel[2] = 1;
            fpixel[3] = 1;

            nelements = info[i].xlen;

            status = 0;
            fits_read_pix(info[i].fptr, TDOUBLE, fpixel, nelements, &dnan, data[i][j], &nullcnt, &status);
         }
      }
   }


   /*****************************************************************/
   /* For each pixel, determine which boxes it is in and the weight */
   /* that image should have, based on distance from the associated */
   /* edge.                                                         */
   /*****************************************************************/

   if(blend)
   {
      for(j=0; j<height; ++j)
      {
         if(debug)
         {
            printf("DEBUG> Row %d\n", j);
            fflush(stdout);
         }

         for(i=0; i<width; ++i)
         {
            hfrac = 0.;
            vfrac = 0.;

            for(k=1; k<9; ++k)
               in[k] = 0;

            if(i <        info[  LEFT].xlen) {in[  LEFT] = 1; hfrac = (double)        i  / info[  LEFT].xlen;}

            if(i > width -info[ RIGHT].xlen) {in[ RIGHT] = 1; hfrac = (double)( width-i) / info[ RIGHT].xlen;}

            if(j <        info[BOTTOM].ylen) {in[BOTTOM] = 1; vfrac = (double)        j  / info[BOTTOM].ylen;}

            if(j > height-info[   TOP].ylen) {in[   TOP] = 1; vfrac = (double)(height-j) / info[   TOP].ylen;}

            if(i <        info[    LL].xlen
            && j <        info[    LL].ylen) {in[    LL] = 1; hfrac = (double)        i  / info[    LL].xlen;
                                                              vfrac = (double)        j  / info[    LL].ylen;}

            if(i <        info[    UL].xlen 
            && j > height-info[    UL].ylen) {in[    UL] = 1; hfrac = (double)        i  / info[    UL].xlen;
                                                              vfrac = (double)(height-j) / info[    UL].ylen;}

            if(i > width -info[    LR].xlen
            && j <        info[    LR].ylen) {in[    LR] = 1; hfrac = (double)( width-i) / info[    LR].xlen;
                                                              vfrac = (double)        j  / info[    LR].ylen;}

            if(i > width -info[    UR].xlen
            && j > height-info[    UR].ylen) {in[    UR] = 1; hfrac = (double)( width-i) / info[ RIGHT].xlen;
                                                              vfrac = (double)(height-j) / info[   TOP].ylen;}

            hfrac = 0.5 * hfrac + 0.5;
            vfrac = 0.5 * vfrac + 0.5;

            if(debug >= 2)
            {
               printf("DEBUG> %d %d: v=%-g, h=%-g [%d %d %d %d %d %d %d %d] ",
                     j, i, vfrac, hfrac,
                     in[LEFT], in[RIGHT], in[TOP], in[BOTTOM], in[LL], in[UL], in[LR], in[UR]); 
               fflush(stdout);
            }

            
            // LEFT

            if(in[LEFT] && !in[LL] && !in[UL] && info[LEFT].fptr != (fitsfile *)NULL)
            {
               if(isnan(data[CENTER][j][i])) data[CENTER][j][i] = 0.;
               if(isnan(data[LEFT  ][j][i])) data[LEFT  ][j][i] = data[CENTER][j][i];

               newval = hfrac * data[CENTER][j][i] + (1.-hfrac) * data[LEFT][j][i];

               if(debug >= 2)
               {
                  printf("LEFT:  CENTER[%d][%d] (%-g) and LEFT[%d][%d] (%-g) -> %-g ", 
                        j, i, data[CENTER][j][i], 
                        j, i, data[LEFT  ][j][i],
                        newval);
                  fflush(stdout);
               }

               data[CENTER][j][i] = newval;
            }

            
            // RIGHT

            else if(in[RIGHT] && !in[LR] && !in[UR] && info[RIGHT].fptr != (fitsfile *)NULL)
            {
               if(isnan(data[CENTER][j][i                                       ])) data[CENTER][j][i                                       ] = 0.;
               if(isnan(data[RIGHT ][j][i-info[RIGHT].outxstart+info[RIGHT].xlen])) data[RIGHT ][j][i-info[RIGHT].outxstart+info[RIGHT].xlen] = data[CENTER][j][i];

               newval = hfrac * data[CENTER][j][i] + (1.-hfrac) * data[RIGHT][j][i-info[RIGHT].outxstart+info[RIGHT].xlen];

               if(debug >= 2)
               {
                  printf("RIGHT:  CENTER[%d][%d] (%-g) and RIGHT[%d][%d] (%-g) -> %-g\n", 
                        j, i                                       , data[CENTER][j][i                                       ],
                        j, i-info[RIGHT].outxstart+info[RIGHT].xlen, data[RIGHT ][j][i-info[RIGHT].outxstart+info[RIGHT].xlen], 
                        newval);
                  fflush(stdout);
               }

               data[CENTER][j][i] = newval;
            }

            
            // TOP

            else if(in[TOP] && !in[UL] && !in[UR] && info[TOP].fptr != (fitsfile *)NULL)
            {
               if(isnan(data[CENTER][j                                   ][i])) data[CENTER][j                                   ][i] = 0.;
               if(isnan(data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i])) data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i] = data[CENTER][j][i];

               newval = vfrac * data[CENTER][j][i] + (1.-vfrac) * data[TOP][j-info[TOP].outystart+info[TOP].ylen][i];

               if(debug >= 2)
               {
                  printf("TOP:  CENTER[%d][%d] (%-g) and TOP[%d][%d] (%-g) -> %-g\n",
                        j,                                    i, data[CENTER][j                                   ][i],
                        j-info[TOP].outystart+info[TOP].ylen, i, data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i], 
                        newval);
                  fflush(stdout);
               }

               data[CENTER][j][i] = newval;
            }

            
            // BOTTOM

            else if(in[BOTTOM] && !in[LL] && !in[LR] && info[BOTTOM].fptr != (fitsfile *)NULL)
            {
               if(isnan(data[CENTER][j][i])) data[CENTER][j][i] = 0.;
               if(isnan(data[BOTTOM][j][i])) data[BOTTOM][j][i] = data[CENTER][j][i];

               newval = vfrac * data[CENTER][j][i] + (1.-vfrac) * data[BOTTOM][j][i];

               if(debug >= 2)
               {
                  printf("BOTTOM:  CENTER[%d][%d] (%-g) and BOTTOM[%d][%d] (%-g) -> %-g\n",
                        j, i, data[CENTER][j][i],
                        j, i, data[BOTTOM][j][i],
                        newval);
                  fflush(stdout);
               }

               data[CENTER][j][i] = newval;
            }

            
            // LL (implies LEFT and BOTTOM)

            else if(in[LL] && in[LEFT] && in[BOTTOM]
                 && info[LL    ].fptr != (fitsfile *)NULL
                 && info[LEFT  ].fptr != (fitsfile *)NULL
                 && info[BOTTOM].fptr != (fitsfile *)NULL)
            {
               if(isnan(data[CENTER][j][i])) data[CENTER][j][i] = 0.;
               if(isnan(data[LL    ][j][i])) data[LL    ][j][i] = data[CENTER][j][i];
               if(isnan(data[LEFT  ][j][i])) data[LEFT  ][j][i] = data[CENTER][j][i];
               if(isnan(data[BOTTOM][j][i])) data[BOTTOM][j][i] = data[CENTER][j][i];

               newval = data[CENTER][j][i] * (vfrac + hfrac) / 2.
                      + data[LEFT  ][j][i] * (1. - hfrac) / 3.
                      + data[BOTTOM][j][i] * (1. - vfrac) / 3.
                      + data[LL    ][j][i] * (1. - (hfrac + vfrac) / 2.) / 3.;

               if(debug >= 2)
               {
                  printf("LL:  CENTER[%d][%d] (%-g), LL[%d][%d] (%-g), LEFT[%d][%d] (%-g) and BOTTOM[%d][%d] (%-g) -> %-g\n",
                        j, i, data[CENTER][j][i],
                        j, i, data[LL    ][j][i],
                        j, i, data[LEFT  ][j][i],
                        j, i, data[BOTTOM][j][i],
                        newval);
                  fflush(stdout);
               }

               data[CENTER][j][i] = newval;
            }

            
            // LR (implies RIGHT and BOTTOM)

            else if(in[LR] && in[RIGHT] && in[BOTTOM]
                 && info[LR    ].fptr != (fitsfile *)NULL
                 && info[RIGHT ].fptr != (fitsfile *)NULL
                 && info[BOTTOM].fptr != (fitsfile *)NULL)
            {
               if(isnan(data[CENTER][j][i                                       ])) data[CENTER][j][i                                       ] = 0.;
               if(isnan(data[LR    ][j][i-info[LR   ].outxstart+info[LR   ].xlen])) data[LR    ][j][i-info[LR   ].outxstart+info[LR   ].xlen] = data[CENTER][j][i];
               if(isnan(data[RIGHT ][j][i-info[RIGHT].outxstart+info[RIGHT].xlen])) data[RIGHT ][j][i-info[RIGHT].outxstart+info[RIGHT].xlen] = data[CENTER][j][i];
               if(isnan(data[BOTTOM][j][i                                       ])) data[BOTTOM][j][i                                       ] = data[CENTER][j][i];

               newval = data[CENTER][j][i                                       ] * (vfrac + hfrac) / 2.
                      + data[RIGHT ][j][i-info[RIGHT].outxstart+info[RIGHT].xlen] * (1. - hfrac) / 3.
                      + data[BOTTOM][j][i                                       ] * (1. - vfrac) / 3.
                      + data[LR    ][j][i-info[LR   ].outxstart+info[LR   ].xlen] * (1. - (hfrac + vfrac) / 2.) / 3.;

               if(debug >= 2)
               {
                  printf("LR:  CENTER[%d][%d] (%-g), LR[%d][%d] (%-g), RIGHT[%d][%d] (%-g) and BOTTOM[%d][%d] (%-g) -> %-g\n",
                         j, i                                    , data[CENTER][j][i                                       ],
                         j, i-info[LR   ].outxstart+info[LR].xlen, data[LR    ][j][i-info[LR   ].outxstart+info[LR   ].xlen],
                         j, i-info[RIGHT].outxstart+info[LR].xlen, data[RIGHT ][j][i-info[RIGHT].outxstart+info[RIGHT].xlen],
                         j, i                                    , data[BOTTOM][j][i                                       ],
                         newval);
                  fflush(stdout);
               }

               data[CENTER][j][i] = newval;
            }

            
            // UL (implies LEFT and TOP)

            else if(in[UL] && in[LEFT] && in[TOP] 
                 && info[UL  ].fptr != (fitsfile *)NULL
                 && info[LEFT].fptr != (fitsfile *)NULL
                 && info[TOP ].fptr != (fitsfile *)NULL)
            {
               if(isnan(data[CENTER][j                                   ][i])) data[CENTER][j                                   ][i] = 0.;
               if(isnan(data[UL    ][j-info[UL ].outystart+info[UL ].ylen][i])) data[UL    ][j-info[UL ].outystart+info[UL ].ylen][i] = data[CENTER][j][i];
               if(isnan(data[LEFT  ][j                                   ][i])) data[LEFT  ][j                                   ][i] = data[CENTER][j][i];
               if(isnan(data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i])) data[UL    ][j-info[TOP].outystart+info[TOP].ylen][i] = data[CENTER][j][i];

               newval = data[CENTER][j                                   ][i] * (vfrac + hfrac) / 2.
                      + data[LEFT  ][j                                   ][i] * (1. - hfrac) / 3.
                      + data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i] * (1. - vfrac) / 3.
                      + data[UL    ][j-info[UL ].outystart+info[UL ].ylen][i] * (1. - (hfrac + vfrac) / 2.) / 3.;

               if(debug >= 2)
               {
                  printf("UL:  CENTER[%d][%d] (%-g), UL[%d][%d] (%-g), LEFT[%d][%d] (%-g) and TOP[%d][%d] (%-g) -> %-g\n", 
                         j                                   , i, data[CENTER][j                                   ][i], 
                         j-info[UL ].outystart+info[UL ].ylen, i, data[UL    ][j-info[UL ].outystart+info[UL ].ylen][i], 
                         j                                   , i, data[LEFT  ][j                                   ][i],
                         j-info[TOP].outystart+info[TOP].ylen, i, data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i],
                         newval);
                  fflush(stdout);
               }

               data[CENTER][j][i] = newval;
            }

            
            // UR (implies RIGHT and TOP)

            else if(in[UR] && in[RIGHT] && in[TOP]
                 && info[UR   ].fptr != (fitsfile *)NULL
                 && info[RIGHT].fptr != (fitsfile *)NULL
                 && info[TOP  ].fptr != (fitsfile *)NULL)
            {
               if(isnan(data[CENTER][j                                   ][i                                       ])) data[CENTER][j                                   ][i                                       ] = 0.;
               if(isnan(data[UR    ][j-info[UR ].outystart+info[UR ].ylen][i-info[UR   ].outxstart+info[UR   ].xlen])) data[UR    ][j-info[UR ].outystart+info[UR ].ylen][i-info[UR   ].outxstart+info[UR   ].xlen] = data[CENTER][j][i];
               if(isnan(data[RIGHT ][j                                   ][i-info[RIGHT].outxstart+info[RIGHT].xlen])) data[RIGHT ][j                                   ][i-info[RIGHT].outxstart+info[RIGHT].xlen] = data[CENTER][j][i];
               if(isnan(data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i                                       ])) data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i                                       ] = data[CENTER][j][i];

               newval = data[CENTER][j                                   ][i                                       ] * (vfrac + hfrac) / 2.
                      + data[RIGHT ][j                                   ][i-info[RIGHT].outxstart+info[RIGHT].xlen] * (1. - hfrac) / 3.
                      + data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i                                       ] * (1. - vfrac) / 3.
                      + data[UR    ][j-info[UR ].outystart+info[UR ].ylen][i-info[UR   ].outxstart+info[UR   ].xlen] * (1. - (hfrac + vfrac) / 2.) / 3.;

               if(debug >= 2)
               {
                  printf("UR:  CENTER[%d][%d] (%-g), UR[%d][%d] (%-g), RIGHT[%d][%d] (%-g) and TOP[%d][%d] (%-g) -> %-g\n",
                        j                                   , i                                       , data[CENTER][j                                   ][i                                       ],
                        j-info[UR ].outystart+info[UR ].ylen, i-info[UR   ].outxstart+info[UR   ].xlen, data[UR    ][j-info[UR ].outystart+info[UR ].ylen][i-info[UR   ].outxstart+info[UR   ].xlen], 
                        j                                   , i-info[RIGHT].outxstart+info[RIGHT].xlen, data[RIGHT ][j                                   ][i-info[RIGHT].outxstart+info[RIGHT].xlen], 
                        j-info[TOP].outystart+info[TOP].ylen, i                                       , data[TOP   ][j-info[TOP].outystart+info[TOP].ylen][i                                       ],
                        newval);
                  fflush(stdout);
               }

               data[CENTER][j][i] = newval;
            }

            
            // CENTER

            else 
            {
               // Do nothing (leave center array value unchanged.

               if(debug >= 2)
               {
                  printf("CENTER[%d][%d] (%-g) unchanged.\n", j, i, data[CENTER][j][i]);
                  fflush(stdout);
               }
            }
         }
      }
   }


   /******************************************/
   /* Write the data to the output FITS file */
   /******************************************/

   if(debug)
   {
      printf("\nDEBUG> Writing out image [%s].\n\n", outfile);
      fflush(stdout);
   }

   nelements = width;

   fpixelo[0] = 1;
   fpixelo[1] = 1;
   fpixelo[2] = 1;
   fpixelo[3] = 1;

   status = 0;

   unlink(outfile);

   if(fits_create_file(&outfptr, outfile, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Can't create output file: %s\"]\n", outfile);
      fflush(stdout);
      exit(0);
   }

   if(fits_copy_header(info[CENTER].fptr, outfptr, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Can't create output file: %s\"]\n", outfile);
      fflush(stdout);
      exit(0);
   }

   if(fits_update_key_lng(outfptr, "NAXIS1", nelements, (char *)NULL, &status))
      mHPXBlendEdges_printFitsError(status);

   if(fits_update_key_lng(outfptr, "NAXIS2", nelements, (char *)NULL, &status))
      mHPXBlendEdges_printFitsError(status);

   if(fits_update_key_dbl(outfptr, "CRPIX1", xmin0, -14, (char *)NULL, &status))
      mHPXBlendEdges_printFitsError(status);

   if(fits_update_key_dbl(outfptr, "CRPIX2", ymin0, -14, (char *)NULL, &status))
      mHPXBlendEdges_printFitsError(status);


   if(debug)
   {
      printf("DEBUG> width     = %d\n", width);
      printf("DEBUG> height    = %d\n", height);
      printf("DEBUG> nelements = %d\n", (int)nelements);
      fflush(stdout);
   }

   for(j=0; j<height; ++j)
   {
      status = 0;

      if(fits_write_pix(outfptr, TDOUBLE, fpixelo, nelements, (void *)(data[CENTER][j]), &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Problem writing pixels.\"]\n");
         fflush(stdout);
         exit(0);
      }

      ++fpixelo[1];
   }


   /***********************/
   /* Close the FITS file */
   /***********************/

   if(fits_close_file(outfptr, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Problem closing output FITS file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   printf("[struct stat=\"OK\", module=\"mBlendEdges\", file=\"%s\"]\n", info[CENTER].infile);
   fflush(stdout);
   exit(0);
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mHPXBlendEdges_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", status_str);
   fflush(stdout);
   exit(0);
}
