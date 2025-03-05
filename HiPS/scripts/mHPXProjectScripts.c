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


/*****************************************************************************/
/*                                                                           */
/*  MHPXPROJECTSCRIPTS                                                       */
/*                                                                           */
/*  Create a set of scripts to that generate the reprojected images for      */
/*                                                                           */
/*  regions of the sky.  These regions are part of a regular grid and        */
/*  are used to allow parallelization of all-sky processing into manageable  */
/*  chunks.  Images that span the border between these regions ("plates")    */
/*  are truncated but the rest of such images will be picked up in another   */
/*  plate or plates.                                                         */
/*                                                                           */
/*  The script also handles the difference fitting between images in a plate */
/*  and we compensate for the truncated images in a separate step before     */
/*  using all this information to determine the background corrections       */
/*  to be applied to all the reprojected images before final coadding        */
/*  (which is also parallelized by plate).                                   */
/*                                                                           */
/*  Plate sizes are always a power of two to facilitate turning the plate    */
/*  data into HiPS tiles.                                                    */
/*                                                                           */
/*  We need the same array of plates all through the processing so we have   */
/*  separated the plate list generation out into a separate program          */
/*  (mHPXPlateList) and just read that back in here.                         */
/*                                                                           */
/*  There are three binary decisions.  The data can be a set of local        */
/*  files or it can be downloaded as part of the processing (using the       */
/*  mArchive tools); the processing may only need to reproject the data or   */
/*  it may need to adjust the backgrounds of each image; and we may wish to  */
/*  process it as a single-threaded script or submit it as a set of jobs to  */
/*  a cluster (or eventually the cloud).                                     */
/*                                                                           */
/*  We haven't encountered all of these but it is prudent to account for     */
/*  all of them:                                                             */
/*                                                                           */
/*    LOCAL vs ARCHIVE we infer from the argument list.  The last arguments  */
/*    are either "dataset band" (e.g. 2MASS J") or the path to a local       */
/*    directory containing the data.                                         */
/*                                                                           */
/*    We will assume that the data needs the full background correction      */
/*    treatment unless we see an explicit "-p" (PROJECT_ONLY) flag.          */
/*                                                                           */
/*    We also assume CLUSTER mode rather than SINGLE_THREADED unless there   */
/*   is an explicit "-s" flag.                                               */
/*                                                                           */
/*                                                                           */
/*  For BACKGROUD_CORRECTION processing, we use mExec and pass it the        */
/*  survey/band information.  For PROJECT_ONLY, we use mProjExec.            */
/*                                                                           */
/*  For CLUSTER processing, the main runProject.sh script submits the        */
/*  plate processing jobs to the cluster all at once and lets the cluster    */
/*  manage their execution.  For SINGLE_THREADED, that same script runs      */
/*  in foreground, running the processing jobs one after another.  The       */
/*  processing job scripts are the same in either case.                      */
/*                                                                           */
/*  Of the eight possible combinations, there are a few we haven't yet had   */
/*  to deal with:  ARCHIVE data that we PROJECT_ONLY and (yet) LOCAL data    */
/*  that needs BACKGROUND_CORRECTION.  The latter is just a matter of time   */
/*  mExec can already be used for that purpose.                              */
/*                                                                           */
/*****************************************************************************/

