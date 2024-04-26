#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#include <mArchiveList.h>
#include <montage.h>

#define STRLEN 1024


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int main(int argc, char **argv)
{
   int    debug, ch;
  
   char   survey   [STRLEN];
   char   band     [STRLEN];
   char   locstr   [STRLEN];
   char   outfile  [STRLEN];

   double width;
   double height;

   struct mArchiveListReturn *returnStruct;


   /* Process command-line parameters */

   debug = 0;

   strcpy(survey,   "");
   strcpy(band,     "");
   strcpy(locstr,   "");
   strcpy(outfile,  "");

   width  = 0.;
   height = 0.;

   while((ch = getopt(argc, argv, "df:")) != EOF)
   {
      switch(ch)
      {
         case 'd':
            debug = 1;
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mArchiveList [-d] survey band object|location width height outfile (object/location must be a single argument string)\"]\n");
            fflush(stdout);
            exit(1);
            break;
      }
   }

   if(argc-optind < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mArchiveList [-d] survey band object|location width height outfile (object/location must be a single argument string)\"]\n");
      fflush(stdout);
      exit(1);
   }

   strcpy(survey,   argv[optind+0]);
   strcpy(band,     argv[optind+1]);
   strcpy(locstr,   argv[optind+2]);

   width  = atof(argv[optind+3]);
   height = atof(argv[optind+4]);

   strcpy(outfile,  argv[optind+5]);

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
