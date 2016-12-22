#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <mTranspose.h>
#include <montage.h>

#define STRLEN  1024


/******************************************************/
/*                                                    */
/*  mTranspose main routine                           */
/*                                                    */
/******************************************************/

int main(int argc, char **argv)
{
   int   i, debug;
   int   order [4];
   int   norder;

   char  inputFile [STRLEN];
   char  outputFile[STRLEN];
   char  statfile  [STRLEN];

   char *end;

   struct mTransposeReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug    = 0;
   montage_status  = stdout;

   strcpy(statfile, "");

   for(i=0; i<argc; ++i)
   {
      if(strcmp(argv[i], "-s") == 0)
      {
         if(i+1 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"No status file name given\"]\n");
            exit(1);
         }

         strcpy(statfile, argv[i+1]);

         if(strlen(statfile) > 0)
         {
            if((montage_status = fopen(statfile, "w+")) == (FILE *)NULL)
            {
               printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s", statfile);
               exit(1);
            }
         }

         argv += 2;
         argc -= 2;
      }

      if(strcmp(argv[i], "-d") == 0)
      {
         if(i+1 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"No debug level given\"]\n");
            exit(1);
         }

         debug = strtol(argv[i+1], &end, 0);

         if(end - argv[i+1] < strlen(argv[i+1]))
         {
            printf("[struct stat=\"ERROR\", msg=\"No debug level given\"]\n");
            exit(1);
         }

         if(debug < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"Debug level value cannot be negative\"]\n");
            exit(1);
         }

         if(debug > 3)
         {
            printf("[struct stat=\"ERROR\", msg=\"Debug level range is 0 to 3\"]\n");
            exit(1);
         }

         argv += 2;
         argc -= 2;
      }
   }

   if (argc >= 3 && argc < 5)
   {
      printf ("[struct stat=\"ERROR\", msg=\"You must give input/output files the output axis order list (which will always be at least two integers).\"]\n");
      exit(1);
   }

   if (argc < 5)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: mTranspose [-d level] [-s statusfile] in.fits out.fits outaxis1 outaxis2 [outaxis3 [outaxis4]]\"]\n");
      exit(1);
   }

   strcpy(inputFile, argv[1]);

   if(inputFile[0] == '-')
   {
      printf ("[struct stat=\"ERROR\", msg=\"Invalid input file '%s'\"]\n", inputFile);
      exit(1);
   }

   strcpy(outputFile, argv[2]);

   if(outputFile[0] == '-')
   {
      printf ("[struct stat=\"ERROR\", msg=\"Invalid output file '%s'\"]\n", outputFile);
      exit(1);
   }

   norder = 0;

   if(argc > 3)
   {
      order[0] = strtol(argv[3], &end, 0);

      norder = 1;

      if(end < argv[3] + (int)strlen(argv[3]))
      {
         fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Axis ID 1 cannot be interpreted as an integer.\"]\n");
         exit(1);
      }

      if(argc > 4)
      {
         order[1] = strtol(argv[4], &end, 0);

         norder = 2;

         if(end < argv[4] + (int)strlen(argv[4]))
         {
            fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Axis ID 2 cannot be interpreted as an integer.\"]\n");
            exit(1);
         }

         if(argc > 5)
         {
            order[2] = strtol(argv[5], &end, 0);

            norder = 3;

            if(end < argv[5] + (int)strlen(argv[5]))
            {
               fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Axis ID 3 cannot be interpreted as an integer.\"]\n");
               exit(1);
            }

            if(argc > 6)
            {
               order[3] = strtol(argv[6], &end, 0);

               norder = 4;

               if(end < argv[6] + (int)strlen(argv[6]))
               {
                  fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Axis ID 4 cannot be interpreted as an integer.\"]\n");
                  exit(1);
               }
            }
         }
      }
   }

   /******************************************/
   /* Call the mTranspose processing routine */
   /******************************************/

   returnStruct = mTranspose(inputFile, outputFile, norder, order, debug);

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
