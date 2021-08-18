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
   int  i, ch, istat, ncols, order, iorder, count;
   int  iplate, single_threaded, pixlev, nsidePix;
   
   double pixscale;

   char platedir  [MAXSTR];
   char platelist [MAXSTR];
   char scriptdir [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];
   char tmpdir    [MAXSTR];
   char plate     [MAXSTR];
   char cwd       [MAXSTR];

   char *ptr;

   FILE *fscript;
   FILE *fdriver;

   getcwd(cwd, 1024);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

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
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle-threaded)] order scriptdir platedir platelist.tbl\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle-threaded)] order scriptdir platedir platelist.tbl\"]\n", argv[0]);
      exit(1);
   }

   order = atoi(argv[optind]);

   strcpy(scriptdir, argv[optind + 1]);
   strcpy(platedir,  argv[optind + 2]);
   strcpy(platelist, argv[optind + 3]);

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

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(debug)
   {

      printf("order           = %d\n",   order);
      printf("single_threaded = %d\n\n", single_threaded);

      printf("scriptdir: [%s]\n", scriptdir);
      printf("platedir:  [%s]\n", platedir);
      printf("platelist: [%s]\n\n", platelist);
      
      fflush(stdout);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%srunShrink.sh", scriptdir);

   fdriver = fopen(driverfile, "w+");

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);


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

      strcpy(plate, tval(iplate));

      sprintf(scriptfile, "%s/jobs/shrink%03d.sh", scriptdir, count);
      
      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] to shrink image %d.\"]\n",
            scriptfile, count);
         fflush(stdout);
         exit(0);
      }

      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "echo jobs/shrink%03d.sh\n\n", count);

      for(iorder=order; iorder>=1; --iorder)
      {
         pixlev = iorder - 1 + 9;

         nsidePix = pow(2., (double)pixlev);

         pixscale  = 90.0 / nsidePix / sqrt(2.0);

         fprintf(fscript, "mShrink -p %.8f $1order%d/%s.fits $1order%d/%s.fits 2\n", 
            pixscale, iorder, plate, iorder-1, plate);
      }

      fflush(fscript);
      fclose(fscript);

      chmod(scriptfile, 0777);

      if(single_threaded)
      {
         fprintf(fdriver, "%sjobs/shrink%03d.sh %s\n", scriptdir, count, platedir);
         fflush(fdriver);
      }
      else
      {
         fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 %ssubmitShrink.bash %sjobs/shrink%03d.sh %s\n", 
            scriptdir, scriptdir, count, platedir);
         fflush(fdriver);
      }

      ++count;
   }

   chmod(driverfile, 0777);


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", count=%d]\n", count);
   fflush(stdout);
   exit(0);
}
