/* Module: mHiPSSetup.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        14Jul19  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define STRLEN 1024

int symlink(const char *target, const char *linkpath);

void mMkDir  (char *directory, int checkExists);
void mSymLink(char *linkdir, char *directory);


/*************************************************************************/
/*                                                                       */
/*  mHiPSSetup                                                           */
/*                                                                       */
/*  The HiPS generation process requires a reasonably involved directory */
/*  structure, especially when we spread the data over multiple storage  */
/*  nodes (in support of efficient parallelization).  This program       */
/*  can be given the basic info about storage locations (one base        */
/*  location and an arbitrary number of additional "subset" locations)   */
/*  and creates all the directories and soft links needed.  It is also   */
/*  given the location of some support scripts, which it uses to         */
/*  populate this space.                                                 */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int    i, j;

   char   mapname         [STRLEN];
   char   basedir         [STRLEN];
   char   sname           [STRLEN];
   char   directory       [STRLEN];
   char   script_directory[STRLEN];
   char   jobs_directory  [STRLEN];
   char   logs_directory  [STRLEN];
   char   cmd             [STRLEN];
   char   cwd             [STRLEN];

   FILE *fout;

   getcwd(cwd, STRLEN);


   // Parse the command-line

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSSetup mapname basedir\"]\n");
      exit(1);
   }

   strcpy(mapname, argv[1]);
   strcpy(basedir, argv[2]);

   if(basedir[0] != '/')
      sprintf(basedir, "%s/%s", cwd, argv[2]);


   // Create the storage location(s)

   sprintf(directory, "%s/%s", basedir, mapname);

   mMkDir(directory, 1);


   // In the base location, we need a scripts directory.

   sprintf(script_directory, "%s/%s/scripts",      basedir, mapname); mMkDir(script_directory, 0);
   sprintf(  jobs_directory, "%s/%s/scripts/jobs", basedir, mapname); mMkDir(  jobs_directory, 0);
   sprintf(  logs_directory, "%s/%s/scripts/logs", basedir, mapname); mMkDir(  logs_directory, 0);

   
   // Create a few support scripts in the scripts directory


   sprintf(sname, "%s/submitProject.bash", script_directory);
   fout = fopen(sname, "w+");

   fprintf(fout, "#!/bin/bash\n");
   fprintf(fout, "#SBATCH -p debug # partition (queue)\n");
   fprintf(fout, "#SBATCH -N 1 # number of nodes a single job will run on\n");
   fprintf(fout, "#SBATCH -n 1 # number of cores a single job will use\n");
   fprintf(fout, "#SBATCH -t 5-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
   fprintf(fout, "#SBATCH -o %s/project.%%N.%%j.out # STDOUT\n", logs_directory);
   fprintf(fout, "#SBATCH -e %s/project.%%N.%%j.err # STDERR\n", logs_directory);
   fprintf(fout, "$1 $2 $3\n");
   fclose(fout);
   chmod(sname, 0755);


   sprintf(sname, "%s/submitMosaic.bash", script_directory);
   fout = fopen(sname, "w+");

   fprintf(fout, "#!/bin/bash\n");
   fprintf(fout, "#SBATCH -p debug # partition (queue)\n");
   fprintf(fout, "#SBATCH -N 1 # number of nodes a single job will run on\n");
   fprintf(fout, "#SBATCH -n 1 # number of cores a single job will use\n");
   fprintf(fout, "#SBATCH -t 5-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
   fprintf(fout, "#SBATCH -o %s/mosaic.%%N.%%j.out # STDOUT\n", logs_directory);
   fprintf(fout, "#SBATCH -e %s/mosaic.%%N.%%j.err # STDERR\n", logs_directory);
   fprintf(fout, "$1 $2 $3\n");
   fclose(fout);
   chmod(sname, 0755);


   sprintf(sname, "%s/submitQuicklook.bash", script_directory);
   fout = fopen(sname, "w+");

   fprintf(fout, "#!/bin/bash\n");
   fprintf(fout, "#SBATCH -p debug # partition (queue)\n");
   fprintf(fout, "#SBATCH -N 1 # number of nodes a single job will run on\n");
   fprintf(fout, "#SBATCH -n 1 # number of cores a single job will use\n");
   fprintf(fout, "#SBATCH -t 5-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
   fprintf(fout, "#SBATCH -o %s/quicklook.%%N.%%j.out # STDOUT\n", logs_directory);
   fprintf(fout, "#SBATCH -e %s/quicklook.%%N.%%j.err # STDERR\n", logs_directory);
   fprintf(fout, "$1 $2 $3\n");
   fclose(fout);
   chmod(sname, 0755);


   sprintf(sname, "%s/%s/scripts/submitDiffFit.bash", basedir, mapname);
   fout = fopen(sname, "w+");

   fprintf(fout, "#!/bin/bash\n");
   fprintf(fout, "#SBATCH -p debug # partition (queue)\n");
   fprintf(fout, "#SBATCH -N 1 # number of nodes a single job will run on\n");
   fprintf(fout, "#SBATCH -n 1 # number of cores a single job will use\n");
   fprintf(fout, "#SBATCH -t 3-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
   fprintf(fout, "#SBATCH -o %s/diffFit.%%N.%%j.out # STDOUT\n", logs_directory);
   fprintf(fout, "#SBATCH -e %s/diffFit.%%N.%%j.err # STDERR\n", logs_directory);
   fprintf(fout, "$1 $2\n");
   fclose(fout);
   chmod(sname, 0755);


   sprintf(sname, "%s/%s/scripts/submitGap.bash", basedir, mapname);
   fout = fopen(sname, "w+");

   fprintf(fout, "#!/bin/bash\n");
   fprintf(fout, "#SBATCH -p debug # partition (queue)\n");
   fprintf(fout, "#SBATCH -N 1 # number of nodes a single job will run on\n");
   fprintf(fout, "#SBATCH -n 1 # number of cores a single job will use\n");
   fprintf(fout, "#SBATCH -t 3-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
   fprintf(fout, "#SBATCH -o %s/gap.%%N.%%j.out # STDOUT\n", logs_directory);
   fprintf(fout, "#SBATCH -e %s/gap.%%N.%%j.err # STDERR\n", logs_directory);
   fprintf(fout, "$1 $2\n");
   fclose(fout);
   chmod(sname, 0755);


   sprintf(sname, "%s/%s/scripts/submitBackground.bash", basedir, mapname);
   fout = fopen(sname, "w+");

   fprintf(fout, "#!/bin/bash\n");
   fprintf(fout, "#SBATCH -p debug # partition (queue)\n");
   fprintf(fout, "#SBATCH -N 1 # number of nodes a single job will run on\n");
   fprintf(fout, "#SBATCH -n 1 # number of cores a single job will use\n");
   fprintf(fout, "#SBATCH -t 3-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
   fprintf(fout, "#SBATCH -o %s/background.%%N.%%j.out # STDOUT\n", logs_directory);
   fprintf(fout, "#SBATCH -e %s/background.%%N.%%j.err # STDERR\n", logs_directory);
   fprintf(fout, "$1 $2 $3\n");
   fclose(fout);
   chmod(sname, 0755);


   sprintf(sname, "%s/%s/scripts/submitShrink.bash", basedir, mapname);
   fout = fopen(sname, "w+");

   fprintf(fout, "#!/bin/bash\n");
   fprintf(fout, "#SBATCH -p debug # partition (queue)\n");
   fprintf(fout, "#SBATCH -N 1 # number of nodes a single job will run on\n");
   fprintf(fout, "#SBATCH -n 1 # number of cores a single job will use\n");
   fprintf(fout, "#SBATCH -t 3-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
   fprintf(fout, "#SBATCH -o %s/shrink.%%N.%%j.out # STDOUT\n", logs_directory);
   fprintf(fout, "#SBATCH -e %s/shrink.%%N.%%j.err # STDERR\n", logs_directory);
   fprintf(fout, "$1 $2\n");
   fclose(fout);
   chmod(sname, 0755);


   sprintf(sname, "%s/%s/scripts/submitHiPSTiles.bash", basedir, mapname);
   fout = fopen(sname, "w+");

   fprintf(fout, "#!/bin/bash\n");
   fprintf(fout, "#SBATCH -p debug # partition (queue)\n");
   fprintf(fout, "#SBATCH -N 1 # number of nodes a single job will run on\n");
   fprintf(fout, "#SBATCH -n 1 # number of cores a single job will use\n");
   fprintf(fout, "#SBATCH -t 3-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
   fprintf(fout, "#SBATCH -o %s/tiles.%%N.%%j.out # STDOUT\n", logs_directory);
   fprintf(fout, "#SBATCH -e %s/tiles.%%N.%%j.err # STDERR\n", logs_directory);
   fprintf(fout, "$1 $2 $3\n");
   fclose(fout);
   chmod(sname, 0755);


   sprintf(sname, "%s/%s/scripts/submitPNG.bash", basedir, mapname);
   fout = fopen(sname, "w+");

   fprintf(fout, "#!/bin/bash\n");
   fprintf(fout, "#SBATCH -p debug # partition (queue)\n");
   fprintf(fout, "#SBATCH -N 1 # number of nodes a single job will run on\n");
   fprintf(fout, "#SBATCH -n 1 # number of cores a single job will use\n");
   fprintf(fout, "#SBATCH -t 3-00:00 # timeout (D-HH:MM)  aka. Don’t let this job run longer than this in case it gets hung\n");
   fprintf(fout, "#SBATCH -o %s/png.%%N.%%j.out # STDOUT\n", logs_directory);
   fprintf(fout, "#SBATCH -e %s/png.%%N.%%j.err # STDERR\n", logs_directory);
   fprintf(fout, "$1\n");
   fclose(fout);
   chmod(sname, 0755);


   // We also need a HiPS directory and per order subdirectories   
   // (we may not fill all of these depending on the data depth).   
   // The applicatons code will make further subdirectories of HiPS
   // depending on the range of tile numbers.

   sprintf(directory, "%s/%s/HiPS", basedir, mapname); mMkDir(directory, 0);

   for(i=0; i<10; ++i)
   {
      sprintf(directory, "%s/%s/HiPS/Norder%d", basedir, mapname, i);
      mMkDir(directory, 0);
   }


   // For now, there are two alternate approaches: Making submosaics and 
   // just reprojecting the images for a tile.

   sprintf(directory, "%s/%s/mosaics",         basedir, mapname); mMkDir(directory, 0);
   sprintf(directory, "%s/%s/mosaics/diffs",   basedir, mapname); mMkDir(directory, 0);

   sprintf(directory, "%s/%s/project",         basedir, mapname); mMkDir(directory, 0);
   sprintf(directory, "%s/%s/project/diffs",   basedir, mapname); mMkDir(directory, 0);


   // The plates directory is the most complicated, since we want to make
   // use of the multiple storage locations and we have to support the 
   // the multiple orders.

   sprintf(directory, "%s/%s/plates", basedir, mapname); mMkDir(directory, 0);

   for(j=0; j<10; ++j)
      sprintf(directory, "%s/%s/plates/order%d",         basedir, mapname, j); mMkDir(directory, 0);

   printf("[struct stat=\"OK\", module=\"mHiPSSetup\"]\n");
   fflush(stdout);
   exit(0);
}


void mMkDir(char *directory, int checkExists)
{
   int status;

   status = mkdir(directory, 0755);

   if(status < 0)
   {
      if(errno == EEXIST && checkExists == 1)
      {
         printf("[struct stat=\"ERROR\", msg=\"Error creating directory [%s]. Already exists.\"]\n", directory);
         fflush(stdout);
         exit(1);
      }

      if(errno != EEXIST)
      {
         printf("[struct stat=\"ERROR\", msg=\"Error creating directory [%s].\"]\n", directory);
         fflush(stdout);
         exit(1);
      }
   }
}


void mSymLink(char *linkdir, char *directory)
{
   int status;

   status = symlink(linkdir, directory);

   if(status < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Error linking directory [%s] to [%s]\"]\n", linkdir, directory);
      fflush(stdout);
      exit(1);
   }
}
