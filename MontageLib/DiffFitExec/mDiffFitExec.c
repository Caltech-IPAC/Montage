#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mtbl.h>

#include <mDiffFitExec.h>
#include <montage.h>

#define MAXSTR 4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*******************************************************************/
/*                                                                 */
/*  mDiffFitExec                                                   */
/*                                                                 */
/*  This routine combines the mDiff and mFit functionality and     */
/*  optionally discards the difference images as it goes (to       */
/*  minimize the file space needed).   It uses the table of        */
/*  oerlaps found by mOverlaps, running mDiff, then mFitplane      */
/*  on the difference images.  These fits are written to an        */
/*  output file which is then used by mBgModel.                    */
/*                                                                 */
/*******************************************************************/

int main(int argc, char **argv)
{
   int    ch, keepAll, noAreas, levelOnly, debug;

   char   template[MAXSTR];
   char   tblfile [MAXSTR];
   char   fitfile [MAXSTR];
   char   diffdir [MAXSTR];
   char   path    [MAXSTR];

   struct mDiffFitExecReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug   = 0;
   noAreas = 0;
   keepAll = 0;

   levelOnly = 0;

   strcpy(path, "");

   opterr = 0;

   montage_status = stdout;

   while ((ch = getopt(argc, argv, "klnp:ds:")) != EOF) 
   {
      switch (ch) 
      {
         case 'k':
            keepAll = 1;
            break;

         case 'l':
            levelOnly = 1;
            break;

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
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-l(evel-only)] [-n(o-areas)] [-p projdir] [-s statusfile] diffs.tbl template.hdr diffdir fits.tbl\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 4) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-l(evel-only)] [-n(o-areas)] [-p projdir] [-s statusfile] diffs.tbl template.hdr diffdir fits.tbl\"]\n", argv[0]);
      exit(1);
   }

   strcpy(tblfile,  argv[optind]);
   strcpy(template, argv[optind + 1]);
   strcpy(diffdir,  argv[optind + 2]);
   strcpy(fitfile,  argv[optind + 3]);


   /********************************************/
   /* Call the mDiffFitExec processing routine */
   /********************************************/

   returnStruct = mDiffFitExec(path, tblfile, template, diffdir, fitfile, keepAll, levelOnly, noAreas, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mDiffFitExec\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
