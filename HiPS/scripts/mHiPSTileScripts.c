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
/*  mHiPSTileScripts                                      */
/*                                                        */
/*  Create scripts that split the plates in the image     */
/*  list into their component HiPS tiles (still FITS      */
/*  image).                                               */
/*                                                        */
/*  Use the plate list determined originally for the      */
/*  mosaic building to once again normalize the I/O       */
/*  load (somewhat since only the iput images are         */
/*  distributed across storage devices; the output        */
/*  is put in a single tree).                             */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  i, ch, istat, ncols, iorder;
   int  count, maxorder, minorder, single_threaded;

   char cwd       [MAXSTR];
   char platelist [MAXSTR];
   char platedir  [MAXSTR];
   char tiledir   [MAXSTR];
   char scriptdir [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];
   char taskfile  [MAXSTR];
   char tmpdir    [MAXSTR];
   char plate     [MAXSTR];

   char *ptr;

   FILE *fscript;
   FILE *fdriver;
   FILE *ftask;

   int    iid;
   int    iplate;

   int    id;

   getcwd(cwd, MAXSTR);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug     = 0;
   maxorder = -1;
   single_threaded = 0;

   opterr = 0;

   while ((ch = getopt(argc, argv, "ds")) != EOF) 
   {
        switch (ch) 
        {
           case 'd':
                debug = 1;
                break;

           case 's':
                single_threaded = 1;
                break;

           default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle_threaded)] maxorder minorder scriptdir platedir tiledir platelist.tbl\"]\n", argv[0]);
                exit(1);
                break;
        }
   }

   if (argc - optind < 6)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle_threaded)] maxorder minorder scriptdir platedir tiledir platelist.tbl\"]\n", argv[0]);
      exit(1);
   }


   maxorder = atoi(argv[optind]);
   minorder = atoi(argv[optind + 1]);

   strcpy(scriptdir, argv[optind + 2]);
   strcpy(platedir,  argv[optind + 3]);
   strcpy(tiledir,   argv[optind + 4]);
   strcpy(platelist, argv[optind + 5]);

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

   if(tiledir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, tiledir);

      strcpy(tiledir, tmpdir);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(tiledir[strlen(tiledir)-1] != '/')
      strcat(tiledir, "/");

   if(debug)
   {
      printf("DEBUG> scriptdir:  [%s]\n", scriptdir);
      printf("DEBUG> platedir:   [%s]\n", platedir);
      printf("DEBUG> tiledir:    [%s]\n", tiledir);
      printf("DEBUG> platelist:  [%s]\n", platelist);
      fflush(stdout);
   }

   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%stileSubmit.sh", scriptdir);

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
      sprintf(taskfile, "%stileTask.bash", scriptdir);

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
      fprintf(ftask, "#SBATCH -o %slogs/tile.%%N.%%j.out # STDOUT\n", scriptdir);
      fprintf(ftask, "#SBATCH -e %slogs/tile.%%N.%%j.err # STDERR\n", scriptdir);
      fprintf(ftask, "%sjobs/tiles_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

      fflush(ftask);
      fclose(ftask);
   }


   /*******************************/
   /* Read through the image list */
   /*******************************/

   ncols = topen(platelist);

   iplate = tcol("plate");

   if(iplate < 0)
      iplate = tcol("fname");

   if(debug)
   {
      printf("\nDEBUG> Image metadata table\n");
      printf("DEBUG> iplate = %d\n", iplate);
      fflush(stdout);
   }

   if(iplate < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need column 'plate' in plate list table.\"]\n");
      fflush(stdout);
      exit(0);
   }

   count = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      strcpy(plate, tval(iplate));

      if(strcmp(plate+strlen(plate)-5, ".fits") != 0)
         strcat(plate, ".fits");

      if(debug)
      {
         printf("DEBUG> Record %5d: %s\n", count, plate);
         fflush(stdout);
      }

      sprintf(scriptfile, "%sjobs/tiles_%d.sh", scriptdir, count);
      
      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] for plate %s.\"]\n", scriptfile, plate);
         fflush(stdout);
         exit(0);
      }

      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "date\n");

      fprintf(fscript, "echo Plate %s, Task %d\n\n", plate, count);

      fprintf(fscript, "mkdir -p %s\n\n", tiledir);

      for(iorder=maxorder; iorder>=minorder; --iorder)
      {
         fprintf(fscript, "echo mHiPSTiles %sorder%d/%s %s\n", platedir, iorder, plate, tiledir);
         fprintf(fscript, "mHiPSTiles %sorder%d/%s %s\n", platedir, iorder, plate, tiledir);
      }

      fflush(fscript);
      fclose(fscript);

      chmod(scriptfile, 0777);

      if(single_threaded)
      {
         fprintf(fdriver, "%sjobs/tiles_%d.sh\n", scriptdir, count);
         fflush(fdriver);
      }

      ++count;
   }

   if(!single_threaded)
   {
      fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %stileTask.bash\n", 
         count, scriptdir);
   }

   fflush(fdriver);
   fclose(fdriver);

   chmod(driverfile, 0777);


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", count=%d]\n", count);
   fflush(stdout);
   exit(0);
}
