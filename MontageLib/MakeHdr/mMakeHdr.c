#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <boundaries.h>

#include <mMakeHdr.h>
#include <montage.h>

#define MAXSTR   4096


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);



int main(int argc, char **argv)
{
   int     debug, c;
   int     northAligned;

   int     pad, isPercentage, maxPixel;
   double  pixelScale, equinox;

   char    tblfile  [MAXSTR];
   char    template [MAXSTR];
   char    csys     [MAXSTR];

   char   *end;

   struct mMakeHdrReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug        = 0;
   pixelScale   = 0.;
   northAligned = 0;
   pad          = 0;
   isPercentage = 0;
   maxPixel     = 0;

   opterr       = 0;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "nd:e:s:P:p:")) != EOF) 
   {
      switch (c) 
      {
         case 'n':
            northAligned = 1;
            break;

         case 'd':
            debug = montage_debugCheck(optarg);

            if(debug < 0)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Invalid debug level.\"]\n");
                exit(1);
            }
            break;

         case 'e':
            pad = atoi(optarg);

            if(pad < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Invalid pad string: %s\"]\n",
                  optarg);
               exit(1);
            }

            if(strstr(optarg, "%"))
               isPercentage = 1;

            break;

         case 'P':
            maxPixel = atoi(optarg);

            if(maxPixel < 0)
               maxPixel = 0;

            break;

         case 'p':
            pixelScale = atof(optarg);

            if(pixelScale <=0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Invalid pixel scale string: %s\"]\n",
                  optarg);
               exit(1);
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
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-s statusfile] [-p(ixel-scale) cdelt | -P maxpixel] [-e edgepixels] [-n] images.tbl template.hdr [system [equinox]] (where system = EQUJ|EQUB|ECLJ|ECLB|GAL|SGAL)\"]\n", argv[0]);
            fflush(stdout);
            exit(1);
            break;
      }
   }

   if (argc - optind < 2) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-s statusfile] [-p(ixel-scale) cdelt | -P maxpixel] [-e edgepixels] [-n] images.tbl template.hdr [system [equinox]] (where system = EQUJ|EQUB|ECLJ|ECLB|GAL|SGAL)\"]\n", argv[0]);
      fflush(stdout);
      exit(1);
   }

   strcpy(tblfile,  argv[optind]);
   strcpy(template, argv[optind + 1]);


   strcpy(csys, "EQUJ");

   equinox = 2000.;

   if (argc - optind > 2) 
      strcpy(csys, argv[optind + 2]);


   if (argc - optind > 3) 
   {
      equinox = strtod(argv[optind + 3], &end);

      if(end < argv[optind + 3] + strlen(argv[optind + 3]))
      {
         printf("[struct stat=\"ERROR\", msg=\"Equinox string is not a number\"]\n");
         exit(1);
      }
   }

   bndSetDebug(debug);


   returnStruct = mMakeHdr(tblfile, template, csys, equinox, pixelScale, northAligned, pad, isPercentage, maxPixel, debug);

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
