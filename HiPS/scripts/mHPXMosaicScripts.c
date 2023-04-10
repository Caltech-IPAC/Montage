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


/****************************************************************************/
/*                                                                          */
/*  MHPXMOSAICSCRIPTS                                                       */
/*                                                                          */
/*  Create a set of scripts to that generate HPX plates with padding.       */
/*  The plates are always part of a regular array and, while not required,  */
/*  the plate size and padding should be powers of two.  Some of the        */
/*  downstream processing depends on this.                                  */
/*                                                                          */
/*  We need the same array of plates all through the processing so we have  */
/*  separated the plate list generation out into a separate program         */
/*  (mHPXPlateList) and just read that back in here.                        */
/*                                                                          */
/*  There are three binary decisions.  The data can be a set of local       */
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
/*  For CLUSTER processing, the main mosaicSubmit.sh script submits the     */
/*  plate processing jobs to the cluster as a SLURM "array" (which lets us  */
/*  control how many are processed at a time).                              */
/*                                                                          */
/*  For SINGLE_THREADED, that same script is structured to execute in       */
/*  foreground, running the processing jobs one after another.  The         */
/*  processing job scripts are the same in either case.                     */
/*                                                                          */
/*  Of the eight possible combinations, there are a few we haven't yet had  */
/*  to deal with:  ARCHIVE data that we PROJECT_ONLY.                       */
/*                                                                          */
/*                                                                          */
/*  There are times when we want the scripts to work in the local           */
/*  directory rather than have a predefined location.  This happens when    */
/*  building mosaics in cloud compute nodes, for instance. So we have       */
/*  included a "-c(loud)" flag to allow this.                               */
/*                                                                          */
/*  It is also desirable when working on a cloud to have the compute        */
/*  resources decommissioned at the end of each run, so we need some way    */
/*  to archive the results away somewhere.  To date, we have only worked    */
/*  with Amazon Web Services S3 storage and have implemented download to    */
/*  that through a "-a(rchive) <3binname>" flag.                            */
/*                                                                          */
/****************************************************************************/

