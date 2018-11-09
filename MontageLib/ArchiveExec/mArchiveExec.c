#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAXSTR  4096

#include <mArchiveExec.h>
#include <montage.h>

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*******************************************************************/
/*                                                                 */
/*  mArchiveExec                                                   */
/*                                                                 */
/*  Reads a listing archive images and calls mArchiveGet to get    */
/*  each one.                                                      */
/*                                                                 */
/*******************************************************************/

int main(int argc, char **argv)
{
   int  c, opterr, debug, timeout, nrestart;

   char tblfile[MAXSTR];
   char path   [MAXSTR];

struct mArchiveExecReturn *returnStruct;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug      = 0;
   opterr     = 0;
   timeout    = 0;
   nrestart   = 0;

   strcpy(path, ".");

   if(debug)
   {
      printf("DEBUGGING OUTPUT\n\n");
      fflush(stdout);
   }

   while ((c = getopt(argc, argv, "d:p:r:t:")) != EOF)
   {
      switch (c)
      {
         case 'd':
            debug = atoi(optarg);
            break;

         case 'p':
            strcpy(path, optarg);
            break;

         case 't':
            timeout = atoi(optarg);
            break;

         case 'r':
            nrestart = atoi(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mArchiveExec [-d level][-p outputdir][-r startrec][-t timeout] region.tbl\"]\n");
            exit(0);
            break;
      }
   }

   if(argc - optind < 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mArchiveExec [-d level][-p outputdir][-r startrec][-t timeout] region.tbl\"]\n");
      exit(1);
   }

   strcpy(tblfile, argv[optind]);


   /********************************************/
   /* Call the mArchiveExec processing routine */
   /********************************************/

   returnStruct = mArchiveExec(tblfile, path, nrestart, timeout, debug);

   if(returnStruct->status == 1)
   {
       printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       printf("[struct stat=\"OK\", module=\"mArchiveExec\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
