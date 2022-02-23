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
/*  mHPXBgScripts                                         */
/*                                                        */
/*  Simple scripts:  Run mBgExec in each of the project   */
/*  plate subdirectories.                                 */
/*                                                        */
/*  Use the plate list determined originally for the      */
/*  projection building to once again normalize the I/O   */
/*  load.                                                 */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  i, ch, ncols, job;

   char cwd       [MAXSTR];
   char imgtbl    [MAXSTR];
   char corrtbl   [MAXSTR];
   char platedir  [MAXSTR];
   char corrdir   [MAXSTR];
   char scriptdir [MAXSTR];
   char tmpstr    [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];
   char taskfile  [MAXSTR];

   FILE *fscript;
   FILE *fdriver;
   FILE *ftask;

   int     iid, ia, ib, ic;

   int    id;

   double *a;
   double *b;
   double *c;

   int    *have;

   int     icntr, cntr, ifname, maxcntr;
   char    fname[MAXSTR];


   getcwd(cwd, MAXSTR);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug  = 0;
   opterr = 0;

   while ((ch = getopt(argc, argv, "nd")) != EOF) 
   {
        switch (ch) 
        {
           case 'd':
                debug = 1;
                break;

           default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] scriptdir platedir images.tbl corrections.tbl correctdir\"]\n", argv[0]);
                exit(1);
                break;
        }
   }

   if (argc - optind < 5)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] scriptdir platedir images.tbl corrections.tbl correctdir\"]\n", argv[0]);
      exit(1);
   }

   strcpy(scriptdir,  argv[optind]);
   strcpy(platedir,   argv[optind + 1]);
   strcpy(imgtbl,     argv[optind + 2]);
   strcpy(corrtbl,    argv[optind + 3]);
   strcpy(corrdir,    argv[optind + 4]);

   if(scriptdir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, scriptdir);

      strcpy(scriptdir, tmpstr);
   }

   if(platedir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, platedir);

      strcpy(platedir, tmpstr);
   }

   if(imgtbl[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, imgtbl);

      strcpy(imgtbl, tmpstr);
   }

   if(corrtbl[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, corrtbl);

      strcpy(corrtbl, tmpstr);
   }

   if(corrdir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, corrdir);

      strcpy(corrdir, tmpstr);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(corrdir[strlen(corrdir)-1] != '/')
      strcat(corrdir, "/");


   /******************************************/ 
   /* Open the corrections table file        */
   /******************************************/ 

   ncols = topen(corrtbl);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid corrections file: %s\"]\n", corrtbl);
      fflush(stdout);
      exit(0);
   }

   iid = tcol( "id");
   ia  = tcol( "a");
   ib  = tcol( "b");
   ic  = tcol( "c");

   if(debug)
   {
      printf("\nCorrections table\n");
      printf("iid = %d\n", iid);
      printf("ia  = %d\n", ia);
      printf("ib  = %d\n", ib);
      printf("ic  = %d\n", ic);
      printf("\n");
      fflush(stdout);
   }

   if(iid < 0
   || ia  < 0
   || ib  < 0
   || ic  < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns: id,a,b,c in corrections file\"]\n");
      fflush(stdout);
      exit(0);
   }


   /*******************************/
   /* Find the highest ID number. */
   /*******************************/

   maxcntr = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      cntr = atoi(tval(icntr));

      if(cntr > maxcntr)
         maxcntr = cntr;
   }

   tclose();

   ++maxcntr;


   /**************************************/ 
   /* Allocate space for the corrections */
   /**************************************/ 

   if(debug)
   {
      printf("mallocking %d\n", maxcntr);
      fflush(stdout);
   }

   a = (double *)malloc(maxcntr * sizeof(double));
   b = (double *)malloc(maxcntr * sizeof(double));
   c = (double *)malloc(maxcntr * sizeof(double));

   have = (int *)malloc(maxcntr * sizeof(int));

   for(i=0; i<maxcntr; ++i)
   {
      a[i]    = 0.;
      b[i]    = 0.;
      c[i]    = 0.;

      have[i] = 0;
   }


   /********************************/
   /* And read in the corrections. */
   /********************************/

   ncols = topen(corrtbl);

   while(1)
   {
      if(tread() < 0)
         break;

      id = atoi(tval(iid));

      if(id >= maxcntr)
      {
         printf("[struct stat=\"ERROR\", msg=\"Correction ID out of range (%d vs. %d)\"]\n",
            id, maxcntr-1);
         fflush(stdout);
         exit(0);
      }

      a[id] = atof(tval(ia));
      b[id] = atof(tval(ib));
      c[id] = atof(tval(ic));

      have[id] = 1;
   }

   tclose();

   if(debug)
   {
      printf("Corrections read\n");
      fflush(stdout);
   }


   /******************************/ 
   /* Open the images table file */
   /******************************/ 

   ncols = topen(imgtbl);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid corrections file: %s\"]\n", imgtbl);
      fflush(stdout);
      exit(0);
   }

   icntr  = tcol("cntr");
   ifname = tcol("fname");

   if(debug)
   {
      printf("\nImages table\n");
      printf("icntr  = %d\n", icntr);
      printf("ifname = %d\n", ifname);
      printf("\n");
      fflush(stdout);
   }

   if(icntr < 0 || ifname  < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns: cntr and fname in corrections file\"]\n");
      fflush(stdout);
      exit(0);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%scorrectSubmit.sh", scriptdir);

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

   sprintf(taskfile, "%scorrectTask.bash", scriptdir);

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
   fprintf(ftask, "#SBATCH -o %slogs/correct.%N.%%j.out # STDOUT\n", scriptdir);
   fprintf(ftask, "#SBATCH -e %slogs/correct.%N.%%j.err # STDERR\n", scriptdir);
   fprintf(ftask, "%sjobs/correct_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

   fflush(ftask);
   fclose(ftask);


   /***************************************************/ 
   /* Iterate over the plates. Create an mBgExec      */
   /* script for each one.                            */
   /***************************************************/ 

   job = 1;

   while(1)
   {
      if(tread() < 0)
         break;

      cntr = atoi(tval(icntr));

      strcpy(fname, tval(ifname));

      if(debug)
      {
         printf("Make script for plate [%s] (%d)\n", fname, job);
         fflush(stdout);
      }

      sprintf(scriptfile, "%sjobs/correct_%d.sh", scriptdir, job);
      
      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s].\"]\n", scriptfile);
         fflush(stdout);
         exit(0);
      }

      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "echo jobs/correct_%d.sh\n", job);
      fprintf(fscript, "mkdir -p %s\n", corrdir);

      fprintf(fscript, "mBackground -n %s%s %s%s %12.5e %12.5e %12.5e\n",
         platedir, fname, corrdir, fname, a[cntr], b[cntr], c[cntr]);

      fflush(fscript);
      fclose(fscript);

      chmod(scriptfile, 0777);

      ++job;
   }

   tclose(); 

   --job;


   fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %scorrectTask.bash\n", job, scriptdir);
   fflush(fdriver);
   fclose(fdriver);

   chmod(driverfile, 0777);


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", count=%d]\n", job);
   fflush(stdout);
   exit(0);
}
