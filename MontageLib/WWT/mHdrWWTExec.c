#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <svc.h>

int  debug = 0;

int  nhdr;

char hdrDir[1024];

void createSubHdrs(char *hdrStr, int level);


/*************************************************************/
/*                                                           */
/* mHdrWWTExec uses mHdrWWT recursively to create a full set */
/* of WWT tile headers down to some user-specified level.    */
/* The names are fixed, since any given header is fixed,     */
/* regardless of anything having to do with the dataset      */
/* so can be reused repeatedly once created.                 */
/*                                                           */
/*************************************************************/

int main(int argc, char **argv)
{
   int    level, index, x, y, fileStat;
   double crpix1, crpix2;

   char   hdrStr  [256];
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

   if(argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHdrWWTExec [-d lev] tileLevel hdrDir\"]\n");
      fflush(stdout);
      exit(0);
   }

   level = atoi(argv[1]);

   strcpy(hdrDir, argv[2]);

   if(debug)
   {
      printf("DEBUG> level    =  %d\n",  level);
      printf("DEBUG> hdrDir   = [%s]\n", hdrDir);
   }


   // Check the output directory

   fileStat = stat(hdrDir, &buf);

   if(fileStat < 0)
   {
      if(errno == ENOENT)
      {
         fileStat = mkdir(hdrDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

         if(fileStat < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"Problem creating output header directory.\"]\n");
            fflush(stdout);
            exit(0);
         }
      }
      else
      {
         printf("[struct stat=\"ERROR\", msg=\"Problem with output header directory.\"]\n");
         fflush(stdout);
         exit(0);
      }
   }
   else if(!S_ISDIR(buf.st_mode))
   {
      printf("[struct stat=\"ERROR\", msg=\"[%s] is not a directory.\"]\n", hdrDir);
      fflush(stdout);
      exit(0);
   }



   // Recursively create headers

   strcpy(hdrStr, "");

   sprintf(cmd, "mHdrWWT \"\" %s/tile%s.hdr",  hdrDir, hdrStr);

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


   nhdr = 1;

   createSubHdrs(hdrStr, level);


   // All done

   printf("[struct stat=\"OK\", level=%d, nheader=%d]\n", level, nhdr);
   fflush(stdout);
   exit(0);
}



void createSubHdrs(char *instr, int level)
{
   int  i;
   char hdrStr  [256];
   char cmd    [1024];
   char status   [32];

   if(level == 0)
      return;

   for(i=0; i<4; ++i)
   {
      sprintf(hdrStr, "%s%d", instr, i);

      sprintf(cmd, "mHdrWWT %s %s/tile%s.hdr", hdrStr, hdrDir, hdrStr);

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

      ++nhdr;

      createSubHdrs(hdrStr, level-1);
   }
}
