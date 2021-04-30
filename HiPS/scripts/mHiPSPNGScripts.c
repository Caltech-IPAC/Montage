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

int  mHiPSPNGScripts_countFiles    (char *pathname);
int  mHiPSPNGScripts_getFiles      (char *pathname);
void mHiPSPNGScripts_printFitsError(int status);
int  mHiPSPNGScripts_mkdir         (const char *path);

int  nimage, nfile, ifile, ncore, count;

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


   if(argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScripts -d scriptdir hipsdir histfile [hipsdir2 histfile2 hipsdir3 histfile3] outdir ncore\"]\n");
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

   strcpy(scriptdir,  argv[1]);
   strcpy(directory1, argv[2]);
   strcpy(histfile1,  argv[3]);
   strcpy(outdir,     argv[4]);

   ncore = atoi(argv[5]);

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

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(directory1[strlen(directory1)-1] != '/')
      strcat(directory1, "/");

   if(outdir[strlen(outdir)-1] != '/')
      strcat(outdir, "/");

   len1 = strlen(directory1);

   if(argc > 6)
   {
      if(argc < 10)
      {
         printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScripts -d scriptdir hipsdir histfile [hipsdir2 histfile2 hipsdir3 histfile3] outdir ncore\"]\n");
         exit(1);
      }

      if(directory1[0] != '/')
      {
         strcpy(tmpdir, cwd);
         strcat(tmpdir, "/");
         strcat(tmpdir, directory1);

         strcpy(directory1, tmpdir);
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

      strcpy(directory2, argv[4]);
      strcpy(histfile2,  argv[5]);
      strcpy(directory3, argv[6]);
      strcpy(histfile3,  argv[7]);
      strcpy(outdir,     argv[8]);

      ncore = atoi(argv[9]);

      if(directory2[strlen(directory2)-1] != '/')
         strcat(directory2, "/");

      if(directory3[strlen(directory3)-1] != '/')
         strcat(directory3, "/");

      if(outdir[strlen(outdir)-1] != '/')
         strcat(outdir, "/");

      color = 1;
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


   sprintf(scriptfile, "%sjobs/png%03d.sh", scriptdir, count);

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
 
   fprintf(fscript, "#!/bin/sh\n\n");

   fprintf(fscript, "echo jobs/png%03d.sh\n\n", count);

   if(ncore == 1)
      fprintf(fdriver, "%sjobs/png%03d.sh\n", scriptdir, count);
   else
      fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 %ssubmitPNG.bash %sjobs/png%03d.sh\n", scriptdir, scriptdir, count);

   fflush(fdriver);


   /*******************/
   /* Count the files */
   /*******************/

   nimage = 0;

   mHiPSPNGScripts_countFiles(directory1);

   nfile = nimage / ncore;

   if(nfile * ncore < nimage)
      ++nfile;

   ifile = 0;

   if(debug)
   {
      printf("DEBUG> nimage = %d; nfile (per job) = %d\n", nimage, nfile);
      fflush(stdout);
   }



   /*****************************************/
   /* Process the files, making the scripts */
   /*****************************************/

   mHiPSPNGScripts_getFiles(directory1);

   fclose(fscript);
   chmod(scriptfile, 0777);

   fclose(fdriver);
   chmod(driverfile, 0777);

   printf("[struct stat=\"OK\", module=\"mHiPSPNGScripts\", nimage=%d]\n", nimage);
   fflush(stdout);
   exit(0);
}



/*******************************/
/*                             */
/*  Count the number of files  */
/*                             */
/*******************************/

