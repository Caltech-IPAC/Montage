#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#include <mArchiveList.h>
#include <montage.h>

#define STRLEN 1024


int main(int argc, char **argv)
{
   int    debug;
  
   char   survey   [STRLEN];
   char   band     [STRLEN];
   char   locstr   [STRLEN];
   char   outfile  [STRLEN];

   double width;
   double height;

   struct mArchiveListReturn *returnStruct;


   /* Process command-line parameters */

   debug = 0;

   if(argc > 2 && strcmp(argv[1], "-d") == 0)
   {
      debug = 1;

      --argc;
      ++argv;
   }

   if(argc < 7)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mArchiveList [-d] survey band object|location width height outfile (object/location must be a single argument string)\"]\n");
      exit(0);
   }

   strcpy(survey,   argv[1]);
   strcpy(band,     argv[2]);
   strcpy(locstr,   argv[3]);
   strcpy(outfile,  argv[6]);

   width  = atof(argv[4]);
   height = atof(argv[5]);

   returnStruct = mArchiveList(survey, band, locstr, width, height, outfile, debug);

   if(returnStruct->status == 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
      exit(1);
   }
   else
   {
      printf("[struct stat=\"OK\", module=\"mArchiveList\", %s]\n", returnStruct->msg);
      exit(0);
   }
}
