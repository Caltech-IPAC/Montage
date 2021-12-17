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
   int  i, ch, status, count;
   
   double shrink_factor;

   char  scriptdir    [MAXSTR];
   char  platedir     [MAXSTR];
   char  quicklookdir [MAXSTR];
   char  scriptfile   [MAXSTR];
   char  driverfile   [MAXSTR];
   char  tmpdir       [MAXSTR];
   char  tmpstr       [MAXSTR];
   char  filename     [MAXSTR];
   char  base         [MAXSTR];
   char  ext          [MAXSTR];
   char  cwd          [MAXSTR];

   char *ptr;

   DIR    *dirp;
   struct  dirent *direntp;

   FILE *fscript;
   FILE *fdriver;

   fitsfile *fptr;

   getcwd(cwd, 1024);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   opterr = 0;

   while ((ch = getopt(argc, argv, "d")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] scriptdir platedir quicklookdir shrink_factor\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 4)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] scriptdir platedir quicklookdir shrink_factor\"]\n", argv[0]);
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
      printf("quicklookdir: [%s]\n", quicklookdir);
      printf("shrink_factor:  %-g\n\n", shrink_factor);
      
      fflush(stdout);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%srunQuicklook.sh", scriptdir);

   fdriver = fopen(driverfile, "w+");

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);


   /********************************************************/
   /* Look through the FITS images in the plate directory, */
   /* creating a quick-look script for each one.           */
   /********************************************************/

   count = 0;

   chdir(platedir);

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


      sprintf(scriptfile, "%s/jobs/quicklook%03d.sh", scriptdir, count);

      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] for quicklook processing [number %d].\"]\n",
            scriptfile, count);
         fflush(stdout);
         exit(0);
      }


      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "echo jobs/quicklook%03d.sh\n\n", count);

      fprintf(fscript, "mShrink $1/%s.%s $2/shrunken_%s.%s %-g\n", 
         base, ext, base, ext, shrink_factor);

      fprintf(fscript, "mViewer -ct 1 -gray $2/shrunken_%s.$s min max gaussian-log -out $2/%shrunken_s.png\n", 
         base, ext, base);

      fflush(fscript);
      fclose(fscript);

      chmod(scriptfile, 0777);

      fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 %ssubmitQuicklook.bash %sjobs/quicklook%03d.sh %s %s\n", 
         scriptdir, scriptdir, count, platedir, quicklookdir);
      fflush(fdriver);

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
