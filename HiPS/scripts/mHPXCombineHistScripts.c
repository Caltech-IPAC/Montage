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


/********************************************************/
/*                                                      */
/*  mHPXCombineHistScripts                              */
/*                                                      */
/*  The original plate mosaics are usually make with a  */
/*  fair amount of overlap.  This script runs           */
/*  mCombineHist for each plate.  We will start with    */
/*  hard-coding a 3x3 box centered on the plate and go  */
/*  from there.                                         */
/*                                                      */
/********************************************************/

int main(int argc, char **argv)
{
   int  i, j, ch, istat, status, ncols, icol, jcol, count;
   int  ioff, joff, nplate, width, single_threaded;
   
   char inhistdir [MAXSTR];
   char outhistdir[MAXSTR];
   char platelist [MAXSTR];
   char scriptdir [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];
   char taskfile  [MAXSTR];
   char tmpdir    [MAXSTR];
   char tmpstr    [MAXSTR];
   char filename  [MAXSTR];
   char base      [MAXSTR];
   char mode      [MAXSTR];
   char cwd       [MAXSTR];

   char *ptr;

   FILE *fscript;
   FILE *fdriver;
   FILE *ftask;

   struct stat buf;

   getcwd(cwd, 1024);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   strcpy(base, "plate");

   strcpy(mode, "min max gaussian-log");

   single_threaded = 0;

   opterr = 0;

   while ((ch = getopt(argc, argv, "db:m:s")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 'b':
            strcpy(base, optarg);
            break;

         case 'm':
            strcpy(mode, optarg);
            break;

         case 's':
            single_threaded = 1;
            break;

         default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-b basename][-m mode][-s(ingle-threaded)] scriptdir platelist.tbl inhistdir outhistdir\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 4)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-b basename][-m mode][-s(ingle-threaded)] scriptdir platelist.tbl inhistdir outhistdir\"]\n", argv[0]);
      exit(1);
   }


   strcpy(scriptdir, argv[optind + 0]);
   strcpy(platelist, argv[optind + 1]);
   strcpy(inhistdir, argv[optind + 2]);
   strcpy(outhistdir,argv[optind + 3]);


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

   if(inhistdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, inhistdir);

      strcpy(inhistdir, tmpdir);
   }

   if(outhistdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, outhistdir);

      strcpy(outhistdir, tmpdir);
   }


   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(inhistdir[strlen(inhistdir)-1] != '/')
      strcat(inhistdir, "/");

   if(outhistdir[strlen(outhistdir)-1] != '/')
      strcat(outhistdir, "/");


   if(debug)
   {

      printf("single_threaded = %d\n\n", single_threaded);

      printf("base:       [%s]\n", base);
      printf("scriptdir:  [%s]\n", scriptdir);
      printf("platelist:  [%s]\n", platelist);
      printf("inhistdir:  [%s]\n", inhistdir);
      printf("outhistdir: [%s]\n", outhistdir);
      
      fflush(stdout);
   }

   if(strcmp(inhistdir, outhistdir) == 0)
   {
      printf ("[struct stat=\"ERROR\", msg=\"We cannot use the same directory for original and smoothed histograms.\"]\n");
      exit(1);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%schistSubmit.sh", scriptdir);

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
      sprintf(taskfile, "%schistTask.bash", scriptdir);

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
      fprintf(ftask, "#SBATCH -o %slogs/chist.%N.%%j.out # STDOUT\n", scriptdir);
      fprintf(ftask, "#SBATCH -e %slogs/chist.%N.%%j.err # STDERR\n", scriptdir);
      fprintf(ftask, "%sjobs/chist_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

      fflush(ftask);
      fclose(ftask);
   }


   /****************************************************************************/
   /* Read the image list, generating a combine histogram script for each one. */
   /****************************************************************************/

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

      i = atoi(tval(icol));
      j = atoi(tval(jcol));

      sprintf(scriptfile, "%s/jobs/chist_%d.sh", scriptdir, count);
      
      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] to histogram image %d.\"]\n",
            scriptfile, count);
         fflush(stdout);
         exit(0);
      }

      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "echo chist_%d.sh\n\n", count);

      fprintf(fscript, "echo %s_%02d_%02d, Task %d\n\n", base, i, j, count);

      fprintf(fscript, "mkdir -p %s\n", outhistdir);

      sprintf(filename, "%s%s_%02d_%02d.hist", inhistdir, base, i, j);

      status = stat(filename, &buf);

      if(debug)
      {
         printf("CENTER: [%s]: %d\n", filename, status);
         fflush(stdout);
      }

      if(status)
         continue;

      sprintf(tmpstr, "mCombineHist -stretch %s -log -out %s%s_%02d_%02d.hist %s",
         mode, outhistdir, base, i, j, filename);

      for(ioff=-1; ioff<=1; ++ioff)
      {
         for(ioff=-1; ioff<=1; ++ioff)
         {
            if(ioff == 0 && joff == 0)
               continue;

            sprintf(filename, "%s%s_%02d_%02d.hist", inhistdir, base, i+ioff, j+joff);

            status = stat(filename, &buf);

            if(debug)
            {
               printf("%d %d [%s]: %d\n", ioff, joff, filename, status);
               fflush(stdout);
            }

            if(status)
               continue;

            strcat(tmpstr, " ");
            strcat(tmpstr, filename);
         }
      }

      fprintf(fscript, "%s\n", tmpstr);

      fflush(fscript);
      fclose(fscript);

      if(debug)
      {
         printf("%s\n", tmpstr);
         fflush(stdout);
      }

      chmod(scriptfile, 0777);

      if(single_threaded)
      {
         fprintf(fdriver, "%sjobs/chist_%d.sh\n", scriptdir, count);
         fflush(fdriver);
      }

      ++count;
   }

   if(!single_threaded)
   {
      fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %schistTask.bash\n", 
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
