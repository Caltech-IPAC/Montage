/* Module: mFitExec.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.11     John Good        29Aug15  Increase plus/minus column size; some people
                                   have a lot of images
1.10     John Good        29Mar08  Add 'level only' capability
1.9      Daniel S. Katz   25Jan07  Fixing some small bugs in the MPI version
                                   that were inadvertently introduced after 1.8
1.8      Daniel S. Katz   15Dec04  Added optional parallel roundrobin
                                   computation
1.7      John Good        03Aug04  Added count for mFitplane WARNINGs
1.6      John Good        20Apr04  Added processing for box fit parameters
1.5      John Good        04Mar03  Added handling for pixel count
1.4      John Good        25Nov03  Added extern optarg references
1.3      John Good        25Aug03  Added status file processing
1.2      John Good        24Mar03  Checked for svc_run() abort (no executable)
1.1      John Good        14Mar03  Modified command-line processing
                                   to use getopt() library. Return error
                                   if mFitplane not in path.  Checks for
                                   invalid diffs.tbl file.
1.0      John Good        29Jan03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <mtbl.h>

#include <mFitExec.h>
#include <montage.h>

#define MAXSTR 4096

int mFitExec_debug;


/*-*****************************************************************/
/*                                                                 */
/*  mFitExec                                                       */
/*                                                                 */
/*  After mDiffExec has been run using the table of overlaps found */
/*  by mOverlaps, use this executive to run mFitplane on each of   */
/*  the differences.  Write the fits to a file to be used by       */
/*  mBModel.                                                       */
/*                                                                 */
/*   char *tblfile     Table file list of images to fit.           */
/*   char *fitfile     Table file for output difference fits info. */
/*   char *diffdir     Directory for temporary output diff files.  */
/*   int levelOnly     Flag to fit level of diff only, not slopes. */
/*   int debug         Debug flag.                                 */
/*                                                                 */
/*******************************************************************/

struct mFitExecReturn *mFitExec(char *tblfile, char *fitfile, char *diffdir, int levelOnly, int debugin)
{
   int    stat, ncols, count, failed;
   int    warning, missing;

   int    icntr1;
   int    icntr2;
   int    idiffname;

   int    cntr1;
   int    cntr2;

   char   diffname[MAXSTR];

   double a;
   double b;
   double c;
   double crpix1;
   double crpix2;
   int    xmin;
   int    xmax;
   int    ymin;
   int    ymax;
   double xcenter;
   double ycenter;
   double npixel;
   double rms;
   double boxx;
   double boxy;
   double boxwidth;
   double boxheight;
   double boxangle;

   FILE   *fout;


   struct mFitExecReturn  *returnStruct;
   struct mFitplaneReturn *fitplane;
   
   returnStruct = (struct mFitExecReturn *)malloc(sizeof(struct mFitExecReturn));
   
   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mFitExec_debug = debugin;

   fout = fopen(fitfile, "w+");

   if(fout == (FILE *)NULL)
   {
      strcpy(returnStruct->msg, "Can't open output file.");
      return returnStruct;
   }


   /***************************************/ 
   /* Open the difference list table file */
   /***************************************/ 

   ncols = topen(tblfile);

   if(ncols <= 0)
   {
      sprintf(returnStruct->msg, "Invalid diffs metadata file: %s", tblfile);
      fclose(fout);
      return returnStruct;
   }

   icntr1    = tcol( "cntr1");
   icntr2    = tcol( "cntr2");
   idiffname = tcol( "diff");

   if(icntr1    < 0
   || icntr2    < 0
   || idiffname < 0)
   {
      strcpy(returnStruct->msg, "Need columns: cntr1 cntr2 diff");
      fclose(fout);
      return returnStruct;
   }


   /***************************************/ 
   /* Read the records and call mFitplane */
   /***************************************/ 

   count   = 0;
   failed  = 0;
   warning = 0;
   missing = 0;

   fprintf(fout, "|%9s|%9s|%16s|%16s|%16s|%14s|%14s|%10s|%10s|%10s|%10s|%13s|%13s|%13s|%16s|%16s|%16s|%16s|%16s|%16s|\n",
      "plus", "minus", "a", "b", "c", "crpix1", "crpix2", "xmin", "xmax", "ymin", "ymax", "xcenter", "ycenter", "npixel", "rms", "boxx", "boxy", "boxwidth", "boxheight", "boxang");
   fflush(fout);

   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      cntr1 = atoi(tval(icntr1));
      cntr2 = atoi(tval(icntr2));

      strcpy(diffname, montage_filePath(diffdir, tval(idiffname)));

      if(montage_checkFile(diffname))
      {
         ++count;
         ++missing;
         continue;
      }

      fitplane = mFitplane(diffname, 0, levelOnly, 0., 0);

      if(mFitExec_debug)
      {
         printf("mFitplane(%s) -> [%s]\n", diffname, fitplane->msg);
         fflush(stdout);
      }

      if(fitplane->status)
         ++failed;
      else
      {
         a         = fitplane->a;
         b         = fitplane->b;
         c         = fitplane->c;
         crpix1    = fitplane->crpix1;
         crpix2    = fitplane->crpix2;
         xmin      = fitplane->xmin;
         xmax      = fitplane->xmax;
         ymin      = fitplane->ymin;
         ymax      = fitplane->ymax;
         xcenter   = fitplane->xcenter;
         ycenter   = fitplane->ycenter;
         npixel    = fitplane->npixel;
         rms       = fitplane->rms;
         boxx      = fitplane->boxx;
         boxy      = fitplane->boxy;
         boxwidth  = fitplane->boxwidth;
         boxheight = fitplane->boxheight;
         boxangle  = fitplane->boxang;

         fprintf(fout, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13.0f %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f \n",
               cntr1, cntr2, a, b, c, crpix1, crpix2, xmin, xmax, ymin, ymax,
               xcenter, ycenter, npixel, rms, boxx, boxy, boxwidth, boxheight, boxangle);
         fflush(fout);
      }

      free(fitplane);

      ++count;
   }


   /* Finish up */

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d, failed=%d, warning=%d, missing=%d",
      count, failed, warning, missing);

   sprintf(returnStruct->json, "{\"count\":%d, \"failed\":%d, \"warning\":%d, \"missing\":%d}",
      count, failed, warning, missing);

   returnStruct->count   = count;
   returnStruct->failed  = failed;
   returnStruct->warning = warning;
   returnStruct->missing = missing;

   return returnStruct;
}
