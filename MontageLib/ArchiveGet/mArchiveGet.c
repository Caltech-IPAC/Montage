#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <mArchiveGet.h>
#include <montage.h>

#define MAXSTR 1024


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*****************************/
/*  mArchiveGet main routine */
/*****************************/

int main(int argc, char **argv)
{
   unsigned int timeout;

   int    debug = 0;

   int    c;
   char   urlStr  [MAXSTR];
   char   fileName[MAXSTR];

   struct mArchiveGetReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   montage_status = stdout;

   debug   =   0;
   opterr  =   0;
   timeout = 300;
 
   while ((c = getopt(argc, argv, "dt:")) != EOF)
   {
      switch (c)
      {
         case 'd':
            debug = 1;
            break;

         case 't':
            timeout = atoi(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage:  %s [-d][-t timeout] remoteref localfile\"]\n",argv[0]);
            exit(0);
            break;
      }
   }

   if(argc - optind < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage:  %s [-d][-t timeout] remoteref localfile\"]\n",argv[0]);
      exit(0);
   }


   /* Get the URL */

   strcpy(urlStr, argv[optind]);


   /* Get the file name */

   strcpy(fileName, argv[optind+1]);
 
  
   /*******************************************/
   /* Call the mArchiveGet processing routine */
   /*******************************************/


   returnStruct = mArchiveGet(urlStr, fileName, timeout, debug);

   if(returnStruct->status == 1)
   {
      fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
      exit(1);
   }
   else
   {
      fprintf(montage_status, "[struct stat=\"OK\", module=\"mArchiveGet\", %s]\n", returnStruct->msg);
      exit(0);
   }
}  
