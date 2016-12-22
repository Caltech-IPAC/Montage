#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <montage.h>

#define MAXSTR 256


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/******************************************************/
/*                                                    */
/*  mBestImage main routine                           */
/*                                                    */
/******************************************************/

int main(int argc, char **argv)
{
   int       i, c, offset, boundaries, haveVal, nMinMax, debug;
   double    NaNvalue;
   double    minblank[256], maxblank[256];
   int       ismin[256], ismax[256];
   char     *end;
   char      input_file[MAXSTR];
   char      output_file[MAXSTR];
   FILE     *montage_status;

   struct mFixNaNReturn *returnStruct;


   /************************************************/
   /* Make a NaN value to use setting blank pixels */
   /************************************************/

   union
   {
      double d;
      char   c[8];
   }
   value;

   double nan;

   for(i=0; i<8; ++i)
      value.c[i] = 255;

   nan = value.d;

   NaNvalue = nan;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   montage_status = stdout;

   debug      = 0;
   haveVal    = 0;
   boundaries = 0;

   opterr      = 0;

   while ((c = getopt(argc, argv, "bd:v:")) != EOF)
   {
      switch (c)
      {
         case 'b':
            boundaries = 1;
            break;

         case 'd':
            debug = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"No debug level given\"]\n");
               fflush(stdout);
               exit(1);
            }

            break;

         case 'v':
            NaNvalue = strtod(argv[i+1], &end);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"NaN conversion value string is invalid: '%s'\"]\n", argv[i+1]);
               exit(1);
            }

            haveVal = 1;

            break;

         default:
            break;
      }
   }
   
   if (argc - optind < 2)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-b(oundary-check)][-d level][-v NaN-value] in.fits out.fits [minblank maxblank] (output file name '-' means no file; min/max ranges can be repeated and can be the words 'min' and 'max')\"]\n", argv[0]);
      exit(1);
   }

   strcpy(input_file, argv[optind]);

   if(input_file[0] == '-')
   {
      printf ("[struct stat=\"ERROR\", msg=\"Invalid input file '%s'\"]\n", input_file);
      exit(1);
   }

   strcpy(output_file, argv[optind+1]);

   nMinMax = 0;

   if(argc-optind-2 >= 2)   // Need at least two range arguments
   {
      while(1)
      {
         offset = optind+2+2*nMinMax;

         if(argc-offset < 2)
            break;

         ismin[nMinMax] = 0;

         if(strcmp(argv[offset], "min") == 0)
         {
            ismin[nMinMax] = 1;

            minblank[nMinMax]= 0.;
         }
         else
         {
            minblank[nMinMax] = strtod(argv[offset], &end);

            if(end < argv[offset] + strlen(argv[offset]))
            {
               printf ("[struct stat=\"ERROR\", msg=\"min blank value string is not a number\"]\n");
               exit(1);
            }
         }


         ismax[nMinMax] = 0;

         if(strcmp(argv[offset+1], "max") == 0)
         {
            ismax[nMinMax] = 1;

            maxblank[nMinMax]= 0.;
         }
         else
         {
            maxblank[nMinMax] = strtod(argv[offset+1], &end);

            if(end < argv[offset+1] + strlen(argv[offset+1]))
            {
               printf ("[struct stat=\"ERROR\", msg=\"max blank value string is not a number\"]\n");
               exit(1);
            }
         }

         ++nMinMax;
      }
   }

   if(debug >= 1)
   {
      printf("input_file       = [%s]\n", input_file);
      printf("output_file      = [%s]\n", output_file);
      printf("boundaryFlag     =  %d\n",  boundaries);
      printf("haveVal          =  %d\n",  haveVal);
      printf("nMinMax          =  %d\n",  nMinMax);

      fflush(stdout);

      for(i=0; i<nMinMax; ++i)
      {
         printf("minblank[%d]    = %-g (%d)\n",  i, minblank[i], ismin[i]);
         printf("maxblank[%d]    = %-g (%d)\n",  i, maxblank[i], ismax[i]);
      }

      fflush(stdout);
   }


   /****************************************/
   /* Call the mFixNaN processing routine  */
   /****************************************/


   returnStruct = mFixNaN(input_file, output_file, boundaries, haveVal, NaNvalue, 
                               nMinMax, minblank, ismin, maxblank, ismax, debug);

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
