/* Module: mBgExec.c

Version         Developer          Date           Change
-------         ---------------  -------  -----------------------
2.2             John Good        15May08  Bug:  When mBgExec encountered a missing
                 image, the correction parameters for the 
                 immediately subsequent image were applied
                 incorrectly.
2.1             Daniel S. Katz   27Jan07  Restored MPI functionality
2.0             John Good        06Sep06  Very large datasets caused memory
                                          problems.  Reworked logic to use 
                                          sorted image/correction lists.  
                                          Removed MPI stuff for now.
1.7             John Good        18Sep04  Added code to copy images for which
                                          there was no correction information
1.6             Daniel S. Katz   06Aug04  Added optional parallel roundrobin
                                          computation
1.5             Daniel S. Katz   04Aug04  Added check for too many images in table
1.4             John Good        17May04  Added "no areas" option
1.3             John Good        25Nov03  Added extern optarg references
1.2             John Good        25Aug03  Added status file output.
1.1             John Good        14Mar03  Added filePath() processing,
                                          -p argument, and getopt()
                                          argument processing.         Also, check
                                          for missing/invalid images table.
                                          Return error if mBackground not in
                                          path.
1.0           John Good          29Jan03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mtbl.h>

#include <mBgExec.h>
#include <montage.h>

#define MAXSTR 4096


/*-*****************************************************************/
/*                                                                 */
/*  mBgExec                                                        */
/*                                                                 */
/*  Take the background correction determined for each image and   */
/*  subtract it using mBackground.                                 */
/*                                                                 */
/*   char *path       Path to images to be background-corrected    */
/*   char *tblfile    Table file list of images to correct         */
/*   char *fitfile    Table of background levels/slopes            */
/*   char *corrdir    Directory for corrected images               */
/*   int noAreas      Flag indicating there are no area images.    */
/*   int debug        Debug flag.                                  */
/*                                                                 */
/*******************************************************************/

struct mBgExecReturn *mBgExec(char *inpath, char *tblfile, char *fitfile, char *corrdir, int noAreas, int debug)
{
   int  i, istat, ncols;
   int  count, nocorrection, failed;

   int    iid;
   int    ifname;

   int    id;
   char   file [MAXSTR];
   char   ifile[MAXSTR];
   char   ofile[MAXSTR];

   char   path [MAXSTR];

   int    icntr;
   int    ia;
   int    ib;
   int    ic;

   int    cntr, maxcntr;
   double *a;
   double *b;
   double *c;
   int    *have;

   struct stat type;

   struct mBgExecReturn     *returnStruct;
   struct mBackgroundReturn *background;


