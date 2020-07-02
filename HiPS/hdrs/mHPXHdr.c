#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


int main(int argc, char **argv)
{
   int level, nside, naxis;

   double cdelt, cos45;

   char hdrfile[1024];

   FILE *fhdr;

   int debug = 0;


   // Command-line arguments

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXHdr [-d] level hdrfile\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(strcmp(argv[1], "-d") == 0)
   {
      debug = 1;
      ++argv;
      --argc;
   }

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXHdr [-d] level hdrfile\"]\n");
      fflush(stdout);
      exit(0);
   }

   level  = atoi(argv[1]);

   level = level + 9;

   strcpy(hdrfile, argv[2]);

   fhdr = fopen(hdrfile, "w+");

   if(fhdr == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: Cannot open output header file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   nside = pow(2., (double)level);

   naxis = 5 * nside;

   cos45 = sqrt(2.0)/2.0;

   cdelt = -90.0 / nside / sqrt(2.0);

   fprintf(fhdr, "SIMPLE  = T\n");
   fprintf(fhdr, "BITPIX  = -64\n");
   fprintf(fhdr, "NAXIS   = 2\n");
   fprintf(fhdr, "NAXIS1  = %d\n", naxis);
   fprintf(fhdr, "NAXIS2  = %d\n", naxis);
   fprintf(fhdr, "CRPIX1  = %.1f\n", (naxis + 1.)/2.);
   fprintf(fhdr, "CRPIX2  = %.1f\n", (naxis + 1.)/2.);
   fprintf(fhdr, "PC1_1   = %.8f\n",  cos45);
   fprintf(fhdr, "PC1_2   = %.8f\n",  cos45);
   fprintf(fhdr, "PC2_1   = %.8f\n", -cos45);
   fprintf(fhdr, "PC2_2   = %.8f\n",  cos45);
   fprintf(fhdr, "CDELT1  = %.8f\n",  cdelt);
   fprintf(fhdr, "CDELT2  = %.8f\n", -cdelt);
   fprintf(fhdr, "CTYPE1  = 'GLON-HPX'\n");
   fprintf(fhdr, "CTYPE2  = 'GLAT-HPX'\n");
   fprintf(fhdr, "CRVAL1  = 0.0\n");
   fprintf(fhdr, "CRVAL2  = 0.0\n");
   fprintf(fhdr, "LONPOLE = 180.0\n");
   fprintf(fhdr, "PV2_1   = 4\n");
   fprintf(fhdr, "PV2_2   = 3\n");
   fprintf(fhdr, "END\n");

   fflush(fhdr);
   fclose(fhdr);

   printf("[struct stat=\"OK\", module=\"mHPXHdr\", level=%d, pixlevel=%d, cdelt=%.8f, naxis=%d]\n",
      level-9, level, -cdelt, naxis);
   fflush(stdout);
   exit(0);
} 
