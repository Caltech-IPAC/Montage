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

int  mHiPSPNGScripts_localFiles    (char *pathname);
int  mHiPSPNGScripts_S3Files       (char *bucket);
int  mHiPSPNGScripts_processFiles  ();
void mHiPSPNGScripts_printFitsError(int status);

int  job, njob, ntile, ntask, ifile, count, offset, haveimages;

char scriptdir [1024];
char scriptfile[1024];
char taskfile  [1024];
char directory1[1024];
char directory2[1024];
char directory3[1024];
char histfile1 [1024];
char histfile2 [1024];
char histfile3 [1024];
char imglist   [1024];
char outdir    [1024];
char flags     [1024];
char jobfile   [1024];
char  newdir   [1024];
char  olddir   [1024];

FILE *fscript;
FILE *fdriver;
FILE *ftask;
FILE *fimages;
FILE *fjob;

double contrast, brightness;
int    ct, cloud;

int  color = 0;

int  single_threaded = 0;

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
/*  histogram.  Or if the data is stored in the cloud, get the file      */
/*  list from there and create the PNGs there.                           */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int i, c;

   char cwd       [MAXSTR];
   char cmd       [MAXSTR];
   char file      [MAXSTR];
   char tmpdir    [MAXSTR];
   char driverfile[MAXSTR];

   FILE *fcount;


   getcwd(cwd, MAXSTR);

   debug           =  0;
   haveimages      =  0;
   cloud           =  0;
   ct              =  0;
   brightness      =  0.;
   contrast        =  0.;
   single_threaded =  0;

   strcpy(scriptdir,  "");
   strcpy(directory1, "");
   strcpy(histfile1,  "");
   strcpy(directory2, "");
   strcpy(histfile2,  "");
   strcpy(directory3, "");
   strcpy(histfile3,  "");
   strcpy(imglist,    "");
   strcpy(outdir,     "");
   strcpy(olddir,     "");
   strcpy(newdir,     "");

   count = 1;

   while((c = getopt(argc, argv, "dhb:c:st:")) != EOF)
   {
      switch(c)
      {
         case 'd':
            debug = 1;
            break;

         case 'h':
            haveimages = 1;
            break;

         case 'b':
            brightness = atof(optarg);
            break;

         case 'c':
            contrast = atof(optarg);
            break;

         case 's':
            single_threaded = 1;
            break;

         case 't':
            ct = atoi(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScripts [-d][-s(ingle-threaded)][-h(ave-images)][-b brightness][-c contrast][-t color-table] scriptdir tiledir histfile [tiledir2 histfile2 tiledir3 histfile3] imglist pngdir\"]\n");
            exit(1);
            break;
      }
   }

   if(argc-optind < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScripts [-d][-s(ingle-threaded)][-h(ave-images)][-b brightness][-c contrast][-t color-table] scriptdir tiledir histfile [tiledir2 histfile2 tiledir3 histfile3] imglist pngdir\"]\n");
      exit(1);
   }

   if(ct < 0)
      ct = 0;

   strcpy(scriptdir,  argv[optind]);
   strcpy(directory1, argv[optind+1]);
   strcpy(histfile1,  argv[optind+2]);
   strcpy(imglist,    argv[optind+3]);
   strcpy(outdir,     argv[optind+4]);

   if(scriptdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, scriptdir);

      strcpy(scriptdir, tmpdir);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
     strcat(scriptdir, "/");


   if(strncmp(directory1, "s3://", 5) == 0)
   {
      cloud = 1;
   }
   else
   {
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
   }

   if(argc-optind > 5)
   {
      if(argc-optind < 9)
      {
         printf("[struct stat=\"ERROR\", msg=\"Usage: mHiPSPNGScript [-d][-s(ingle-threaded)][-h(ave-images)][-b brightness][-c contrast][-t color-table] scriptdir tiledir histfile [tiledir2 histfile2 tiledir3 histfile3] imglist pngdir\"]\n");

         fflush(stdout);
         exit(1);
      }

      strcpy(directory2, argv[optind+3]);
      strcpy(histfile2,  argv[optind+4]);
      strcpy(directory3, argv[optind+5]);
      strcpy(histfile3,  argv[optind+6]);
      strcpy(imglist,    argv[optind+7]);
      strcpy(outdir,     argv[optind+8]);

      if(!cloud)
      {
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
      }

      color = 1;
   }

   if(debug)
   {
      printf("DEBUG> cloud         = %d\n",   cloud);
      printf("DEBUG> single_thread = %d\n",   single_threaded);
      printf("DEBUG> color         = %d\n",   color);
      printf("DEBUG> brightness    = %-g\n",  brightness);
      printf("DEBUG> contrast      = %-g\n",  contrast);
      printf("DEBUG> directory1    = [%s]\n", directory1);
      printf("DEBUG> histfile1     = [%s]\n", histfile1);
      printf("DEBUG> directory2    = [%s]\n", directory2);
      printf("DEBUG> histfile2     = [%s]\n", histfile2);
      printf("DEBUG> directory3    = [%s]\n", directory3);
      printf("DEBUG> histfile3     = [%s]\n", histfile3);
      printf("DEBUG> imglist       = [%s]\n", imglist);
      printf("DEBUG> outdir        = [%s]\n", outdir);
      fflush(stdout);
   }

   if(!cloud)
   {
      /*******************************/
      /* Open the driver script file */
      /*******************************/

      sprintf(driverfile, "%spngSubmit.sh", scriptdir);

      if(debug)
      {
         printf("\nDEBUG> Creating driverfile = [%s]\n", driverfile);
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

      if(!single_threaded)
      {
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
      }
   }


   /********************************/
   /* Determine the tile file list */
   /********************************/

   ntile = 0;

   if(!haveimages)
   {
      if(cloud)
         mHiPSPNGScripts_S3Files(directory1);
      else
      {
         fimages = fopen(imglist, "w+");

         if(fimages == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open output image list.\"]\n");
            fflush(stdout);
            exit(0);
         }

         offset = strlen(directory1);

         mHiPSPNGScripts_localFiles(directory1);

         fclose(fimages);
      }
   }

   // Count tiles

   sprintf(cmd, "wc %s > %s.count", imglist, imglist);

   if(debug)
   {
      printf("COMMAND> [%s]\n", cmd);
      fflush(stdout);
   }

   system(cmd);


   sprintf(file, "%s.count", imglist);

   fcount = fopen(file, "r");

   fscanf(fcount, "%d", &ntile);

   if(debug)
   {
      printf("DEBUG>   ntile = %d\n", ntile);
      fflush(stdout);
   }

   fclose(fcount);



   njob = ntile;

   while(njob > 1000)
      njob = (int)(njob / 100.); 

   ++njob;

   ntask = ntile / njob;

   while(ntask * njob > ntile)
      ++ntask;

   if(debug)
   {
      printf("DEBUG> ntile         = %d\n", ntile);
      printf("DEBUG> njob          = %d\n", njob);
      printf("DEBUG> ntask         = %d\n", ntask);
      fflush(stdout);
   }

   /***********************************************/
   /* Process the directories, making the scripts */
   /***********************************************/

   mHiPSPNGScripts_processFiles(imglist);

   if(single_threaded)
   {
      fclose(fdriver);
      chmod(driverfile, 0774);
   }

   else if(!cloud)  // i.e., SLURM
   {
      fprintf(fdriver, "sbatch --array=1-%d%%20 --mem=8192 --mincpus=1 %spngTask.bash\n", njob, scriptdir);
      fflush(fdriver);

      fclose(fdriver);
      chmod(driverfile, 0774);
   }

   printf("[struct stat=\"OK\", module=\"mHiPSPNGScripts\", ntile=%d, njob=%d, njobtasks==%d]\n", ntile, njob, ntask);
   fflush(stdout);
   exit(0);
}



