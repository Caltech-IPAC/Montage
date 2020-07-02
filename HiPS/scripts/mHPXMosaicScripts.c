#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include <mtbl.h>

#define ARCHIVE 0
#define LOCAL   1

#define BACKGROUND_CORRECTION 0
#define PROJECT_ONLY          1

#define SINGLE_THREADED 0
#define CLUSTER         1

extern int optind, opterr;

int sky[5][5] = {{1,1,0,0,0},
                 {1,1,1,0,0},
                 {0,1,1,1,0},
                 {0,0,1,1,1},
                 {0,0,0,1,1}};

int naxis;


/****************************************************************************/
/*                                                                          */
/*  MHPXMOSAICSCRIPTS                                                       */
/*                                                                          */
/*  Create a set of scripts to that generate HPX plates with padding.       */
/*  The plates are always part of a regular array and, while not required,  */
/*  the plate size and padding should be powers of two.  Some of the        */
/*  downstream processing depends on this.                                  */
/*                                                                          */
/*  We need the same array of plates all through the processing.  Also,     */
/*  if we are making use of a cluster for processing and want to balance    */
/*  the I/O across a set of storage units we need to round-robin the        */
/*  processing accordingly.  For both these reasons, we have separated the  */
/*  plate list generation out into a separate program (mHPXPlateList) and   */
/*  just read that back in here.                                            */
/*                                                                          */
/*  There are three binary modes here.  The data can be a set of local      */
/*  files or it can be downloaded as part of the processing (using the      */
/*  mArchive tools); the processing may only need to reproject the data or  */
/*  it may need to adjust the backgrounds of each image; and we may wish to */
/*  process it as a single-threaded script or submit it as a set of jobs to */
/*  a cluster (or eventually the cloud).                                    */
/*                                                                          */
/*  We haven't encountered all of these but it is prudent to account for    */
/*  all of them:                                                            */
/*                                                                          */
/*    LOCAL vs ARCHIVE we infer from the argument list.  The last arguments */
/*    are either "dataset band" (e.g. 2MASS J") or the path to a local      */
/*    directory containing the data.                                        */
/*                                                                          */
/*    We will assume that the data needs the full background correction     */
/*    treatment unless we see an explicit "-p" (PROJECT_ONLY) flag.         */
/*                                                                          */
/*    We also assume CLUSTER mode rather than SINGLE_THREADED unless there  */
/*   is an explicit "-s" flag.                                              */
/*                                                                          */
/*                                                                          */
/*  For BACKGROUD_CORRECTION processing, we use mExec and pass it the       */
/*  survey/band information.  For PROJECT_ONLY, we use mProjExec.           */
/*                                                                          */
/*  For CLUSTER processing, the main runMosaics.sh script submits the       */
/*  plate processing jobs to the cluster all at once and lets the cluster   */
/*  manage their execution.  For SINGLE_THREADED, that same script runs     */
/*  in foreground, running the processing jobs one after another.  The      */
/*  processing job scripts are the same in either case.                     */
/*                                                                          */
/*  Of the eight possible combinations, there are a few we haven't yet had  */
/*  to deal with:  ARCHIVE data that we PROJECT_ONLY and (yet) LOCAL data   */
/*  that needs BACKGROUND_CORRECTION.  The latter is just a matter of time  */
/*  mExec can already be used for that purpose.                             */
/*                                                                          */
/****************************************************************************/

