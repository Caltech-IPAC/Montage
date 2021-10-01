/* Module: mDiffFit.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        25Sep21  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <mtbl.h>

#include <montage.h>

#define MAXSTR 4096

int mDiffFit_debug;

static char montage_msgstr[1024];
static char montage_json  [1024];


/*-*****************************************************************/
/*                                                                 */
/*  mDiffFit                                                       */
/*                                                                 */
/*  This routine combines the mDiff and mFit functionality and     */
/*  optionally discards the difference images as it goes (to       */
/*  minimize the file space needed).                               */
/*                                                                 */
/*   char *fname1      First image.                                */
/*   char *fname2      Second image.                               */
/*   char *diffname    Difference image (output).                  */
/*   char *template    FITS header file used to define the desired */
/*                     output.                                     */
/*                                                                 */
/*   int keep          Flag to keep temporary diff image.          */
/*   int levelOnly     Flag to fit level of diff only, not slopes. */
/*   int noAreas       Flag indicating there are no area images.   */             
/*   int border        Border to ignore in diff/fitting.           */             
/*   int debug         Debug flag.                                 */
/*                                                                 */
/*******************************************************************/

struct mDiffFitReturn *mDiffFit(char *fname1, char *fname2, char *diffname, char *template,
                                int keep, int levelOnly, int noAreas, int border, int debugin)

{
   char   tmpstr[1024];

   double a;
   double b;
   double c;
   double crpix1;
   double crpix2;
   double xmin;
   double xmax;
   double ymin;
   double ymax;
   double xcenter;
   double ycenter;
   double npixel;
   double rms;
   double boxx;
   double boxy;
   double boxwidth;
   double boxheight;
   double boxangle;

   struct mDiffFitReturn  *returnStruct;
   struct mDiffReturn     *diff;
   struct mFitplaneReturn *fitplane;

   returnStruct = (struct mDiffFitReturn *)malloc(sizeof(struct mDiffFitReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mDiffFit_debug = debugin;


   diff = mDiff(fname1, fname2, diffname, template, noAreas, 1., 0);

   if(mDiffFit_debug)
   {
      printf("mDiff(%s, %s, %s) -> [%s]\n", 
         fname1, fname2, diffname, diff->msg);
      fflush(stdout);
   }

   if(diff->status)
   {
      strcpy(montage_msgstr, diff->msg);
      return returnStruct;
   }

   free(diff);

   fitplane = mFitplane(diffname, levelOnly, border, 0);

   if(mDiffFit_debug)
   {
      printf("mFitplane(%s) -> [%s]\n", 
         diffname, fitplane->msg);
      fflush(stdout);
   }

   if(fitplane->status)
   {
      strcpy(montage_msgstr, fitplane->msg);
      return returnStruct;
   }
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
   }

   free(fitplane);


   /* Remove the diff file */

   if(!keep)
   {
      if(mDiffFit_debug)
      {
         printf("Remove [%s]\n", diffname);
         fflush(stdout);
      }

      unlink(diffname);

      if(!noAreas)
      {
         strcpy(tmpstr, diffname);

         tmpstr[strlen(tmpstr)-5] = '\0';

         strcat(tmpstr, "_area.fits");

         if(mDiffFit_debug)
         {
            printf("Remove [%s]\n", tmpstr);
            fflush(stdout);
         }

         unlink(tmpstr);
      }
   }


   /* Output fit */

   if(boxwidth == 0.)
   {
      boxx      = xmin;
      boxwidth  = 1.;
      boxangle  = 0.;
   }

   if(boxheight == 0.)
   {
      boxy      = ymin;
      boxheight = 1.;
      boxangle  = 0.;
   }

   sprintf(montage_msgstr, "a=%-g, b=%-g, c=%-g, crpix1=%-g, crpix2=%-g, xmin=%-g, xmax=%-g, ymin=%-g, ymax=%-g, xcenter=%-g, ycenter=%-g, npixel=%-g, rms=%-g, boxx=%-g, boxy=%-g, boxwidth=%-g, boxheight=%-g, boxang=%-g",
      a, b, c, crpix1, crpix2, xmin, xmax, ymin, ymax, xcenter, ycenter, npixel, rms,
      boxx, boxy, boxwidth, boxheight, boxangle);

   sprintf(montage_json, "{\"a\":%-g, \"b\":%-g, \"c\":%-g, \"crpix1\":%-g, \"crpix2\":%-g, \"xmin\":%-g, \"xmax\":%-g, \"ymin\":%-g, \"ymax\":%-g, \"xcenter\":%-g, \"ycenter\":%-g, \"npixel\":%-g, \"rms\":%-g, \"boxx\":%-g, \"boxy\":%-g, \"boxwidth\":%-g, \"boxheight\":%-g, \"boxang\":%-g}",
      a, b, c, crpix1, crpix2, xmin, xmax, ymin, ymax, xcenter, ycenter, npixel, rms,
      boxx, boxy, boxwidth, boxheight, boxangle);

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   returnStruct->a         = a;
   returnStruct->b         = b;
   returnStruct->c         = c;
   returnStruct->crpix1    = crpix1;
   returnStruct->crpix2    = crpix2;
   returnStruct->xmin      = xmin;
   returnStruct->xmax      = xmax;
   returnStruct->ymin      = ymin;
   returnStruct->ymax      = ymax;
   returnStruct->xcenter   = xcenter;
   returnStruct->ycenter   = ycenter;
   returnStruct->npixel    = npixel;
   returnStruct->rms       = rms;
   returnStruct->boxx      = boxx;
   returnStruct->boxy      = boxy;
   returnStruct->boxwidth  = boxwidth;
   returnStruct->boxheight = boxheight;
   returnStruct->boxang    = boxangle;

   return returnStruct;
}
