/* Module: mFlatten.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        25Apr23  Baseline code

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

int mFlatten_debug;

static char montage_msgstr[1024];
static char montage_json  [1024];


/*-*****************************************************************/
/*                                                                 */
/*  mFlatten                                                       */
/*                                                                 */
/*  This routine combines the mFitplane and mBackground functions  */
/*  to remove a flat background from an image.                     */
/*                                                                 */
/*   char *infile      Input image.                                */
/*   char *outfile     Flattened image.                            */
/*                                                                 */
/*   int debug         Debug flag.                                 */
/*                                                                 */
/*******************************************************************/

struct mFlattenReturn *mFlatten(char *infile, char *outfile, int levelOnly, int border, int debugin)
{
   double a;
   double b;
   double c;

   struct mFlattenReturn    *returnStruct;
   struct mFitplaneReturn   *fitplane;
   struct mBackgroundReturn *background;

   returnStruct = (struct mFlattenReturn *)malloc(sizeof(struct mFlattenReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;

   mFlatten_debug = debugin;


   /***************************************/
   /* Fit a background plane to the input */
   /***************************************/

   fitplane = mFitplane(infile, 0, levelOnly, border, 0);

   if(mFlatten_debug)
   {
      printf("mFitplane(%s) -> [%s]\n", 
         infile, fitplane->msg);
      fflush(stdout);
   }

   if(fitplane->status)
   {
      strcpy(montage_msgstr, fitplane->msg);
      return returnStruct;
   }
   else
   {
      a = fitplane->a;
      b = fitplane->b;
      c = fitplane->c;
   }

   free(fitplane);

   fitplane = (struct mFitplaneReturn *)NULL;


   /**********************************************************/
   /* Subtract the background plane to generate output image */
   /**********************************************************/

   background = mBackground(infile, outfile, a, b, c, 1, 0);

   if(background->status)
   {
      strcpy(montage_msgstr, fitplane->msg);
      return returnStruct;
   }

   free(background);

   background = (struct mBackgroundReturn *)NULL;



   /* Output fit */

   sprintf(montage_msgstr, "a=%-g, b=%-g, c=%-g", a, b, c);

   sprintf(montage_json, "{\"a\":%-g, \"b\":%-g, \"c\":%-g}", a, b, c);

   returnStruct->status = 0;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   returnStruct->a = a;
   returnStruct->b = b;
   returnStruct->c = c;

   return returnStruct;
}