int main(int argc, char **argv)
{
   int  level, c, pad, count, ncols, nplate;
   int  id, iplate, i, j, bin, onebin;
   int  iid, ii, ij, ibin, location, background, processing;

   char cwd       [1024];
   char survey    [1024];
   char band      [1024];
   char datadir   [1024];
   char scriptfile[1024];
   char driverfile[1024];
   char mosaicdir [1024];
   char scriptdir [1024];
   char dir       [1024];
   char platelist [1024];
   char platename [1024];
   char tmpdir    [1024];

   FILE *fscript;
   FILE *fdriver;

   int debug = 0;

   getcwd(cwd, 1024);


   // Command-line arguments

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXMosaicScripts [-d][-s(ingle-threaded)[-p(roject-only)] scriptdir mosaicdir platelist.tbl survey/band | datadir\"]\n");
      fflush(stdout);
      exit(1);
   }

   debug = 0;

   location   = ARCHIVE;
   background = BACKGROUND_CORRECTION;
   processing = CLUSTER;

   while ((c = getopt(argc, argv, "dsp")) != EOF)
   {
      switch (c)
      {
         case 'd':
            debug = 1;
            break;

         case 's':
            processing = SINGLE_THREADED;
            break;

         case 'p':
            background = PROJECT_ONLY;
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXMosaicScripts [-d][-s(ingle-threaded)[-p(roject-only)] scriptdir mosaicdir platelist.tbl survey/band | datadir\"]\n");
            fflush(stdout);
            exit(1);
      }
   }

   strcpy(scriptdir,  argv[optind]);
   strcpy(mosaicdir,  argv[optind + 1]);
   strcpy(platelist,  argv[optind + 2]);

   if(argc - optind < 5)
      location = LOCAL;

   if(location == ARCHIVE)
   {
      strcpy(survey, argv[optind + 3]);
      strcpy(band,   argv[optind + 4]);
   }

   if(location == LOCAL)
      strcpy(datadir, argv[optind + 3]);


   if(scriptdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, scriptdir);

      strcpy(scriptdir, tmpdir);
   }
      
   if(mosaicdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, mosaicdir);

      strcpy(mosaicdir, tmpdir);
   }
      
   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(mosaicdir[strlen(mosaicdir)-1] != '/')
      strcat(mosaicdir, "/");


   // Make sure the necessary subdirectories exist

   if(mkdir(mosaicdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create [%s].\"]\n", mosaicdir);
      fflush(stdout);
      exit(0);
   }

   if(mkdir(scriptdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create [%s].\"]\n", scriptdir);
      fflush(stdout);
      exit(0);
   }

   sprintf(tmpdir, "%sjobs", scriptdir);

   if(mkdir(tmpdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create [%s].\"]\n", tmpdir);
      fflush(stdout);
      exit(0);
   }

   sprintf(tmpdir, "%slogs", scriptdir);

   if(mkdir(tmpdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create [%s].\"]\n", tmpdir);
      fflush(stdout);
      exit(0);
   }


   // Open the driver script file

   sprintf(driverfile, "%s/runMosaics.sh", scriptdir);

   if(debug)
   {
      printf("DEBUG> driverfile: [%s]\n", driverfile);
      fflush(stdout);
   }

   fdriver = fopen(driverfile, "w+");

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file [%s].\"]\n", driverfile);
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);


   // Create the scripts. We double check that the plate intersects with
   // at least a part of the sky
   
   count = 0;

   ncols = topen(platelist);

   if(ncols < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open plate list file [%s].\"]\n", platelist);
      fflush(stdout);
      exit(0);
   }

   if(tfindkey("order"))
      level = atoi(tfindkey("order"));
   else
   {
      printf("[struct stat=\"ERROR\", msg=\"No 'order' keyword in plate list file [%s].\"]\n", platelist);
      fflush(stdout);
      exit(0);
   }

   if(tfindkey("nplate"))
      nplate = atoi(tfindkey("nplate"));
   else
   {
      printf("[struct stat=\"ERROR\", msg=\"No 'nplate' keyword in plate list file [%s].\"]\n", platelist);
      fflush(stdout);
      exit(0);
   }

   iid    = tcol("id");
   iplate = tcol("plate");
   ii     = tcol("i");
   ij     = tcol("j");
   ibin   = tcol("bin");

   if(iid < 0 || iplate < 0 || ii < 0 || ij < 0 || ibin < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns id, plate, i, j, bin in plate list file [%s].\"]\n", platelist);
      fflush(stdout);
      exit(0);
   }


   pad = (int)pow(2., (double)(level-1));

   if(pad <  32) pad =  32;
   if(pad > 256) pad = 256;


   if(debug)
   {
      printf("DEBUG> iid    = %d\n", iid);
      printf("DEBUG> iplate = %d\n", iplate);
      printf("DEBUG> ii     = %d\n", ii);
      printf("DEBUG> ij     = %d\n", ij);
      printf("DEBUG> ibin   = %d\n", ibin);
      printf("DEBUG> level  = %d\n", level);
      printf("DEBUG> nplate = %d\n", nplate);
      printf("DEBUG> pad    = %d\n", pad);
      fflush(stdout);
   }

   onebin = 1;

   while(1)
   {
      if(tread() < 0)
         break;

      bin = atoi(tval(ibin));

      if(bin > 0)
      {
         onebin = 0;
         break;
      }
   }

   tclose();

   ncols = topen(platelist);

   while(1)
   {
      if(tread() < 0)
         break;

      id = atoi(tval(iid));

      strcpy(platename, tval(iplate));

      i = atoi(tval(ii));
      j = atoi(tval(ij));

      bin = atoi(tval(ibin));

      if(debug)
      {
         printf("DEBUG> id    = %d\n", id);
         printf("DEBUG> plate = [%s]\n", platename);
         printf("DEBUG> i     = %d\n", i);
         printf("DEBUG> j     = %d\n", j);
         printf("DEBUG> bin   = %d\n", bin);
         fflush(stdout);
      }


      sprintf(scriptfile, "%sjobs/plate_%02d_%02d.sh", scriptdir, i, j);

      if(debug)
      {
         printf("DEBUG> scriptfile: [%s]\n", scriptfile);
         fflush(stdout);
      }

      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file for plate %d %d.\"]\n", i, j);
         fflush(stdout);
         exit(0);
      }


      if(location == ARCHIVE)
      {
         fprintf(fscript, "#!/bin/sh\n\n");

         fprintf(fscript, "echo jobs/plate_%02d_%02d.sh", i, j);

         fprintf(fscript, "mkdir $1\n");

         fprintf(fscript, "rm -rf $1/work_%02d_%02d\n", i, j);

         fprintf(fscript, "mkdir $1/work_%02d_%02d\n", i, j);

         fprintf(fscript, "mHPXHdr %d $1/hpx%d_%02d_%02d.hdr\n", level, level, i, j);

         fprintf(fscript, "mTileHdr $1/hpx%d_%02d_%02d.hdr $1/plate_%02d_%02d.hdr %d %d %d %d %d %d\n", 
            level, i, j, i, j, nplate, nplate, i, j, pad, pad);

         fprintf(fscript, "mExec -q -a -l -c -d 3 -f $1/plate_%02d_%02d.hdr -o $1/plate_%02d_%02d.fits %s %s $1/work_%02d_%02d\n", 
             i, j, i, j, survey, band, i, j);

         fprintf(fscript, "rm -rf $1/work_%02d_%02d\n", i, j);
         fprintf(fscript, "rm -f $1/hpx%d_%02d_%02d.hdr\n", level, i, j);
         fprintf(fscript, "rm -f $1/plate_%02d_%02d.hdr\n", i, j);

         fflush(fscript);
         fclose(fscript);
      }

      if(location == LOCAL)
      {
         fprintf(fscript, "#!/bin/sh\n\n");

         fprintf(fscript, "echo jobs/plate_%02d_%02d.sh\n\n", i, j);

         fprintf(fscript, "mkdir $1\n");

         fprintf(fscript, "rm -rf $1/work_%02d_%02d\n", i, j);

         fprintf(fscript, "mkdir $1/work_%02d_%02d\n", i, j);

         fprintf(fscript, "mkdir $1/work_%02d_%02d/projected\n", i, j);

         fprintf(fscript, "mHPXHdr %d $1/hpx%d_%02d_%02d.hdr\n", level, level, i, j);

         fprintf(fscript, "mTileHdr $1/hpx%d_%02d_%02d.hdr $1/plate_%02d_%02d.hdr %d %d %d %d %d %d\n", 
            level, i, j, i, j, nplate, nplate, i, j, pad, pad);

         fprintf(fscript, "mImgtbl %s $1/work_%02d_%02d/rimages.tbl\n", datadir, i, j);

         fprintf(fscript, "mProjExec -d -q -p %s $1/work_%02d_%02d/rimages.tbl $1/plate_%02d_%02d.hdr $1/work_%02d_%02d/projected $1/work_%02d_%02d/stats.tbl\n",
            datadir, i, j, i, j, i, j, i, j);

         fprintf(fscript, "mImgtbl $1/work_%02d_%02d/projected $1/work_%02d_%02d/pimages.tbl\n", i, j, i, j);
         
         fprintf(fscript, "mAdd -p $1/work_%02d_%02d/projected $1/work_%02d_%02d/pimages.tbl $1/plate_%02d_%02d.hdr $1/plate_%02d_%02d.fits\n",
            i, j, i, j, i, j, i, j);

         fprintf(fscript, "rm -rf $1/work_%02d_%02d\n", i, j);
         fprintf(fscript, "rm -f $1/hpx%d_%02d_%02d.hdr\n", level, i, j);
         fprintf(fscript, "rm -f $1/plate_%02d_%02d.hdr\n", i, j);
         fprintf(fscript, "rm -f $1/plate_%02d_%02d_area.fits\n", i, j);

         fflush(fscript);
         fclose(fscript);
      }

      chmod(scriptfile, 0777);

      if(processing == CLUSTER)
      {
         if(onebin)
            fprintf(fdriver, "sbatch submitMosaic.bash %sjobs/plate_%02d_%02d.sh %s\n",
               scriptdir, i, j, mosaicdir);
         else
            fprintf(fdriver, "sbatch submitMosaic.bash %sjobs/plate_%02d_%02d.sh %ssubset%d\n",
               scriptdir, i, j, mosaicdir, bin);
      }

      if(processing == SINGLE_THREADED)
         fprintf(fdriver, "%sjobs/plate_%02d_%02d.sh %s\n",
            scriptdir, i, j, mosaicdir);

      fflush(fdriver);

      ++count;
   }

   fclose(fdriver);
   chmod(driverfile, 0777);

   printf("[struct stat=\"OK\", module=\"mHPXMosaicScripts\", level=%d, pixlevel=%d,count=%d]\n",
      level, level+9, count);
   fflush(stdout);
   exit(0);
} 