/************************************************/
/*                                              */
/*  Get a list of local files (local file mode) */
/*                                              */
/************************************************/

int mHiPSPNGScripts_localFiles(char *pathname)
{
   int             len, i;
   char            dirname [MAXSTR];
   char            filename[MAXSTR];
   char            pngdir  [MAXSTR];
   char           *ptr;
   DIR            *dp;
   struct dirent  *entry;
   struct stat     type;

   if(debug)
   {
      printf("\nDEBUG> Entering [%s]\n", pathname);
      fflush(stdout);
   }

   dp = opendir(pathname);

   if (dp == NULL) 
      return 0;

   while ((entry=(struct dirent *)readdir(dp)) != (struct dirent *)0) 
   {
      if(pathname[strlen(pathname)-1] == '/')
         sprintf (dirname, "%s%s", pathname, entry->d_name);
      else
         sprintf (dirname, "%s/%s", pathname, entry->d_name);

      if(debug)
      {
         printf("DEBUG> Checking [%s]\n", dirname);
         fflush(stdout);
      }

      if (stat(dirname, &type) == 0) 
      {
         if (S_ISDIR(type.st_mode) == 1)
         {
            if(strcmp (entry->d_name, "." ) != 0
            && strcmp (entry->d_name, "..") != 0)

               mHiPSPNGScripts_localFiles(dirname);
         }
         else if( strlen(entry->d_name) > 5
              && strncmp(entry->d_name + strlen(entry->d_name) - 5, ".fits", 5) == 0)
         {
            if(debug)
            {
               printf("%s\n", dirname+offset); 
               fflush(stdout);
            }

            fprintf(fimages, "%s\n", dirname+offset); 
            fflush(fimages);
         }
      }
   }

   closedir(dp);

   return 0;
}



