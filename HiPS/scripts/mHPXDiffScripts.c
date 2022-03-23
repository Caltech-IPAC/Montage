#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mtbl.h>

int NFILE   = 1024;
int MAXFILE = 1024;

int STRLEN  = 1024;

int makePlateList(int nplate);
int makePlatePairs();

struct 
{
   int  n;
   int  x    [250000];
   int  y    [250000];
}
plateList;


struct 
{
   int  n;
   int  x1    [250000];
   int  y1    [250000];
   int  x2    [250000];
   int  y2    [250000];
   int  cntr1 [250000];
   int  cntr2 [250000];
   char plate1[250000][1024];
   char plate2[250000][1024];
}
platePair;


struct
{
   int n;
   int cntr  [5000];
   char fname[5000][1024];
}
plates;


/***************************************************************************/
/*                                                                         */
/*  Split a diffs table up unto subsets.  We will run mDiffFitExec on      */
/*  each one and then concatenate the fits.tbl files that result.          */
/*                                                                         */
/***************************************************************************/

int main(int argc, char **argv)
{
   int  i, order, nplate, nTask, nPerTask, ncols, set, nread;
   int  j, xbase, ybase, xref, yref, namelen, nPair;
   int  level_only;

   char cwd       [STRLEN];
   char tmpdir    [STRLEN];
   char scriptdir [STRLEN];
   char scriptFile[STRLEN];
   char driverfile[STRLEN];
   char taskfile  [STRLEN];
   char combofile [STRLEN];
   char platedir  [STRLEN];
   char imgtbl    [STRLEN];

   char difffile  [STRLEN];
   char outDiffs  [STRLEN];
   char fmt       [STRLEN];

   char *ptr;

   int  icntr, ifile, nfile;
   int  *cntr;
   int  *xindex;
   int  *yindex;
   char **fname;

   FILE *fscript;
   FILE *fdriver;
   FILE *ftask;
   FILE *fcombo;
   FILE *fdiff;

   int debug = 0;

   getcwd(cwd, 1024);

   nTask = 50;


   /**************************/
   /* Command-line arguments */
   /**************************/

   level_only = 0;

   if(argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXDiffScripts [-d(ebug)][-l(evel-only)] scriptdir platedir images.tbl order nplate\"]\n");
      fflush(stdout);
      exit(0);
   }

   for(i=1; i<3; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
         debug = 1;

      if(strcmp(argv[i], "-l") == 0)
         level_only = 1;
   }

   if(debug)
   {
      ++argv;
      --argc;
   }

   if(level_only)
   {
      ++argv;
      --argc;
   }

   if(argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXDiffScripts [-d(ebug)][-l(evel-only)] scriptdir platedir images.tbl order nplate\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(scriptdir, argv[1]);
   strcpy(platedir,  argv[2]);
   strcpy(imgtbl,    argv[3]);

   order  = atoi(argv[4]);
   nplate = atoi(argv[5]);

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

   if(imgtbl[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, imgtbl);

      strcpy(imgtbl, tmpdir);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(debug)
   {
      printf("\n");
      printf("DEBUG> nplate       =  %d\n",  nplate);
      printf("DEBUG> nTask        =  %d (hard-coded)\n",  nTask);
      printf("DEBUG> scriptdir    = [%s]\n", scriptdir);
      printf("DEBUG> platedir     = [%s]\n", platedir);
      printf("DEBUG> imgtbl       = [%s]\n", imgtbl);
      printf("\n");
      fflush(stdout);
   }


   /********************************************************/
   /* The plates follow a regular pattern so we can define */
   /* the differences that exist algorithmically.          */
   /********************************************************/
   
   ncols = topen(imgtbl);

   icntr = tcol("cntr");
   ifile = tcol("fname");

   if(icntr < 0 || ifile < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need 'cntr' and 'fname' columns in image metadata file.\n");
      fflush(stdout);
      exit(0);
   }


   plates.n = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      plates.cntr[plates.n] = atoi(tval(icntr));

      strcpy(plates.fname[plates.n], tval(ifile));

      ++plates.n;
   }

   tclose();


   makePlateList(nplate);

   makePlatePairs();

   namelen = strlen(plates.fname[0]);

   if(debug)
   {
      printf("DEBUG> Exact plate pair (diff) count: %d\n\n", platePair.n);

      for(i=0; i<platePair.n; ++i)
      {
         printf("DEBUG> %5d:  plate_%02d_%02d (%4d:%s) vs. plate_%02d_%02d (%4d:%s)\n", 
               i, platePair.x1[i], platePair.y1[i], platePair.cntr1[i], platePair.plate1[i],
                  platePair.x2[i], platePair.y2[i], platePair.cntr2[i], platePair.plate2[i]);
         fflush(stdout);
      }
   }


   /***********************************************************************/
   /* Using the list of differences, we need to build a set of scripts to */
   /* generate them.                                                      */
   /***********************************************************************/
   
   nPair = platePair.n;

   sprintf(difffile, "%s/diffs.tbl", platedir);

   fdiff = fopen(difffile, "w+");

   if(fdiff == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create diffs.tbl file (in %s).\n", platedir);
      fflush(stdout);
      exit(0);
   }

   sprintf(fmt, "| cntr1 | cntr2 |%%%ds |%%%ds |         diff             |\n", namelen, namelen);
   fprintf(fdiff, fmt, "plus", "minus");

   sprintf(fmt, "| int   | int   |%%%ds |%%%ds |         char             |\n", namelen, namelen);
   fprintf(fdiff, fmt, "char", "char");

   fflush(stdout);

   sprintf(fmt, "%%8d%%8d %%%ds  %%%ds  diff.%%06d.%%06d.fits\n", namelen, namelen);

   for(i=0; i<nPair; ++i)
   {
      fprintf(fdiff, fmt, platePair.cntr1[i], platePair.cntr2[i], platePair.plate1[i], platePair.plate2[i], platePair.cntr1[i], platePair.cntr2[i]);
      fflush(fdiff);
   }

   fclose(fdiff);

   nPerTask = nPair / nTask;

   if(nTask * nPerTask < nPair)
      ++nPerTask;

   if(debug)
   {
      printf("\nDEBUG> Number of diffs in each task: %d (last one may be shorter)\n\n", nPerTask);
      fflush(stdout);
   }


   /**********************************/
   /* Open the fit combo script file */
   /**********************************/

   sprintf(combofile, "%s/comboFit.sh", scriptdir);

   if(debug)
   {
      printf("DEBUG> combo file:  [%s]\n\n", combofile);
      fflush(stdout);
   }

   fcombo = fopen(combofile, "w+");

   if(fcombo == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open fit combo script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fcombo, "#!/bin/sh\n\n");
   fflush(fcombo);


   /*************************************/
   /* Create the task submission script */
   /*************************************/

   sprintf(taskfile, "%sdiffFitTask.bash", scriptdir);

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
   fprintf(ftask, "#SBATCH -o %slogs/diffFit.%%N.%%j.out # STDOUT\n", scriptdir);
   fprintf(ftask, "#SBATCH -e %slogs/diffFit.%%N.%%j.err # STDERR\n", scriptdir);
   fprintf(ftask, "%sjobs/diffFit_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

   fflush(ftask);
   fclose(ftask);


   /***********************/
   /* Create the scripts. */
   /***********************/
   
   ncols = topen(difffile);

   if(ncols < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Error opening diffs.tbl file.\"]\n");
      fflush(stdout);
      exit(0);
   }


   set = 1;

   nread = 0;

   while(1)
   {
      if(set == 1 || nread >= nPerTask)
      {
         // Create a script to run mDiffFitExec on this set

         sprintf(scriptFile, "%sjobs/diffFit_%d.sh", scriptdir, set);

         if(debug)
         {
            printf("\nDEBUG> Set %d\n", set);
            printf("DEBUG> scriptFile:        [%s]\n", scriptFile);
            fflush(stdout);
         }

         fscript = fopen(scriptFile, "w+");

         if(fscript == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file for diff subset %d.\"]\n", set);
            fflush(stdout);
            exit(0);
         }

         fprintf(fscript, "#!/bin/sh\n\n");

         fprintf(fscript, "echo jobs/diffFit_%d.sh\n\n", set);

         fprintf(fscript, "mkdir -p %sdiffs\n", platedir);

         fprintf(fscript, "mHPXHdr %d %shpx%d_%d.hdr\n", order, platedir, order, set);

         if(level_only)
            fprintf(fscript, "mDiffFitExec -d -n -l -p %s %sdiffs_%d.tbl %shpx%d_%d.hdr %sdiffs %sfits_%d.tbl\n", 
               platedir, platedir, set, platedir, order, set, platedir, platedir, set);
         else
            fprintf(fscript, "mDiffFitExec -d -n -p %s %sdiffs_%d.tbl %shpx%d_%d.hdr %sdiffs %sfits_%d.tbl\n", 
               platedir, platedir, set, platedir, order, set, platedir, platedir, set);

         fprintf(fscript, "rm -f %shpx%d_%d.hdr\n", platedir, order, set);

         fflush(fscript);
         fclose(fscript);

         chmod(scriptFile, 0777);

         if(set == 1)
            fprintf(fcombo, "cat  %sfits_%d.tbl >  %sfitcombo.tbl\n", platedir, set, platedir);
         else
            fprintf(fcombo, "grep -v \"|\" %sfits_%d.tbl >> %sfitcombo.tbl\n", platedir, set, platedir);

         fflush(fcombo);


         // Close the current output diff table and open the next

         nread = 0;

         if(set != 1)
            fclose(fdiff);
        
         sprintf(outDiffs, "%sdiffs_%d.tbl", platedir, set);
        
         if(debug)
         {
            printf("DEBUG> subset diffs file: [%s]\n", outDiffs);
            fflush(stdout);
         }
        
         fdiff = fopen(outDiffs, "w+");

         if(fdiff == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Error opening diffs_%d.tbl file.\"]\n", set);
            fflush(stdout);
            exit(0);
         }
        
         fprintf(fdiff, "%s\n", tbl_hdr_string);

         ++set;
      }

      if(tread() < 0)
      {
         if(debug)
         {
            printf("\nDEBUG> End of data read (nread = %d)\n", nread);
            fflush(stdout);
         }

         break;
      }

      if(debug)
      {
         printf("DEBUG> Copy one diff record\n");
         fflush(stdout);
      }

      fprintf(fdiff, "%s\n", tbl_rec_string);
      fflush(fdiff);

      ++nread;
   }

   --set;

   fclose(fdiff);


   // If we created a set with actually nothing left to process,
   // get rid of the set script/diffs table.

   if(nread == 0)
   {
      if(debug)
      {
         printf("DEBUG> unlink(%s)\n", outDiffs);
         printf("DEBUG> unlink(%s)\n", scriptFile);
         printf("\nDEBUG> set -> %d\n\n", set-1);
         fflush(stdout);
      }

      unlink(outDiffs);
      unlink(scriptFile);

      --set;
   }


   /*********************************/
   /* Create the driver script file */
   /*********************************/

   sprintf(driverfile, "%s/diffFitSubmit.sh", scriptdir);

   if(debug)
   {
      printf("DEBUG> driver file: [%s]\n", driverfile);
      fflush(stdout);
   }

   fdriver = fopen(driverfile, "w+");

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);

   fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %sdiffFitTask.bash\n", 
      set, scriptdir);
   fflush(fdriver);
   fclose(fdriver);

   chmod(driverfile, 0777);


   /*************/
   /* Finish up */
   /*************/

   fclose(fcombo);
   chmod(combofile, 0777);

   printf("[struct stat=\"OK\", module=\"mHPXDiffScripts\", nPair=%d, nTask=%d, nPerTask=%d]\n",
      nPair, set, nPerTask);
   fflush(stdout);
   exit(0);
} 


