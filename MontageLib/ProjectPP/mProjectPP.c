#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <mProjectPP.h>
#include <montage.h>

#define MAXSTR 256

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int main(int argc, char **argv)
{
   int       c, hdu;
   int       expand;
   int       debug, fullRegion;

   double    threshold, fluxScale;
   double    drizzle, fixedWeight;

   char      input_file   [MAXSTR];
   char      weight_file  [MAXSTR];
   char      output_file  [MAXSTR];
   char      template_file[MAXSTR];
   char      borderstr    [MAXSTR];
   char      altout       [MAXSTR];
   char      altin        [MAXSTR];

   char     *end;

   struct mProjectPPReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   drizzle     = 1.0;
   threshold   = 0.0;
   fluxScale   = 1.0;
   fixedWeight = 1.0;

   debug       = 0;
   hdu         = 0;
   expand      = 0;
   fullRegion  = 0;

   opterr = 0;

   strcpy(weight_file, "");
   strcpy(borderstr,   "");
   strcpy(altout,      "");
   strcpy(altin,       "");

   montage_status = stdout;

   while ((c = getopt(argc, argv, "z:d:s:b:h:w:W:t:x:Xfi:o:")) != EOF) 
   {
      switch (c) 
      {
         case 'z':
            drizzle = strtod(optarg, &end);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Drizzle factor string (%s) cannot be interpreted as a real number\"]\n", 
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

         case 'i':
            strcpy(altin, optarg);
            break;

         case 'o':
            strcpy(altout, optarg);
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

         case 'f':
            fullRegion = 1;
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-z factor][-d level][-b border][-s statusfile][-o altout.hdr][-i altin.hdr][-h hdu][-x scale][-w weightfile][-W fixed-weight][-t threshold][-X(expand)][-b border-string] in.fits out.fits template.hdr\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-z factor][-d level][-b border][-s statusfile][-o altout.hdr][-i altin.hdr][-h hdu][-x scale][-w weightfile][-W fixed-weight][-t threshold][-X(expand)][-b border-string] in.fits out.fits template.hdr\"]\n", argv[0]);
      exit(1);
   }

   strcpy(input_file,    argv[optind]);
   strcpy(output_file,   argv[optind + 1]);
   strcpy(template_file, argv[optind + 2]);


   returnStruct = mProjectPP(input_file, hdu, output_file, template_file,
                             weight_file, fixedWeight, threshold, borderstr,
                             altin, altout, drizzle, fluxScale,
                             expand, fullRegion, debug);

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