/****************************/
/*                          */
/*  Get a list of S3 files  */
/*                          */
/****************************/

int mHiPSPNGScripts_S3Files(char *bucket)
{
   int   i;

   char  cmd [1024];
   char  file[1024];
   char  raw [1024];
   char  list[1024];
   char  line[1024];
   char *ptr;

   FILE *fraw;
   FILE *flist;


   // Get S3 list
   
   sprintf(cmd, "aws s3 ls %s --recursive > %s.raw", bucket, imglist);

   if(debug)
   {
      printf("COMMAND> [%s]\n", cmd);
      fflush(stdout);
   }

   system(cmd);


   // Clean up list

   sprintf(raw, "%s.raw", imglist);

   fraw = fopen(raw, "r");

   flist = fopen(imglist, "w+");

   while(1)
   {
      if(fgets(line, 1024, fraw) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';

      if(debug)
         printf("DEBUG>   line: [%s]", line);

      ptr = line;

      for(i=0; i<3; ++i)
      {
         while(*ptr != ' ') ++ptr;
         while(*ptr == ' ') ++ptr;
      }

      if(debug)
      {
         printf("  -->  [%s]\n", ptr);
         fflush(stdout);
      }

      fprintf(flist, "/%s\n", ptr);
      fflush(flist);
   }

   fclose(fraw);
   fclose(flist);

   return 0;
}


/****************************/
/*                          */
/*  Make processing scripts */
/*                          */
/****************************/

int mHiPSPNGScripts_processFiles(char *pathname)
{
   int   i, task;
   char  origfile  [1024];
   char  filename  [1024];
   char  relative  [1024];
   char  filepath1 [1024];
   char  filepath2 [1024];
   char  filepath3 [1024];
   char  filename1 [1024];
   char  filename2 [1024];
   char  filename3 [1024];
   char  filebase1 [1024];
   char  filebase2 [1024];
   char  filebase3 [1024];
   char  transfile1[1024];
   char  transfile2[1024];
   char  transfile3[1024];
   char  pngfile   [1024];
   char  pngpath   [1024];
   char  cmd       [1024];

   char *ptr;

   FILE *flist;


   strcpy(newdir, "");
   strcpy(olddir, "");


   // Initialize the njob script file
   
   for(job=1; job<=njob; ++job)
   {
      if(cloud)
         sprintf(jobfile, "%s/pngs_%d.sh", scriptdir, job);
      else
      {
         sprintf(jobfile, "%s/jobs/pngs_%d.sh", scriptdir, job);

         fprintf(fdriver, "%s\n", jobfile);
         fflush(fdriver);
      }

      fjob = fopen(jobfile, "w+");

      fprintf(fjob, "#!/bin/sh\n\n");
      fflush(fjob);

      if(cloud)
      {
	 sprintf(cmd, "aws s3 cp %s histfile1", histfile1);

	 if(debug)
	 {
	    printf("DEBUG> COMMAND: [%s]\n", cmd);
	    fflush(stdout);
	 }

         fprintf(fjob, "%s\n", cmd);
         fflush(fjob);

         if(color)
         {
            sprintf(cmd, "aws s3 cp %s histfile2", histfile2);

            if(debug)
            {
               printf("DEBUG> COMMAND: [%s]\n", cmd);
               fflush(stdout);
            }

            fprintf(fjob, "%s\n", cmd);
            fflush(fjob);


            sprintf(cmd, "aws s3 cp %s histfile3", histfile2);

            if(debug)
            {
               printf("DEBUG> COMMAND: [%s]\n", cmd);
               fflush(stdout);
            }

            fprintf(fjob, "%s\n", cmd);
            fflush(fjob);
         }
         
         fprintf(fjob, "\n\n");
      }

      fflush(fjob);
      fclose(fjob);

      chmod(jobfile, 0774);
   }


   // Loop through the tile list

   flist = fopen(pathname, "r");

   task = 0;

   while(1)
   {
      job = task / ntask + 1;

      if(cloud)
         sprintf(jobfile, "%s/pngs_%d.sh", scriptdir, job);
      else
         sprintf(jobfile, "%s/jobs/pngs_%d.sh", scriptdir, job);

      fjob = fopen(jobfile, "a");


      if(fgets(filename, 1024, flist) == (char *)NULL)
         break;

      if(filename[strlen(filename)-1] == '\n')
         filename[strlen(filename)-1]  = '\0';

      strcpy(relative, filename);

      ptr = relative + strlen(relative) - 1;;

      while(*ptr != '/')
         --ptr;

      *ptr = '\0';

      sprintf(filepath1, "%s%s", directory1, filename);
      sprintf(filepath2, "%s%s", directory2, filename);
      sprintf(filepath3, "%s%s", directory3, filename);

      sprintf(filename1, "%s1", filename);
      sprintf(filename2, "%s2", filename);
      sprintf(filename3, "%s3", filename);

      ptr = filename + strlen(filename) - 1;

      while(*ptr != '/') 
         --ptr;

      ++ptr;

      strcpy(origfile, ptr);

      strcpy(filebase1, ptr);
      strcpy(filebase2, ptr);
      strcpy(filebase3, ptr);

      strcat(filebase1, "1");
      strcat(filebase2, "2");
      strcat(filebase3, "3");

      sprintf(transfile1, "%s_trans", filebase1);
      sprintf(transfile2, "%s_trans", filebase2);
      sprintf(transfile3, "%s_trans", filebase3);

      strcpy(pngfile, origfile);

      ptr = strstr(pngfile, ".fits");

      *ptr = '\0';

      strcat(pngfile, ".png");


      sprintf(newdir, "%s%s", outdir, relative);

      if(!cloud)
      {
         if(strcmp(newdir, olddir) != 0)
         {
            sprintf(cmd, "mkdir -p %s\n", newdir);
            
            if(debug)
            {
               printf("DEBUG> COMMAND: [%s]\n", cmd);
               fflush(stdout);
            }

            fprintf(fjob, "%s\n", cmd);
            fflush(fjob);

            strcpy(olddir, newdir);
         }
      }

      sprintf(pngpath, "%s%s/%s", outdir, relative, origfile);

      ptr = strstr(pngpath, ".fits");

      *ptr = '\0';

      strcat(pngpath, ".png");

      if(!cloud)
         strcpy(pngfile, pngpath);


      // Run mTranspose and mViewer

      if(color)
      {
         if(cloud)
         {
            sprintf(cmd, "aws s3 cp %s %s", filepath1, filebase1);

            if(debug)
            {
               printf("DEBUG> COMMAND: [%s]\n", cmd);
               fflush(stdout);
            }

            fprintf(fjob, "%s\n", cmd);
            fflush(fjob);

            sprintf(cmd, "aws s3 cp %s %s", filepath2, filebase2);

            if(debug)
            {
               printf("DEBUG> COMMAND: [%s]\n", cmd);
               fflush(stdout);
            }

            fprintf(fjob, "%s\n", cmd);
            fflush(fjob);

            sprintf(cmd, "aws s3 cp %s %s", filepath3, filebase3);

            if(debug)
            {
               printf("DEBUG> COMMAND: [%s]\n", cmd);
               fflush(stdout);
            }

            fprintf(fjob, "%s\n\n", cmd);
            fflush(fjob);
         }
         else
         {
            strcpy(filebase1, filepath1);
            strcpy(filebase2, filepath2);
            strcpy(filebase3, filepath3);
         }

         sprintf(cmd, "mTranspose %s %s 2 1", filebase1, transfile1);

         if(debug)
         {
            printf("DEBUG> COMMAND: [%s]\n", cmd);
            fflush(stdout);
         }

         fprintf(fjob, "%s\n", cmd);
         fflush(fjob);

         sprintf(cmd, "mTranspose %s %s 2 1", filebase2, transfile2);

         if(debug)
         {
            printf("DEBUG> COMMAND: [%s]\n", cmd);
            fflush(stdout);
         }

         fprintf(fjob, "%s\n", cmd);
         fflush(fjob);

         sprintf(cmd, "mTranspose %s %s 2 1", filebase3, transfile3);

         if(debug)
         {
            printf("DEBUG> COMMAND: [%s]\n", cmd);
            fflush(stdout);
         }

         fprintf(fjob, "%s\n\n", cmd);
         fflush(fjob);

         if(cloud)
            sprintf(cmd, "mViewer -brightness %-g -contrast %-g \\\n        -blue  %s -histfile histfile1 \\\n        -green %s -histfile histfile2 \\\n        -red   %s -histfile histfile3 \\\n        -out   %s",
               brightness, contrast, transfile1, transfile2, transfile3, pngfile);
         else
            sprintf(cmd, "mViewer -brightness %-g -contrast %-g \\\n        -blue  %s -histfile %s \\\n        -green %s -histfile %s \\\n        -red   %s -histfile %s \\\n        -out   %s",
               brightness, contrast, transfile1, histfile1, transfile2, histfile2, transfile3, histfile3, pngfile);

         if(debug)
         {
            printf("DEBUG> COMMAND: [%s]\n", cmd);
            fflush(stdout);
         }

         fprintf(fjob, "%s\n\n", cmd);
         fflush(fjob);

         if(cloud)
            fprintf(fjob, "rm %s %s %s\n", filebase1, filebase2, filebase3);

         fprintf(fjob, "rm %s %s %s\n\n", transfile1, transfile2, transfile3);
      }

      else
      {
         if(cloud)
         {
            sprintf(cmd, "aws s3 cp %s %s", filepath1, filebase1);

            if(debug)
            {
               printf("DEBUG> COMMAND: [%s]\n", cmd);
               fflush(stdout);
            }

            fprintf(fjob, "%s\n\n", cmd);
            fflush(fjob);
         }
         else
            strcpy(filebase1, filepath1);

         sprintf(cmd, "mTranspose %s %s 2 1", filebase1, transfile1);

         if(debug)
         {
            printf("DEBUG> COMMAND: [%s]\n\n", cmd);
            fflush(stdout);
         }

         fprintf(fjob, "%s\n", cmd);
         fflush(fjob);

         if(cloud)
            sprintf(cmd, "mViewer -ct %d -brightness %-g -contrast %-g \\\n -gray %s -histfile histfile1 \\\n -png %s",
               ct, brightness, contrast, transfile1, pngfile);
         else
            sprintf(cmd, "mViewer -ct %d -brightness %-g -contrast %-g \\\n -gray %s -histfile %s \\\n -png %s",
            ct, brightness, contrast, transfile1, histfile1, pngfile);

         if(debug)
         {
            printf("DEBUG> COMMAND: [%s]\n", cmd);
            fflush(stdout);
         }

         fprintf(fjob, "%s\n\n", cmd);
         fflush(fjob);

         if(cloud)
            fprintf(fjob, "rm %s\n", filebase1);

         fprintf(fjob, "rm %s\n\n", transfile1);
      }


      if(cloud)
      {
         sprintf(cmd, "aws s3 cp %s %s", pngfile, pngpath);

         if(debug)
         {
            printf("DEBUG> COMMAND: [%s]\n", cmd);
            fflush(stdout);
         }

         fprintf(fjob, "%s\n\n", cmd);
         fflush(fjob);

         fprintf(fjob, "rm %s\n\n\n", pngfile);
      }

      ++task;

      fclose(fjob);
   }

   return 0;
}
