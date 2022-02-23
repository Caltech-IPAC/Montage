#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <dirent.h>
#include <fitsio.h>

#define MAXSTR  4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int debug;


/**********************************************************/
/*                                                        */
/*  mHPXQuicklookScripts                                  */
/*                                                        */
/*  Make shrunken versions and PNGs for all FITS images   */
/*  ina directory.                                        */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  i, ch, status, job, mode, shrink;
   
   double shrink_factor;

   char  scriptdir    [MAXSTR];
   char  platedir     [MAXSTR];
   char  brightness   [MAXSTR];
   char  contrast     [MAXSTR];
   char  quicklookdir [MAXSTR];
   char  scriptfile   [MAXSTR];
   char  driverfile   [MAXSTR];
   char  taskfile     [MAXSTR];
   char  tmpdir       [MAXSTR];
   char  tmpstr       [MAXSTR];
   char  filename     [MAXSTR];
   char  histfile     [MAXSTR];
   char  base         [MAXSTR];
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

   shrink = 1;

   strcpy(histfile,   "");
   strcpy(brightness, "");
   strcpy(contrast  , "");

   while ((ch = getopt(argc, argv, "dnb:c:h:")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 'n':
            shrink = 0;
            break;

         case 'h':
            strcpy(histfile, optarg);
            break;

         case 'c':
            strcpy(contrast, optarg);
            break;

         case 'b':
            strcpy(brightness, optarg);
            break;

         default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-n(o-shrink)][-h histfile][-b brightness][-c contrast] scriptdir platedir quicklookdir shrink_factor\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 4)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-n(o-shrink)][-h histfile][-b brightness][-c contrast] scriptdir platedir quicklookdir shrink_factor\"]\n", argv[0]);
      exit(1);
   }

   strcpy(scriptdir,    argv[optind]);
   strcpy(platedir,     argv[optind + 1]);
   strcpy(quicklookdir, argv[optind + 2]);

   shrink_factor = atof(argv[optind + 3]);

   if(shrink_factor <= 0.)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Shrink factor must be a positive number.\"]\n");
      exit(1);
   }

   if(scriptdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, scriptdir);

      strcpy(scriptdir, tmpdir);
   }

   if(platedir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platedir);

      strcpy(platedir, tmpdir);
   }

   if(histfile[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, histfile);

      strcpy(histfile, tmpdir);
   }

   if(quicklookdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, quicklookdir);

      strcpy(quicklookdir, tmpdir);
   }


   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(quicklookdir[strlen(quicklookdir)-1] != '/')
      strcat(quicklookdir, "/");

   if(debug)
   {
      printf("scriptdir:    [%s]\n", scriptdir);
      printf("platedir:     [%s]\n", platedir);
      printf("histfile:     [%s]\n", histfile);
      printf("quicklookdir: [%s]\n", quicklookdir);
      printf("shrink_factor:  %-g\n\n", shrink_factor);
      
      fflush(stdout);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%squicklookSubmit.sh", scriptdir);

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

   sprintf(taskfile, "%squicklookTask.bash", scriptdir);

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
   fprintf(ftask, "#SBATCH -o %slogs/quicklook.%N.%%j.out # STDOUT\n", scriptdir);
   fprintf(ftask, "#SBATCH -e %slogs/quicklook.%N.%%j.err # STDERR\n", scriptdir);
   fprintf(ftask, "%sjobs/quicklook_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

   fflush(ftask);
   fclose(ftask);


   /********************************************************/
   /* Look through the FITS images in the plate directory, */
   /* creating a quick-look script for each one.           */
   /********************************************************/

   job = 1;

   chdir(platedir);

   strcpy(flags, "");

   if(strlen(brightness) > 0)
   {
      sprintf(tmpstr, "-brightness %s ", brightness);
      strcat(flags, tmpstr);
   }

   if(strlen(contrast) > 0)
   {
      sprintf(tmpstr, "-contrast %s ", contrast);
      strcat(flags, tmpstr);
   }
      

   dirp = opendir(platedir);

   if(dirp == NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open plate directory.\"]\n");
      fflush(stdout);
      return(0);
   }

   while ( (direntp = readdir( dirp )) != NULL )
   {
      status = 0;

      strcpy(filename, direntp->d_name);

      strcpy(tmpstr, filename);

      ptr = tmpstr + strlen(tmpstr) - 1;

      while(ptr != tmpstr && *ptr != '.')
         --ptr;

      if(ptr == tmpstr || *ptr != '.')
      {
         if(debug)
         {
            printf("DEBUG> Invalid filename format (expected base.fits)\n");
            fflush(stdout);
         }
         
         continue;
      }

      *ptr = '\0';

      strcpy(base, tmpstr);

      strcpy(ext, ptr+1);


      if(fits_open_file(&fptr, filename, READONLY, &status))
      {
         if(debug)
         {
            printf("DEBUG> File [%s] is not FITS.\n", filename);
            fflush(stdout);
         }

         continue;
      }

      fits_close_file(fptr, &status);

      if(debug)
      {
         printf("DEBUG> Found FITS file: [%s]\n", filename);
         fflush(stdout);
      }


      sprintf(scriptfile, "%s/jobs/quicklook_%d.sh", scriptdir, job);

      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] for quicklook processing [number %d].\"]\n",
            scriptfile, job);
         fflush(stdout);
         exit(0);
      }


      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "echo quicklook_%d.sh\n\n", job);

      fprintf(fscript, "echo Plate %s, Task %d\n\n", base, job);

      fprintf(fscript, "mkdir -p %s\n", quicklookdir);

      fprintf(fscript, "mkdir -p %ssmall\n", quicklookdir);


      if(shrink)
         fprintf(fscript, "mShrink %s%s.%s %s%s.%s %-g\n", 
            platedir, base, ext, quicklookdir, base, ext, shrink_factor);

      if(strlen(histfile) == 0)
         fprintf(fscript, "mViewer -ct 0 %s -gray %s%s.%s min max gaussian-log -out %s%s.png\n", 
            flags, quicklookdir, base, ext, quicklookdir, base);
      else
         fprintf(fscript, "mViewer -ct 0 %s -gray %s%s.%s -histfile %s -out %s%s.png\n", 
            flags, quicklookdir, base, ext, histfile, quicklookdir, base);


      if(shrink)
         fprintf(fscript, "mShrink %s%s.%s %ssmall/%s.%s 16\n", 
            quicklookdir, base, ext, quicklookdir, base, ext);

      if(strlen(histfile) == 0)
         fprintf(fscript, "mViewer -ct 0 %s -gray %ssmall/%s.%s min max gaussian-log -out %ssmall/%s.png\n", 
            flags, quicklookdir, base, ext, quicklookdir, base);
      else
         fprintf(fscript, "mViewer -ct 0 %s -gray %ssmall/%s.%s -histfile %s -out %ssmall/%s.png\n", 
            flags, quicklookdir, base, ext, histfile, quicklookdir, base);

      fflush(fscript);
      fclose(fscript);

      chmod(scriptfile, 0777);

      ++job;
   }

   --job;

   fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %squicklookTask.bash\n", 
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
