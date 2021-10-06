#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <fitsio.h>

#include <mFitplane.h>
#include <montage.h>

#define MAXSTR  256

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);



/*************************************************************************/
/*                                                                       */
/*  mFitplane                                                            */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mFitplane, is used in conjuction with mDiff and         */
/*  mBgModel to determine how overlapping images relate to each          */
/*  other.  It is assumed that difference images have matching structure */
/*  information and that what is left when you difference them is just   */
/*  the relative offsets, slopes, etc.  By fitting the difference image, */
/*  we obtain the 'correction' that needs to be applied to one or the    */
/*  other (or in part to both) to bring them together.                   */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int   c, nofit, levelOnly, border, debug;

   char  input_file[MAXSTR];

   char *end;

   struct mFitplaneReturn *returnStruct;

   FILE *montage_status;
   

   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug     = 0;
   border    = 0;
   nofit     = 0;
   levelOnly = 0;

   opterr    = 0;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "b:d:lns:")) != EOF) 
   {
      switch (c) 
      {
         case 'b':
            border = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -b (%s) cannot be interpreted as an integer\"]\n", 
                  optarg);
               exit(1);
            }

            if(border < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -b (%s) must be a positive integer\"]\n", 
                  optarg);
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

         case 'l':
            levelOnly = 1;
            break;

         case 'n':
            nofit = 1;
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
            printf("[struct stat=\"ERROR\", msg=\"Usage: mFitplane [-b border] [-d level] [-s statusfile] [-l(evel-only)] in.fits\"]\n");
            exit(1);
            break;
      }
   }

   if (argc - optind < 1) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mFitplane [-b border] [-d level] [-s statusfile] [-l(evel-only)] in.fits\"]\n");
      exit(1);
   }

   strcpy(input_file, argv[optind]);

   returnStruct = mFitplane(input_file, nofit, levelOnly, border, debug);

   if(returnStruct->status == 1)
   {
      fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
      free(returnStruct);
      exit(1);
   }
   else
   {
      fprintf(montage_status, "[struct stat=\"OK\", module=\"mFitplane\", %s]\n", returnStruct->msg);
      free(returnStruct);
      exit(0);
   }
}
