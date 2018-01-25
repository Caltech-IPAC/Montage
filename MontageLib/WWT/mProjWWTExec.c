#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <svc.h>

#define GRAYSCALE 0
#define COLOR     1

int  debug = 0;

int  nimage;

char fitsFile  [1024];
char baseName  [1024];
char tileDir   [1024];
char hdrDir    [1024];

void createSubTiles(char *tileStr, int level);


int main(int argc, char **argv)
{
   int    level, fileStat;

   char   tileStr [256];
   char   cmd    [1024];
   char   status   [32];

   struct stat buf;



   // Process command line

   if(argc > 2 && strcmp(argv[1], "-d") == 0)
   {
      debug = atoi(argv[2]);

      if(debug > 1)
         svc_debug(stdout);

      argv += 2;
      argc -= 2;
   }

   if(argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mProjWWTExec [-d lev] tileLevel input.fits baseName hdrDir tileDir\"]\n");
      fflush(stdout);
      exit(0);
   }

   level = atoi(argv[1]);

   strcpy(fitsFile,  argv[2]);
   strcpy(baseName,  argv[3]);
   strcpy(hdrDir,    argv[4]);
   strcpy(tileDir,   argv[5]);

   if(debug)
   {
      printf("DEBUG> level    =  %d\n",  level);
      printf("DEBUG> fitsFile = [%s]\n", fitsFile);
      printf("DEBUG> baseName = [%s]\n", baseName);
      printf("DEBUG> hdrDir   = [%s]\n", hdrDir);
      printf("DEBUG> tileDir  = [%s]\n", tileDir);
   }


   // Check input files

   fileStat = stat(fitsFile, &buf);

   if(fileStat < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Problem with FITS file.\"]\n");
      fflush(stdout);
      exit(0);
   }


   // Check the header directory

   fileStat = stat(hdrDir, &buf);

   if(fileStat < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot find/address header directory [%s].\"]\n", hdrDir);
      fflush(stdout);
      exit(0);
   }

   else if(!S_ISDIR(buf.st_mode))
   {
      printf("[struct stat=\"ERROR\", msg=\"[%s] is not a directory.\"]\n", hdrDir);
      fflush(stdout);
      exit(0);
   }


   // Check the output directory (it has to already exist)

   fileStat = stat(tileDir, &buf);

   if(fileStat < 0)
   {
      if(errno == ENOENT)
      {
         fileStat = mkdir(tileDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

         if(fileStat < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"Problem creating output tile directory.\"]\n");
            fflush(stdout);
            exit(0);
         }
      }
      else
      {
         printf("[struct stat=\"ERROR\", msg=\"Problem with output tile directory.\"]\n");
         fflush(stdout);
         exit(0);
      }
   }
   else if(!S_ISDIR(buf.st_mode))
   {
      printf("[struct stat=\"ERROR\", msg=\"[%s] is not a directory.\"]\n", tileDir);
      fflush(stdout);
      exit(0);
   }


   // Recursively create tiles

   strcpy(tileStr, "");

   sprintf(cmd, "mProjectQL %s %s/%s%s.fits %s/tile%s.hdr",
      fitsFile, tileDir, baseName, tileStr, hdrDir, tileStr);

   if(debug)
   {
      printf("%s\n", cmd);
      fflush(stdout);
   }

   svc_run(cmd);

   strcpy(status, svc_value("stat"));

   if(strcmp(status, "OK") != 0)
   {
      printf("[stat=\"ERROR\", msg=\"%s\"]\n", svc_value("msg"));
      fflush(stdout);
      exit(0);
   }

   nimage = 1;

   createSubTiles(tileStr, level);


   // All done

   printf("[struct stat=\"OK\", module=\"mProjWWTExec\", level=%d, nimage=%d]\n", level, nimage);
   fflush(stdout);
   exit(0);
}



void createSubTiles(char *instr, int level)
{
   int  i;
   char tileStr [256];
   char cmd    [1024];
   char status   [32];

   if(level == 0)
      return;

   for(i=0; i<4; ++i)
   {
      sprintf(tileStr, "%s%d", instr, i);

      sprintf(cmd, "mProjectQL %s %s/%s%s.fits %s/tile%s.hdr",
         fitsFile, tileDir, baseName, tileStr, hdrDir, tileStr);

      if(debug)
      {
         printf("%s\n", cmd);
         fflush(stdout);
      }

      svc_run(cmd);

      strcpy(status, svc_value("stat"));

      if(strcmp(status, "OK") != 0)
      {
         printf("[stat=\"ERROR\", msg=\"%s\"]\n", svc_value("msg"));
         fflush(stdout);
         exit(0);
      }

      ++nimage;

      createSubTiles(tileStr, level-1);
   }
}
