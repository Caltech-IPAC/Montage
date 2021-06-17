#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <math.h>

#define STRLEN 1024

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int montage_debugCheck(char *debugStr);



/*************************************************************************/
/*                                                                       */
/*  mHPXGap                                                              */
/*                                                                       */
/*  In an HPX all-sky projection there are six sets of "adjacent" plates */
/*  that are actually across gaps in the projection space.  We have      */
/*  developed a routine to simulate the overlap difference measures      */
/*  needed by the background modeling by looking at the image background */
/*  levels along the edges that would match up.                          */
/*                                                                       */
/*  Rather than have that routine loop over all the pairs, we just let   */
/*  it do one and use this routine to build a list of the pairs, or for  */
/*  a cluster/cloud environment sets of lists so we can run multiple     */
/*  parallel threads.                                                    */
/*                                                                       */
/*  We aren't going to try determining if there are missing regions;     */
/*  any such will just fail.  The input for this program is therefore    */
/*  just the HPX order and number of plates (so we know how many there   */
/*  are along each edge) and the number of "scripts" (for the cluster)   */
/*  we want. There are also optional parameters to pass to the           */
/*  underlying routine.                                                  */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int   c, levelOnly, width, debug;
   int   N, fx, fy, Fx, Fy, x, y, X, Y;
   int   ix, iy, jx, jy;
   int   ifile, nplate, ngap;

   char  mosaicdir [STRLEN];
   char  scriptdir [STRLEN];
   char  driverfile[STRLEN];
   char  cmd       [STRLEN];
   char  tmpstr    [STRLEN];
   char  cwd       [STRLEN];

   FILE *fdriver;
   FILE *scriptfile;

   char *end;

   getcwd(cwd, STRLEN);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug     = 0;
   width     = 4096;
   levelOnly = 0;

   opterr    = 0;

   strcpy(mosaicdir, "");

   while ((c = getopt(argc, argv, "n:p:dw:l")) != EOF) 
   {
      switch (c) 
      {
         case 'w':
            width = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) cannot be interpreted as an integer\"]\n", 
                  optarg);
               exit(1);
            }

            if(width < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) must be a positive integer\"]\n", 
                  optarg);
               exit(1);
            }

            break;

         case 'd':
            debug = 1;
            break;

         case 'l':
            levelOnly = 1;
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXGapScripts [-d] [-w width] [-l(evel-only)] scriptdir mosaicdir nplate\"]\n");
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXGapScripts [-d] [-w width] [-l(evel-only)] scriptdir mosaicdir nplate\"]\n");
      exit(1);
   }


   strcpy(scriptdir, argv[optind]);

   if(scriptdir[0] != '/')
   {
      strcpy(tmpstr, cwd);

      if(strlen(scriptdir) > 0)
      {
         strcat(tmpstr, "/");
         strcat(tmpstr, scriptdir);
      }

      strcpy(scriptdir, tmpstr);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");


   strcpy(mosaicdir, argv[optind+1]);

   if(mosaicdir[0] != '/')
   {
      strcpy(tmpstr, cwd);

      if(strlen(mosaicdir) > 0)
      {
         strcat(tmpstr, "/");
         strcat(tmpstr, mosaicdir);
      }

      strcpy(mosaicdir, tmpstr);
   }

   if(mosaicdir[strlen(mosaicdir)-1] != '/')
      strcat(mosaicdir, "/");


   nplate = atoi(argv[optind+2]);

   N = nplate/5;

   if(debug)
   {
      printf("DEBUG> scriptdir = [%s]\n", scriptdir);
      printf("DEBUG> nplate    =  %d\n", nplate);
      printf("DEBUG> N         =  %d\n", N);
      printf("DEBUG> levelOnly =  %d\n", levelOnly);
      printf("DEBUG> width     =  %d\n", width);
      fflush(stdout);
   }

   
   // Open the driver script file

   if(mkdir(scriptdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create script directory [%s].\n", scriptdir);
      exit(1);
   }

   sprintf(driverfile, "%srunGap.sh", scriptdir);

   fdriver = fopen(driverfile, "w+");

   if(debug)
   {
      printf("DEBUG> driver file: [%s]\n", driverfile);
      fflush(stdout);
   }

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);


   // Open the output script files

   sprintf(tmpstr, "%s/jobs", scriptdir);

   if(mkdir(scriptdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create jobs directory [%s].\n", scriptdir);
      exit(1);
   }

   sprintf(tmpstr, "%s/logs", scriptdir);

   if(mkdir(scriptdir, 0775) < 0 && errno != EEXIST)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create logs directory [%s].\n", scriptdir);
      exit(1);
   }


   // TOP to LEFT
   
   ngap = 0;

   ifile = 0;

   for(fx=0; fx<=3; ++fx)
   {
      fy = fx+1;
      Fx = (fx+1)%4;
      Fy = Fx+1;

      for(x=0; x<N; ++x)
      {
         y = N-1;

         Y = N-1-x;
         X = 0;

         ix = fx*N + x;
         iy = fy*N + y;

         jx = Fx*N + X;
         jy = Fy*N + Y;

         ++ngap;

         if(debug)
         {
            printf("DEBUG> (%2d,%2d,%2d,%2d) -> (%2d,%2d,%2d,%2d) : plate_%02d_%02d.fits (T) -> plate_%02d_%02d.fits (L)\n", 
                  fx, x, fy, y, Fx, X, Fy, Y, ix, iy, jx, jy);
            fflush(stdout);
         }

         sprintf(cmd, "mHPXGapDiff -w %d ", width);

         strcat(cmd, "-g $1 ");

         if(levelOnly)
            strcat(cmd, "-l ");

         
         sprintf(tmpstr, "%sjobs/gap_%03d.sh", scriptdir, ifile);

         if(debug)
         {
            printf("DEBUG> scriptfile = [%s]\n", tmpstr);
            fflush(stdout);
         }

         scriptfile = fopen(tmpstr, "w+");

         if(scriptfile == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open script file [%s].\n", tmpstr);
            exit(1);
         }

         fprintf(scriptfile, "#!/bin/sh\n\n");
         fflush(scriptfile);


         sprintf(cmd, "%s$1/plate_%02d_%02d.fits top $1/plate_%02d_%02d.fits left", 
               cmd, ix, iy, jx, jy);

         fprintf(scriptfile, "%s\n", cmd);
         fflush(scriptfile);
         fclose(scriptfile);

         chmod(tmpstr, 0775);


         fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 submitGap.bash %sjobs/gap_%03d.sh %s\n",
            scriptdir, ifile, mosaicdir);
         fflush(fdriver);
         
         ++ifile;
      }
   }


   // BOTTOM to RIGHT

   for(Fy=0; Fy<=3; ++Fy)
   {
      Fx = Fy+1;
      fy = (Fy+1)%4;
      fx = fy+1;

      for(x=0; x<N; ++x)
      {
         y = 0;

         Y = N-1-x;
         X = N-1;

         ix = fx*N + x;
         iy = fy*N + y;

         jx = Fx*N + X;
         jy = Fy*N + Y;

         ++ngap;

         if(debug)
         {
            printf("DEBUG> (%2d,%2d,%2d,%2d) -> (%2d,%2d,%2d,%2d) : plate_%02d_%02d.fits (B) -> plate_%02d_%02d.fits (R)\n", 
                  fx, x, fy, y, Fx, X, Fy, Y, ix, iy, jx, jy);
            fflush(stdout);
         }

         sprintf(cmd, "mHPXGapDiff -w %d ", width);

         strcat(cmd, "-g $1 ");

         if(levelOnly)
            strcat(cmd, "-l ");

         
         sprintf(tmpstr, "%sjobs/gap_%03d.sh", scriptdir, ifile);

         if(debug)
         {
            printf("DEBUG> scriptfile = [%s]\n", tmpstr);
            fflush(stdout);
         }

         scriptfile = fopen(tmpstr, "w+");

         if(scriptfile == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open script file [%s].\n", tmpstr);
            exit(1);
         }

         fprintf(scriptfile, "#!/bin/sh\n\n");
         fflush(scriptfile);


         sprintf(cmd, "%s$1/plate_%02d_%02d.fits top $1/plate_%02d_%02d.fits left", 
               cmd, ix, iy, jx, jy);

         fprintf(scriptfile, "%s\n", cmd);
         fflush(scriptfile);
         fclose(scriptfile);

         chmod(tmpstr, 0775);


         fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 submitGap.bash %sjobs/gap_%03d.sh %s\n",
            scriptdir, ifile, mosaicdir);
         fflush(fdriver);
            
         ++ifile;
      }
   }

   fclose(fdriver);
   chmod(driverfile, 0775);

   printf("[struct stat=\"OK\", module=\"mHPXGap\", ngap=%d]\n", ngap);
   fflush(stdout);
   exit(0);
}
