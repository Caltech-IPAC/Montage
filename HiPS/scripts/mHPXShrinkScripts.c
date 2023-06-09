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


/**********************************************************/
/*                                                        */
/*  mHPXShrinkScripts                                     */
/*                                                        */
/*  Create scripts that turn a set of plates for one      */
/*  HiPS order into plates for all the lower orders by    */
/*  successively shrinking by factors of two.             */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  i, ch, istat, ncols, order, iorder, count, cloud;
   int  iplate, single_threaded, pixlev, nsidePix;
   
   double pixscale;

   char platedir    [MAXSTR];
   char platelist   [MAXSTR];
   char scriptdir   [MAXSTR];
   char scriptfile  [MAXSTR];
   char driverfile  [MAXSTR];
   char taskfile    [MAXSTR];
   char tmpdir      [MAXSTR];
   char plate       [MAXSTR];
   char cwd         [MAXSTR];
   char cmd         [MAXSTR];
   char archive     [MAXSTR];
   char platearchive[MAXSTR];

   char *ptr;

   FILE *fscript;
   FILE *fdriver;
   FILE *ftask;

   getcwd(cwd, 1024);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   single_threaded = 0;
   cloud           = 0;

   strcpy(archive,      "");
   strcpy(platearchive, "");

   opterr = 0;

   while ((ch = getopt(argc, argv, "dsca:p:")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 's':
            single_threaded = 1;
            break;

         case 'c':
            cloud = 1;
            break;

         case 'a':
            strcpy(archive, optarg);
            break;

         case 'p':
            strcpy(platearchive, optarg);
            break;

         default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle-threaded)][-c(loud)][-a(rchive) bucket][-p(late-archive) bucket] order scriptdir platedir platelist.tbl\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle-threaded)][-c(loud)][-a(rchive) bucket][-p(late-archive) bucket] order scriptdir platedir platelist.tbl\"]\n", argv[0]);
      exit(1);
   }

   if(strlen(archive) > 0 && strlen(platearchive) == 0)
   {
      printf ("[struct stat=\"ERROR\", msg=\"If data is coming from an S3 bucket shrunken plates must go to another bucket.\"]\n");
      exit(1);
   }

   if(strlen(archive) > 0 && strlen(platearchive) > 0 && strcmp(archive, platearchive) == 0)
   {
      printf ("[struct stat=\"ERROR\", msg=\"S3 buckets for mosaics and shrunken plates cannot be the same.\"]\n");
      exit(1);
   }

   order = atoi(argv[optind]);

   strcpy(scriptdir, argv[optind + 1]);
   strcpy(platedir,  argv[optind + 2]);
   strcpy(platelist, argv[optind + 3]);

   if(cloud)
   {
      strcpy(platedir, "plates");
      strcpy(scriptdir, "");
   }
   else
   {
      if(platedir[0] != '/')
      {
         strcpy(tmpdir, cwd);
         strcat(tmpdir, "/");
         strcat(tmpdir, platedir);

         strcpy(platedir, tmpdir);
      }

      if(scriptdir[0] != '/')
      {
         strcpy(tmpdir, cwd);
         strcat(tmpdir, "/");
         strcat(tmpdir, scriptdir);

         strcpy(scriptdir, tmpdir);
      }

      if(scriptdir[strlen(scriptdir)-1] != '/')
         strcat(scriptdir, "/");
   }

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(debug)
   {

      printf("order           = %d\n",   order);
      printf("single_threaded = %d\n\n", single_threaded);

      printf("scriptdir: [%s]\n", scriptdir);
      printf("platedir:  [%s]\n", platedir);
      printf("platelist: [%s]\n\n", platelist);
      
      fflush(stdout);
   }


   if(!cloud)
   {
      /*******************************/
      /* Open the driver script file */
      /*******************************/

      sprintf(driverfile, "%sshrinkSubmit.sh", scriptdir);

      fdriver = fopen(driverfile, "w+");

      if(fdriver == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
         fflush(stdout);
         exit(0);
      }

      fprintf(fdriver, "#!/bin/sh\n\n");
      fflush(fdriver);


      /******************************************/
      /* Create the task submission script file */
      /******************************************/

      if(!single_threaded)
      {
         sprintf(taskfile, "%sshrinkTask.bash", scriptdir);

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
         fprintf(ftask, "#SBATCH -o %slogs/shrink.%%N.%%j.out # STDOUT\n", scriptdir);
         fprintf(ftask, "#SBATCH -e %slogs/shrink.%%N.%%j.err # STDERR\n", scriptdir);
         fprintf(ftask, "%sjobs/shrink_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

         fflush(ftask);
         fclose(ftask);
      }
   }



   /*****************************************************************/
   /* Read the image list, generating a shrink script for each one. */
   /*****************************************************************/

   ncols = topen(platelist);

   iplate = tcol("plate");

   if(iplate < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Platelist file does not contain 'plate' column.\"]\n");
      fflush(stdout);
      exit(0);
   }

   count = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      ++count;

      strcpy(plate, tval(iplate));

      if(strlen(archive) > 0)
         sprintf(scriptfile, "shrink_%d.sh", count);
      else
         sprintf(scriptfile, "%s/jobs/shrink_%d.sh", scriptdir, count);
      
      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] to shrink image %d.\"]\n",
            scriptfile, count);
         fflush(stdout);
         exit(0);
      }

      fprintf(fscript, "#!/bin/sh\n\n");
      fflush(fscript);

      fprintf(fscript, "date\n");
      fflush(fscript);

      fprintf(fscript, "echo Plate %s, Task %d\n\n", plate, count);
      fflush(fscript);

      fprintf(fscript, "if [ ! -d \"%s\" ]\n", platedir);
      fprintf(fscript, "then\n");
      fprintf(fscript, "    echo \"Directory %s DOES NOT exists.\"\n", platedir);
      fprintf(fscript, "    exit 9999\n");
      fprintf(fscript, "fi\n\n");

      fprintf(fscript, "if [ ! -d \"%sorder%d\" ]\n", platedir, order);
      fprintf(fscript, "then\n");
      fprintf(fscript, "    echo \"Directory %sorder%d DOES NOT exists.\"\n", platedir, order);
      fprintf(fscript, "    exit 9999\n");
      fprintf(fscript, "fi\n\n");

      if(strlen(archive) > 0)
      {
         sprintf(cmd, "aws s3 cp s3://%s/%s.fits %sorder%d/%s.fits --quiet",
            archive, plate, platedir, order, plate);

         fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
         fprintf(fscript, "%s\n", cmd);
         fflush(fscript);
      }

      for(iorder=order; iorder>=1; --iorder)
      {
         pixlev = iorder - 1 + 9;

         nsidePix = pow(2., (double)pixlev);

         pixscale  = 90.0 / nsidePix / sqrt(2.0);

         sprintf(cmd, "mkdir %sorder%d", platedir, iorder-1);

         fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
         fprintf(fscript, "%s\n", cmd);
         fflush(fscript);

         sprintf(cmd, "echo mShrink -p %.8f %sorder%d/%s.fits %sorder%d/%s.fits 2", 
            pixscale, platedir, iorder, plate, platedir, iorder-1, plate);

         fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
         fprintf(fscript, "%s\n", cmd);
         fflush(fscript);

         sprintf(cmd, "mShrink -p %.8f %sorder%d/%s.fits %sorder%d/%s.fits 2", 
            pixscale, platedir, iorder, plate, platedir, iorder-1, plate);

         fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
         fprintf(fscript, "%s\n", cmd);
         fflush(fscript);

         if(strlen(archive) > 0)
         {
            sprintf(cmd, "aws s3 cp %sorder%d/%s.fits s3://%s/order%d/%s.fits --quiet",
               platedir, iorder, plate, platearchive, iorder, plate);

            fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
            fprintf(fscript, "%s\n", cmd);
            fflush(fscript);
         }
      }

      if(strlen(archive) > 0)
      {
         sprintf(cmd, "aws s3 cp %sorder0/%s.fits s3://%s/order0/%s.fits --quiet",
            platedir, plate, platearchive, plate);

         fprintf(fscript, "\necho 'COMMAND: %s'\n", cmd);
         fprintf(fscript, "%s\n\n", cmd);
         fflush(fscript);
      }

      fclose(fscript);

      chmod(scriptfile, 0777);

      if(!cloud && single_threaded)
      {
         fprintf(fdriver, "%sjobs/shrink_%d.sh %s\n", scriptdir, count, platedir);
         fflush(fdriver);
      }
   }

   if(!cloud && !single_threaded)
   {
      fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %sshrinkTask.bash\n", 
         count, scriptdir, scriptdir);
      fflush(fdriver);
   }

   if(!cloud)
   {
      fclose(fdriver);

      chmod(driverfile, 0777);
   }


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", count=%d]\n", count);
   fflush(stdout);
   exit(0);
}
