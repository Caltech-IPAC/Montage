#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <math.h>

#define STRLEN 1024

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int montage_debugCheck(char *debugStr);



/*************************************************************************/
/*                                                                       */
/*  mHPXGap                                                              */
/*                                                                       */
/*  In an HPX all-sky projection there are six sets of "adjacent" plates */
/*  that are actually across gaps in the projection space.  We have      */
/*  developed a routine to simulate the overlap difference measures      */
/*  needed by the background modeling by looking at the image background */
/*  levels along the edges that would match up.                          */
/*                                                                       */
/*  Rather than have that routine loop over all the pairs, we just let   */
/*  it do one and use this routine to build a list of the pairs, or for  */
/*  a cluster/cloud environment sets of lists so we can run multiple     */
/*  parallel threads.                                                    */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int   c, levelOnly, single_threaded, pad, width, debug;
   int   i, N;
   int   xface, yface;
   int   xmax, ymax;
   int   ix, jy, IX, JY;
   int   ifile, nplate;

   char  mosaicdir [STRLEN];
   char  scriptdir [STRLEN];
   char  driverfile[STRLEN];
   char  scriptfile[STRLEN];
   char  taskfile  [STRLEN];
   char  gaptbl    [STRLEN];
   char  wraptbl   [STRLEN];
   char  cmd       [STRLEN];
   char  tmpstr    [STRLEN];
   char  cwd       [STRLEN];

   FILE *fdriver;
   FILE *fscript;
   FILE *ftask;
   FILE *fgap;
   FILE *fwrap;

   char *end;

   getcwd(cwd, STRLEN);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug           = 0;
   width           = 4096;
   levelOnly       = 0;
   single_threaded = 0;

   opterr = 0;

   strcpy(mosaicdir, "");

   while ((c = getopt(argc, argv, "n:sdp:w:l")) != EOF) 
   {
      switch (c) 
      {
         case 'w':
            width = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) cannot be interpreted as an integer\"]\n", 
                  optarg);
               exit(1);
            }

            if(width < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) must be a positive integer\"]\n", 
                  optarg);
               exit(1);
            }

            break;

         case 'p':
            pad = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) cannot be interpreted as an integer\"]\n", 
                  optarg);
               exit(1);
            }

            if(pad < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) must be a positive integer\"]\n", 
                  optarg);
               exit(1);
            }

            break;

         case 's':
            single_threaded = 1;
            break;

         case 'd':
            debug = 1;
            break;

         case 'l':
            levelOnly = 1;
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXGapScripts [-d] [-p pad] [-w width] [-l(evel-only)] scriptdir mosaicdir nplate\"]\n");
            exit(1);
            break;
      }
   }

   if (argc - optind < 4) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXGapScripts [-d] [-p pad] [-w width] [-l(evel-only)] scriptdir mosaicdir nplate\"]\n");
      exit(1);
   }


   strcpy(scriptdir, argv[optind]);

   if(scriptdir[0] != '/')
   {
      strcpy(tmpstr, cwd);

      if(strlen(scriptdir) > 0)
      {
         strcat(tmpstr, "/");
         strcat(tmpstr, scriptdir);
      }

      strcpy(scriptdir, tmpstr);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");


   strcpy(mosaicdir, argv[optind+1]);

   if(mosaicdir[0] != '/')
   {
      strcpy(tmpstr, cwd);

      if(strlen(mosaicdir) > 0)
      {
         strcat(tmpstr, "/");
         strcat(tmpstr, mosaicdir);
      }

      strcpy(mosaicdir, tmpstr);
   }

   if(mosaicdir[strlen(mosaicdir)-1] != '/')
      strcat(mosaicdir, "/");


   nplate = atoi(argv[optind+2]);
   pad    = atoi(argv[optind+3]);

   N = nplate/5;

   if(debug)
   {
      printf("DEBUG> scriptdir = [%s]\n", scriptdir);
      printf("DEBUG> nplate    =  %d\n", nplate);
      printf("DEBUG> N         =  %d\n", N);
      printf("DEBUG> levelOnly =  %d\n", levelOnly);
      printf("DEBUG> width     =  %d\n", width);
      printf("DEBUG> pad       =  %d\n", pad);
      fflush(stdout);
   }

   
   // Make a list of the wrap-around plates

   sprintf(wraptbl, "%sgap/wrap.tbl", mosaicdir);

   fwrap = fopen(wraptbl, "w+");

   fprintf(fwrap, "|%-20s|%-20s|\n", "plus", "minus");

   for(ix=0; ix<N; ++ix)
   {
      for(jy=0; jy<N; ++jy)
      {
         fprintf(fwrap, " plate_%02d_%02d.fits     plate_%02d_%02d.fits  \n", ix, jy, ix+4*N, jy+4*N);
         fflush(fwrap);
      }
   }

   fclose(fwrap);


   // Open the driver script file

   if(mkdir(scriptdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create script directory [%s].\n", scriptdir);
      exit(1);
   }

   sprintf(driverfile, "%sgapSubmit.sh", scriptdir);

   fdriver = fopen(driverfile, "w+");

   if(debug)
   {
      printf("DEBUG> driver file: [%s]\n", driverfile);
      fflush(stdout);
   }

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);


   sprintf(tmpstr, "%sgap", mosaicdir);

   if(mkdir(scriptdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create gap directory [%s].\n", tmpstr);
      exit(1);
   }

   sprintf(gaptbl, "%sgap/gap.tbl", mosaicdir);
   
   fgap = fopen(gaptbl, "w+");

   fprintf(fgap, "|           file          |\n");



   /*************************************/
   /* Create the task submission script */
   /*************************************/

   if(!single_threaded)
   {
      sprintf(taskfile, "%sgapTask.bash", scriptdir);

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
      fprintf(ftask, "#SBATCH -o %slogs/gap.%%N.%%j.out # STDOUT\n", scriptdir);
      fprintf(ftask, "#SBATCH -e %slogs/gap.%%N.%%j.err # STDERR\n", scriptdir);
      fprintf(ftask, "%sjobs/gap_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

      fflush(ftask);
      fclose(ftask);
   }


   // Create the output script files

   sprintf(tmpstr, "%s/jobs", scriptdir);

   if(mkdir(scriptdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create jobs directory [%s].\n", scriptdir);
      exit(1);
   }

   sprintf(tmpstr, "%s/logs", scriptdir);

   if(mkdir(scriptdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create logs directory [%s].\n", scriptdir);
      exit(1);
   }


   // TOP to LEFT
   
   ifile = 1;

   for(xface=0; xface<4; ++xface)
   {
      ymax = (3 * N - 1) + N * xface;

      if(ymax > nplate)
         ymax = 2 * N -1;

      IX = ((xface + 1) % 4) * N;

      jy = (xface + 2) * N - 1;

      for(i=0; i<N; ++i)
      {
         ix = xface*N + i;

         JY = ymax - i;

         sprintf(scriptfile, "%sjobs/gap_%d.sh", scriptdir, ifile);

         if(debug)
         {
            printf("DEBUG> scriptfile = [%s]\n", scriptfile);
            fflush(stdout);
         }

         fscript = fopen(scriptfile, "w+");

         if(fscript == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open script file [%s].\n", tmpstr);
            exit(1);
         }

         fprintf(fgap, " plate_%02d_%02d.diff \n", ix, jy);
         fflush(fgap);

         fprintf(fscript, "#!/bin/sh\n\n");
         fflush(fscript);

         if(debug)
            sprintf(cmd, "mHPXGapDiff -d -p %d -w %d -g %s ", pad, width, mosaicdir);
         else
            sprintf(cmd, "mHPXGapDiff -p %d -w %d -g %s ", pad, width, mosaicdir);

         if(levelOnly)
            strcat(cmd, "-l ");

         sprintf(cmd, "%s%splate_%02d_%02d.fits top %splate_%02d_%02d.fits left", 
               cmd, mosaicdir, ix, jy, mosaicdir, IX, JY);

         if(debug)
         {
            printf("%s\n", cmd);
            fflush(stdout);
         }

         fprintf(fscript, "%s\n", cmd);
         fflush(fscript);
         fclose(fscript);

         chmod(scriptfile, 0775);

         if(single_threaded)
         {
            fprintf(fdriver, "%sjobs/gap_%d.sh\n", scriptdir, ifile);
            fflush(fdriver);
         }
         
         ++ifile;
      }
   }


   // BOTTOM to RIGHT

   for(yface=0; yface<4; ++yface)
   {
      xmax = (yface + 3) * N - 1;

      if(xmax > nplate)
         xmax = 2 * N - 1;

      JY = ((yface + 1) % 4) * N;

      ix = (yface + 2) * N - 1;

      for(i=0; i<N; ++i)
      {
         jy = yface * N + i;

         IX = xmax - i;

         
         sprintf(scriptfile, "%sjobs/gap_%d.sh", scriptdir, ifile);

         if(debug)
         {
            printf("DEBUG> scriptfile = [%s]\n", scriptfile);
            fflush(stdout);
         }

         fscript = fopen(scriptfile, "w+");

         if(fscript == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open script file [%s].\n", tmpstr);
            exit(1);
         }

         fprintf(fgap, " plate_%02d_%02d.diff \n", IX, JY);
         fflush(fgap);

         fprintf(fscript, "#!/bin/sh\n\n");
         fflush(fscript);

         if(debug)
            sprintf(cmd, "mHPXGapDiff -d -p %d -w %d -g %s ", pad, width, mosaicdir);
         else
            sprintf(cmd, "mHPXGapDiff -p %d -w %d -g %s ", pad, width, mosaicdir);

         if(levelOnly)
            strcat(cmd, "-l ");

         sprintf(cmd, "%s%splate_%02d_%02d.fits bottom %splate_%02d_%02d.fits right", 
               cmd, mosaicdir, IX, JY, mosaicdir, ix, jy);

         if(debug)
         {
            printf("%s\n", cmd);
            fflush(stdout);
         }

         fprintf(fscript, "%s\n", cmd);
         fflush(fscript);
         fclose(fscript);

         chmod(scriptfile, 0775);

         if(single_threaded)
         {
            fprintf(fdriver, "%sjobs/gap_%d.sh\n", scriptdir, ifile);
            fflush(fdriver);
         }

         ++ifile;
      }
   }

   fclose(fgap);

   --ifile;

   if(!single_threaded)
   {
      fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %sgapTask.bash\n",
         ifile, scriptdir);
      fflush(fdriver);
   }

   fclose(fdriver);

   chmod(driverfile, 0775);

   printf("[struct stat=\"OK\", module=\"mHPXGap\", ngap=%d]\n", ifile);
   fflush(stdout);
   exit(0);
}
