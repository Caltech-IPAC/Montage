#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "montage.h"
#include "mGetHdr.h"

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);



/*************************************************************************/
/*                                                                       */
/*  mGetHdr                                                              */
/*                                                                       */
/*  This program extracts the FITS header from an image into a text file */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int  debug, htmlMode;
   int  c, hdu;

   char infile [1024];
   char hdrfile[1024];
   
   char *end;

   struct mGetHdrReturn *returnStruct;

   FILE *montage_status;


   debug    = 0;
   opterr   = 0;
   hdu      = 0;
   htmlMode = 0;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "ds:h:")) != EOF) 
   {
      switch (c) 
      {
         case 'd':
            debug = 1;
            break;

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
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

         case 'H':
            htmlMode = 1;
            break;

         default:
	    printf("[struct stat=\"ERROR\", msg=\"Usage: mGetHdr [-d][-H][-h hdu][-s statusfile] img.fits img.hdr\"]\n");
            exit(1);
            break;
      }
   }

   if (argc - optind < 2) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-h hdu][-s statusfile] img.fits img.hdr\"]\n", argv[0]);
      exit(1);
   }

   strcpy(infile,  argv[optind]);
   strcpy(hdrfile, argv[optind + 1]);

   returnStruct = mGetHdr(infile, hdu, hdrfile, htmlMode, debug);

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
