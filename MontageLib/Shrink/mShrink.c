#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <mShrink.h>
#include <montage.h>

#define MAXSTR  256

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int main(int argc, char **argv)
{
   int       c, debug, hdu, fixedSize;
   double    shrinkFactor, cdelt;

   char      input_file [MAXSTR];
   char      output_file[MAXSTR];

   char     *end;

   struct mShrinkReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug     = 0;
   fixedSize = 0;
   hdu       = 0;

   cdelt = 0.;

   opterr = 0;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "d:p:h:s:f")) != EOF) 
   {
      switch (c) 
      {
         case 'p':
            cdelt = atof(optarg);

            if(cdelt <= 0.)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Replacement pixel scale must be positive number.\"]\n");
                exit(1);
            }
            break;

         case 'd':
            debug = montage_debugCheck(optarg);

            if(debug < 0)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Invalid debug level.\"]\n");
                exit(1);
            }
            break;

         case 'h':
            hdu = strtol(optarg, &end, 10);

            if(end < optarg + strlen(optarg) || hdu < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"HDU value (%s) must be a non-negative integer\"]\n",
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

         case 'f':
            fixedSize = 1;
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-f(ixed-size)] [-p pixel-scale] [-d level] [-h hdu] [-s statusfile] in.fits out.fits factor\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-f(ixed-size)] [-d level] [-p pixel-scale] [-h hdu] [-s statusfile] in.fits out.fits factor\"]\n", argv[0]);
      exit(1);
   }
  
   strcpy(input_file,    argv[optind]);
   strcpy(output_file,   argv[optind + 1]);

   shrinkFactor = strtod(argv[optind + 2], &end);

   if(end < argv[optind + 2] + strlen(argv[optind + 2]))
   {
      printf("[struct stat=\"ERROR\", msg=\"Shrink factor (%s) cannot be interpreted as an real number\"]\n",
         argv[optind + 2]);
      exit(1);
   }

   returnStruct = mShrink(input_file, output_file, shrinkFactor, cdelt, hdu, fixedSize, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mShrink\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
