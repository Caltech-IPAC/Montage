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

int  nimage, mode, colorTable, trueColor;

char grayDir   [1024];
char redDir    [1024];
char greenDir  [1024];
char blueDir   [1024];
char grayHist  [1024];
char redHist   [1024];
char greenHist [1024];
char blueHist  [1024];
char pngDir    [1024];
char baseName  [1024];

void createSubTiles(char *tileStr, int level);


int main(int argc, char **argv)
{
   int    level;
   int    fileStat;

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

   if(argc < 7)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mPNGWWTExec [-d lev] colorTable tileLevel baseName grayDir gray.hist pngDir | mPNGWWTExec [-d lev] -c trueColor tileLevel baseName blueTileDir blue.hist greenTileDir green.hist redTileDir red.hist pngDir\"]\n"); 
      fflush(stdout);
      exit(0);
   }

   mode = GRAYSCALE;

   if(argc > 1 && strcmp(argv[1], "-c") == 0)
   {
      mode = COLOR;

      ++argv;
      --argc;

      if(argc < 11)
      {
         printf("[struct stat=\"ERROR\", msg=\"Usage: mPNGWWTExec [-d lev] colorTable tileLevel baseName grayDir gray.hist pngDir | mPNGWWTExec [-d lev] -c trueColor tileLevel baseName blueTileDir blue.hist greenTileDir green.hist redTileDir red.hist pngDir\"]\n"); 
         fflush(stdout);
         exit(0);
      }
   }

   if(mode == GRAYSCALE)
   {
      colorTable = atoi(argv[1]);
      level      = atoi(argv[2]);

      strcpy(baseName,  argv[3]);
      strcpy(grayDir,   argv[4]);
      strcpy(grayHist,  argv[5]);
      strcpy(pngDir,    argv[6]);

      if(debug)
      {
         printf("DEBUG> colorTable =  %d\n",  colorTable);
         printf("DEBUG> level      =  %d\n",  level);
         printf("DEBUG> baseName   = [%s]\n", baseName);
         printf("DEBUG> grayDir    = [%s]\n", grayDir);
         printf("DEBUG> grayHist   = [%s]\n", grayHist);
         printf("DEBUG> pngDir     = [%s]\n", pngDir);
      }
   }

   else  // mode == COLOR
   {
      trueColor  = atoi(argv[1]);
      level      = atoi(argv[2]);

      strcpy(baseName,  argv[3]);

      strcpy(blueDir,   argv[4]);
      strcpy(blueHist,  argv[5]);
      strcpy(greenDir,  argv[6]);
      strcpy(greenHist, argv[7]);
      strcpy(redDir,    argv[8]);
      strcpy(redHist,   argv[9]);
      strcpy(pngDir,    argv[10]);

      if(debug)
      {
         printf("DEBUG> colorTable =  %d\n",   colorTable);
         printf("DEBUG> level      =  %d\n",   level);
         printf("DEBUG> baseName   = [%s]\n",  baseName);
         printf("DEBUG> blueDir    = [%s]\n",  blueDir);
         printf("DEBUG> blueHist   = [%s]\n",  blueHist);
         printf("DEBUG> greenDir    = [%s]\n", greenDir);
         printf("DEBUG> greenHist   = [%s]\n", greenHist);
         printf("DEBUG> redDir    = [%s]\n",   redDir);
         printf("DEBUG> redHist   = [%s]\n",   redHist);
         printf("DEBUG> pngDir     = [%s]\n",  pngDir);
      }
   }



   // Check the tile directory/directories

   if(mode == GRAYSCALE)
   {
      fileStat = stat(grayDir, &buf);

      if(fileStat < 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot find/address tile directory [%s].\"]\n", grayDir);
         fflush(stdout);
         exit(0);
      }

      else if(!S_ISDIR(buf.st_mode))
      {
         printf("[struct stat=\"ERROR\", msg=\"[%s] is not a directory.\"]\n", grayDir);
         fflush(stdout);
         exit(0);
      }
   }

   else  // mode == COLOR
   {
      fileStat = stat(blueDir, &buf);

      if(fileStat < 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot find/address tile directory [%s].\"]\n", blueDir);
         fflush(stdout);
         exit(0);
      }

      else if(!S_ISDIR(buf.st_mode))
      {
         printf("[struct stat=\"ERROR\", msg=\"[%s] is not a directory.\"]\n", blueDir);
         fflush(stdout);
         exit(0);
      }
       

      fileStat = stat(greenDir, &buf);

      if(fileStat < 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot find/address tile directory [%s].\"]\n", greenDir);
         fflush(stdout);
         exit(0);
      }

      else if(!S_ISDIR(buf.st_mode))
      {
         printf("[struct stat=\"ERROR\", msg=\"[%s] is not a directory.\"]\n", greenDir);
         fflush(stdout);
         exit(0);
      }
       

      fileStat = stat(redDir, &buf);

      if(fileStat < 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot find/address tile directory [%s].\"]\n", redDir);
         fflush(stdout);
         exit(0);
      }

      else if(!S_ISDIR(buf.st_mode))
      {
         printf("[struct stat=\"ERROR\", msg=\"[%s] is not a directory.\"]\n", redDir);
         fflush(stdout);
         exit(0);
      }
   }


   // Check the histogram file(s)

   if(mode == GRAYSCALE)
   {
      fileStat = stat(grayHist, &buf);

      if(fileStat < 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot find histogram file [%s].\"]\n", grayHist);
         fflush(stdout);
         exit(0);
      }
   }

   else  // mode == COLOR
   {
      fileStat = stat(blueHist, &buf);

      if(fileStat < 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot find histogram file [%s].\"]\n", blueHist);
         fflush(stdout);
         exit(0);
      }
       

      fileStat = stat(greenHist, &buf);

      if(fileStat < 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot find histogram file [%s].\"]\n", greenHist);
         fflush(stdout);
         exit(0);
      }
       

      fileStat = stat(redHist, &buf);

      if(fileStat < 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot find histogram file [%s].\"]\n", redHist);
         fflush(stdout);
         exit(0);
      }
   }


   // Check the output directory (it has to already exist)

   fileStat = stat(pngDir, &buf);

   if(fileStat < 0)
   {
      if(errno == ENOENT)
      {
         fileStat = mkdir(pngDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

         if(fileStat < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"Problem creating output PNG directory.\"]\n");
            fflush(stdout);
            exit(0);
         }
      }
      else
      {
         printf("[struct stat=\"ERROR\", msg=\"Problem with output PNG directory.\"]\n");
         fflush(stdout);
         exit(0);
      }
   }
   else if(!S_ISDIR(buf.st_mode))
   {
      printf("[struct stat=\"ERROR\", msg=\"[%s] is not a directory.\"]\n", pngDir);
      fflush(stdout);
      exit(0);
   }


   // Recursively create PNG files

   strcpy(tileStr, "");

   if(mode == GRAYSCALE)
   {
      sprintf(cmd, "mViewer -ct %d -gray %s/%s%s.fits -histfile %s -out %s/%s%s.png",
            colorTable, grayDir, baseName, tileStr, grayHist, pngDir, baseName, tileStr);
   }
   else
   {
      sprintf(cmd, "mViewer -t %d -blue %s/%s%s.fits -histfile %s -green %s/%s%s.fits -histfile %s -red %s/%s%s.fits -histfile %s -out %s/%s%s.png",
            trueColor, 
            blueDir, baseName, tileStr, blueHist,
            greenDir, baseName, tileStr, greenHist,
            redDir, baseName, tileStr, redHist,
            pngDir, baseName, tileStr);
   }

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

   printf("[struct stat=\"OK\", module=\"mPNGWWTExec\", level=%d, nimage=%d]\n", level, nimage);
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

      if(mode == GRAYSCALE)
      {
         sprintf(cmd, "mViewer -ct %d -gray %s/%s%s.fits -histfile %s -out %s/%s%s.png",
               colorTable, grayDir, baseName, tileStr, grayHist, pngDir, baseName, tileStr);
      }
      else
      {
         sprintf(cmd, "mViewer -t %d -blue %s/%s%s.fits -histfile %s -green %s/%s%s.fits -histfile %s -red %s/%s%s.fits -histfile %s -out %s/%s%s.png",
               trueColor, 
               blueDir, baseName, tileStr, blueHist,
               greenDir, baseName, tileStr, greenHist,
               redDir, baseName, tileStr, redHist,
               pngDir, baseName, tileStr);
      }

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
