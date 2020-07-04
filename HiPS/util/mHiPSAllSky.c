#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fitsio.h>
#include <svc.h>


#define MAXSTR 1024

int debug = 0;


int main(int argc, char **argv)
{
   int       i, j, nullcnt, nfound, tile;
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

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSAllSky [-d] hipsdir allsky.fits\"]\n");
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

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSAllSky [-d] hipsdir allsky.fits\"]\n");
      exit(0);
   }

   strcpy(hipsdir, argv[1]);
   strcpy(outfile, argv[2]);

   if(hipsdir[strlen(hipsdir)-1] != '/')
      strcat(hipsdir, "/");




   /******************************************/
   /* Create the output array and FITS image */
   /******************************************/

   nx = 27;
   ny = 29;

   naxiso = 2;

   naxeso[0] = nx * 64;
   naxeso[1] = ny * 64;

   if(debug)
   {
      printf("DEBUG> naxeso[0] = %ld\n", naxeso[0]);
      printf("DEBUG> naxeso[1] = %ld\n", naxeso[1]);
      fflush(stdout);
   }

   obuffer = (double **)malloc(naxeso[1]*sizeof(double *));

   for(j=0; j<naxeso[1]; ++j)
      obuffer[j] = (double *)malloc(naxeso[0]*sizeof(double));

   if(debug)
   {
      printf("DEBUG> Output buffer allocated.\n");
      fflush(stdout);
   }

   unlink(outfile);

   status = 0;

   if(fits_create_file(&outfptr, outfile, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Can't open input FITS file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   if (fits_create_img(outfptr, bitpix, naxiso, naxeso, &status))
   {
      printf("[struct stat=\"ERROR\", msg=\"Can't create image in FITS file.\"]\n");
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
   
   ibuffer = (double **)malloc(64*sizeof(double *));

   for(j=0; j<64; ++j)
      ibuffer[j] = (double *)malloc(64*sizeof(double));

   if(debug)
   {
      printf("DEBUG> Input buffer allocated.\n");
      fflush(stdout);
   }



   /******************************/
   /* Loop over the input images */
   /******************************/

   for(tile=0; tile<768; ++tile)
   {
      // Shrink the HiPS tile down to 64x64
      
      sprintf(cmd, "mShrink %sNorder3/Dir0/Npix%d.fits %sNorder3/Dir0/Npix%d_small.fits 8",
         hipsdir, tile, hipsdir, tile);

      svc_run(cmd);


      // Read in the shrunken file
      
      sprintf(infile, "%sNorder3/Dir0/Npix%d_small.fits", hipsdir, tile);

      status = 0;

      if(fits_open_file(&infptr, infile, READONLY, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Can't open input FITS file.\"]\n");
         fflush(stdout);
         exit(0);
      }

      if(fits_read_keys_lng(infptr, "NAXIS", 1, 2, naxes, &nfound, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Can't find NAXIS keywords.\"]\n");
         fflush(stdout);
         exit(0);
      }

      if(debug)
      {
         printf("DEBUG> naxes: %ldx%ld\n", naxes[0], naxes[1]);
         fflush(stdout);
      }

      if(naxes[0]!=64 || naxes[1]!=64)
      {
         printf("[struct stat=\"ERROR\", msg=\"HiPS tiles must be 64x64.\"]\n");
         fflush(stdout);
         exit(0);
      }

      fpixel[0] = 1;
      fpixel[1] = 1;
      fpixel[2] = 1;
      fpixel[3] = 1;

      for(j=0; j<naxes[1]; ++j)
      {
         if(fits_read_pix(infptr, TDOUBLE, fpixel, naxes[0], &dnan, ibuffer[j], &nullcnt, &status))
         {
            printf("[struct stat=\"ERROR\", msg=\"Problem reading pixels.\"]\n");
            fflush(stdout);
            exit(0);
         }

         ++fpixel[1];
      }

      if(debug)
      {
         printf("DEBUG> File %s read.\n", infile);
         fflush(stdout);
      }

      if(fits_close_file(infptr, &status))
      {
         printf("[struct stat=\"ERROR\", msg=\"Problem closing input FITS file.\"]\n");
         fflush(stdout);
         exit(0);
      }

      unlink(infile);


      // Determine the offset in the output array for this input

      jtile = tile / 27;
      itile = tile - jtile * 27;

      joffset = jtile * 64;
      ioffset = itile * 64;

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

            obuffer[jout][iout] = ibuffer[j][63-i];
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
