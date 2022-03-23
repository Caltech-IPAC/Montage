/* mHiPSPNGScripts.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        10Dec19  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <svc.h>
#include <errno.h>

#define MAXSTR 4096

int  mHiPSPNGScripts_processDirs   (char *pathname);
void mHiPSPNGScripts_printFitsError(int status);
int  mHiPSPNGScripts_mkdir         (const char *path);

int  njob, nfile, ifile, count, update_only;

char scriptdir [1024];
char scriptfile[1024];
char taskfile  [1024];
char directory1[1024];
char directory2[1024];
char directory3[1024];
char histfile1 [1024];
char histfile2 [1024];
char histfile3 [1024];
char outdir    [1024];
char contfile  [1024];
char flags     [1024];

FILE *fscript;
FILE *fdriver;
FILE *ftask;

double contrast, brightness;
int    order, nplate, ct;

int  len1;

int  color = 0;

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int  debug;


/*************************************************************************/
/*                                                                       */
/*  mHiPSPNGScripts                                                      */
/*                                                                       */
/*  This program recursively finds all the FITS files in a directory     */
/*  tree and generates a matching PNG (grayscale) using the supplied     */
/*  histogram.                                                           */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int i, c;

   char cwd       [MAXSTR];
   char tmpdir    [MAXSTR];
   char driverfile[MAXSTR];


   getcwd(cwd, MAXSTR);

   debug       =  0;
   ct          =  0;
   order       = -1;
   nplate      = -1;
   brightness  =  0.;
   contrast    =  0.;
   len1        =  0;
   update_only =  0;

   strcpy(contfile, "");

   count = 1;

   while((c = getopt(argc, argv, "dnub:c:C:t:p:")) != EOF)
   {
      switch(c)
      {
         case 'd':
            debug = 1;
            break;

         case 'u':
            update_only = 1;
            break;

         case 'b':
            brightness = atof(optarg);
            break;

         case 'c':
            contrast = atof(optarg);
            break;

         case 'C':
            strcpy(contfile, optarg);
            break;

         case 't':
            ct = atoi(optarg);
            break;

         case 'p':
            nplate = atoi(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScript [-d][-b brightness][-c contrast][-t color-table][-p nplate] scriptdir directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
            exit(1);
            break;
      }
   }

   if(argc-optind < 4)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScripts [-d][-b brightness][-c contrast][-t color-table][-p nplate] scriptdir directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
      exit(1);
   }

   if(ct < 0)
      ct = 0;

   strcpy(scriptdir,  argv[optind]);
   strcpy(directory1, argv[optind+1]);
   strcpy(histfile1,  argv[optind+2]);
   strcpy(outdir,     argv[optind+3]);

   if(scriptdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, scriptdir);

      strcpy(scriptdir, tmpdir);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
     strcat(scriptdir, "/");

   if(directory1[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, directory1);

      strcpy(directory1, tmpdir);
   }

   len1 = strlen(directory1);

   if(histfile1[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, histfile1);

      strcpy(histfile1, tmpdir);
   }

   if(outdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, outdir);

      strcpy(outdir, tmpdir);
   }


   if(argc-optind > 4)
   {
      if(argc-optind < 8)
      {
         printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScripts [-d][-b brightness][-c contrast][-t color-table][-p nplate] scriptdir directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
         exit(1);
      }

      strcpy(directory2, argv[optind+3]);
      strcpy(histfile2,  argv[optind+4]);
      strcpy(directory3, argv[optind+5]);
      strcpy(histfile3,  argv[optind+6]);
      strcpy(outdir,     argv[optind+7]);

      if(directory2[0] != '/')
      {
         strcpy(tmpdir, cwd);
         strcat(tmpdir, "/");
         strcat(tmpdir, directory2);

         strcpy(directory2, tmpdir);
      }

      if(histfile2[0] != '/')
      {
         strcpy(tmpdir, cwd);
         strcat(tmpdir, "/");
         strcat(tmpdir, histfile2);

         strcpy(histfile2, tmpdir);
      }

      if(directory3[0] != '/')
      {
         strcpy(tmpdir, cwd);
         strcat(tmpdir, "/");
         strcat(tmpdir, directory3);

         strcpy(directory3, tmpdir);
      }

      if(histfile3[0] != '/')
      {
         strcpy(tmpdir, cwd);
         strcat(tmpdir, "/");
         strcat(tmpdir, histfile3);

         strcpy(histfile3, tmpdir);
      }

      if(outdir[0] != '/')
      {
         strcpy(tmpdir, cwd);
         strcat(tmpdir, "/");
         strcat(tmpdir, outdir);

         strcpy(outdir, tmpdir);
      }

      color = 1;
   }

   if(debug)
   {
      printf("DEBUG> color      = %d\n",   color);
      printf("DEBUG> brightness = %-g\n",  brightness);
      printf("DEBUG> contrast   = %-g\n",  contrast);
      printf("DEBUG> contfile   = [%s]\n", contfile);
      printf("DEBUG> directory1 = [%s]\n", directory1);
      printf("DEBUG> len1       = %d\n",   len1);
      printf("DEBUG> histfile1  = [%s]\n", histfile1);
      printf("DEBUG> directory2 = [%s]\n", directory2);
      printf("DEBUG> histfile2  = [%s]\n", histfile2);
      printf("DEBUG> directory3 = [%s]\n", directory3);
      printf("DEBUG> histfile3  = [%s]\n", histfile3);
      printf("DEBUG> outdir     = [%s]\n", outdir);
      fflush(stdout);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%spngSubmit.sh", scriptdir);

   if(debug)
   {
      printf("DEBUG> creating driverfile = [%s]\n", driverfile);
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


   /*************************************/
   /* Create the task submission script */
   /*************************************/

   sprintf(taskfile, "%spngTask.bash", scriptdir);

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
   fprintf(ftask, "#SBATCH -o %slogs/png.%%N.%%j.out # STDOUT\n", scriptdir);
   fprintf(ftask, "#SBATCH -e %slogs/png.%%N.%%j.err # STDERR\n", scriptdir);
   fprintf(ftask, "%sjobs/pngs_$SLURM_ARRAY_TASK_ID.sh\n", scriptdir);

   fflush(ftask);
   fclose(ftask);


   /***********************************************/
   /* Process the directories, making the scripts */
   /***********************************************/

   njob = 0;

   mHiPSPNGScripts_processDirs(directory1);

   fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %spngTask.bash\n", njob, scriptdir);
   fflush(fdriver);

   fclose(fdriver);
   chmod(driverfile, 0777);

   printf("[struct stat=\"OK\", module=\"mHiPSPNGScripts\", njob=%d]\n", njob);
   fflush(stdout);
   exit(0);
}



/*******************************/
/*                             */
/*  Count the number of files  */
/*                             */
/*******************************/

int mHiPSPNGScripts_processDirs (char *pathname)
{
   int             len, i;
   char            dirname1[MAXSTR];
   char            dirname2[MAXSTR];
   char            dirname3[MAXSTR];
   char            pngdir  [MAXSTR];
   char           *ptr;
   DIR            *dp;
   struct dirent  *entry;
   struct stat     type;

   if(debug)
   {
      printf("DEBUG> Entering [%s]\n", pathname);
      fflush(stdout);
   }

   dp = opendir (pathname);

   if (dp == NULL) 
      return 0;

   while ((entry=(struct dirent *)readdir(dp)) != (struct dirent *)0) 
   {
      if(pathname[strlen(pathname)-1] == '/')
         sprintf (dirname1, "%s%s", pathname, entry->d_name);
      else
         sprintf (dirname1, "%s/%s", pathname, entry->d_name);

      if(debug)
      {
         printf("DEBUG> Checking [%s]\n", dirname1);
         fflush(stdout);
      }

      if (strncmp(entry->d_name, "Dir", 3) == 0)
      {
         ++njob;
   
         ptr = strstr(dirname1, "Norder");

         if(ptr)
         {
            order = atoi(ptr + 6);

            if(debug)
            {
               printf("DEBUG> order = %d\n", order);
               fflush(stdout);
            }
         }

         if(debug)
         {
            printf("DEBUG> njob -> %d\n", njob);
            fflush(stdout);
         }

         sprintf(scriptfile, "%sjobs/pngs_%d.sh", scriptdir, count);

         ++count;

         if(debug)
         {
            printf("DEBUG> creating scriptfile = [%s]\n", scriptfile);
            fflush(stdout);
         }

         fscript = fopen(scriptfile, "w+");

         if(fscript == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file  [%s].\"]\n", scriptfile);
            fflush(stdout);
            exit(0);
         }
       
         sprintf(pngdir, "%s%s", outdir, dirname1+len1);


         fprintf(fscript, "#!/bin/sh\n\n");

         fprintf(fscript, "echo Directory: %s\n\n", dirname1);

         fprintf(fscript, "mkdir -p %s\n", pngdir);

         strcpy(flags, "");

         if(strlen(contfile) > 0)
            sprintf(flags, "-C %s", contfile);

         if(update_only)
            strcat(flags, " -u");

         if(color)
         {
            sprintf(dirname2, "%s%s", directory2, dirname1+len1);
            sprintf(dirname3, "%s%s", directory3, dirname1+len1);

            if(nplate > 0)
               fprintf(fscript, "mHiPSPNGs -b %-g -c %-g %s -o %d -p %d %s %s %s %s %s %s %s\n",
                  brightness, contrast, flags, order, nplate, dirname1, histfile1, dirname2, histfile2, dirname3, histfile3, pngdir);
            else
               fprintf(fscript, "mHiPSPNGs -b %-g -c %s %-g %s %s %s %s %s %s %s\n",
                  brightness, contrast, flags, dirname1, histfile1, dirname2, histfile2, dirname3, histfile3, pngdir);

            fflush(fscript);
         }
         else
         {
            if(nplate > 0)
               fprintf(fscript, "mHiPSPNGs -b %-g -c %-g %s -t %d -o %d -p %d %s %s %s\n",
                  brightness, contrast, flags, ct, order, nplate, dirname1, histfile1, pngdir);
            else
               fprintf(fscript, "mHiPSPNGs -b %-g -c %-g %s -t %d %s %s %s\n",
                  brightness, contrast, flags, ct, dirname1, histfile1, pngdir);
            fflush(fscript);
         }

         fclose(fscript);
         chmod(scriptfile, 0777);
      }

      if (stat(dirname1, &type) == 0) 
      {
         if (strncmp(entry->d_name, "Dir", 3) != 0)
         {
            if (S_ISDIR(type.st_mode) == 1)
            {
               if(strcmp (entry->d_name, "." ) != 0
               && strcmp (entry->d_name, "..") != 0)

                  mHiPSPNGScripts_processDirs (dirname1);
            }
         }
      }
   }

   closedir(dp);
   return 0;
}
