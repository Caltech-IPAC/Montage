#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <mTileImage.h>
#include <montage.h>

#define STRLEN  1024

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int main(int argc, char **argv) 
{
   int  debug;
   int  hdu;
   int  c;

   int  nx, ny, xpad, ypad;

   char input_file [1024];
   char output_base[1024];

   char *end;

   struct mTileImageReturn *returnStruct;

   FILE *montage_status;


   /* Process basic command-line arguments */

   debug = 0;
   hdu   = 0;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "dh:")) != EOF) 
   {
      switch (c) 
      {
         case 'd':
            debug = 1;
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

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-h hdu] in.fits outbase nx ny [xpad [ypad]]\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 4) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-h hdu] in.fits outbase nx ny [xpad [ypad]]\"]\n", argv[0]);
      exit(1);
   }

   strcpy(input_file,    argv[optind]);
   strcpy(output_base,   argv[optind + 1]);


   nx = strtol(argv[optind+2], &end, 10);

   if(end < argv[optind+2] + strlen(argv[optind+2]) || nx < 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"nx value (%s) must be an integer greater than zero\"]\n",
         argv[optind+2]);
      exit(1);
   }


   ny = strtol(argv[optind+3], &end, 10);

   if(end < argv[optind+2] + strlen(argv[optind+2]) || ny < 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"ny value (%s) must be an integer greater than zero\"]\n",
         argv[optind+2]);
      exit(1);
   }


   xpad = 0;

   if(argc - optind > 4)
   {
      xpad = strtol(argv[optind+4], &end, 10);

      if(end < argv[optind+4] + strlen(argv[optind+4]) || xpad < 1)
      {
         printf("[struct stat=\"ERROR\", msg=\"xpad value (%s) must be an integer >= zero\"]\n",
            argv[optind+4]);
         exit(1);
      }
   }


   ypad = xpad;

   if(argc - optind > 5)
   {
      ypad = strtol(argv[optind+5], &end, 10);

      if(end < argv[optind+5] + strlen(argv[optind+5]) || ypad < 1)
      {
         printf("[struct stat=\"ERROR\", msg=\"ypad value (%s) must be an integer >= zero\"]\n",
            argv[optind+5]);
         exit(1);
      }
   }


   returnStruct = mTileImage(input_file, output_base, hdu, nx, ny, xpad, ypad, debug);


   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mTileImage\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