   returnStruct = (struct mBgExecReturn *)malloc(sizeof(struct mBgExecReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;


   if(inpath == (char *)NULL)
      strcpy(path, ".");
   else
      strcpy(path, inpath);


   /**********************************/ 
   /* Check to see if corrdir exists */
   /**********************************/ 

   istat = stat(corrdir, &type);

   if(istat < 0)
   {
      sprintf(returnStruct->msg, "Cannot access %s", corrdir);
      return returnStruct;
   }

   if (S_ISDIR(type.st_mode) != 1)
   {
      sprintf(returnStruct->msg, "%s is not a directory", corrdir);
      return returnStruct;
   }


   /*********************************/ 
   /* Open the image metadata file  */
   /* The first time is really just */
   /* to get max id value.          */
   /*********************************/ 

   ncols = topen(tblfile);

   if(ncols <= 0)
   {
        sprintf(returnStruct->msg, "Invalid image metadata file: %s", tblfile);
        return returnStruct;
   }

   icntr  = tcol( "cntr");
   ifname = tcol( "fname");

   if(debug)
   {
      printf("\nImage metdata table\n");
      printf("icntr   = %d\n", icntr);
      printf("ifname  = %d\n", ifname);
      fflush(stdout);
   }

   if(icntr  < 0
   || ifname < 0)
   {
      strcpy(returnStruct->msg, "Need columns: cntr and fname in image list");
      return returnStruct;
   }

   while(1)
   {
      if(tread() < 0)
         break;

      cntr = atoi(tval(icntr));

      if(cntr > maxcntr)
         maxcntr = cntr;
   }

   tclose();

   if(debug)
   {
      printf("maxcntr = %d\n", maxcntr);
      fflush(stdout);
   }



   /**************************************/ 
   /* Allocate space for the corrections */
   /**************************************/ 

   ++maxcntr;

   a = (double *)malloc(maxcntr * sizeof(double));
   b = (double *)malloc(maxcntr * sizeof(double));
   c = (double *)malloc(maxcntr * sizeof(double));

   have = (int *)malloc(maxcntr * sizeof(int));

   for(i=0; i<maxcntr; ++i)
   {
      a[i]    = 0.;
      b[i]    = 0.;
      c[i]    = 0.;
      have[i] = 0;
   }


   /******************************************/ 
   /* Open the corrections table file        */
   /******************************************/ 

   ncols = topen(fitfile);

   if(ncols <= 0)
   {
        sprintf(returnStruct->msg, "Invalid corrections  file: %s", fitfile);
        return returnStruct;
   }

   iid = tcol( "id");
   ia  = tcol( "a");
   ib  = tcol( "b");
   ic  = tcol( "c");

   if(debug)
   {
      printf("\nCorrections table\n");
      printf("iid = %d\n", iid);
      printf("ia  = %d\n", ia);
      printf("ib  = %d\n", ib);
      printf("ic  = %d\n", ic);
      printf("\n");
      fflush(stdout);
   }

   if(iid < 0
   || ia  < 0
   || ib  < 0
   || ic  < 0)
   {
      strcpy(returnStruct->msg, "Need columns: id,a,b,c in corrections file");
      return returnStruct;
   }


   /********************************/
   /* And read in the corrections. */
   /********************************/

   while(1)
   {
      if(tread() < 0)
         break;

      id = atoi(tval(iid));

      a[id] = atof(tval(ia));
      b[id] = atof(tval(ib));
      c[id] = atof(tval(ic));

      have[id] = 1;
   }

   tclose();


   /**************************/
   /* Reopen the image list  */
   /**************************/

   ncols = topen(tblfile);


   /***************************************************/ 
   /* Read through the image list.                    */
   /*                                                 */
   /* If there is no correction for an image file,    */
   /* increment 'nocorrection' and copy it unchanged. */
   /* Then run mBackground to create the corrected    */
   /* image.  If there is an image in the list for    */
   /* which we don't actually have a projected file   */
   /* (can happen if the list was created from the    */
   /* 'raw' set), increment the 'failed' count.       */
   /***************************************************/ 

   count        = 0;
   nocorrection = 0;
   failed       = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      cntr = atoi(tval(icntr));

      strcpy(file, tval(ifname));

      sprintf(ifile, "%s/%s", path,    montage_fileName(file));
      sprintf(ofile, "%s/%s", corrdir, montage_fileName(file));

      if(have[cntr] == 0)
         ++nocorrection;

      if(debug)
      {
         printf("mBackground(%s, %s, %-g, %-g, %-g)\n", 
            file, ofile, a[cntr], b[cntr], c[cntr]);
         fflush(stdout);
      }

      background = mBackground(ifile, ofile, a[cntr], b[cntr], c[cntr], noAreas, 0);

      if(background->status)
      {
         ++failed;

         if(debug)
         {
            printf("Failed.  Message: [%s]\n", background->msg);
            fflush(stdout);
         }
      }

      ++count;

      free(background);
   }

   if(debug)
   {
      printf("\nFreeing a,b,c,have arrays\n");
      fflush(stdout);
   }

   free(a);
   free(b);
   free(c);

   free(have);


   /*************/
   /* Finish up */
   /*************/

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d, nocorrection=%d, failed=%d", count, nocorrection, failed);

   sprintf(returnStruct->json, "{\"count\":%d, \"nocorrection\":%d, \"failed\":%d}",
      count, nocorrection, failed);

   returnStruct->count        = count;
   returnStruct->nocorrection = nocorrection;
   returnStruct->failed       = failed;

   return returnStruct;
}
