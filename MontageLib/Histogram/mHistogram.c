#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <errno.h>

#include <fitsio.h>

#include <mHistogram.h>
#include <montage.h>

#define  MAXSTR 1024


int main(int argc, char **argv)
{
   int       i, debug;

   int       graylogpower = 0;

   char      grayfile   [1024];
   char      histfile   [1024];
   char      grayminstr  [256];
   char      graymaxstr  [256];
   char      graytype    [256];
   char      graybetastr [256];

   char     *end;

   struct mHistogramReturn *returnStruct;

   FILE *montage_status;


   /**************************/
   /* Command-line arguments */
   /**************************/

   debug          = 0;
   montage_status = stdout;

   strcpy(grayfile,   "");

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] -file in.fits minrange maxrange [logpower/gaussian/gaussian-log/asinh [asinh-beta]] -out out.hist\"]\n", argv[0]);
      exit(1);
   }

   for(i=0; i<argc; ++i)
   {
      /* DEBUG */

      if(strcmp(argv[i], "-d") == 0)
         debug = 1;
      
      /* GRAY */

      else if(strcmp(argv[i], "-file") == 0
           || strcmp(argv[i], "-gray") == 0
           || strcmp(argv[i], "-grey") == 0)
      {
         if(i+3 >= argc)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Too few arguments following -file flag\"]\n");
            fflush(stdout);
            exit(1);
         }

         strcpy(grayfile,   argv[i+1]);
         strcpy(grayminstr, argv[i+2]);
         strcpy(graymaxstr, argv[i+3]);

         strcpy(graytype, "power");

         if(i+4 < argc)
         {
            if(argv[i+4][0] == 'g')
            {
         strcpy(graytype, "gaussian");

               if(strlen(argv[i+4]) > 1 
               && (   argv[i+4][strlen(argv[i+4])-1] == 'g'
                   || argv[i+4][strlen(argv[i+4])-1] == 'l'))
                  strcpy(graytype, "gaussianlog");

               i+=1;
            }
            
            else if(argv[i+4][0] == 'a')
            {
               strcpy(graytype, "asinh");

               strcpy(graybetastr, "2s");

               if(i+5 < argc)
                  strcpy(graybetastr, argv[i+5]);
               
               i += 2;
            }
            
            else if(strcmp(argv[i+4], "lin") == 0)
            {
               strcpy(graytype, "linear");
               graylogpower = 0;
            }
            
            else if(strcmp(argv[i+4], "log") == 0)
               graylogpower = 1;

            else if(strcmp(argv[i+4], "loglog") == 0)
               graylogpower = 2;

            else
            {
               graylogpower = strtol(argv[i+4], &end, 10);
 
               if(graylogpower < 0  || end < argv[i+4] + strlen(argv[i+4]))
                  graylogpower = 0;
               else
                  i += 1;
            }
         }
         
         i += 2;
      }

      else if(strcmp(argv[i], "-out") == 0)
      {
         if(i+1 >= argc)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Too few arguments following -out flag\"]\n");
            fflush(stdout);
            exit(1);
         }

         strcpy(histfile, argv[i+1]);

         ++i;
      }
   }

   if(debug)
   {
      printf("DEBUG> grayfile        = [%s]\n", grayfile);
      printf("DEBUG> grayminstr      = [%s]\n", grayminstr);
      printf("DEBUG> graymaxstr      = [%s]\n", graymaxstr);
      printf("DEBUG> graylogpower    = [%d]\n", graylogpower);
      printf("DEBUG> graytype        = [%s]\n", graytype);
      printf("DEBUG> graybetastr     = [%s]\n", graybetastr);
      printf("\n");
      fflush(stdout);
   }


   returnStruct = mHistogram(grayfile, histfile, grayminstr, graymaxstr, graytype, graylogpower, graybetastr, debug);

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