int main(int argc, char **argv)
{
   int  level, level_only, lev_slope, c, pad, ncols, nplate;
   int  id, iplate, i, j, status, nimages, exists;
   int  iid, ii, ij, location, background, processing;
   int  exec_debug, cloud, nside, naxis, noff;

   char cwd       [1024];
   char survey    [1024];
   char band      [1024];
   char datadir   [1024];
   char outfile   [1024];
   char scriptfile[1024];
   char driverfile[1024];
   char taskfile  [1024];
   char mosaicdir [1024];
   char scriptdir [1024];
   char dir       [1024];
   char platelist [1024];
   char platename [1024];
   char tmpdir    [1024];
   char flags     [1024];
   char archive   [1024];
   char cmd       [1024];

   struct stat buf;

   FILE *fscript;
   FILE *fdriver;
   FILE *ftask;

   int debug = 0;

   cloud = 0;
   
   exec_debug = 2;

   strcpy(archive, "");

   getcwd(cwd, 1024);


   // Command-line arguments

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXMosaicScripts [-d][-e(xec-debug) level][-l(evel-only)][-t(oggle-level-and-slope)][-s(ingle-threaded)[-p(roject-only)][-o(overlap) pad][-c(loud)][-a(rchive) bucket] scriptdir mosaicdir platelist.tbl survey/band | datadir\"]\n");
      fflush(stdout);
      exit(1);
   }

   debug = 0;

   level_only   = 0;

   location     = ARCHIVE;
   background   = BACKGROUND_CORRECTION;
   processing   = CLUSTER;
   pad          = -1;

   while ((c = getopt(argc, argv, "de:lca:ltspo:")) != EOF)
   {
      switch (c)
      {
         case 'd':
            debug = 1;
            break;

         case 'e':
            exec_debug = atoi(optarg);
            break;

         case 'l':
            level_only = 1;
            break;

         case 'c':
            cloud = 1;
            break;

         case 'a':
            strcpy(archive, optarg);
            break;

         case 't':
            lev_slope = 1;
            break;

         case 's':
            processing = SINGLE_THREADED;
            break;

         case 'p':
            background = PROJECT_ONLY;
            break;

         case 'o':
            pad = atoi(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXMosaicScripts [-d][-e(xec-debug) level][-l(evel-only)][-t(oggle-level-and-slope)][-s(ingle-threaded)[-p(roject-only)][-o(overlap) pad][-c(loud)][-a(rchive) bucket] scriptdir mosaicdir platelist.tbl survey/band | datadir\"]\n");
            fflush(stdout);
            exit(1);
      }
   }

   if(cloud)
   {
      strcpy(scriptdir,  "");
      strcpy(mosaicdir,  "");
   }
   else
   {
      strcpy(scriptdir,  argv[optind]);
      strcpy(mosaicdir,  argv[optind + 1]);
   }

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

   if(!cloud)
   {
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

      if(mosaicdir[strlen(mosaicdir)-1] != '/')
         strcat(mosaicdir, "/");
   }

   strcpy(flags, "");

   if(level_only)
      strcat(flags, "-l ");
   else if(lev_slope)
      strcat(flags, "-t ");

   if(background == PROJECT_ONLY)
      strcat(flags, "-b ");


   // Make sure the necessary subdirectories exist

   if(!cloud)
   {
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
   }

   if(debug)
   {
      printf("DEBUG> exec_debug: [%d]\n", exec_debug);
      printf("DEBUG> level_only: [%d]\n", level_only);
      printf("DEBUG> lev_slope:  [%d]\n", lev_slope);
      printf("DEBUG> pad:         %d\n",  pad);
      printf("DEBUG> processing:  %d\n",  processing);
      printf("DEBUG> background:  %d\n",  background);
      printf("DEBUG> mosaicdir:  [%s]\n", mosaicdir);
      printf("DEBUG> scriptdir:  [%s]\n", scriptdir);
      printf("DEBUG> platelist:  [%s]\n", platelist);
      printf("DEBUG> survey:     [%s]\n", survey);
      printf("DEBUG> band:       [%s]\n", band);
      fflush(stdout);
   }


   // Open the driver script file

   if(cloud)
      sprintf(driverfile, "mosaicSubmit.sh");
   else
      sprintf(driverfile, "%smosaicSubmit.sh", scriptdir);

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


   // Open the task submission script file

   if(processing != SINGLE_THREADED)
   {
      sprintf(taskfile, "%smosaicTask.bash", scriptdir);

      if(debug)
      {
         printf("DEBUG> taskfile:   [%s]\n", taskfile);
         fflush(stdout);
      }

      ftask = fopen(taskfile, "w+");

      if(ftask == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open task submission file.\"]\n");
         fflush(stdout);
         exit(0);
      }

      fprintf(ftask, "#!/bin/bash\n");
      fprintf(ftask, "#SBATCH -p debug # partition (queue)\n");
      fprintf(ftask, "#SBATCH -N 1 # number of nodes a single job will run on\n");
      fprintf(ftask, "#SBATCH -n 1 # number of cores a single job will use\n");
      fprintf(ftask, "#SBATCH -t 5-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
      fprintf(ftask, "#SBATCH -o %slogs/mosaic.%%N.%%j.out # STDOUT\n", scriptdir);
      fprintf(ftask, "#SBATCH -e %slogs/mosaic.%%N.%%j.err # STDERR\n", scriptdir);
      fprintf(ftask, "%sjobs/mosaic_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

      fflush(ftask);
      fclose(ftask);
   }


   // Create the mosaic scripts. We double check that the plate intersects with
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

   nside = (int)pow(2., level+9.);

   naxis = 5 * nside / nplate;

   noff = 16 * naxis;

   if(pad == -1)
   {
      pad = (int)pow(2., (double)(level+4));

      if(pad < 128) pad = 128;
   }


   if(debug)
   {
      printf("DEBUG> iid    = %d\n", iid);
      printf("DEBUG> iplate = %d\n", iplate);
      printf("DEBUG> ii     = %d\n", ii);
      printf("DEBUG> ij     = %d\n", ij);
      printf("DEBUG> level  = %d\n", level);
      printf("DEBUG> nplate = %d\n", nplate);
      printf("DEBUG> naxis  = %d\n", naxis);
      printf("DEBUG> noff   = %d\n", noff);
      printf("DEBUG> pad    = %d\n", pad);
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

      sprintf(outfile, "%splate_%02d_%02d.fits", mosaicdir, i, j);


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

      if(cloud)
         sprintf(scriptfile, "mosaic_%d.sh", nimages);
      else
         sprintf(scriptfile, "%sjobs/mosaic_%d.sh", scriptdir, nimages);

      if(debug)
      {
         printf("DEBUG> scriptfile: [%s]\n", scriptfile);
         fflush(stdout);
      }

      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file for plate %d %d (number %d).\"]\n", i, j, nimages);
         fflush(stdout);
         exit(0);
      }

      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "date\n");

      fprintf(fscript, "echo 'Building plate_%02d_%02d.fits'\n", i, j);

      if(!cloud)
         fprintf(fscript, "mkdir -p %s\n", mosaicdir);

      sprintf(cmd, "mHPXHdr %d %shpx%d_%02d_%02d.hdr", level, mosaicdir, level, i, j);

         fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
      fprintf(fscript, "%s\n", cmd);

      sprintf(cmd, "mTileHdr %shpx%d_%02d_%02d.hdr %splate_%02d_%02d.hdr %d %d %d %d %d %d", 
         mosaicdir, level, i, j, mosaicdir, i, j, nplate, nplate, i, j, pad, pad);

      fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
      fprintf(fscript, "%s\n", cmd);

      if(location == ARCHIVE)
      {
         sprintf(cmd, "mExec -d %d -q %s -c -W \"%d %d\" -f %splate_%02d_%02d.hdr -o %splate_%02d_%02d.fits %s %s %swork_%02d_%02d", 
             exec_debug, flags, noff, noff, mosaicdir, i, j, mosaicdir, i, j, survey, band, mosaicdir, i, j);

         fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
         fprintf(fscript, "%s\n", cmd);
      }
         
      if(location == LOCAL)
      {
         sprintf(cmd, "mExec -d %d -q %s -c -W \"%d %d\" -f %splate_%02d_%02d.hdr -o %splate_%02d_%02d.fits -r %s %swork_%02d_%02d", 
          exec_debug, flags, noff, noff, mosaicdir, i, j, mosaicdir, i, j, datadir, mosaicdir, i, j);

         fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
         fprintf(fscript, "%s\n", cmd);
      }

      fprintf(fscript, "\nrm -rf %swork_%02d_%02d\n", mosaicdir, i, j);
      fprintf(fscript, "rm -f %shpx%d_%02d_%02d.hdr\n", mosaicdir, level, i, j);
      fprintf(fscript, "rm -f %splate_%02d_%02d.hdr\n", mosaicdir, i, j);
      fprintf(fscript, "rm -f %splate_%02d_%02d_area.fits\n", mosaicdir, i, j);

      if(strlen(archive) > 0)
      {
         sprintf(cmd, "aws s3 cp plate_%02d_%02d.fits s3://%s/plate_%02d_%02d.fits",
            i, j, archive, i, j);

         fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
         fprintf(fscript, "%s\n", cmd);
      }

      fflush(fscript);
      fclose(fscript);

      chmod(scriptfile, 0777);

      if(processing == SINGLE_THREADED)
      {
         if(cloud)
            fprintf(fdriver, "mosaic_%d.sh\n", nimages);
         else
            fprintf(fdriver, "%sjobs/mosaic_%d.sh\n", scriptdir, nimages);
      }

      fflush(fdriver);
   }

   if(processing == CLUSTER)
   {
      fprintf(fdriver, "sbatch --array=1-%d%20 --mem=16384 --mincpus=1 %smosaicTask.bash\n",
         nimages, scriptdir);

      fflush(fdriver);
   }

   fclose(fdriver);
   chmod(driverfile, 0777);

   printf("[struct stat=\"OK\", module=\"mHPXMosaicScripts\", level=%d, pixlevel=%d, nimages=%d, exists=%d]\n",
      level, level+9, nimages, exists);
   fflush(stdout);
   exit(0);
}
