#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include <mHdr.h>
#include <montage.h>

#define MAXLEN 20000


extern char *optarg;
extern int optind, opterr;

extern int getopt (int argc, char *const *argv, const char *options);


int main(int argc, char **argv)
{
   int    debug, ch;
  
   char   outfile   [MAXLEN];
   char   bandStr   [MAXLEN];
   char   band2MASS [MAXLEN];
   char   locstr    [MAXLEN];
   char   csys      [MAXLEN];

   double equinox, width, height, resolution, rotation;

   struct mHdrReturn *returnStruct;

   FILE *montage_status;

   /* Process command-line parameters */

   debug  = 0;
   opterr = 0;

   strcpy(csys, "");

   equinox    = 2000.;
   rotation   = 0.;
   resolution = 1.;
   height     = 0.;

   strcpy(band2MASS, "");

   montage_status = stdout;
   
   debug = 0;

   while ((ch = getopt(argc, argv, "ds:c:e:h:p:r:t:")) != EOF)
   {
      switch (ch)
      {
         case 'd':
            debug = 1;
            break;

         case 'c':
            strcpy(csys, optarg);
            break;

         case 'e':
            equinox = atof(optarg);
            break;

         case 'h':
            height = atof(optarg);
            break;

         case 'p':
            resolution = atof(optarg);
            break;

         case 'r':
            rotation = atof(optarg);
            break;

         case 't':
            strcpy(bandStr, optarg);

                 if(bandStr[0] == 'j') strcpy(band2MASS, "j");
            else if(bandStr[0] == 'h') strcpy(band2MASS, "h");
            else if(bandStr[0] == 'k') strcpy(band2MASS, "k");
            else if(bandStr[0] == 'J') strcpy(band2MASS, "j");
            else if(bandStr[0] == 'H') strcpy(band2MASS, "h");
            else if(bandStr[0] == 'K') strcpy(band2MASS, "k");

            else
            {
               printf("[struct stat=\"ERROR\", msg=\"If 2MASS band is given, it must be 'J', 'H', or 'K'\"]\n");
               exit(0);
            }

            break;

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
            break;
      }
   }

   if(argc - optind < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-s system] [-e equinox] [-h height(deg)] [-p pixsize(arcsec)] [-r rotation] [-t 2mass-band] object|location width(deg) outfile (object/location must be a single argument string)\"]\n", argv[0]);
      exit(0);
   }

   strcpy(locstr, argv[optind]);

   width = atof(argv[optind+1]);

   strcpy(outfile, argv[optind+2]);

   if(height == 0.)
      height = width;

   returnStruct = mHdr(locstr, width, height, csys, equinox, resolution, rotation, band2MASS, outfile, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