int makePlateList(int nplate)
{
   int ii, jj, i, j, k, imin, imax, nsub;

   nsub = nplate / 5;

   plateList.n = 0;

   for(jj = 0; jj<5; ++jj)
   {
      if     (jj == 0) { imin = 0; imax=2;}
      else if(jj == 1) { imin = 0; imax=3;}
      else if(jj == 2) { imin = 1; imax=4;}
      else if(jj == 3) { imin = 2; imax=5;}
      else if(jj == 4) { imin = 3; imax=5;}

      for(j=jj*nsub; j<(jj+1)*nsub; ++j)
      {
         for(i=imin*nsub; i<imax*nsub; ++i)
         {
            plateList.x[plateList.n] = i;
            plateList.y[plateList.n] = j;

            ++plateList.n;
         }
      }
   }
}
      


int makePlatePairs()
{
   int i, i1, i2, found, found2;

   platePair.n = 0;

   for(i1=0; i1<plateList.n; ++i1)
   {
      for(i2=0; i2<plateList.n; ++i2)
      {
         // printf("XXX> compare %d (%d,%d) to %d (%d,%d)\n", i1, plateList.x[i1], plateList.y[i1],
         //                                                   i2, plateList.x[i2], plateList.y[i2]);
         // fflush(stdout);

         if((plateList.x[i2] == plateList.x[i1]   && plateList.y[i2] == plateList.y[i1]-1)
         || (plateList.x[i2] == plateList.x[i1]   && plateList.y[i2] == plateList.y[i1]+1)
         || (plateList.x[i2] == plateList.x[i1]-1 && plateList.y[i2] == plateList.y[i1])
         || (plateList.x[i2] == plateList.x[i1]+1 && plateList.y[i2] == plateList.y[i1])
         || (plateList.x[i2] == plateList.x[i1]-1 && plateList.y[i2] == plateList.y[i1]-1)
         || (plateList.x[i2] == plateList.x[i1]+1 && plateList.y[i2] == plateList.y[i1]+1)
         || (plateList.x[i2] == plateList.x[i1]-1 && plateList.y[i2] == plateList.y[i1]+1)
         || (plateList.x[i2] == plateList.x[i1]+1 && plateList.y[i2] == plateList.y[i1]-1))
         {
            found = 0;

            for(i=0; i<platePair.n; ++i)
            {
               if(plateList.x[i1] == platePair.x1[i]
               && plateList.y[i1] == platePair.y1[i]
               && plateList.x[i2] == platePair.x2[i]
               && plateList.y[i2] == platePair.y2[i])
               {
                  found = 1;
                  break;
               }

               if(plateList.x[i1] == platePair.x2[i]
               && plateList.y[i1] == platePair.y2[i]
               && plateList.x[i2] == platePair.x1[i]
               && plateList.y[i2] == platePair.y1[i])
               {
                  found = 1;
                  break;
               }
            }

            if(!found)
            {
               // printf("XXX> nominally good\n");
               // fflush(stdout);

               platePair.x1[platePair.n] = plateList.x[i1];
               platePair.y1[platePair.n] = plateList.y[i1];
               platePair.x2[platePair.n] = plateList.x[i2];
               platePair.y2[platePair.n] = plateList.y[i2];

               sprintf(platePair.plate1[platePair.n], "plate_%02d_%02d.fits", 
                  plateList.x[i1], plateList.y[i1]);

               sprintf(platePair.plate2[platePair.n], "plate_%02d_%02d.fits", 
                  plateList.x[i2], plateList.y[i2]);

               found2 = 0;

               for(i=0; i<plates.n; ++i)
               {
                  if(strcmp(platePair.plate1[platePair.n], plates.fname[i]) == 0)
                  {
                     platePair.cntr1[platePair.n] = plates.cntr[i];
                     found2 = 1;
                  }
               }

               if(!found2)
                  break;

               found2 = 0;

               for(i=0; i<plates.n; ++i)
               {
                  if(strcmp(platePair.plate2[platePair.n], plates.fname[i]) == 0)
                  {
                     platePair.cntr2[platePair.n] = plates.cntr[i];
                     found2 = 1;
                  }
               }

               if(!found2)
                  break;

               // printf("XXX> saved\n");
               // fflush(stdout);

               ++platePair.n;
            }
         }
      }
   }
}
