#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <mtbl.h>

#define MAXSTR  4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int debug;


/***********************************************************/
/*                                                         */
/*  mHPXBlendScripts                                       */
/*                                                         */
/*  The original plate mosaics are usually make with a     */
/*  fair amount of overlap.  This script runs              */
/*  mHPXBlendEdges for each plate (or a flattend version)  */
/*  making a set of images where the transitions are       */
/*  invisible.                                             */
/*                                                         */
/***********************************************************/

int main(int argc, char **argv)
{
   int  i, j, ch, istat, ncols, icol, jcol, count;
   int  nplate, width, blend, single_threaded;
   
   char platedir  [MAXSTR];
   char blenddir  [MAXSTR];
   char platelist [MAXSTR];
   char scriptdir [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];
   char taskfile  [MAXSTR];
   char tmpdir    [MAXSTR];
   char plate     [MAXSTR];
   char base      [MAXSTR];
   char cwd       [MAXSTR];

   char *ptr;

   FILE *fscript;
   FILE *fdriver;
   FILE *ftask;

   getcwd(cwd, 1024);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   blend = 1;

   strcpy(base, "plate");

   single_threaded = 0;

   opterr = 0;

   while ((ch = getopt(argc, argv, "dcb:s")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 'c':
            blend = 0;
            break;

         case 'b':
            strcpy(base, optarg);
            break;

         case 's':
            single_threaded = 1;
            break;

         default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-c(enter-only)][-b basename][-s(ingle-threaded)] scriptdir platelist.tbl width platedir blenddir\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 5)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-c(enter-only)][-b basename][-s(ingle-threaded)] scriptdir platelist.tbl width platedir blenddir\"]\n", argv[0]);
      exit(1);
   }


   strcpy(scriptdir, argv[optind + 0]);
   strcpy(platelist, argv[optind + 1]);

   width = atoi(argv[optind + 2]);

   strcpy(platedir,  argv[optind + 3]);
   strcpy(blenddir,  argv[optind + 4]);


   if(scriptdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, scriptdir);

      strcpy(scriptdir, tmpdir);
   }

   if(platelist[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platelist);

      strcpy(platelist, tmpdir);
   }

   if(platedir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platedir);

      strcpy(platedir, tmpdir);
   }

   if(blenddir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, blenddir);

      strcpy(blenddir, tmpdir);
   }


   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(blenddir[strlen(blenddir)-1] != '/')
      strcat(blenddir, "/");


   if(debug)
   {

      printf("single_threaded = %d\n\n", single_threaded);
      printf("width           = %d\n\n", width);

      printf("base:      [%s]\n", base);
      printf("scriptdir: [%s]\n", scriptdir);
      printf("platelist: [%s]\n", platelist);
      printf("platedir:  [%s]\n", platedir);
      printf("blenddir:  [%s]\n", blenddir);
      
      fflush(stdout);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%sblendSubmit.sh", scriptdir);

   fdriver = fopen(driverfile, "w+");

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);


   /*************************************/
   /* Create the task submission script */
   /*************************************/

   if(!single_threaded)
   {
      sprintf(taskfile, "%sblendTask.bash", scriptdir);

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
      fprintf(ftask, "#SBATCH -o %slogs/blend.%%N.%%j.out # STDOUT\n", scriptdir);
      fprintf(ftask, "#SBATCH -e %slogs/blend.%%N.%%j.err # STDERR\n", scriptdir);
      fprintf(ftask, "%sjobs/blend_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

      fflush(ftask);
      fclose(ftask);
   }


   /****************************************************************/
   /* Read the image list, generating a blend script for each one. */
   /****************************************************************/

   ncols = topen(platelist);

   nplate = atoi(tfindkey("nplate"));

   icol = tcol("i");
   jcol = tcol("j");

   if(icol < 0
   || jcol < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Platelist file does not contain 'i', 'j' colums.\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(debug)
   {
      printf("nplate          = %d\n", nplate);
      printf("icol            = %d\n", icol);
      printf("jcol            = %d\n", jcol);
      fflush(stdout);
   }


   count = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      ++count;

      i = atoi(tval(icol));
      j = atoi(tval(jcol));

      sprintf(scriptfile, "%s/jobs/blend_%d.sh", scriptdir, count);
      
      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] to blend image %d.\"]\n",
            scriptfile, count);
         fflush(stdout);
         exit(0);
      }

      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "echo blend_%d.sh\n\n", count);
      fprintf(fscript, "echo Plate %02d_%02d, Task %d\n\n", i, j, count);

      fprintf(fscript, "mkdir -p %s\n\n", blenddir);

      if(blend)
         fprintf(fscript, "mHPXBlendEdges -b %s -p %s %d %d %d %d %s%s_%02d_%02d.fits\n",
            base, platedir, i, j, nplate, width, blenddir, base, i, j);
      else
         fprintf(fscript, "mHPXBlendEdges -c -b %s -p %s %d %d %d %d %s%s_%02d_%02d.fits\n",
            base, platedir, i, j, nplate, width, blenddir, base, i, j);

      fflush(fscript);
      fclose(fscript);

      if(debug)
      {
         if(blend)
            printf("mHPXBlendEdges -b %s -p %s %d %d %d %d %s%s_%02d_%02d.fits\n",
               base, platedir, i, j, nplate, width, blenddir, base, i, j);
         else
            printf("mHPXBlendEdges -c -b %s -p %s %d %d %d %d %s%s_%02d_%02d.fits\n",
               base, platedir, i, j, nplate, width, blenddir, base, i, j);
         fflush(stdout);
      }

      chmod(scriptfile, 0777);

      if(single_threaded)
      {
         fprintf(fdriver, "%sjobs/blend_%d.sh\n", scriptdir, count);
         fflush(fdriver);
      }
   }

   if(!single_threaded)
   {
      fprintf(fdriver, "sbatch --array=1-%d --mem=8192 --mincpus=1 %sblendTask.bash\n", 
         count, scriptdir);
      fflush(fdriver);
   }

   fclose(fdriver);

   chmod(driverfile, 0777);


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", count=%d]\n", count);
   fflush(stdout);
   exit(0);
}
