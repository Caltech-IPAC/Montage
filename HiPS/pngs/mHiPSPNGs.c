/* Module: mHiPSPNGs.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        14Jul19  Baseline code
1.1      John Good        27Aug19  Added color image support

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

int  mHiPSPNGs_getFiles (char *pathname);
void mHiPSPNGs_printFitsError(int status);
int  mHiPSPNGs_mkdir(const char *path);

int  nimage, nerror;

double brightness, contrast;

char directory1[1024];
char directory2[1024];
char directory3[1024];
char histfile1 [1024];
char histfile2 [1024];
char histfile3 [1024];
char outdir    [1024];

int  no_transpose, is_dir, ct;
int  len1, lenout;

int  color = 0;

int  debug;


/*************************************************************************/
/*                                                                       */
/*  mHiPSPNGs                                                            */
/*                                                                       */
/*  This program recursively finds all the FITS files in a directory     */
/*  tree and generates a matching PNG (grayscale) using the supplied     */
/*  histogram.                                                           */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int i;
   struct stat type;

   no_transpose =  0;
   is_dir       =  0;
   ct           = -1;

   if(argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGs [-d] [-n(o-transpose)] brightness contrast directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
      exit(1);
   }

   for(i=0; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
      {
         debug = 1;

         svc_debug(stdout);
      }

      if(strcmp(argv[i], "-n") == 0)
         no_transpose = 1;

      if(strcmp(argv[i], "-ct") == 0)
      {
         ct = atoi(argv[i+1]);
         ++i;
      }
   }
      
   if(debug)
   {
      ++argv;
      --argc;
   }
      
   if(no_transpose)
   {
      ++argv;
      --argc;
   }
      
   if(ct >= 0)
   {
      argv += 2;
      argc -= 2;
   }

   if(ct < 0)
      ct = 0;

   brightness = atof(argv[1]);
   contrast   = atof(argv[2]);

   strcpy(directory1, argv[3]);
   strcpy(histfile1,  argv[4]);

   if(directory1[strlen(directory1)-1] != '/')
      strcat(directory1, "/");

   if (stat(histfile1, &type) == 0) 
      if (S_ISDIR(type.st_mode) == 1)
         is_dir = 1;

   if(is_dir && histfile1[strlen(histfile1)-1] != '/')
      strcat(histfile1, "/");

   len1 = strlen(directory1);

   strcpy(outdir, argv[5]);

   if(outdir[strlen(outdir)-1] != '/')
      strcat(outdir, "/");


   if(argc > 6)
   {
      if(argc < 10)
      {
         printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGs [-d] [-n(o-transpose)] brightness contrast directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
         exit(1);
      }

      strcpy(directory2, argv[5]);
      strcpy(histfile2,  argv[6]);

      if(is_dir && histfile2[strlen(histfile2)-1] != '/')
         strcat(histfile2, "/");

      strcpy(directory3, argv[7]);
      strcpy(histfile3,  argv[8]);

      if(is_dir && histfile3[strlen(histfile3)-1] != '/')
         strcat(histfile3, "/");

      strcpy(outdir, argv[9]);

      if(directory2[strlen(directory2)-1] != '/')
         strcat(directory2, "/");

      if(directory3[strlen(directory3)-1] != '/')
         strcat(directory3, "/");

      if(outdir[strlen(outdir)-1] != '/')
         strcat(outdir, "/");

      lenout = strlen(outdir);

      color = 1;
   }

   nimage = 0;
   nerror = 0;

   mHiPSPNGs_getFiles(directory1);

   printf("[struct stat=\"OK\", module=\"mHiPSPNGs\", directory=\"%s\", nimage=%d, nerror=%d]\n", 
         directory1, nimage, nerror);
   fflush(stdout);
   exit(0);
}



/*******************************/
/*                             */
/*  Step through the directory */
/*                             */
/*******************************/

