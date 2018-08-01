#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <mProjectQL.h>
#include <montage.h>

#define MAXSTR  256

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int main(int argc, char **argv)
{
   int       c, hdu;
   int       expand;
   int       debug, fullRegion;
   int       interp, noAreas;

   double    threshold, fluxScale;
   double    fixedWeight;

   char      input_file   [MAXSTR];
   char      weight_file  [MAXSTR];
   char      output_file  [MAXSTR];
   char      template_file[MAXSTR];
   char      borderstr    [MAXSTR];

   char     *end;

   struct mProjectQLReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   threshold   = 0.0;
   fluxScale   = 1.0;
   fixedWeight = 1.0;

   debug       = 0;
   hdu         = 0;
   expand      = 0;
   fullRegion  = 0;

   interp      = NEAREST;
   noAreas     = 1;

   opterr = 0;

   strcpy(weight_file, "");
   strcpy(borderstr,   "");

   montage_status = stdout;

   while ((c = getopt(argc, argv, "ab:d:Ls:h:w:W:t:x:Xf")) != EOF) 
   {
      switch (c) 
      {
         case 'a':
            noAreas = 0;
            break;

         case 'L':
            interp = LANCZOS;
            break;

         case 'd':
            debug = montage_debugCheck(optarg);

            if(debug < 0)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Invalid debug level.\"]\n");
                exit(1);
            }
            break;

         case 'w':
            strcpy(weight_file, optarg);
            break;

         case 'W':
            fixedWeight = strtod(optarg, &end);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Fixed weight value (%s) cannot be interpreted as a real number\"]\n",
                  optarg);
               exit(1);
            }

            break;

         case 't':
            threshold = strtod(optarg, &end);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Weight threshold string (%s) cannot be interpreted as a real number\"]\n",
                  optarg);
               exit(1);
            }

            break;

         case 'x':
            fluxScale = strtod(optarg, &end);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Flux scale string (%s) cannot be interpreted as a real number\"]\n",
                  optarg);
               exit(1);
            }

            break;

         case 'X':
            expand = 1;
            break;

         case 'b':
            strcpy(borderstr, optarg);
            break;

         case 'f':
            fullRegion = 1;
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

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level][-s statusfile][-h hdu][-x scale][-w weightfile][-W fixed-weight][-t threshold][-X(expand)][-b border-string][-f(ull-region)] in.fits out.fits hdr.template\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level][-s statusfile][-h hdu][-x scale][-w weightfile][-W fixed-weight][-t threshold][-X(expand)][-b border-string][-f(ull-region)] in.fits out.fits hdr.template\"]\n", argv[0]);
      exit(1);
   }

   strcpy(input_file,    argv[optind]);
   strcpy(output_file,   argv[optind + 1]);
   strcpy(template_file, argv[optind + 2]);

   returnStruct = mProjectQL(input_file, output_file, template_file, hdu, interp,
                             weight_file, fixedWeight, threshold, borderstr,
                             fluxScale, expand, fullRegion, noAreas, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mProjectQL\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
