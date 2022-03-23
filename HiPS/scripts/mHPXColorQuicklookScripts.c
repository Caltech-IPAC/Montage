#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <dirent.h>
#include <fitsio.h>
#include <mtbl.h>

#define MAXSTR  4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int debug;


/**********************************************************/
/*                                                        */
/*  mHPXColorQuicklookScripts                             */
/*                                                        */
/*  Use three bands of quicklook files to build a color   */
/*  PNG quicklook.                                        */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  i, j, ch, status, job, mode;
   int  icol, jcol, ncols;

   char  scriptdir    [MAXSTR];
   char  platelist    [MAXSTR];
   char  quicklook1   [MAXSTR];
   char  quicklook2   [MAXSTR];
   char  quicklook3   [MAXSTR];
   char  colordir     [MAXSTR];
   char  scriptfile   [MAXSTR];
   char  driverfile   [MAXSTR];
   char  taskfile     [MAXSTR];
   char  tmpdir       [MAXSTR];
   char  tmpstr       [MAXSTR];
   char  filename     [MAXSTR];
   char  histfile1    [MAXSTR];
   char  histfile2    [MAXSTR];
   char  histfile3    [MAXSTR];
   char  ext          [MAXSTR];
   char  cwd          [MAXSTR];
   char  flags        [MAXSTR];

   char *ptr;

   DIR    *dirp;
   struct  dirent *direntp;

   struct stat buf;

   FILE *fscript;
   FILE *fdriver;
   FILE *ftask;

   fitsfile *fptr;

   getcwd(cwd, 1024);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   opterr = 0;

   strcpy(quicklook1, "");
   strcpy(quicklook2, "");
   strcpy(quicklook3, "");

   strcpy(histfile1,  "");
   strcpy(histfile2,  "");
   strcpy(histfile3,  "");

   strcpy(flags,      "");

   while ((ch = getopt(argc, argv, "df:")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 'f':
            strcpy(flags, optarg);
            break;

         default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] scriptdir platelist.tbl quicklookblue histblue quicklookgreen histgreen quicklookred histred colordir\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 9)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] scriptdir platelist.tbl quicklookblue histblue quicklookgreen histgreen quicklookred histred colordir\"]\n", argv[0]);
      exit(1);
   }

   strcpy(scriptdir,  argv[optind]);
   strcpy(platelist,  argv[optind + 1]);
   strcpy(quicklook1, argv[optind + 2]);
   strcpy(histfile1,  argv[optind + 3]);
   strcpy(quicklook2, argv[optind + 4]);
   strcpy(histfile2,  argv[optind + 5]);
   strcpy(quicklook3, argv[optind + 6]);
   strcpy(histfile3,  argv[optind + 7]);
   strcpy(colordir,   argv[optind + 8]);

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

   if(quicklook1[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, quicklook1);

      strcpy(quicklook1, tmpdir);
   }

   if(quicklook2[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, quicklook2);

      strcpy(quicklook2, tmpdir);
   }

   if(quicklook3[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, quicklook3);

      strcpy(quicklook3, tmpdir);
   }

   if(histfile1[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, histfile1);

      strcpy(histfile1, tmpdir);
   }

   if(histfile2[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, histfile2);

      strcpy(histfile2, tmpdir);
   }

   if(histfile3[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, histfile3);

      strcpy(histfile3, tmpdir);
   }

   if(colordir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, colordir);

      strcpy(colordir, tmpdir);
   }


   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(quicklook1[strlen(quicklook1)-1] != '/')
      strcat(quicklook1, "/");

   if(quicklook2[strlen(quicklook2)-1] != '/')
      strcat(quicklook2, "/");

   if(quicklook3[strlen(quicklook3)-1] != '/')
      strcat(quicklook3, "/");

   if(colordir[strlen(colordir)-1] != '/')
      strcat(colordir, "/");

   if(debug)
   {
      printf("scriptdir:    [%s]\n", scriptdir);
      printf("platelist:    [%s]\n", platelist);
      printf("quicklook1:   [%s]\n", quicklook1);
      printf("histfile1:    [%s]\n", histfile1);
      printf("quicklook2:   [%s]\n", quicklook2);
      printf("histfile2:    [%s]\n", histfile2);
      printf("quicklook3:   [%s]\n", quicklook3);
      printf("histfile3:    [%s]\n", histfile3);
      printf("colordir:     [%s]\n", colordir);
      
      fflush(stdout);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%scolorSubmit.sh", scriptdir);

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

   sprintf(taskfile, "%scolorTask.bash", scriptdir);

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
   fprintf(ftask, "#SBATCH -o %slogs/color.%%N.%%j.out # STDOUT\n", scriptdir);
   fprintf(ftask, "#SBATCH -e %slogs/color.%%N.%%j.err # STDERR\n", scriptdir);
   fprintf(ftask, "%sjobs/color_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

   fflush(ftask);
   fclose(ftask);


   /********************************************************************/
   /* Read the image list, generating a histogram script for each one. */
   /********************************************************************/

   ncols = topen(platelist);

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
      printf("icol            = %d\n", icol);
      printf("jcol            = %d\n", jcol);
      fflush(stdout);
   }

   job = 1;

   while(1)
   {
      if(tread() < 0)
         break;

      i = atoi(tval(icol));
      j = atoi(tval(jcol));

      sprintf(scriptfile, "%s/jobs/color_%d.sh", scriptdir, job);

      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] for color quicklook processing [number %d].\"]\n",
            scriptfile, job);
         fflush(stdout);
         exit(0);
      }

      sprintf(filename, "plate_%02d_%02d.", i, j);

      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "echo color_%d.sh\n\n", job);

      fprintf(fscript, "echo Plate %sfits, Task %d\n\n", filename, job);

      fprintf(fscript, "mkdir -p %s\n", colordir);
      fprintf(fscript, "mkdir -p %s/small\n", colordir);

      fprintf(fscript, "mViewer -nowcs %s -blue %s%sfits -histfile %s -green %s%sfits -histfile %s -red %s%sfits -histfile %s -out %s%spng\n",
            flags, quicklook1, filename, histfile1, quicklook2, filename, histfile2, quicklook3, filename, histfile3, colordir, filename);

      fprintf(fscript, "mViewer -nowcs %s -blue %ssmall/%sfits -histfile %s -green %ssmall/%sfits -histfile %s -red %ssmall/%sfits -histfile %s -out %ssmall/%spng\n",
            flags, quicklook1, filename, histfile1, quicklook2, filename, histfile2, quicklook3, filename, histfile3, colordir, filename);

      fflush(fscript);
      fclose(fscript);

      chmod(scriptfile, 0777);

      ++job;
   }

   --job;

   fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %scolorTask.bash\n", 
      job, scriptdir);
   fflush(fdriver);

   fclose(fdriver);

   chmod(driverfile, 0777);


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", job=%d]\n", job);
   fflush(stdout);
   exit(0);
}
