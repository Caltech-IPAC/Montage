#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <montage.h>

#define MAXSTR 256 


/******************************************************/
/*                                                    */
/*  mBestImage main routine                           */
/*                                                    */
/******************************************************/

int main(int argc, char **argv)
{
   int    i, debug;
   double ra, dec;

   char   tblfile [MAXSTR];
   char   statfile[MAXSTR];

   struct mBestImageReturn *returnStruct;

   FILE  *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug  = 0;

   montage_status = stdout;

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
         debug = 1;
         ++argv;
         --argc;
      }
   }

   if (argc < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mBestImages [-d] images.tbl ra dec\"]\n");
      exit(1);
   }

   strcpy(tblfile, argv[1]);

   ra  = atof(argv[2]);
   dec = atof(argv[3]);


   /******************************************/
   /* Call the mBestImage processing routine */
   /******************************************/

   returnStruct = mBestImage(tblfile, ra, dec, debug);

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
