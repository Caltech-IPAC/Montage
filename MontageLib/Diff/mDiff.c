#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <mDiff.h>
#include <montage.h>

#define MAXSTR  256


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int main(int argc, char **argv)
{
   int    c, debug;
   int    noAreas;

   char   input_file1 [MAXSTR];
   char   input_file2 [MAXSTR];
   char   template_file[MAXSTR];
   char   output_file [MAXSTR];

   double factor;

   struct mDiffReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug   = 0;
   noAreas = 0;

   opterr =  0;

   factor =  1.;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "nd:s:z:")) != EOF) 
   {
      switch (c) 
      {
         case 'n':
            noAreas = 1;
            break;

         case 'd':
            debug = montage_debugCheck(optarg);

            if(debug < 0)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Invalid debug level.\"]\n");
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

         case 'z':
            factor = atof(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-n(o-areas)] [-z factor] [-s statusfile] in1.fits in2.fits out.fits hdr.template\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 4) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-n(o-areas)] [-z factor] [-s statusfile] in1.fits in2.fits out.fits hdr.template\"]\n", argv[0]);
      exit(1);
   }

   strcpy(input_file1,   argv[optind]);
   strcpy(input_file2,   argv[optind + 1]);
   strcpy(output_file,   argv[optind + 2]);
   strcpy(template_file, argv[optind + 3]);

   returnStruct = mDiff(input_file1, input_file2, output_file, template_file, noAreas, factor, debug);

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
