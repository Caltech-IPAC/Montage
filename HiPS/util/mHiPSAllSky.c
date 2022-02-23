#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <fitsio.h>
#include <svc.h>


#define MAXSTR 1024

int debug = 0;

int ntiles[4] = {12, 48, 192, 768};


int main(int argc, char **argv)
{
   int       i, j, nullcnt, nfound, tile, order, bad;
   int       nx, ny, itile, jtile, ioffset, joffset, iout, jout;

   int       bitpix = DOUBLE_IMG;

   long      fpixel [4];
   long      fpixelo[4];
   long      naxis,  naxes [10];
   long      naxiso, naxeso[10];
   double  **ibuffer, tmp;
   double  **obuffer;

   char      infile [MAXSTR];
   char      outfile[MAXSTR];
   char      hipsdir[MAXSTR];
   char      cmd    [MAXSTR];

   fitsfile *infptr, *outfptr;

   int       status = 0;


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

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSAllSky [-d] hipsdir\"]\n");
      exit(0);
   }

   for(i=0; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
      {
         debug = 1;

         svc_debug(stdout);
      }
   }

   if(debug)
   {
      ++argv;
      --argc;
   }

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSAllSky [-d] hipsdir\"]\n");
      exit(0);
   }

   strcpy(hipsdir, argv[1]);

   if(hipsdir[strlen(hipsdir)-1] != '/')
      strcat(hipsdir, "/");



   /********************************************/
   /* Allocate space for the input images data */
   /********************************************/
   
   ibuffer = (double **)malloc(512*sizeof(double *));

   for(j=0; j<512; ++j)
      ibuffer[j] = (double *)malloc(512*sizeof(double));

   if(debug)
   {
      printf("DEBUG> Input buffer allocated.\n");
      fflush(stdout);
   }


   /******************************************/
   /* Create the output array and FITS image */
   /******************************************/

   order = 3;

   tmp = sqrt(ntiles[order]);
   nx = (int)tmp;

   tmp = (double)ntiles[order]/nx;
   ny = (int)tmp + 1;


   naxiso = 2;

   naxeso[0] = nx * 512;
   naxeso[1] = ny * 512;

   if(debug)
   {
      printf("DEBUG> naxeso[0] = %ld\n", naxeso[0]);
      printf("DEBUG> naxeso[1] = %ld\n", naxeso[1]);
      fflush(stdout);
   }

   obuffer = (double **)malloc(naxeso[1]*sizeof(double *));

   for(j=0; j<naxeso[1]; ++j)
      obuffer[j] = (double *)malloc(naxeso[0]*sizeof(double));

   for(j=0; j<naxeso[1]; ++j)
      for(i=0; i<naxeso[0]; ++i)
         obuffer[j][i] = dnan;

   if(debug)
   {
      printf("DEBUG> Output buffer allocated.\n");
      fflush(stdout);
   }

   sprintf(outfile, "%sNorder%d/Allsky.fits", hipsdir, order);
   unlink(outfile);

   if(debug)
   {
      printf("DEBUG> Output file: [%s]\n", outfile);
      fflush(stdout);
   }

   status = 0;

   if(fits_create_file(&outfptr, outfile, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Can't open output FITS file [%s].\"]\n", outfile);
      fflush(stdout);
      exit(0);
   }

   if (fits_create_img(outfptr, bitpix, naxiso, naxeso, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Can't create image in FITS file [%s].\"]\n", outfile);
      fflush(stdout);
      exit(0);
   }

   if(debug)
   {
      printf("DEBUG> Output image initialized.\n");
      fflush(stdout);
   }


   /********************************************/
   /* Allocate space for the input images data */
   /********************************************/
   
   ibuffer = (double **)malloc(512*sizeof(double *));

   for(j=0; j<512; ++j)
      ibuffer[j] = (double *)malloc(512*sizeof(double));

   if(debug)
   {
      printf("DEBUG> Input buffer allocated.\n");
      fflush(stdout);
   }



   /******************************/
   /* Loop over the input images */
   /******************************/

   for(tile=0; tile<ntiles[order]; ++tile)
   {
      // Read in each FITS file
      
      itile = tile / 1000;

      sprintf(infile, "%sNorder%d/Dir%d/Npix%d.fits", hipsdir, order, itile, tile);

      if(debug)
      {
         printf("DEBUG> Input file: [%s]\n", infile);
         fflush(stdout);
      }

      status = 0;

      if(fits_open_file(&infptr, infile, READONLY, &status))
      {
         printf("[struct stat=\"WARNING\", msg=\"Can't open input FITS file [%s].\"]\n", infile);
         fflush(stdout);
         continue;
      }

      if(fits_read_keys_lng(infptr, "NAXIS", 1, 2, naxes, &nfound, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Can't find NAXIS keywords [%s].\"]\n", infile);
         fflush(stdout);
         exit(0);
      }

      if(debug)
      {
         printf("DEBUG> naxes: %ldx%ld\n", naxes[0], naxes[1]);
         fflush(stdout);
      }

      if(naxes[0]!=512 || naxes[1]!=512)
      {
         printf("[struct stat=\"ERROR\", msg=\"HiPS tiles must be 512x512 [%s].\"]\n", infile);
         fflush(stdout);
         exit(0);
      }

      fpixel[0] = 1;
      fpixel[1] = 1;
      fpixel[2] = 1;
      fpixel[3] = 1;

      bad = 0;

      for(j=0; j<naxes[1]; ++j)
      {
         if(fits_read_pix(infptr, TDOUBLE, fpixel, naxes[0], &dnan, ibuffer[j], &nullcnt, &status))
         {
            printf("[struct stat=\"WARNING\", msg=\"Problem reading pixels.\"]\n");
            fflush(stdout);

            bad = 1;

            break;
         }

         ++fpixel[1];
      }

      if(bad)
         continue;

      if(debug)
      {
         printf("DEBUG> File %s read.\n", infile);
         fflush(stdout);
      }

      if(fits_close_file(infptr, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Problem closing input FITS file.\"]\n");
         fflush(stdout);
         continue;
      }


      // Determine the offset in the output array for this input

      jtile = tile / nx;
      itile = tile - jtile * nx;

      joffset = jtile * 512;
      ioffset = itile * 512;

      if(debug)
      {
         printf("DEBUG> tile    = %d\n", tile);
         printf("DEBUG> jtile   = %d\n", jtile);
         printf("DEBUG> itile   = %d\n", itile);
         printf("DEBUG> joffset = %d\n", joffset);
         printf("DEBUG> ioffset = %d\n", ioffset);
         fflush(stdout);
      }


      // Transfer the data to the output array (transposing)
      
      for(j=0; j<naxes[1]; ++j)
      {
         for(i=0; i<naxes[0]; ++i)
         {
            iout = j + ioffset;
            jout = i + joffset;

            obuffer[jout][iout] = ibuffer[j][512-1-i];
         }
      }
   }


   /******************************************/
   /* Write the data to the output FITS file */
   /******************************************/

   fpixelo[0] = 1;
   fpixelo[1] = 1;
   fpixelo[2] = 1;
   fpixelo[3] = 1;

   for(j=0; j<naxeso[1]; ++j)
   {
      if (fits_write_pix(outfptr, TDOUBLE, fpixelo, naxeso[0], (void *)(obuffer[j]), &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Problem reading pixels.\"]\n");
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


   printf("[struct stat=\"OK\", module=\"mHiPSAllSky\"]\n");
   fflush(stdout);
   exit(0);
}
