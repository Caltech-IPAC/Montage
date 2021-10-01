#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mtbl.h>

#include <montage.h>

#define MAXSTR 4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*******************************************************************/
/*                                                                 */
/*  mDiffFit                                                       */
/*                                                                 */
/*  Run mDiff immediatly followed by mFitplane and check the first */
/*  to decide whether to run the second.                           */
/*                                                                 */
/*******************************************************************/

int main(int argc, char **argv)
{
   int   ch;

   int   keep;
   int   levelOnly;
   int   noAreas;
   int   border;
   int   debug;

   char *end;

   char input_file1  [MAXSTR];
   char input_file2  [MAXSTR];
   char output_file  [MAXSTR];
   char template_file[MAXSTR];

   struct mDiffFitReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   keep      = 0;
   border    = 0;
   levelOnly = 0;
   noAreas   = 0;
   debug     = 0;

   opterr  = 0;

   montage_status = stdout;

   while ((ch = getopt(argc, argv, "ndlb:s:")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 'l':
            levelOnly = 1;
            break;

         case 'k':
            keep = 1;
            break;

         case 'n':
            noAreas = 1;
            break;

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

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
	    printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-n(o-areas)] [-k(eep-diff)] [-l(evel-only)] [-b border] [-s statusfile] in1.fits in2.fits out.fits hdr.template\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-n(o-areas)] [-k(eep-diff)] [-l(evel-only)] [-b border] [-s statusfile] in1.fits in2.fits out.fits hdr.template\"]\n", argv[0]);
      exit(1);
   }

   strcpy(input_file1,   argv[optind]);
   strcpy(input_file2,   argv[optind + 1]);
   strcpy(output_file,   argv[optind + 2]);
   strcpy(template_file, argv[optind + 3]);


   /********************************************/
   /* Call the mDiffFit processing routine */
   /********************************************/

   returnStruct = mDiffFit(input_file1, input_file2, output_file, template_file, keep, levelOnly, noAreas, border, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mDiffFit\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
