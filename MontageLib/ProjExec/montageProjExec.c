/* Module: mProjExec.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
3.10     John Good        11Sep15  Incorrectly using column 0 as scale sometimes.
3.9      T. P. Robitaille 19Aug10  fixed gap issue in MPI version
3.8      Daniel S. Katz   16Jul10  fixes for MPI version
3.7      John Good        07Oct07  When using the -r flag, append to stats.tbl
3.6      John Good        06Dec06  Restructured the mTANHdr checks.  It wasn't
                                   properly catching coordinate system 
                                   differences.
3.5      John Good        01Jun06  Added support for "hdu" column in image
                                   table
3.4      John Good        21Mar06  Behaved incorrectly if mTANHdr failed
                                   (should go ahead and use mProject)
3.3      John Good        04Aug05  Added option (-X) to force reprojection
                                   of whole images
3.2      John Good        31May05  Added option flux rescaling
                                   (e.g. magnitude zero point correction)
3.1      John Good        22Feb05  Updates to output messages: double errors
                                   in one case and counts were off if restart
3.0      John Good        07Feb05  Updated logic to allow automatic selection
                                   of mTANHdr/mProjectPP processing if it is
                                   possible to do so without large errors
                                   (> 0.1 pixel).
2.1      Daniel S. Katz   16Dec04  Added optional parallel roundrobin
                                   computation
2.0      John Good        10Sep04  Changed border handling to allow polygon
                                   outline
1.10     John Good        27Aug04  Fixed restart logic (and usage message)
1.9      John Good        05Aug04  Added "restart" to usage and fixed
                                   restart error message
1.8      John Good        29Jul04  Fixed "Usage" statement text
1.7      John Good        28Jul04  Added a "restart" index flag '-s n' to
                                   allow starting back up after an error
1.6      John Good        28Jan04  Added switch to allow use of mProjectPP
1.5      John Good        25Nov03  Added extern optarg references
1.4      John Good        25Aug03  Added status file processing
1.3      John Good        25Mar03  Checked -p argument (if given) to see
                                   if it is a directory, the output directory
                                   to see if it exists and the images.tbl
                                   file to see if it exists
1.2      John Good        23Mar03  Modified output table to include mProject
                                   message string for errors
1.1      John Good        14Mar03  Added filePath() processing,
                                   -p argument, and getopt()
                                   argument processing.  Return error
                                   if mProject not in path.
1.0      John Good        29Jan03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <mtbl.h>
#include <fitsio.h>
#include <wcs.h>

#include <mProjExec.h>
#include <montage.h>

#define MAXHDR 80000
#define MAXSTR 1024

#define NEAREST 0
#define LANCZOS 1

struct WorldCoor *wcsin, *wcsout;

int mProjExec_debug;

FILE *mProjExec_fdebug;

static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mProjExec                                                            */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mProjExec, runs one of the reprojection routines        */
/*  (mProject, mProjectPP or mProjectQL) on a set of images to get them  */
/*  all into a common projection.                                        */
/*                                                                       */
/*   char  *path           Path to raw image directory                   */
/*   char  *tblfile        Table file list of raw images                 */
/*   char  *template       FITS header file used to define the desired   */
/*                         output                                        */
/*                                                                       */
/*   char  *projdir        Path to output projected image directory      */
/*   int    quickMode      Flag to force use of mProjectQL function      */
/*   int    exact          Flag to force use of mProject over mProjectPP */
/*                         function.  No-op if quickMode is used.        */
/*                                                                       */
/*   int    expand         Flag to force reprojection of all of each     */
/*                         image, even outside template region           */
/*                                                                       */
/*   int    energyMode     Pixel values are total energy rather than     */
/*                         energy density.                               */
/*                                                                       */
/*   char  *borderStr      Optional string that contains either a border */
/*                         width or comma-separated "x1,y1,x2,y2, ..."   */
/*                         pairs defining a pixel region polygon where   */
/*                         we keep only the data inside.                 */
/*                                                                       */
/*   char  *scaleCol       Table file can have columns defining scale    */
/*   char  *weightCol      and weighting values to be applied to data    */
/*                         and "area" output files.                      */
/*                                                                       */
/*   int    restart        Bulk reprojection sometimes needs to be       */
/*                         restarted part way through the list. This is  */
/*                         the record in the table to start with.        */
/*                                                                       */
/*   char  *stats          The programs keeps statistics on each         */
/*                         reprojection in this file.                    */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mProjExecReturn *mProjExec(char *path, char *tblfile, char *template, char *projdir, int quickMode,
                                  int exact, int expand, int energyMode, char *borderStr, char *scaleCol,
                                  char *weightCol, int restart, char *stats, int debugin)
{
   int    stat, ncols, count, hdu, failed, nooverlap;
   int    ifname, ihdu, iweight, iscale, inp2p, outp2p;
   int    naxes, tryAltIn, tryAltOut, wcsMatch, status;
   int    interp, noAreas, fullRegion;

   double maxerror, weight, time;
   double drizzle, threshold, fluxScale;

   char   fname      [MAXSTR];
   char   infile     [MAXSTR];
   char   outfile    [MAXSTR];
   char   weightFile [MAXSTR];
   char   hdustr     [MAXSTR];

   char   origHdr    [MAXSTR];
   char   altin      [MAXSTR];
   char   altout     [MAXSTR];

   char   msg        [MAXSTR];

   char  *inheader;

   FILE   *fout;

   fitsfile *infptr;

   struct mGetHdrReturn      *getHdr;
   struct mTANHdrReturn      *tanHdr;
   struct mProjectReturn     *project;
   struct mProjectPPReturn   *projectPP;
   struct mProjectQLReturn   *projectQL;
   struct mProjectCubeReturn *projectCube;

   struct mProjExecReturn *returnStruct;

   int    fitsstat = 0;

   inheader = malloc(MAXHDR);

   mProjExec_debug = debugin;

   mProjExec_fdebug = stdout;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mProjExecReturn *)malloc(sizeof(struct mProjExecReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");



   if(montage_checkFile(tblfile) != 0)
   {
      sprintf(returnStruct->msg, "Image metadata file (%s) does not exist", tblfile);
      free(inheader);
      return returnStruct;
   }

   if(montage_checkFile(projdir) != 2)
   {
      sprintf(returnStruct->msg, "Output directory (%s) does not exist", projdir);
      free(inheader);
      return returnStruct;
   }

   montage_checkHdr(template, 1, 0);

   sprintf(origHdr, "%s/orig.hdr",   projdir);
   sprintf(altin,   "%s/altin.hdr",  projdir);
   sprintf(altout,  "%s/altout.hdr", projdir);

   strcpy(weightFile, "");

   energyMode = 0;
   fullRegion = 0;
   noAreas    = 0;
   interp     = NEAREST;

   drizzle    = 1.;
   threshold  = 0.;

   if(strlen(stats) > 0)
   {
      if(restart > 0)
         fout = fopen(stats, "a+");
      else
         fout = fopen(stats, "w+");

      if(fout == (FILE *)NULL)
      {
         sprintf(returnStruct->msg, "Can't open output file.");
         free(inheader);
         return returnStruct;
      }
   }


   /*************************************************/
   /* Try to generate an alternate header so we can */
   /* use the fast projection                       */
   /*************************************************/

   outp2p = FAILED;

   naxes = mProjExec_readTemplate(template);

   if(naxes == 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);

      if(strlen(stats) > 0)
         fclose(fout);

      return returnStruct;
   }

   if(quickMode)
      exact = 1;

   tryAltOut = 1;

   if(exact)
      tryAltOut = 0;

   if(mProjExec_debug)
   {
      fprintf(mProjExec_fdebug, "Output wcs ptype: [%s]\n", wcsout->ptype);
      fflush(mProjExec_fdebug);
   }

   if(   strcmp(wcsout->ptype, "TAN") == 0
      || strcmp(wcsout->ptype, "SIN") == 0
      || strcmp(wcsout->ptype, "ZEA") == 0
      || strcmp(wcsout->ptype, "STG") == 0
      || strcmp(wcsout->ptype, "ARC") == 0)
   {
      tryAltOut = 0;

      outp2p = INTRINSIC;
   }

   if(tryAltOut)
   {
      // mTANHdr

      tanHdr = mTANHdr(template, altout, 5, 50, 0.01, 0, 0);

      if(mProjExec_debug)
      {
         fprintf(mProjExec_fdebug, "mTANHdr(%s) -> [%s]\n", template, tanHdr->msg);
         fflush(mProjExec_fdebug);
      }

      if(tanHdr->status)
         outp2p = FAILED;
      else
      {
         outp2p = COMPUTED;

         maxerror = 0.;

         if(tanHdr->fwdxerr > maxerror) maxerror = tanHdr->fwdxerr;
         if(tanHdr->fwdyerr > maxerror) maxerror = tanHdr->fwdyerr;
         if(tanHdr->revxerr > maxerror) maxerror = tanHdr->revxerr;
         if(tanHdr->revyerr > maxerror) maxerror = tanHdr->revyerr;

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "Using distorted TAN on output: max error = %-g\n", maxerror);
            fflush(mProjExec_fdebug);
         }

         if(maxerror > 0.1)
            outp2p = FAILED;
      }

      free(tanHdr);
   }


   /**********************************/ 
   /* Open the image list table file */
   /**********************************/ 

   ncols = topen(tblfile);

   if(ncols < 0)
   {
      sprintf(returnStruct->msg, "Error opening image list table file.");
      free(inheader);

      if(strlen(stats) > 0)
         fclose(fout);

      return returnStruct;
   }

   ihdu = tcol("hdu");

   ifname = tcol( "fname");

   if(ifname < 0)
   {
      sprintf(returnStruct->msg, "Need column fname in input");
      free(inheader);

      if(strlen(stats) > 0)
         fclose(fout);

      return returnStruct;
   }


   iweight = -1;

   if(strlen(weightCol) > 0)
   {
      iweight = tcol(weightCol);

      if(iweight < 0)
      {
         sprintf(returnStruct->msg, "Need column %s in input", weightCol);
         free(inheader);

         if(strlen(stats) > 0)
            fclose(fout);

         return returnStruct;
      }
   }


   iscale = -1;

   if(strlen(scaleCol) > 0)
   {
      iscale = tcol(scaleCol);

      if(iscale < 0)
      {
         sprintf(returnStruct->msg, "Need column %s in input", scaleCol);
         free(inheader);

         if(strlen(stats) > 0)
            fclose(fout);

         return returnStruct;
      }
   }


   /**************************************/ 
   /* Read the records and call mProject */
   /**************************************/ 

   count     = 0;
   failed    = 0;
   nooverlap = 0;

   while(1)
   {
      if(mProjExec_debug)
      {
         fprintf(mProjExec_fdebug, "\nImage %d:\n", count);
         fflush(mProjExec_fdebug);
      }

      stat = tread();

      if(stat < 0)
         break;

      hdu = 0;
      if(ihdu >= 0)
         hdu = atoi(tval(ihdu));

      ++count;

      if(count <= restart)
      {
         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "Skipping [%s]\n", montage_filePath(path, tval(ifname)));
            fflush(mProjExec_fdebug);
         }

         continue;
      }

      strcpy(infile,  montage_filePath(path, tval(ifname)));

      strcpy(outfile, projdir);

      if(outfile[strlen(outfile) - 1] != '/')
         strcat(outfile, "/");

      strcpy(hdustr, "");

      if(ihdu >= 0)
         sprintf(hdustr, "hdu%d_", hdu);

      sprintf(fname, "%s%s", hdustr, montage_fileName(tval(ifname)));

      strcat(outfile, fname);

      if(strcmp(infile, outfile) == 0)
      {
         sprintf(returnStruct->msg, "Output would overwrite input");
         free(inheader);

         if(strlen(stats) > 0)
            fclose(fout);

         return returnStruct;
      }


      /* Try to generate an alternate input header so we can */
      /* use the fast projection                             */

      fitsstat = 0;

      if(montage_checkFile(infile) != 0)
      {
         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "Image file [%s] does not exist\n", infile);
            fflush(mProjExec_fdebug);
         }

         ++failed;
         continue;
      }

      if(fits_open_file(&infptr, infile, READONLY, &fitsstat))
      {
         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "FITS open failed for [%s]\n", infile);
            fflush(mProjExec_fdebug);
         }

         ++failed;
         continue;
      }

      if(hdu > 0)
      {
         if(fits_movabs_hdu(infptr, hdu+1, NULL, &fitsstat))
         {
            if(mProjExec_debug)
            {
               fprintf(mProjExec_fdebug, "FITS move to HDU failed for [%s]\n", infile);
               fflush(mProjExec_fdebug);
            }

            ++failed;
            continue;
         }
      }

      if(fits_get_image_wcs_keys(infptr, &inheader, &fitsstat))
      {
         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "FITS get WCS keys failed for [%s]\n", infile);
            fflush(mProjExec_fdebug);
         }

         ++failed;
         continue;
      }

      if(fits_close_file(infptr, &fitsstat))
      {
         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "FITS close failed for [%s]\n", infile);
            fflush(mProjExec_fdebug);
         }

         ++failed;
         continue;
      }

      wcsin = wcsinit(inheader);

      if(wcsin == (struct WorldCoor *)NULL)
      {
         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "WCS init failed for [%s]\n", infile);
            fflush(mProjExec_fdebug);
         }

         ++failed;
         continue;
      }

      inp2p = FAILED;
      
      tryAltIn = 1;

      if(exact)
         tryAltIn = 0;
      
      wcsMatch = 1;

      if(wcsin->syswcs != wcsout->syswcs)
      {
         tryAltIn = 0;
         wcsMatch = 0;
      }

      if(mProjExec_debug)
      {
         fprintf(mProjExec_fdebug, "Input wcs ptype: [%s]\n", wcsin->ptype);
         fflush(mProjExec_fdebug);
      }

      if(   strcmp(wcsin->ptype, "TAN") == 0
         || strcmp(wcsin->ptype, "SIN") == 0
         || strcmp(wcsin->ptype, "ZEA") == 0
         || strcmp(wcsin->ptype, "STG") == 0
         || strcmp(wcsin->ptype, "ARC") == 0)
      {
         tryAltIn = 0;

         inp2p = INTRINSIC;
      }

      if(tryAltIn)
      {
         getHdr = mGetHdr(infile, origHdr, hdu, 0, 0);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mGetHdr(%s) -> [%s]\n", infile, getHdr->msg);
            fflush(mProjExec_fdebug);
         }
            
         if(getHdr->status)
         {
            ++failed;
            continue;
         }

         free(getHdr);


         // mTANHdr

         tanHdr = mTANHdr(origHdr, altout, 5, 50, 0.01, 0, 0);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mTANHdr() -> [%s]\n", tanHdr->msg);
            fflush(mProjExec_fdebug);
         }

         if(tanHdr->status)
         {
            inp2p = FAILED;

            ++failed;
            continue;
         }
         else
         {
            inp2p = COMPUTED;

            maxerror = 0.;

            if(tanHdr->fwdxerr > maxerror) maxerror = tanHdr->fwdxerr;
            if(tanHdr->fwdyerr > maxerror) maxerror = tanHdr->fwdyerr;
            if(tanHdr->revxerr > maxerror) maxerror = tanHdr->revxerr;
            if(tanHdr->revyerr > maxerror) maxerror = tanHdr->revyerr;

            if(mProjExec_debug)
            {
               fprintf(mProjExec_fdebug, "Using distorted TAN on input: max error = %-g\n", maxerror);
               fflush(mProjExec_fdebug);
            }

            if(maxerror > 0.1)
               inp2p = FAILED;
         }

         free(tanHdr);
      }


      /* Now run mProject, mProjectPP, mProjectQL or mProjectCube */
      /* (depending on what we have to work with)                 */

      weight = 1.;

      if(iweight >= 0)
      {
         weight = atof(tval(iweight));

         if(weight == 0.)
            weight = 1;
      }


      fluxScale = 1.;

      if(iscale >= 0)
      {
         fluxScale = atof(tval(iscale));

         if(fluxScale == 0.)
            fluxScale = 1;
      }


      if(exact && (inp2p != INTRINSIC || outp2p != INTRINSIC))
      {
         inp2p  = FAILED;
         outp2p = FAILED;
      }

      if(naxes > 2)
      {
         projectCube = mProjectCube(infile, outfile, template, hdu, weightFile, weight, threshold,
                                    drizzle, fluxScale, energyMode, expand, fullRegion, 0);

         status = projectCube->status;
         time   = projectCube->time;

         free(projectCube);

         strcpy(msg, projectCube->msg);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mProjectCube(%s) -> [%s]\n", infile, msg);
            fflush(mProjExec_fdebug);
         }
      }

      else if(quickMode)
      {
         projectQL = mProjectQL(infile, outfile, template, hdu, interp, weightFile, weight, threshold,
                                borderStr, fluxScale, expand, fullRegion, noAreas, 0);

         status = projectQL->status;
         time   = projectQL->time;

         strcpy(msg, projectQL->msg);

         free(projectQL);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mProjectQL(%s) -> [%s]\n", infile, msg);
            fflush(mProjExec_fdebug);
         }
      }

      else if(!wcsMatch)
      {
         project = mProject(infile, outfile, template, hdu, weightFile, weight, threshold,
                            borderStr, drizzle, fluxScale, energyMode, expand, fullRegion, 0);

         status = project->status;
         time   = project->time;

         strcpy(msg, project->msg);

         free(project);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mProject(%s) -> [%s]\n", infile, msg);
            fflush(mProjExec_fdebug);
         }
      }

      else if(inp2p == COMPUTED  && outp2p == COMPUTED )
      {
         projectPP = mProjectPP(infile, outfile, template, hdu, weightFile, weight, threshold,
                                borderStr, altin, altout, drizzle, fluxScale, energyMode, expand, fullRegion, 0);

         status = projectPP->status;
         time   = projectPP->time;

         strcpy(msg, projectPP->msg);

         free(projectPP);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mProjectPP(%s) -> [%s] (COMPUTED/COMPUTED)\n", infile, msg);
            fflush(mProjExec_fdebug);
         }
      }

      else if(inp2p == COMPUTED  && outp2p == INTRINSIC)
      {
         strcpy(altout, "");
         projectPP = mProjectPP(infile, outfile, template, hdu, weightFile, weight, threshold,
                                borderStr, altin, altout, drizzle, fluxScale, energyMode, expand, fullRegion, 0);

         status = projectPP->status;
         time   = projectPP->time;

         strcpy(msg, projectPP->msg);

         free(projectPP);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mProjectPP(%s) -> [%s] (COMPUTED/INTRINSIC)\n", infile, msg);
            fflush(mProjExec_fdebug);
         }
      }

      else if(inp2p == INTRINSIC && outp2p == COMPUTED )
      {
         strcpy(altin, "");
         projectPP = mProjectPP(infile, outfile, template, hdu, weightFile, weight, threshold, 
                                borderStr, altin, altout, drizzle, fluxScale, energyMode, expand, fullRegion, 0);

         status = projectPP->status;
         time   = projectPP->time;

         strcpy(msg, projectPP->msg);

         free(projectPP);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mProjectPP(%s) -> [%s] (INTRINSIC/COMPUTED)\n", infile, msg);
            fflush(mProjExec_fdebug);
         }
      }

      else if(inp2p == INTRINSIC && outp2p == INTRINSIC)
      {
         strcpy(altin, "");
         strcpy(altout, "");
         projectPP = mProjectPP(infile, outfile, template, hdu, weightFile, weight, threshold,
                                borderStr, altin, altout, drizzle, fluxScale, energyMode, expand, fullRegion, 0);

         status = projectPP->status;
         time   = projectPP->time;

         strcpy(msg, projectPP->msg);

         free(projectPP);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mProjectPP(%s) -> [%s] (INTRINSIC/INTRINSIC)\n", infile, msg);
            fflush(mProjExec_fdebug);
         }
      }

      else
      {
         project = mProject(infile, outfile, template, hdu, weightFile, weight, threshold,
                            borderStr, drizzle, fluxScale, energyMode, expand, fullRegion, 0);

         status = project->status;
         time   = project->time;

         strcpy(msg, project->msg);

         free(project);

         if(mProjExec_debug)
         {
            fprintf(mProjExec_fdebug, "mProject(%s) -> [%s]\n", infile, msg);
            fflush(mProjExec_fdebug);
         }
      }

      if(mProjExec_debug)
      {
         if(wcsMatch)
         {
            if( inp2p == COMPUTED)  fprintf(mProjExec_fdebug, " inp2p = COMPUTED\n");
            if( inp2p == INTRINSIC) fprintf(mProjExec_fdebug, " inp2p = INTRINSIC\n");
            if( inp2p == FAILED)    fprintf(mProjExec_fdebug, " inp2p = FAILED\n");

            if(outp2p == COMPUTED)  fprintf(mProjExec_fdebug, "outp2p = COMPUTED\n");
            if(outp2p == INTRINSIC) fprintf(mProjExec_fdebug, "outp2p = INTRINSIC\n");
            if(outp2p == FAILED)    fprintf(mProjExec_fdebug, "outp2p = FAILED\n");
         }
      }

      if(strlen(stats) > 0)
      {
         if(status)
         {
            if(strcmp( msg, "No overlap")           == 0
                  || strcmp( msg, "All pixels are blank") == 0)
            {
               ++nooverlap;
               fprintf(fout, " %-60s %-30s %10s\n", montage_fileName(tval(ifname)), msg, "");
            }
            else
            {
               unlink(outfile);
               ++failed;
               fprintf(fout, " %-60s %-30s %10s\n", montage_fileName(tval(ifname)), msg, "");
            }
         }
         else
            fprintf(fout, " %-60s %-30s %10.1f\n", montage_fileName(tval(ifname)), "", time);
      }
   }

   if(strlen(stats) > 0)
      fclose(fout);

   free(inheader);

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d, failed=%d, nooverlap=%d", count-restart, failed, nooverlap);
   sprintf(returnStruct->json, "{\"count\"=%d, \"failed\"=%d, \"nooverlap\"=%d}", count-restart, failed, nooverlap);

   returnStruct->count     = count-restart;
   returnStruct->failed    = failed;
   returnStruct->nooverlap = nooverlap;

   return returnStruct;
}