int mHiPSPNGScripts_countFiles (char *pathname)
{
   int             len, i;
   char            dirname[MAXSTR];
   DIR            *dp;
   struct dirent  *entry;
   struct stat     type;

   dp = opendir (pathname);

   if (dp == NULL) 
      return 0;

   while ((entry=(struct dirent *)readdir(dp)) != (struct dirent *)0) 
   {
      if(pathname[strlen(pathname)-1] == '/')
         sprintf (dirname, "%s%s", pathname, entry->d_name);
      else
         sprintf (dirname, "%s/%s", pathname, entry->d_name);

      if (stat(dirname, &type) == 0) 
      {
         if (S_ISDIR(type.st_mode) == 1)
         {
            if((strcmp(entry->d_name, "." ) != 0)
            && (strcmp(entry->d_name, "..") != 0))
            {
               if(mHiPSPNGScripts_countFiles (dirname) > 1)
                  return 1;
            }
         }
         else
         {
            len = strlen(dirname);

            if (strncmp(dirname+len-5, ".fits", 5) == 0)
               ++nimage;
         }
      }
   }

   closedir(dp);
   return 0;
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

int mHiPSPNGScripts_getFiles (char *pathname)
{
   int             len, i;

   char            dirname1  [MAXSTR];
   char            dirname2  [MAXSTR];
   char            dirname3  [MAXSTR];
   char            transfile1[MAXSTR];
   char            transfile2[MAXSTR];
   char            transfile3[MAXSTR];
   char            pngfile   [MAXSTR];
   char            tmpfile   [MAXSTR];
   char            newdir    [MAXSTR];
   char            cmd       [MAXSTR];

   char           *ptr;

   DIR            *dp;
   struct dirent  *entry;
   struct stat     type;

   dp = opendir (pathname);

   if (dp == NULL) 
      return 0;

   while ((entry=(struct dirent *)readdir(dp)) != (struct dirent *)0) 
   {
      if(pathname[strlen(pathname)-1] == '/')
         sprintf (dirname1, "%s%s", pathname, entry->d_name);
      else
         sprintf (dirname1, "%s/%s", pathname, entry->d_name);

      if (stat(dirname1, &type) == 0) 
      {
         if (S_ISDIR(type.st_mode) == 1)
         {
            if((strcmp(entry->d_name, "." ) != 0)
            && (strcmp(entry->d_name, "..") != 0))
            {
               if(mHiPSPNGScripts_getFiles (dirname1) > 1)
                  return 1;
            }
         }
         else
         {
            len = strlen(dirname1);

            if(color)
            {
               sprintf(dirname2, "%s%s", directory2, dirname1+len1);
               sprintf(dirname3, "%s%s", directory3, dirname1+len1);
            }

            if (strncmp(dirname1+len-5, ".fits", 5) == 0)
            {
               strcpy(pngfile, dirname1);

               pngfile[len-5] = '\0';

               strcat(pngfile, ".png");

               strcpy(tmpfile, pngfile);

               sprintf(pngfile, "%s%s", outdir, tmpfile+len1);

               strcpy(newdir, pngfile);

               ptr = (char *)NULL;

               for(i=0; i<strlen(newdir); ++i)
               {
                  if(newdir[i] == '/')
                     ptr = newdir + i;
               }

               if(ptr == (char *)NULL)
               {
                  printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScripts -d scriptdir hipsdir histfile [hipsdir2 histfile2 hipsdir3 histfile3] outdir ncore\"]\n");
                  exit(1);
               }

               *ptr = '\0';

               if(debug)
               {
                  printf("DEBUG: mkdir: [%s] for output of file: [%s] processing\n", newdir, transfile1);
                  fflush(stdout);
               }

               mHiPSPNGScripts_mkdir(newdir);

               if(color)
               {
                  sprintf(transfile1, "%s_trans", dirname1);
                  sprintf(transfile2, "%s_trans", dirname2);
                  sprintf(transfile3, "%s_trans", dirname3);

                  fprintf(fscript, "mTranspose %s %s 2 1\n", dirname1, transfile1);
                  fprintf(fscript, "mTranspose %s %s 2 1\n", dirname2, transfile2);
                  fprintf(fscript, "mTranspose %s %s 2 1\n", dirname3, transfile3);

                  fprintf(fscript, "mViewer -blue %s -histfile %s -green %s -histfile %s -red %s -histfile %s -png %s\n",
                     transfile1, histfile1, transfile2, histfile2, transfile3, histfile3, pngfile);

                  fprintf(fscript, "rm %s\n", transfile1);
                  fprintf(fscript, "rm %s\n", transfile2);
                  fprintf(fscript, "rm %s\n", transfile3);
                  fprintf(fscript, "\n");
               }

               else
               {
                  sprintf(transfile1, "%s_trans", dirname1);

                  fprintf(fscript, "mTranspose %s %s 2 1\n", dirname1, transfile1);

                  fprintf(fscript, "mViewer -ct 0 -gray %s -histfile %s -png %s\n", transfile1, histfile1, pngfile);

                  fprintf(fscript, "rm %s\n", transfile1);
                  fprintf(fscript, "\n");
               }

               ++ifile;

               if(ifile >= nfile)
               {
                  ifile = 0;

                  fclose(fscript);
                  chmod(scriptfile, 0777);

                  ++count;

                  sprintf(scriptfile, "%sjobs/png%03d.sh", scriptdir, count);

                  if(debug)
                  {
                     printf("DEBUG> creating scriptfile = [%s]\n", scriptfile);
                     fflush(stdout);
                  }

                  fscript = fopen(scriptfile, "w+");

                  if(fscript == (FILE *)NULL)
                  {
                     printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s].\"]\n", scriptfile);
                     fflush(stdout);
                     exit(0);
                  }

                  fprintf(fscript, "#!/bin/sh\n\n");

                  fprintf(fscript, "echo jobs/png%03d.sh\n\n", count);

                  if(ncore == 1)
                     fprintf(fdriver, "%sjobs/png%03d.sh\n", scriptdir, count);
                  else
                     fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 submitPNG.bash %sjobs/png%03d.sh\n", scriptdir, count);

                  fflush(fdriver);

               }
            }
         }
      }
   }

   closedir(dp);

   return 0;
}



int mHiPSPNGScripts_mkdir(const char *path)
{
   const size_t len = strlen(path);

   char _path[32768];

   char *p; 

   errno = 0;

   if (len > sizeof(_path)-1) 
   {
      errno = ENAMETOOLONG;
      return -1; 
   }   

   strcpy(_path, path);

   /* Iterate the string */
   for (p = _path + 1; *p; p++) 
   {
      if (*p == '/') 
      {
         *p = '\0';

         if (mkdir(_path, S_IRWXU) != 0) 
         {
            if (errno != EEXIST)
               return -1; 
         }

         *p = '/';
      }
   }   

   if (mkdir(_path, S_IRWXU) != 0) 
   {
      if (errno != EEXIST)
         return -1; 
   }   

   return 0;
}
