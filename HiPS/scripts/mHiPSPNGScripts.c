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

int  njob, nfile, ifile, count;

char scriptdir [1024];
char scriptfile[1024];
char directory1[1024];
char directory2[1024];
char directory3[1024];
char histfile1 [1024];
char histfile2 [1024];
char histfile3 [1024];
char outdir    [1024];

FILE *fscript;
FILE *fdriver;

double contrast, brightness;

int  len1;

int  color = 0;

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
   int i;

   char cwd       [MAXSTR];
   char tmpdir    [MAXSTR];
   char driverfile[MAXSTR];


   getcwd(cwd, MAXSTR);

   count = 0;


   if(argc < 7)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScripts -d brightness contrast scriptdir hipsdir histfile(/histdir) [hipsdir2 histfile2 hipsdir3 histfile3] outdir\"]\n");
      exit(1);
   }

   for(i=0; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
      {
         debug = 1;

         svc_debug(stdout);
      }
   }
      
   if(debug)
   {
      ++argv;
      --argc;
   }

   brightness = atof(argv[1]);
   contrast   = atof(argv[2]);

   strcpy(scriptdir,  argv[3]);
   strcpy(directory1, argv[4]);
   strcpy(histfile1,  argv[5]);
   strcpy(outdir,     argv[6]);

   if(scriptdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, scriptdir);

      strcpy(scriptdir, tmpdir);
   }

   if(directory1[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, directory1);

      strcpy(directory1, tmpdir);
   }

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

   strcpy(directory2, "");
   strcpy(histfile2,  "");
   strcpy(directory3, "");
   strcpy(histfile3,  "");

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(directory1[strlen(directory1)-1] != '/')
      strcat(directory1, "/");

   if(outdir[strlen(outdir)-1] != '/')
      strcat(outdir, "/");

   len1 = strlen(directory1);

   if(argc > 7)
   {
      if(argc < 11)
      {
         printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScripts -d brightness contrast scriptdir hipsdir histfile(/histdir) [hipsdir2 histfile2 hipsdir3 histfile3] outdir\"]\n");
         exit(1);
      }

      strcpy(directory2, argv[ 6]);
      strcpy(histfile2,  argv[ 7]);
      strcpy(directory3, argv[ 8]);
      strcpy(histfile3,  argv[ 9]);
      strcpy(outdir,     argv[10]);

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

      if(directory2[strlen(directory2)-1] != '/')
         strcat(directory2, "/");

      if(directory3[strlen(directory3)-1] != '/')
         strcat(directory3, "/");

      if(outdir[strlen(outdir)-1] != '/')
         strcat(outdir, "/");

      color = 1;
   }

   if(debug)
   {
      printf("DEBUG> color      = %d\n",   color);
      printf("DEBUG> brightness = %-g\n",  brightness);
      printf("DEBUG> contrast   = %-g\n",  contrast);
      printf("DEBUG> directory1 = [%s]\n", directory1);
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
   /* and the initial script file */
   /*******************************/

   sprintf(driverfile, "%srunPNGs.sh", scriptdir);

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



   /***********************************************/
   /* Process the directories, making the scripts */
   /***********************************************/

   njob = 0;

   mHiPSPNGScripts_processDirs(directory1);

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

         if(debug)
         {
            printf("DEBUG> njob -> %d\n", njob);
            fflush(stdout);
         }

         sprintf(scriptfile, "%sjobs/png%03d.sh", scriptdir, count);

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

         fprintf(fscript, "echo jobs/png%03d.sh\n\n", count);

         if(color)
         {
            sprintf(dirname2, "%s%s", directory2, dirname1+len1);
            sprintf(dirname3, "%s%s", directory3, dirname1+len1);

            fprintf(fscript, "mHiPSPNGs %-g %-g %s %s %s %s  %s %s %s\n",
               brightness, contrast, dirname1, histfile1, dirname2, histfile2, dirname3, histfile3, pngdir);
            fflush(fscript);
         }
         else
         {
            fprintf(fscript, "mHiPSPNGs %-g %-g %s %s %s\n",
               brightness, contrast, dirname1, histfile1, pngdir);
            fflush(fscript);
         }

         fclose(fscript);
         chmod(scriptfile, 0777);



         fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 %ssubmitPNG.bash %sjobs/png%03d.sh\n", scriptdir, scriptdir, count);

         fflush(fdriver);

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