/**************************************************/
/*                                                */
/*  Read the output header template file.         */
/*  Create a single-string version of the         */
/*  header data and use it to initialize the      */
/*  output WCS transform.                         */
/*                                                */
/**************************************************/

int mProjExec_readTemplate(char *filename)
{
   int       j, naxes;
   FILE     *fp;
   char      line[MAXSTR];
   char      header[80000];
   char     *ptr;


   /********************************************************/
   /* Open the template file, read and parse all the lines */
   /********************************************************/

   fp = fopen(filename, "r");

   if(fp == (FILE *)NULL)
   {
      sprintf(montage_msgstr, "Template file %s not found.", filename);
      return 0;
   }

   strcpy(header, "");

   for(j=0; j<1000; ++j)
   {
      if(fgets(line, MAXSTR, fp) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';
      
      if(line[strlen(line)-1] == '\r')
         line[strlen(line)-1]  = '\0';

      if(mProjExec_debug >= 3)
      {
         fprintf(mProjExec_fdebug, "Template line: [%s]\n", line);
         fflush(mProjExec_fdebug);
      }

      ptr = strstr(line, "NAXIS   =");

      if(ptr != (char *)NULL)
         naxes = atoi(ptr + 10);

      mProjExec_stradd(header, line);
   }

   fclose(fp);


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   wcsout = wcsinit(header);

   if(wcsout == (struct WorldCoor *)NULL)
   {
      sprintf(montage_msgstr, "Output wcsinit() failed.");
      return 0;
   }

   return naxes;
}


/* stradd adds the string "card" to a header line, and */
/* pads the header out to 80 characters.               */

int mProjExec_stradd(char *header, char *card)
{
   int i;

   int hlen = strlen(header);
   int clen = strlen(card);

   for(i=0; i<clen; ++i)
      header[hlen+i] = card[i];

   if(clen < 80)
      for(i=clen; i<80; ++i)
         header[hlen+i] = ' ';

   header[hlen+80] = '\0';

   return(strlen(header));
}
