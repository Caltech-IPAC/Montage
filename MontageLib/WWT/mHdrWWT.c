#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int  id2index  (char *id, int *level, int *index);
void splitIndex(unsigned long index, int level, int *x, int *y);


int debug = 0;

int main(int argc, char **argv)
{
   int    level, index, x, y;
   double crpix1, crpix2;

   char   tileID  [256];
   char   outFile[1024];

   FILE  *fout;


   // Process command line

   if(argc > 1 && strcmp(argv[1], "-d") == 0)
   {
      debug = 1;
      ++argv;
      --argc;
   }

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHdrWWT tileID outFile\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(tileID,  argv[1]);
   strcpy(outFile, argv[2]);


   // Convert tile ID to X and Y offsets (and get level)

   if(id2index(tileID, &level, &index))
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid character in ID string (must be in [0,1,2,3])\"]\n");
      fflush(stdout);
      exit(0);
   }

   splitIndex(index, level, &x, &y);

   y = (int)(pow(2., level)+ 0.5) - y - 1;

   crpix1 = -0.5 - x * 256;
   crpix2 = -0.5 - y * 256;


   if(debug)
   {
      printf("\nDEBUG> ID     = [%s]\n\n", tileID);

      printf("DEBUG> level  =  %d\n", level);
      printf("DEBUG> index  =  %o\n", index);
      printf("DEBUG> x      =  %d\n", x);
      printf("DEBUG> y      =  %d\n", y);

      printf("DEBUG> crpix1 = %8.1f\n", crpix1);
      printf("DEBUG> crpix2 = %8.1f\n", crpix2);
      printf("\n");
      
      fflush(stdout);
   }

   // Open the output header file

   fout = fopen(outFile, "w+");
   
   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output header file.\"]\n");
      fflush(stdout);
      exit(0);
   }


   // Create TOAST header for this tile

   fprintf(fout, "SIMPLE  =                    T\n");
   fprintf(fout, "BITPIX  =                  -64\n");
   fprintf(fout, "NAXIS   =                    2\n");
   fprintf(fout, "NAXIS1  =                  256 / All WWT tiles are 256x256 pixels.\n");
   fprintf(fout, "NAXIS2  =                  256\n");
   fprintf(fout, "CTYPE1  = 'RA---TOA'          \n");
   fprintf(fout, "CTYPE2  = 'DEC--TOA'          \n");
   fprintf(fout, "CRPIX1  =          %11.2f\n", crpix1);
   fprintf(fout, "CRPIX2  =          %11.2f\n", crpix2);
   fprintf(fout, "PV2_1   =                   %2d / HTM level for this tile.\n", level);
   fprintf(fout, "XTILE   =               %6d / X and Y tile indices.  That is, the tile\n", x);
   fprintf(fout, "YTILE   =               %6d / location in the array of tiles at this level.\n", y);
   fprintf(fout, "CDELT1  =                 1.00 / The rest of the header is really just\n");
   fprintf(fout, "CDELT2  =                 1.00 / boilerplate.  Don't let the values get\n");
   fprintf(fout, "CRVAL1  =                   0. / modified as it might affect proper\n");
   fprintf(fout, "CRVAL2  =                   0. / processing.\n");
   fprintf(fout, "PC1_1   =                 1.00\n");
   fprintf(fout, "PC1_2   =                 0.00\n");
   fprintf(fout, "PC2_1   =                 0.00\n");
   fprintf(fout, "PC2_2   =                 1.00\n");
   fprintf(fout, "END\n");

   fflush(fout);
   fclose(fout);


   // All done

   printf("[struct stat=\"OK\", module=\"mHdrWWT\", level=%d, xtile=%d, ytile=%d]\n", 
      level, x, y);

   fflush(stdout);
   exit(0);
}


/***************************************************/
/*                                                 */
/* id2index()                                      */
/*                                                 */
/* Convert a cell ID string into the equivalent    */
/* integer value.                                  */
/*                                                 */
/***************************************************/

int id2index(char *id, int *level, int *index)
{
   int i;

   *level = strlen(id);
   *index = 0;

   for(i=0; i<*level; ++i)
   {
      *index *= 4;

      if(id[i] == '0')
         *index += 0;

      else if(id[i] == '1')
         *index += 1;

      else if(id[i] == '2')
         *index += 2;

      else if(id[i] == '3')
         *index += 3;

      else
         return 1;
   }

   return 0;
}


/***************************************************/
/*                                                 */
/* splitIndex()                                    */
/*                                                 */
/* Cell indices are Z-order binary constructs.     */
/* The x and y pixel offsets are constructed by    */
/* extracting the pattern made by every other bit  */
/* to make new binary numbers.                     */
/*                                                 */
/***************************************************/

void splitIndex(unsigned long index, int level, int *x, int *y)
{
   int i;
   unsigned long val;

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
