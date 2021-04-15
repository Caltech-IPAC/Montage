#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <mBgModel.h>
#include <montage.h>

#define MAXSTR  256

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int debugCheck(char *debugStr);


int main(int argc, char **argv)
{
   int     c, debug, mode, zones, useall, niteration;

   char    imgfile[MAXSTR];
   char    fitfile[MAXSTR];
   char    corrtbl[MAXSTR];
   char    gapdir [MAXSTR];

   char   *end;

   struct mBgModelReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug      = 0;
   mode       = 0;
   zones      = 0;
   useall     = 0;
   niteration = 10000;

   opterr = 0;

   strcpy(gapdir, "");

   montage_status = stdout;

   while ((c = getopt(argc, argv, "ag:i:r:s:ltd:z")) != EOF) 
   {
      switch (c) 
      {
         case 'a':
            useall = 1;
            break;

         case 'i':
            niteration = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument for -i (%s) cannot be interpreted as an integer\"]\n", 
                  optarg);
               exit(1);
            }

            if(niteration < 1)
            {
               printf ("[struct stat=\"ERROR\", msg=\"Number of iterations too small (%d). This parameter is normally around 5000.\"]\n", niteration);
               exit(1);
            }

            break;

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         case 'g':
            strcpy(gapdir, optarg);
            break;

         case 't':
            mode = 2;
            break;

         case 'l':
            mode = 1;
            break;

         case 'z':
            zones = 1;
            break;

         case 'd':
            debug = montage_debugCheck(optarg);

            if(debug < 0)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Invalid debug level.\"]\n");
                exit(1);
            }
            break;

         default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-i niter] [-t(oggle between background and slope)] [-l(evel-only)] [-z(ones)] [-g gapdir] [-d level] [-a(ll-overlaps)] [-s statusfile] images.tbl fits.tbl corrections.tbl\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-i niter] [-t(oggle between background and slope)] [-l(evel-only)] [-z(ones)] [-g gapdir] [-d level] [-a(ll-overlaps)] [-s statusfile] images.tbl fits.tbl corrections.tbl\"]\n", argv[0]);
      exit(1);
   }

   strcpy(imgfile, argv[optind]);
   strcpy(fitfile, argv[optind + 1]);
   strcpy(corrtbl, argv[optind + 2]);

   returnStruct = mBgModel(imgfile, fitfile, corrtbl, zones, gapdir, mode, useall, niteration, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mBgModel\"]\n");
       exit(0);
   }
}