int mHiPSPNGs_getFiles (char *pathname)
{
   int             len, i;

   char            dirname1  [MAXSTR];
   char            dirname2  [MAXSTR];
   char            dirname3  [MAXSTR];
   char            transfile1[MAXSTR];
   char            transfile2[MAXSTR];
   char            transfile3[MAXSTR];
   char            hist1     [MAXSTR];
   char            hist2     [MAXSTR];
   char            hist3     [MAXSTR];
   char            pngfile   [MAXSTR];
   char            newdir    [MAXSTR];
   char            cmd       [MAXSTR];

   char            status[32];

   char           *ptr;

   DIR            *dp;
   struct dirent  *entry;
   struct stat     type;

   dp = opendir (pathname);

   if(debug)
   {
      printf("DEBUG: Opening path    [%s]\n", pathname);
      fflush(stdout);
   }

   if (dp == NULL) 
      return 0;

   // Histogram file logic:  If we are given a single histogram file
   // for the image (or first image set if there are three), we use that.
   // If what we are given turns out to be a directory, we assume it
   // contains one histogram file per image, with the same base name.
   
   while ((entry=(struct dirent *)readdir(dp)) != (struct dirent *)0) 
   {
      if(debug)
      {
         printf("\nDEBUG:  entry [%s]\n", entry->d_name);
         fflush(stdout);
      }

      sprintf (dirname1, "%s%s", pathname, entry->d_name);


      if(debug)
      {
         printf("DEBUG: file/directory: [%s]\n", dirname1);
         fflush(stdout);
      }

      if (stat(dirname1, &type) == 0) 
      {
         if (S_ISDIR(type.st_mode) == 1)
         {
            if((strcmp(entry->d_name, "." ) != 0)
            && (strcmp(entry->d_name, "..") != 0))
            {
               if(debug)
               {
                  printf("DEBUG: Found directory [%s]\n", dirname1);
                  fflush(stdout);
               }

               if(mHiPSPNGs_getFiles (dirname1) > 1)
                  return 1;
            }
         }
         else
         {
            sprintf(pngfile, "%s%s", outdir, dirname1+len1);

            pngfile[strlen(pngfile)-5] = '\0';

            strcat(pngfile, ".png");

            if(is_dir)
            {
               sprintf(hist1, "%s%s", histfile1, entry->d_name);
               *(hist1+strlen(hist1)-5) = '\0';
               strcat(hist1, ".hist");
            }
            else
            {
               strcpy(hist1, histfile1);
               strcpy(hist2, histfile2);
               strcpy(hist3, histfile3);
            }


            if(color)
            {
               sprintf(dirname2, "%s%s", directory2, dirname1+len1);
               sprintf(dirname3, "%s%s", directory3, dirname1+len1);

               sprintf(hist2, "%s%s", histfile2, entry->d_name);
               *(hist2+strlen(hist2)-5) = '\0';
               strcat(hist2, ".hist");

               sprintf(hist3, "%s%s", histfile3, entry->d_name);
               *(hist3+strlen(hist3)-5) = '\0';
               strcat(hist3, ".hist");
            }

            if (strncmp(dirname1+strlen(dirname1)-5, ".fits", 5) == 0)
            {
               if(debug)
               {
                  printf("DEBUG: Found FITS file [%s]\n", dirname1);
                  fflush(stdout);
               }

               if(debug)
               {
                  printf("DEBUG> directory1: [%s]\n", directory1);
                  printf("DEBUG> len1:        %d \n", len1);
                  printf("DEBUG> directory2: [%s]\n", directory2);
                  printf("DEBUG> directory3: [%s]\n", directory3);
                  printf("DEBUG> dirname1:   [%s]\n", dirname1);
                  printf("DEBUG> dirname2:   [%s]\n", dirname2);
                  printf("DEBUG> dirname3:   [%s]\n", dirname3);
                  printf("DEBUG> hist1:      [%s]\n", hist1);
                  printf("DEBUG> hist2:      [%s]\n", hist2);
                  printf("DEBUG> hist3:      [%s]\n", hist3);
                  printf("DEBUG> pngfile:    [%s]\n", pngfile);
                  fflush(stdout);
               }

               if(color)
               {
                  strcpy(newdir, pngfile);

                  ptr = (char *)NULL;

                  for(i=0; i<strlen(newdir); ++i)
                  {
                     if(newdir[i] == '/')
                        ptr = newdir + i;
                  }

                  if(ptr == (char *)NULL)
                  {
                     printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGs -d directory histfile(/histdir) [directory2 histfile2 directory3 histfile3] outdir\"]\n");
                     exit(1);
                  }

                  *ptr = '\0';

                  if(debug)
                  {
                     printf("DEBUG: Recursive mkdir: [%s]\n", newdir);
                     fflush(stdout);
                  }

                  mHiPSPNGs_mkdir(newdir);

                  sprintf(transfile1, "%s_trans", dirname1);
                  sprintf(transfile2, "%s_trans", dirname2);
                  sprintf(transfile3, "%s_trans", dirname3);

                  if(!no_transpose)
                  {
                     sprintf(cmd, "mTranspose %s %s 2 1", dirname1, transfile1);

                     if(debug)
                     {
                        printf("DEBUG: Command: [%s]\n", cmd);
                        fflush(stdout);
                     }

                     svc_run(cmd);

                     sprintf(cmd, "mTranspose %s %s 2 1", dirname2, transfile2);

                     if(debug)
                     {
                        printf("DEBUG: Command: [%s]\n", cmd);
                        fflush(stdout);
                     }

                     svc_run(cmd);

                     sprintf(cmd, "mTranspose %s %s 2 1", dirname3, transfile3);

                     if(debug)
                     {
                        printf("DEBUG: Command: [%s]\n", cmd);
                        fflush(stdout);
                     }
                  }
                  else
                  {
                     strcpy(transfile1, dirname1);
                     strcpy(transfile2, dirname2);
                     strcpy(transfile3, dirname3);
                  }

                  svc_run(cmd);

                  sprintf(cmd, "mViewer -brightness %-g -contrast %-g -blue %s -histfile %s -green %s -histfile %s -red %s -histfile %s -png %s",
                     brightness, contrast, transfile1, hist1, transfile2, hist2, transfile3, hist3, pngfile);

                  if(debug)
                  {
                     printf("DEBUG: Command: [%s]\n", cmd);
                     fflush(stdout);
                  }

                  svc_run(cmd);

                  strcpy(status, svc_value("stat"));
                  
                  if(strcmp(status, "ERROR") == 0)
                     ++nerror;

                  svc_closeall();

                  if(!no_transpose)
                  {
                     unlink(transfile1);
                     unlink(transfile2);
                     unlink(transfile3);
                  }
               }

               else
               {
                  sprintf(transfile1, "%s_trans", dirname1);

                  if(!no_transpose)
                  {
                     sprintf(cmd, "mTranspose %s %s 2 1", dirname1, transfile1);

                     if(debug)
                     {
                        printf("DEBUG: Command: [%s]\n", cmd);
                        fflush(stdout);
                     }
                  }
                  else
                     strcpy(transfile1, dirname1);

                  svc_run(cmd);

                  sprintf(cmd, "mViewer -ct %d -brightness %-g -contrast %-g -gray %s -histfile %s -png %s",
                     ct, brightness, contrast, transfile1, hist1, pngfile);

                  if(debug)
                  {
                     printf("DEBUG: Command: [%s]\n", cmd);
                     fflush(stdout);
                  }

                  svc_run(cmd);

                  strcpy(status, svc_value("stat"));
                  
                  if(strcmp(status, "ERROR") == 0)
                     ++nerror;

                  if(!no_transpose)
                     unlink(transfile1);
               }

               ++nimage;
            }
         }
      }
   }

   closedir(dp);
   return 0;
}



int mHiPSPNGs_mkdir(const char *path)
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
