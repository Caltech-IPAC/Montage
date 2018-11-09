#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mtbl.h>
#include <svc.h>

#include <mArchiveExec.h>
#include <montage.h>

#define MAXSTR  4096
#define NPIX   23552

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

char *svc_value();


/*-*****************************************************************/
/*                                                                 */
/*  mArchiveExec                                                   */
/*                                                                 */
/*  Reads a listing archive images and calls mArchiveGet to get    */
/*  each one.                                                      */
/*                                                                 */
/*   char *tblfile     Table file list of images to get.           */
/*                                                                 */
/*   char *path        Directory to write retrieved files.         */
/*   int   nrestart    Restart record, if download interupted.     */
/*   int   timeout     Download timeout (sec) per image.           */
/*   int   debug       Debug flag.                                 */
/*                                                                 */
/*******************************************************************/

struct mArchiveExecReturn *mArchiveExec(char *tblfile, char *inpath, int nrestart, int timeout, int debug)
{
   int    i, c, stat, ncols, count, failed, nread;

   int    iurl;
   int    ifile;

   int    iimin, iimax, ijmin, ijmax;
   int    imin, imax, jmin, jmax;
   int    itmin, itmax, jtmin, jtmax;
   int    nx, ny, ix, jy;

   char  *ptr;
   char   url     [MAXSTR];
   char   urlbase [MAXSTR];
   char   file    [MAXSTR];
   char   filebase[MAXSTR];
   char   path    [MAXSTR];

   char   cmd     [MAXSTR];
   char   status  [32];

   struct mArchiveExecReturn *returnStruct;
   struct mArchiveGetReturn  *archive;

   returnStruct = (struct mArchiveExecReturn *)malloc(sizeof(struct mArchiveExecReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));

   returnStruct->status = 1;


   if(inpath == (char *)NULL)
      strcpy(path, ".");
   else
      strcpy(path, inpath);


   /***********************************/ 
   /* Open the region list table file */
   /***********************************/ 

   if(debug)
   {
      printf("DEBUG> tblfile = [%s]\n", tblfile);
      fflush(stdout);
   }

   ncols = topen(tblfile);

   iurl = tcol( "URL");
   if(iurl < 0)
      iurl = tcol( "url");

   ifile = tcol( "fname");
   if(ifile < 0)
      ifile = tcol("file");

   if(debug)
   {
      printf("DEBUG> iurl    = %d\n", iurl);
      printf("DEBUG> ifile   = %d\n", ifile);
      fflush(stdout);
   }

   iimin  = tcol( "imin");
   iimax  = tcol( "imax");
   ijmin  = tcol( "jmin");
   ijmax  = tcol( "jmax");

   if(iurl < 0)
   {
      sprintf(returnStruct->msg, "Table %s needs column 'URL' or 'url' and can optionally have columns 'fname'/'file' and pixel ranges 'imin'..'jmax'",
         tblfile);
      return returnStruct;
   }


   /*****************************************/ 
   /* Read the records and call mArchiveGet */
   /*****************************************/ 

   count  = 0;
   failed = 0;
   nread  = 0;

   chdir(path);

   while(1)
   {
      stat = tread();

      ++nread;
      if(nread < nrestart)
         continue;

      if(stat < 0)
         break;

      strcpy(url, tval(iurl));

      if(debug)
      {
         printf("DEBUG> url  = [%s]\n", url);
         fflush(stdout);
      }


      if(ifile >= 0)
         strcpy(file, tval(ifile));
      else
      {
         ptr = url+strlen(url)-1;

         while(1)
         {
            if(ptr == url || *ptr == '/')
            {
               strcpy(file, ptr+1);
               break;
            }

            --ptr;
         }
      }
      
      if(debug)
      {
         printf("DEBUG> file = [%s]\n", file);
         fflush(stdout);
      }


      /* Special processing for DPOSS */
      /* (get the image in tiles)     */

      if(iimin >= 0
      && iimax >= 0
      && ijmin >= 0
      && ijmax >= 0)
      {
         strcpy(filebase, file);

         for(i=0; i<strlen(filebase); ++i)
            if(filebase[i] == '.')
               filebase[i] = '\0';

         strcpy(urlbase, url);

         for(i=0; i<strlen(urlbase); ++i)
            if(urlbase[i] == '&')
               urlbase[i] = '\0';

         imin =    1;
         imax = NPIX;
         jmin =    1;
         jmax = NPIX;

         imin = atoi(tval(iimin));
         imax = atoi(tval(iimax));
         jmin = atoi(tval(ijmin));
         jmax = atoi(tval(ijmax));

         nx = NPIX / 500;
         ny = NPIX / 500;

         for(ix=3; ix<nx-3; ++ix)
         {
            for(jy=3; jy<nx-3; ++jy)
            {
               itmin = ix * 500 - 50;
               jtmin = jy * 500 - 50;

               itmax = (ix+1) * 500 + 50;
               jtmax = (jy+1) * 500 + 50;

               if(itmax < imin) continue;
               if(itmin > imax) continue;
               if(jtmax < jmin) continue;
               if(jtmin > jmax) continue;

               sprintf(url, "%s&X1=%d&X2=%d&Y1=%d&Y2=%d", urlbase, itmin, itmax - itmin + 1, jtmin, jtmax - jtmin + 1);

               sprintf(file, "%s_%d_%d.fits", filebase, ix, jy);

               archive = mArchiveGet(url, file, timeout, debug);

               ++count;

               if(archive->status)
               {
                  ++failed;
                  continue;
               }

               if(strcmp( status, "ERROR") == 0)
               {
                  ++failed;
                  continue;
               }
            }
         }
      }


      /* Normal URL-based retrieval */

      else
      {
         archive = mArchiveGet(url, file, timeout, debug);

         ++count;

         if(archive->status)
         {
            ++failed;
            continue;
         }

         if(strlen(file) > 3 && strcmp(file+strlen(file)-3, ".gz") == 0)
         {
            sprintf(cmd, "gunzip %s", file);
            system(cmd);
         }
      }
   }


   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d, failed=%d", count, failed);

   sprintf(returnStruct->json, "{\"count\":%d, \"failed\":%d}", count, failed);

   returnStruct->count   = count;
   returnStruct->failed  = failed;

   return returnStruct;

}
