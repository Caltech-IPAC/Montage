/* Module: mDiffExec.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.5      Daniel S. Katz   04Aug04  Added optional parallel roundrobin
                                   computation
1.4      John Good        16May04  Added "noAreas" option
1.3      John Good        25Nov03  Added extern optarg references
1.2      John Good        25Aug03  Added status file processing
1.1      John Good        14Mar03  Added filePath() processing,
                                   -p argument, and getopt()
                                   argument processing.  Return error
                                   if mDiff not in path.  Check for 
                                   missing/invalid diffs table or diffs
                                   directory.  
1.0      John Good        29Jan03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mtbl.h>

#include <mDiffExec.h>
#include <montage.h>

#define MAXSTR 4096

int mDiffExec_debug;


/*******************************************************************/
/*                                                                 */
/*  mDiffExec                                                      */
/*                                                                 */
/*  Read the table of overlaps found by mOverlap and rune mDiff to */
/*  generate the difference files.                                 */
/*                                                                 */
/*   char *path        Path to images to be diffed.                */
/*   char *tblfile     Table file list of images to diff.          */
/*   char *template    FITS header file used to define the desired */
/*                     output.                                     */
/*                                                                 */
/*   char *diffdir     Directory for temporary output diff files.  */
/*   int noAreas       Flag indicating there are no area images.   */
/*   int debug         Debug flag.                                 */
/*                                                                 */
/*******************************************************************/

struct mDiffExecReturn *mDiffExec(char *path, char *tblfile, char *template, char *diffdir, int noAreas, int debugin)
{
   int    istat, ncols, count, failed;

   int    ifname1;
   int    ifname2;
   int    idiffname;

   char   fname1  [MAXSTR];
   char   fname2  [MAXSTR];
   char   diffname[MAXSTR];

   struct stat type;

   struct mDiffExecReturn *returnStruct;
   struct mDiffReturn     *diff;
   
   returnStruct = (struct mDiffExecReturn *)malloc(sizeof(struct mDiffExecReturn));
   
   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mDiffExec_debug = debugin;

   montage_checkHdr(template, 1, 0);


   /**********************************/
   /* Check to see if diffdir exists */
   /**********************************/

   istat = stat(diffdir, &type);

   if(istat < 0)
   {
      sprintf(returnStruct->msg, "Cannot access %s", diffdir);
      return returnStruct;
   }

   else if (S_ISDIR(type.st_mode) != 1)
   {
      sprintf(returnStruct->msg, "%s is not a directory", diffdir);
      return returnStruct;
   }


   /***************************************/ 
   /* Open the difference list table file */
   /***************************************/ 

   ncols = topen(tblfile);

   if(ncols <= 0)
   {
      sprintf(returnStruct->msg, "Invalid image metadata file: %s", tblfile);
      return returnStruct;
   }

   ifname1   = tcol( "plus");
   ifname2   = tcol( "minus");
   idiffname = tcol( "diff");

   if(ifname1   < 0
   || ifname2   < 0
   || idiffname < 0)
   {
      strcpy(returnStruct->msg, "Need columns: plus minus diff");
      return returnStruct;
   }


   /***********************************/ 
   /* Read the records and call mDiff */
   /***********************************/ 

   count  = 0;
   failed = 0;

   while(1)
   {
      istat = tread();

      if(istat < 0)
         break;

      strcpy(fname1,   montage_filePath(path, tval(ifname1)));
      strcpy(fname2,   montage_filePath(path, tval(ifname2)));
      strcpy(diffname, tval(idiffname));

      diff = mDiff(fname1, fname2, montage_filePath(diffdir, diffname), template, noAreas, 1., 0);

      if(mDiffExec_debug)
      {
         printf("mDiff(%s, %s, %s) -> [%s]\n",
            fname1, fname2, montage_filePath(diffdir, diffname), diff->msg);
         fflush(stdout);
      }

      if(diff->status)
         ++failed;

      free(diff);

      ++count;
   }


   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d, failed=%d", count, failed);

   sprintf(returnStruct->json, "{\"count\"=%d, \"failed\"=%d}", count, failed);

   returnStruct->count   = count;
   returnStruct->failed  = failed;

   return returnStruct;
}
