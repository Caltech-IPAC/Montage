#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <mtbl.h>

#include "montage.h"

#define MAXSTR  4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*******************************************************************/
/*                                                                 */
/*  mBgExec                                                        */
/*                                                                 */
/*  Take the background correction determined for each image and   */
/*  subtract it using mBackground.                                 */
/*                                                                 */
/*******************************************************************/

int main(int argc, char **argv)
{
   int  ch, debug;
   int  noAreas;

   char path      [MAXSTR];
   char tblfile   [MAXSTR];
   char fitfile   [MAXSTR];
   char corrdir   [MAXSTR];

   struct mBgExecReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug   = 0;
   noAreas = 0;

   strcpy(path, ".");

   opterr = 0;

   montage_status = stdout;

   while ((ch = getopt(argc, argv, "np:s:d")) != EOF) 
   {
        switch (ch) 
        {
           case 'p':
            strcpy(path, optarg);
                break;

           case 'd':
                debug = 1;
                break;

           case 'n':
                noAreas = 1;
                break;

           case 's':
                if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
                {
                   printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                        optarg);
                   exit(1);
                }
                break;

           default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-p projdir] [-s statusfile] [-d] [-n(o-areas)] images.tbl corrections.tbl corrdir\"]\n", argv[0]);
                exit(1);
                break;
        }
   }

   if (argc - optind < 3) 
   {
        fprintf (montage_status, "[struct stat=\"ERROR\", msg=\"Usage: %s [-p projdir] [-s statusfile] [-d] [-n(o-areas)] images.tbl corrections.tbl corrdir\"]\n", argv[0]);
        exit(1);
   }

   strcpy(tblfile,  argv[optind]);
   strcpy(fitfile,  argv[optind + 1]);
   strcpy(corrdir,  argv[optind + 2]);
            

   /***************************************/
   /* Call the mBgExec processing routine */
   /***************************************/

   returnStruct = mBgExec(path, tblfile, fitfile, corrdir, noAreas, debug);

   if(returnStruct->status == 1)
   {
      fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);

      free(returnStruct);

      returnStruct = (struct mBgExecReturn *)NULL;

      exit(1);
   }
   else
   {
      fprintf(montage_status, "[struct stat=\"OK\", module=\"mBgExec\", %s]\n", returnStruct->msg);

      free(returnStruct);

      returnStruct = (struct mBgExecReturn *)NULL;

      exit(0);
   }
}
