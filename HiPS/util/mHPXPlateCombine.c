#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <svc.h>

#define MAXSTR 1024

int main(int argc, char **argv)
{
   int  face, i, j, order, maxorder, status;

   char cmd     [MAXSTR];
   char filename[MAXSTR];
   char platedir[MAXSTR];
   char from    [MAXSTR];
   char to      [MAXSTR];

   int debug = 0;

   int nplate = 13;

   int xoffset[] = {1, 0, 3, 2, 2, 1, 0, 3, 2, 1, 4, 3, 4};
   int yoffset[] = {2, 1, 4, 3, 2, 1, 0, 3, 1, 0, 3, 2, 4};


   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXPlateCombine [-d(ebug)] platedir maxorder\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(strcmp(argv[1], "-d") == 0)
   {
      debug = 1;

      --argc;
      ++argv;
   }

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXPlateCombine [-d(ebug)] platedir maxorder\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(platedir, argv[1]);

   maxorder = atoi(argv[2]);

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(debug)
   {
      printf("#!/bin/sh\n\n");
      fflush(stdout);
   }

   for(order=maxorder; order>=0; --order)
   {
      sprintf(from, "%sorder%d",      platedir, order);
      sprintf(to,   "%sorder%d.orig", platedir, order);

      if(debug)
      {
         printf("mv %s %s\n", from, to);
         fflush(stdout);
      }
      else
      {
         status = rename(from, to);

         if(status)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot rename directory %s\"]\n", from);
            fflush(stdout);
            exit(0);
         }
      }

      if(debug)
      {
         printf("mkdir %s\n", from);
         printf("chmod 755 %s\n", from);
         fflush(stdout);
      }
      else
         mkdir(from, 0755);

      sprintf(cmd, "mImgtbl %sorder%d.orig %sorder%d.tbl", platedir, order, platedir, order);
      
      if(debug)
      {
         printf("%s\n", cmd);
         fflush(stdout);
      }
      else
      {
         status = svc_run(cmd);

         if(status)
         {
            printf("[struct stat=\"ERROR\", msg=\"mImgtbl failed\"]\n");
            fflush(stdout);
            exit(0);
         }
      }

      sprintf(cmd, "mHPXHdr %d %shpx%d.hdr", order, platedir, order);
      
      if(debug)
      {
         printf("%s\n", cmd);
         fflush(stdout);
      }
      else
      {
         status = svc_run(cmd);

         if(status)
         {
            printf("[struct stat=\"ERROR\", msg=\"mHPXHdr failed\"]\n");
            fflush(stdout);
            exit(0);
         }
      }

      for(face=0; face<13; ++face)
      {
         i = xoffset[face];
         j = yoffset[face];

         sprintf(cmd, "mTileHdr %shpx%d.hdr %shpx%d_%02d_%02d.hdr 5 5 %02d %02d",
            platedir, order, platedir, order, i, j, i, j);

         if(debug)
         {
            printf("%s\n", cmd);
            fflush(stdout);
         }
         else
         {
            status = svc_run(cmd);

            if(status)
            {
               printf("[struct stat=\"ERROR\", msg=\"mTileHdr failed\"]\n");
               fflush(stdout);
               exit(0);
            }
         }


         sprintf(cmd, "mAdd -n -p %sorder%d.orig %sorder%d.tbl %shpx%d_%02d_%02d.hdr %sorder%d/plate_%02d_%02d.fits",
            platedir, order, platedir, order, platedir, order, i, j, platedir, order, i, j);

         if(debug)
         {
            printf("%s\n", cmd);
            fflush(stdout);
         }
         else
         {
            status = svc_run(cmd);

            if(status)
            {
               printf("[struct stat=\"ERROR\", msg=\"mAdd failed\"]\n");
               fflush(stdout);
               exit(0);
            }
         }

         sprintf(filename, "%sorder%d/plate_%02d_%02d_area.fits",
            platedir, order, i, j, platedir, order, i, j);

         if(debug)
         {
            printf("unlink %s\n", filename);
            fflush(stdout);
         }
         
         unlink(filename);

         sprintf(filename, "%shpx%d_%02d_%02d.hdr", platedir, order, i, j);

         if(debug)
         {
            printf("unlink %s\n", filename);
            fflush(stdout);
         }
         
         unlink(filename);
      }
   }

   if(!debug)
      printf("[struct stat=\"OK\"]\n");
   fflush(stdout);

   exit(0);
}
