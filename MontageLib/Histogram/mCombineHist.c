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

#include <mCombineHist.h>
#include <montage.h>

#define  MAXSTR 1024

int main(int argc, char **argv)
{
   int       i, ihist, nhist, debug;
   int       logpower;

   char     *end;

   char      minstr  [MAXSTR];
   char      maxstr  [MAXSTR];
   char      betastr [MAXSTR];
   char      type    [MAXSTR];
   char      outhist [MAXSTR];

   char    **inhist;
 
   struct mCombineHistReturn *returnStruct;

   FILE *montage_status;


   /**************************/
   /* Command-line arguments */
   /**************************/

   debug          = 0;
   montage_status = stdout;

   strcpy(outhist, "");

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] -stretch minrange maxrange [logpower/gaussian/gaussian-log/asinh [asinh-beta]] -out out.hist in.hist [in2.hist .. inN.hist]\"]\n", argv[0]);
      exit(1);
   }


   ihist = 1;

   strcpy(betastr, "");

   logpower = 0;

   for(i=0; i<argc; ++i)
   {
      /* DEBUG */

      if(strcmp(argv[i], "-d") == 0)
      {
         debug = 1;
         ++ihist;
      }


      /* STRETCH */

      else if(strcmp(argv[i], "-stretch") == 0)
      {
         if(i+3 >= argc)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Too few arguments following -stretch flag\"]\n");
            fflush(stdout);
            exit(1);
         }

         strcpy(minstr, argv[i+1]);
         strcpy(maxstr, argv[i+2]);

         ihist += 3;

         strcpy(type, "power");

         if(i+4 < argc)
         {
            if(argv[i+3][0] == 'g')
            {
               ++ihist;

               strcpy(type, "gaussian");

               if(strlen(argv[i+3]) > 1 
               && (   argv[i+3][strlen(argv[i+3])-1] == 'g'
                   || argv[i+3][strlen(argv[i+3])-1] == 'l'))
                  strcpy(type, "gaussianlog");

               i+=1;
            }
            
            else if(argv[i+3][0] == 'a')
            {
               strcpy(type, "asinh");

               ihist += 2;

               strcpy(betastr, "2s");

               if(i+5 < argc)
                  strcpy(betastr, argv[i+4]);
               
               i += 2;
            }
            
            else if(strcmp(argv[i+3], "lin") == 0)
            {
               strcpy(type, "linear");
               logpower = 0;
               ++ihist;
            }
            
            else if(strcmp(argv[i+3], "log") == 0)
            {
               logpower = 1;
               ++ihist;
            }

            else if(strcmp(argv[i+3], "loglog") == 0)
            {
               logpower = 2;
               ++ihist;
            }

            else
            {
               logpower = strtol(argv[i+3], &end, 10);
 
               if(logpower < 0  || end < argv[i+3] + strlen(argv[i+3]))
                  logpower = 0;
               else
                  i += 1;

               ++ihist;
            }
         }
         
         i += 2;
      }
      

      /* OUTPUT HISTOGRAM */

      else if(strcmp(argv[i], "-out") == 0)
      {
         if(i+1 >= argc)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Too few arguments following -out flag.\"]\n");
            fflush(stdout);
            exit(1);
         }

         strcpy(outhist, argv[i+1]);

         ihist += 2;

         ++i;
      }
   }

   if(strlen(outhist) == 0)
   {
      printf ("[struct stat=\"ERROR\", msg=\"No output histogram file name given.\"]\n");
      fflush(stdout);
      exit(1);
   }

   if(debug)
   {
      printf("DEBUG> minstr     = [%s]\n", minstr);
      printf("DEBUG> maxstr     = [%s]\n", maxstr);
      printf("DEBUG> betastr    = [%s]\n", betastr);
      printf("DEBUG> logpower   =  %d \n", logpower);
      printf("DEBUG> type       = [%s]\n", type);
      printf("DEBUG> outhist    = [%s]\n\n", outhist);
   }


   /* INPUT HISTOGRAM FILES */

   if(i>argc)
   {
      printf ("[struct stat=\"ERROR\", msg=\"No input histogram files given.\"]\n");
      fflush(stdout);
      exit(1);
   }

   nhist = argc - ihist;

   inhist = (char **)malloc(nhist * sizeof(char *));

   for(i=0; i<nhist; ++i)
   {
      inhist[i] = (char *)malloc(1024 * sizeof(char));
      strcpy(inhist[i], argv[ihist+i]);

      if(debug)
      {
         printf("DEBUG> inhist[%2d] = [%s]\n", i, inhist[i]);
         fflush(stdout);
      }
   }


   /* Call the main routine */

   returnStruct = mCombineHist(inhist, nhist, minstr, maxstr, type, betastr, logpower, outhist, debug);

   for(i=0; i<nhist; ++i)
      free(inhist[i]);


   /* Clean up and exit */

   free(inhist);

   if(returnStruct->status == 1)
   {
      fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
      fflush(montage_status);
      free(returnStruct);
      exit(1);
   }
   else
   {
      fprintf(montage_status, "[struct stat=\"OK\", module=\"mCombineHist\", %s]\n", returnStruct->msg);
      fflush(montage_status);
      free(returnStruct);
      exit(0);
   }
}
