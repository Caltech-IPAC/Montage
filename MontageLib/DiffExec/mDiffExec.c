#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mtbl.h>

#include <mDiffExec.h>
#include <montage.h>

#define MAXSTR 4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*******************************************************************/
/*                                                                 */
/*  mDiffExec                                                      */
/*                                                                 */
/*  Read the table of overlaps found by mOverlap and rune mDiff to */
/*  generate the difference files.                                 */
/*                                                                 */
/*******************************************************************/

int main(int argc, char **argv)
{
   int    c, noAreas, debug;

   char   path    [MAXSTR];
   char   tblfile [MAXSTR];
   char   diffdir [MAXSTR];
   char   template[MAXSTR];

  struct mDiffExecReturn *returnStruct;

  FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug   = 0;
   noAreas = 0;

   strcpy(path, "");

   opterr = 0;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "np:ds:")) != EOF) 
   {
      switch (c) 
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
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
	    printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-p projdir] [-d] [-n(o-areas)] [-s statusfile] diffs.tbl template.hdr diffdir\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-p projdir] [-d] [-n(o-areas)] [-s statusfile] diffs.tbl template.hdr diffdir\"]\n", argv[0]);
      exit(1);
   }

   strcpy(tblfile,  argv[optind]);
   strcpy(template, argv[optind + 1]);
   strcpy(diffdir,  argv[optind + 2]);


   /*****************************************/
   /* Call the mDiffExec processing routine */
   /*****************************************/

   returnStruct = mDiffExec(path, tblfile, template, diffdir, noAreas, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mDiffExec\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