int main(int argc, char **argv)
{
   int  level, level_only, c, ncols, nplate;
   int  id, iplate, i, j, status, nimages, exists;
   int  iid, ii, ij, location, background, processing;

   char cwd       [1024];
   char survey    [1024];
   char band      [1024];
   char datadir   [1024];
   char outfile   [1024];
   char scriptfile[1024];
   char driverfile[1024];
   char projectdir [1024];
   char scriptdir [1024];
   char dir       [1024];
   char platelist [1024];
   char platename [1024];
   char tmpdir    [1024];

   struct stat buf;

   FILE *fscript;
   FILE *fdriver;

   int debug = 0;

   getcwd(cwd, 1024);


   // Command-line arguments

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXProjectScripts [-d][-l(evel-only)][-s(ingle-threaded)[-p(roject-only)] scriptdir projectdir platelist.tbl survey/band | datadir\"]\n");
      fflush(stdout);
      exit(1);
   }

   debug = 0;
   level_only = 0;

   location   = ARCHIVE;
   background = BACKGROUND_CORRECTION;
   processing = CLUSTER;

   while ((c = getopt(argc, argv, "dlsp")) != EOF)
   {
      switch (c)
      {
         case 'd':
            debug = 1;
            break;

         case 'l':
            level_only = 1;
            break;

         case 's':
            processing = SINGLE_THREADED;
            break;

         case 'p':
            background = PROJECT_ONLY;
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXProjectScripts [-d][-s(ingle-threaded)[-p(roject-only)] scriptdir projectdir platelist.tbl survey/band | datadir\"]\n");
            fflush(stdout);
            exit(1);
      }
   }

   strcpy(scriptdir,  argv[optind]);
   strcpy(projectdir,  argv[optind + 1]);
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
      
   if(projectdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, projectdir);

      strcpy(projectdir, tmpdir);
   }
      
   if(platelist[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platelist);

      strcpy(platelist, tmpdir);
   }
      
   if(location == LOCAL)
   {
      if(datadir[0] != '/')
      {
         strcpy(tmpdir, cwd);
         strcat(tmpdir, "/");
         strcat(tmpdir, datadir);

         strcpy(datadir, tmpdir);
      }
   }
      
   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(projectdir[strlen(projectdir)-1] != '/')
      strcat(projectdir, "/");


   // Make sure the necessary subdirectories exist

   if(mkdir(projectdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create [%s].\"]\n", projectdir);
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

   sprintf(driverfile, "%s/runProject.sh", scriptdir);

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

   if(iid < 0 || iplate < 0 || ii < 0 || ij < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns id, plate, i, j in plate list file [%s].\"]\n", platelist);
      fflush(stdout);
      exit(0);
   }


   if(debug)
   {
      printf("DEBUG> iid    = %d\n", iid);
      printf("DEBUG> iplate = %d\n", iplate);
      printf("DEBUG> ii     = %d\n", ii);
      printf("DEBUG> ij     = %d\n", ij);
      printf("DEBUG> level  = %d\n", level);
      printf("DEBUG> nplate = %d\n", nplate);
      fflush(stdout);
   }

   ncols = topen(platelist);

   nimages = 0;
   exists  = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      ++nimages;

      id = atoi(tval(iid));

      strcpy(platename, tval(iplate));

      i = atoi(tval(ii));
      j = atoi(tval(ij));

      sprintf(outfile, "%splate_%02d_%02d.fits", projectdir, i, j);


      if(debug)
      {
         printf("DEBUG> id      = %d\n", id);
         printf("DEBUG> plate   = [%s]\n", platename);
         printf("DEBUG> i       = %d\n", i);
         printf("DEBUG> j       = %d\n", j);
         printf("DEBUG> outfile = [%s]\n", outfile);
         fflush(stdout);
      }

      status = stat(outfile, &buf);

      if(status == 0)
      {
         ++exists;

         printf("[struct stat=\"WARNING\", msg=\"%s already exists. Delete it first if you want it regenerated.\"]\n",
            outfile);
         fflush(stdout);
         continue;
      }

      sprintf(scriptfile, "%sjobs/project_%02d_%02d.sh", scriptdir, i, j);

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

         fprintf(fscript, "echo \"Reprojecting images for plate_%02d_%02d\"\n", i, j);

         fprintf(fscript, "mkdir -p $1\n");

         fprintf(fscript, "rm -rf $1/plate_%02d_%02d\n",         i, j);

         fprintf(fscript, "mkdir $1/plate_%02d_%02d\n",           i, j);
         fprintf(fscript, "mkdir $1/plate_%02d_%02d/raw\n",       i, j);
         fprintf(fscript, "mkdir $1/plate_%02d_%02d/projected\n", i, j);

         fprintf(fscript, "mkdir $1/plate_%02d_%02d/diffs\n",     i, j);

         fprintf(fscript, "mHPXHdr %d $1/plate_%02d_%02d/hpx%d.hdr\n", level, i, j, level);
 
         fprintf(fscript, "mTileHdr $1/plate_%02d_%02d/hpx%d.hdr $1/plate_%02d_%02d/region.hdr %d %d %d %d\n", 
            i, j, level, i, j, nplate, nplate, i, j);

         fprintf(fscript, "mArchiveList -f $1/plate_%02d_%02d/region.hdr %s %s $1/plate_%02d_%02d/remote_big.tbl\n", 
            i, j, survey, band, i, j);

         fprintf(fscript, "mCoverageCheck -p $1/plate_%02d_%02d/projected $1/plate_%02d_%02d/remote_big.tbl $1/plate_%02d_%02d/remote.tbl -header $1/plate_%02d_%02d/region.hdr\n", 
            i, j, i, j, i, j, i, j);

         fprintf(fscript, "mArchiveExec -p $1/plate_%02d_%02d/raw $1/plate_%02d_%02d/remote.tbl\n", 
            i, j, i, j);

         fprintf(fscript, "mImgtbl $1/plate_%02d_%02d/raw $1/plate_%02d_%02d/rimages.tbl\n", i, j, i, j);

         fprintf(fscript, "mProjExec -q -X -p $1/plate_%02d_%02d/raw $1/plate_%02d_%02d/rimages.tbl $1/plate_%02d_%02d/region.hdr $1/plate_%02d_%02d/projected $1/plate_%02d_%02d/stats.tbl\n",
               i, j, i, j, i, j, i, j, i, j);

         fprintf(fscript, "mImgtbl $1/plate_%02d_%02d/projected $1/plate_%02d_%02d/pimages.tbl\n", i, j, i, j);

         fprintf(fscript, "mOverlaps $1/plate_%02d_%02d/pimages.tbl $1/plate_%02d_%02d/diffs.tbl\n", i, j, i, j);

         if(level_only)
            fprintf(fscript, "mDiffFitExec -n -l -p $1/plate_%02d_%02d/projected $1/plate_%02d_%02d/diffs.tbl $1/plate_%02d_%02d/region.hdr $1/plate_%02d_%02d/diffs $1/plate_%02d_%02d/fits.tbl\n",
            i, j, i, j, i, j, i, j, i, j);
         else
            fprintf(fscript, "mDiffFitExec -n -p $1/plate_%02d_%02d/projected $1/plate_%02d_%02d/diffs.tbl $1/plate_%02d_%02d/region.hdr $1/plate_%02d_%02d/diffs $1/plate_%02d_%02d/fits.tbl\n",
                  i, j, i, j, i, j, i, j, i, j);

         fprintf(fscript, "rm $1/plate_%02d_%02d/hpx%d.hdr\n", i, j, level);
         fprintf(fscript, "rm $1/plate_%02d_%02d/remote.tbl $1/plate_%02d_%02d/diffs.tbl\n", i, j, i, j);
         fprintf(fscript, "rm $1/plate_%02d_%02d/rimages.tbl\n", i, j);
         fprintf(fscript, "rm $1/plate_%02d_%02d/stats.tbl\n", i, j);
         fprintf(fscript, "rm -rf $1/plate_%02d_%02d/diffs $1/plate_%02d_%02d/raw\n", i, j, i, j);
         fprintf(fscript, "rm $1/plate_%02d_%02d/projected/*_area.fits\n", i, j);

         fflush(fscript);
         fclose(fscript);
      }

      if(location == LOCAL)
      {
         fprintf(fscript, "#!/bin/sh\n\n");

         fprintf(fscript, "echo \"Reprojecting images for plate_%02d_%02d\"\n", i, j);

         fprintf(fscript, "mkdir -p $1\n");

         fprintf(fscript, "rm -rf $1/plate_%02d_%02d\n",         i, j);

         fprintf(fscript, "mkdir $1/plate_%02d_%02d\n",           i, j);
         fprintf(fscript, "mkdir $1/plate_%02d_%02d/projected\n", i, j);
         fprintf(fscript, "mkdir $1/plate_%02d_%02d/diffs\n",     i, j);

         fprintf(fscript, "mHPXHdr %d $1/plate_%02d_%02d/hpx%d.hdr\n", level, i, j, level);

         fprintf(fscript, "mTileHdr $1/plate_%02d_%02d/hpx%d.hdr $1/plate_%02d_%02d/region.hdr %d %d %d %d\n", 
            i, j, level, i, j, nplate, nplate, i, j);

         fprintf(fscript, "mImgtbl %s $1/plate_%02d_%02d/rimages.tbl\n", datadir, i, j);

         fprintf(fscript, "mProjExec -q -X -p %s $1/plate_%02d_%02d/rimages.tbl $1/plate_%02d_%02d/region.hdr $1/plate_%02d_%02d/projected $1/plate_%02d_%02d/stats.tbl\n",
               datadir, i, j, i, j, i, j, i, j);

         fprintf(fscript, "mImgtbl $1/plate_%02d_%02d/projected $1/plate_%02d_%02d/pimages.tbl\n", i, j, i, j);

         fprintf(fscript, "mOverlaps $1/plate_%02d_%02d/pimages.tbl $1/plate_%02d_%02d/diffs.tbl\n", i, j, i, j);
         
         if(level_only)
            fprintf(fscript, "mDiffFitExec -n -l -p $1/plate_%02d_%02d/projected $1/plate_%02d_%02d/diffs.tbl $1/plate_%02d_%02d/region.hdr $1/plate_%02d_%02d/diffs $1/plate_%02d_%02d/fits.tbl\n",
                  i, j, i, j, i, j, i, j, i, j);
         else
            fprintf(fscript, "mDiffFitExec -n -p $1/plate_%02d_%02d/projected $1/plate_%02d_%02d/diffs.tbl $1/plate_%02d_%02d/region.hdr $1/plate_%02d_%02d/diffs $1/plate_%02d_%02d/fits.tbl\n",
                  i, j, i, j, i, j, i, j, i, j);

         fprintf(fscript, "rm $1/plate_%02d_%02d/hpx%d.hdr $1/plate_%02d_%02d/region.hdr\n", i, j, level, i, j);
         fprintf(fscript, "rm $1/plate_%02d_%02d/diffs.tbl\n", i, j);
         fprintf(fscript, "rm $1/plate_%02d_%02d/rimages.tbl\n", i, j);
         fprintf(fscript, "rm $1/plate_%02d_%02d/stats.tbl $1/plate_%02d_%02d/hpx9.hdr\n", i, j, i, j);
         fprintf(fscript, "rm -rf $1/plate_%02d_%02d/diffs $1/plate_%02d_%02d/raw\n", i, j, i, j);
         fprintf(fscript, "rm $1/plate_%02d_%02d/projected/*_area.fits\n", i, j);

         fflush(fscript);
         fclose(fscript);
      }

      chmod(scriptfile, 0777);

      if(processing == CLUSTER)
         fprintf(fdriver, "sbatch --mem=16384 --mincpus=1 %sslurmProject.bash %sjobs/project_%02d_%02d.sh %s\n",
            scriptdir, scriptdir, i, j, projectdir);

      if(processing == SINGLE_THREADED)
         fprintf(fdriver, "%sjobs/project_%02d_%02d.sh %s\n",
            scriptdir, i, j, projectdir);

      fflush(fdriver);
   }

   fclose(fdriver);
   chmod(driverfile, 0777);

   printf("[struct stat=\"OK\", module=\"mHPXProjectScripts\", level=%d, pixlevel=%d, nimages=%d, exists=%d]\n",
      level, level+9, nimages, exists);
   fflush(stdout);
   exit(0);
} 
