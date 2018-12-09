#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mtbl.h>

#include <mFitExec.h>
#include <montage.h>

#define MAXSTR 4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*******************************************************************/
/*                                                                 */
/*  mFitExec                                                       */
/*                                                                 */
/*  After mDiffExec has been run using the table of overlaps found */
/*  by mOverlaps, use this executive to run mFitplane on each of   */
/*  the differences.  Write the fits to a file to be used by       */
/*  mBModel.                                                       */
/*                                                                 */
/*******************************************************************/

int main(int argc, char **argv)
{
   int    ch, levelOnly, debug;

   char   tblfile [MAXSTR];
   char   fitfile [MAXSTR];
   char   diffdir [MAXSTR];

   struct mFitExecReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug     = 0;
   levelOnly = 0;

   opterr = 0;

   montage_status = stdout;

   while ((ch = getopt(argc, argv, "dls:")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 'l':
            levelOnly = 1;
            break;

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-l(evel-only)] [-s statusfile] diffs.tbl fits.tbl diffdir\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-l(evel-only)] [-s statusfile] diffs.tbl fits.tbl diffdir\"]\n", argv[0]);
      exit(1);
   }

   strcpy(tblfile, argv[optind]);
   strcpy(fitfile, argv[optind + 1]);
   strcpy(diffdir, argv[optind + 2]);


   /****************************************/
   /* Call the mFitExec processing routine */
   /****************************************/

   returnStruct = mFitExec(tblfile, fitfile, diffdir, levelOnly, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mFitExec\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
