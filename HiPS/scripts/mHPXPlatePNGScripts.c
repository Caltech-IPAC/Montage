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
/*  mHPXPlatePNGScripts                                 */
/*                                                      */
/*  This program makes scripts for generating PNGs for  */
/*  each of a set of plates.  It can be used in         */
/*  multiple contexts but usually to get an idea of     */
/*  what the plates look like as a set before           */
/*  generating all the HiPS PNGs.                       */
/*                                                      */
/********************************************************/

int main(int argc, char **argv)
{
   int  i, j, ch, istat, ncols, icol, jcol, count;
   int  nplate, width, single_threaded;
   
   char platedir  [MAXSTR];
   char histodir  [MAXSTR];
   char pngdir    [MAXSTR];
   char platelist [MAXSTR];
   char scriptdir [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];
   char tmpdir    [MAXSTR];
   char base      [MAXSTR];
   char cwd       [MAXSTR];

   char *ptr;

   FILE *fscript;
   FILE *fdriver;

   getcwd(cwd, 1024);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   strcpy(base, "plate");

   single_threaded = 0;

   opterr = 0;

   while ((ch = getopt(argc, argv, "db:s")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 'b':
            strcpy(base, optarg);
            break;

         case 's':
            single_threaded = 1;
            break;

         default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-b basename][-s(ingle-threaded)] scriptdir platelist.tbl platedir histodir pngdir\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 5)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-b basename][-s(ingle-threaded)] scriptdir platelist.tbl platedir histodir pngdir\"]\n", argv[0]);
      exit(1);
   }


   strcpy(scriptdir, argv[optind + 0]);
   strcpy(platelist, argv[optind + 1]);
   strcpy(platedir,  argv[optind + 2]);
   strcpy(histodir,  argv[optind + 3]);
   strcpy(pngdir,    argv[optind + 4]);


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

   if(platedir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platedir);

      strcpy(platedir, tmpdir);
   }

   if(histodir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, histodir);

      strcpy(histodir, tmpdir);
   }

   if(pngdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, pngdir);

      strcpy(pngdir, tmpdir);
   }


   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(histodir[strlen(histodir)-1] != '/')
      strcat(histodir, "/");

   if(pngdir[strlen(pngdir)-1] != '/')
      strcat(pngdir, "/");


   if(debug)
   {

      printf("single_threaded = %d\n\n", single_threaded);

      printf("base:      [%s]\n", base);
      printf("scriptdir: [%s]\n", scriptdir);
      printf("platelist: [%s]\n", platelist);
      printf("platedir:  [%s]\n", platedir);
      printf("histodir:  [%s]\n", histodir);
      printf("pngdir:    [%s]\n", pngdir);
      
      fflush(stdout);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%srunPlatePNG.sh", scriptdir);

   fdriver = fopen(driverfile, "w+");

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);


   /********************************************************************/
   /* Read the image list, generating a histogram script for each one. */
   /********************************************************************/

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

      sprintf(scriptfile, "%s/jobs/platePNG%03d.sh", scriptdir, count);
      
      fscript = fopen(scriptfile, "w+");

      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] to create PNG for image %d.\"]\n",
            scriptfile, count);
         fflush(stdout);
         exit(0);
      }

      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "echo jobs/platePNG%03d.sh\n\n", count);

      fprintf(fscript, "mViewer -ct 1 -gray %s%s_%02d_%02d.fits -histfile %s%s_%02d_%02d.hist -out %s%s_%02d_%02d.png\n",
         platedir, base, i, j, histodir, base, i, j, pngdir, base, i, j);

      fflush(fscript);
      fclose(fscript);

      if(debug)
      {
         printf("mViewer -ct 1 -gray %s%s_%02d_%02d.fits -histfile %s%s_%02d_%02d.hist -out %s%s_%02d_%02d.png\n",
            platedir, base, i, j, histodir, base, i, j, pngdir, base, i, j);
         fflush(stdout);
      }

      chmod(scriptfile, 0777);

      if(single_threaded)
      {
         fprintf(fdriver, "%sjobs/platePNG%03d.sh\n", scriptdir, count);
         fflush(fdriver);
      }
      else
      {
         fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 %ssubmitPlatePNG.bash %sjobs/platePNG%03d.sh\n", 
            scriptdir, scriptdir, count);
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
