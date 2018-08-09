/* Module: mDiffFitExec.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.2      John Good        29Mar08  Add 'level only' capability
1.1      John Good        01Aug07  Leave more space in the output table columns
1.0      John Good        29Aug06  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <mtbl.h>

#include <mDiffFitExec.h>
#include <montage.h>

#define MAXSTR 4096

int mDiffFitExec_debug;


/*-*****************************************************************/
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
/*   char *path        Path to images to be diffed.                */
/*   char *tblfile     Table file list of images to diff.          */
/*   char *template    FITS header file used to define the desired */
/*                     output.                                     */
/*                                                                 */
/*   char *diffdir     Directory for temporary output diff files.  */
/*   char *fitfile     Table file for output difference fits info. */
/*   int keepAll       Flag to keep temporary diff images.         */
/*   int levelOnly     Flag to fit level of diff only, not slopes. */
/*   int noAreas       Flag indicating there are no area images.   */             
/*   int debug         Debug flag.                                 */
/*                                                                 */
/*******************************************************************/

struct mDiffFitExecReturn *mDiffFitExec(char *inpath, char *tblfile, char *template, char *diffdir,
                                        char *fitfile, int keepAll, int levelOnly, int noAreas, int debugin)
{
   int    stat, ncols, count, ffailed, warning, dfailed;

   int    icntr1;
   int    icntr2;
   int    ifname1;
   int    ifname2;
   int    idiffname;

   int    cntr1;
   int    cntr2;

   char   fname1  [MAXSTR];
   char   fname2  [MAXSTR];
   char   diffname[MAXSTR];
   char   rmname  [MAXSTR];
   char   path    [MAXSTR];

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

   struct mDiffFitExecReturn *returnStruct;
   struct mDiffReturn        *diff;
   struct mFitplaneReturn    *fitplane;

   returnStruct = (struct mDiffFitExecReturn *)malloc(sizeof(struct mDiffFitExecReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;


   if(inpath == (char *)NULL)
      strcpy(path, ".");
   else
      strcpy(path, inpath);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mDiffFitExec_debug = debugin;

   fout = fopen(fitfile, "w+");

   if(fout == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Can't open output file.");
      fclose(fout);
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
   ifname1   = tcol( "plus");
   ifname2   = tcol( "minus");
   idiffname = tcol( "diff");

   if(icntr1    < 0
   || icntr2    < 0
   || ifname1   < 0
   || ifname2   < 0
   || idiffname < 0)
   {
      sprintf(returnStruct->msg, "Need columns: cntr1 cntr2 plus minus diff");
      fclose(fout);
      return returnStruct;
   }


   /*********************************************/ 
   /* Read the records and call mDiff/mFitPlane */
   /*********************************************/ 

   count   = 0;
   ffailed = 0;
   warning = 0;
   dfailed = 0;

   fprintf(fout, "|   plus  |  minus  |         a      |        b       |        c       |    crpix1    |    crpix2    |   xmin   |   xmax   |   ymin   |   ymax   |   xcenter   |   ycenter   |    npixel   |      rms       |      boxx      |      boxy      |    boxwidth    |   boxheight    |     boxang     |\n");
   fflush(fout);

   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      ++count;

      cntr1 = atoi(tval(icntr1));
      cntr2 = atoi(tval(icntr2));

      strcpy(fname1,   montage_filePath(path, tval(ifname1)));
      strcpy(fname2,   montage_filePath(path, tval(ifname2)));
      strcpy(diffname, tval(idiffname));

      if(diffname[strlen(diffname)-1] != 's')
         strcat(diffname, "s");

      diff = mDiff(fname1, fname2, montage_filePath(diffdir, diffname), template, noAreas, 1., 0);

      if(mDiffFitExec_debug)
      {
         printf("mDiff(%s, %s, %s) -> [%s]\n", 
            fname1, fname2, montage_filePath(diffdir, diffname), diff->msg);
         fflush(stdout);
      }

      if(diff->status)
         ++dfailed;

      free(diff);

      fitplane = mFitplane(montage_filePath(diffdir, diffname), levelOnly, 0., 0);

      if(mDiffFitExec_debug)
      {
         printf("mFitplane(%s) -> [%s]\n", 
            montage_filePath(diffdir, diffname), fitplane->msg);
         fflush(stdout);
      }

      if(fitplane->status)
         ++ffailed;
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


      /* Remove the diff file */

      if(!keepAll)
      {
         strcpy(rmname, montage_filePath(diffdir, diffname));

         if(mDiffFitExec_debug)
         {
            printf("Remove [%s]\n", rmname);
            fflush(stdout);
         }

         unlink(rmname);

         if(!noAreas)
         {
            rmname[strlen(rmname)-5] = '\0';
            strcat(rmname, "_area.fits");

            if(mDiffFitExec_debug)
            {
               printf("Remove [%s]\n", rmname);
               fflush(stdout);
            }

            unlink(rmname);
         }
      }
   }

   fclose(fout);


   /* Finish up */

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d, diff_failed=%d, fit_failed=%d, warning=%d", 
      count, dfailed, ffailed, warning);

   sprintf(returnStruct->json, "{\"count\":%d, \"diff_failed\":%d, \"fit_failed\":%d, \"warning\":%d}",
      count, dfailed, ffailed, warning);

   returnStruct->count       = count;
   returnStruct->diff_failed = dfailed;
   returnStruct->fit_failed  = ffailed;
   returnStruct->warning     = warning;

   return returnStruct;
}
