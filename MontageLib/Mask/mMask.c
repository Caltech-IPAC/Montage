#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <mMask.h>
#include <montage.h>


/**************************************************************************************/
/*                                                                                    */
/*  mMask takes a set of pixel range definitions ("boxes") and an input image and     */
/*  creates a duplicate output image with those regions set to NaN.                   */
/*                                                                                    */
/*     mMask [-d][-h hdu] in.fit out.fit region.boxes                                 */
/*                                                                                    */
/*  The region.boxes ASCII file is currently very simple.  Each line has four integer */
/*  values:                                                                           */
/*                                                                                    */
/*     xmin xmax ymin ymax                                                            */
/*                                                                                    */
/*  These are inclusive; i.e., the values at the min and max are also set to NaN.     */
/*  To support oddball projections, the full header (for the HDU) is copied to the    */
/*  output file.  The datatype of the input is converted to TDOUBLE.  In theory, we   */
/*  could preserve the datatype but this opens a can of worms concerning what value   */
/*  to use for blank pixels (NaN can't be used) and getting that value added as a     */
/*  BLANK keyword in the header.                                                      */
/*                                                                                    */
/**************************************************************************************/

int main(int argc, char **argv)
{
   int       i, debug, hdu, haveHDU;

   char      infile [1024];
   char      outfile[1024];
   char      boxes  [1024];
   char      appname[1024];

   char     *end;

   struct mMaskReturn *returnStruct;

   FILE *montage_status;


   debug      = 0;
   hdu        = 0;
   haveHDU    = 0;

   montage_status = stdout;

   strcpy(appname, argv[0]);
      
   if(argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-h hdu][-s statusfile] in.fits out.fits region.boxes\"]\n", appname);
      exit(1);
   }
   
   for(i=1; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
         debug = 1;
      
      if(i<argc-1 && strncmp(argv[i], "-h", 2) == 0)
      {
         hdu = strtol(argv[i+1], &end, 10);

         if(end < argv[i+1] + strlen(argv[i+1]) || hdu < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"HDU value (%s) must be a non-negative integer\"]\n",
               argv[i+1]);
            exit(1);
         }

         haveHDU = 1;
         ++i;
      }
      
      if(i<argc-1 && strncmp(argv[i], "-s", 2) == 0)
      {
         if((montage_status = fopen(argv[i+1], "w+")) == (FILE *)NULL)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
               argv[i+1]);
            exit(1);
         }

         ++i;
      }
   }
      
   if(debug)
   {
      ++argv;
      --argc;
   }

   if(montage_status != stdout)
   {
      argv += 2;
      argc -= 2;
   }

   if(haveHDU)
   {
      argv += 2;
      argc -= 2;
   }

   if (argc < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-h hdu][-s statusfile] in.fits out.fits boxes.boxes\"]\n", appname);
      exit(1);
   }

   strcpy(infile,  argv[1]);
   strcpy(outfile, argv[2]);
   strcpy(boxes,   argv[3]);

   returnStruct = mMask(infile, outfile, boxes, hdu, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mMask\"]\n");
       exit(0);
   }
}
