#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mtbl.h>

int NFILE   = 1024;
int MAXFILE = 1024;

int STRLEN  = 1024;


struct 
{
   int n;
   int x[5000];
   int y[5000];
}
cellList;


struct 
{
   int  n;
   int  x1    [50000];
   int  y1    [50000];
   int  x2    [50000];
   int  y2    [50000];
   int  cntr1 [50000];
   int  cntr2 [50000];
   char plate1[50000][1024];
   char plate2[50000][1024];
}
cellPair;


struct
{
   int n;
   int cntr  [5000];
   char fname[5000][1024];
}
plates;


/***************************************************************************/
/*                                                                         */
/*  Split a diffs table up unto subsets.  We will run mDiffFitExec on      */
/*  each one and then concatenate the fits.tbl files that result.          */
/*                                                                         */
/***************************************************************************/

int main(int argc, char **argv)
{
   int  i, level, njob, nsubset, count, ncols, set, nread;
   int  j, xbase, ybase, xref, yref, namelen, nmatches;
   int  level_only;

   char cwd       [STRLEN];
   char tmpdir    [STRLEN];
   char scriptdir [STRLEN];
   char scriptfile[STRLEN];
   char driverfile[STRLEN];
   char combofile [STRLEN];
   char mosaicdir [STRLEN];
   char imgfile   [STRLEN];

   char difffile  [STRLEN];
   char outfile   [STRLEN];
   char fmt       [STRLEN];

   char *ptr;

   int  icntr, ifile, nfile;
   int  *cntr;
   int  *xindex;
   int  *yindex;
   char **fname;

   FILE *fscript;
   FILE *fdriver;
   FILE *fcombo;
   FILE *fout;

   int debug = 0;

   getcwd(cwd, 1024);


   // Command-line arguments

   level_only = 0;

   if(argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXDiffScripts [-l(evel-only)] scriptdir mosaicdir images.tbl order njob\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(strcmp(argv[1], "-l") == 0)
   {
      level_only = 1;
      ++argv;
      --argc;
   }

   if(argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXDiffScripts [-l(evel-only)] scriptdir mosaicdir images.tbl order njob\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(scriptdir, argv[1]);
   strcpy(mosaicdir, argv[2]);
   strcpy(imgfile,   argv[3]);

   level = atoi(argv[4]);
   njob  = atoi(argv[5]);

   if(scriptdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, scriptdir);

      strcpy(scriptdir, tmpdir);
   }

   if(mosaicdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, mosaicdir);

      strcpy(mosaicdir, tmpdir);
   }

   if(imgfile[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, imgfile);

      strcpy(imgfile, tmpdir);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(mosaicdir[strlen(mosaicdir)-1] != '/')
      strcat(mosaicdir, "/");


   // The plates follow a regular pattern so we can define
   // the differences that exist algorithmically.  
   
   ncols = topen(imgfile);

   icntr = tcol("cntr");
   ifile = tcol("fname");

   if(icntr < 0 || ifile < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need 'cntr' and 'fname' columns in image metadata file.\n");
      fflush(stdout);
      exit(0);
   }


   plates.n = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      plates.cntr[plates.n] = atoi(tval(icntr));

      strcpy(plates.fname[plates.n], tval(ifile));

      ++plates.n;
   }

   tclose();


   makeCellList();

   makeCellPairs();

   namelen = strlen(plates.fname[0]);

   for(i=0; i<cellPair.n; ++i)
   {
      printf("%5d:  plate_%02d_%02d (%4d:%s) vs. plate_%02d_%02d (%4d:%s)\n", 
            i, cellPair.x1[i], cellPair.y1[i], cellPair.cntr1[i], cellPair.plate1[i],
               cellPair.x2[i], cellPair.y2[i], cellPair.cntr2[i], cellPair.plate2[i]);
      fflush(stdout);
   }


   // Using the list of differences, we need to build a set of scripts to
   // generate them.
   
   nmatches = cellPair.n;

   sprintf(difffile, "%s/diffs.tbl", mosaicdir);

   fout = fopen(difffile, "w+");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot create diffs.tbl file (in %s).\n", mosaicdir);
      fflush(stdout);
      exit(0);
   }

   sprintf(fmt, "| cntr1 | cntr2 |%%%ds |%%%ds |         diff             |\n", namelen, namelen);
   fprintf(fout, fmt, "plus", "minus");

   sprintf(fmt, "| int   | int   |%%%ds |%%%ds |         char             |\n", namelen, namelen);
   fprintf(fout, fmt, "char", "char");

   fflush(stdout);

   sprintf(fmt, "%%8d%%8d %%%ds  %%%ds  diff.%%06d.%%06d.fits\n", namelen, namelen);

   for(i=0; i<nmatches; ++i)
   {
      fprintf(fout, fmt, cellPair.cntr1[i], cellPair.cntr2[i], cellPair.plate1[i], cellPair.plate2[i], cellPair.cntr1[i], cellPair.cntr2[i]);
      fflush(fout);
   }

   fclose(fout);

   nsubset = nmatches / njob;

   if(njob * nsubset < nmatches)
      ++nsubset;



   // Open the driver script file

   sprintf(driverfile, "%s/runDiffFit.sh", scriptdir);

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


   // Open the fit combo script file

   sprintf(combofile, "%s/runComboFit.sh", scriptdir);

   if(debug)
   {
      printf("DEBUG> combo file: [%s]\n", combofile);
      fflush(stdout);
   }

   fcombo = fopen(combofile, "w+");

   if(fcombo == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open fit combo script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fcombo, "#!/bin/sh\n\n");
   fflush(fcombo);


   // Create the scripts. 
   
   count = 0;

   ncols = topen(difffile);

   if(ncols < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Error opening diffs.tbl file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   set = 0;

   sprintf(outfile, "%sdiffs_%03d.tbl", mosaicdir, set);

   if(debug)
   {
      printf("DEBUG> output file %d: [%s]\n", set, outfile);
      fflush(stdout);
   }

   fout = fopen(outfile, "w+");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Error opening diffs_%03d.tbl file.\"]\n", set);
      fflush(stdout);
      exit(0);
   }

   fprintf(fout, "%s\n", tbl_hdr_string);
   fflush(fout);

   nread = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      fprintf(fout, "%s\n", tbl_rec_string);
      fflush(fout);

      ++nread;

      if(nread >= nsubset)
      {
         // Close this output and open the next

         nread = 0;

         ++set;

         fclose(fout);

         sprintf(outfile, "%sdiffs_%03d.tbl", mosaicdir, set);

         if(debug)
         {
            printf("DEBUG> output file %d: [%s]\n", set, outfile);
            fflush(stdout);
         }

         fout = fopen(outfile, "w+");

         if(fout == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Error opening diffs_%03d.tbl file.\"]\n", set);
            fflush(stdout);
            exit(0);
         }

         fprintf(fout, "%s\n", tbl_hdr_string);


         // Create a script to run mDiffFitExec on the set we just created

         sprintf(scriptfile, "%sjobs/diffFit_%03d.sh", scriptdir, set-1);

         if(debug)
         {
            printf("DEBUG> scriptfile: [%s]\n", scriptfile);
            fflush(stdout);
         }

         fscript = fopen(scriptfile, "w+");

         if(fscript == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file for diff subset %d.\"]\n", set-1);
            fflush(stdout);
            exit(0);
         }

         fprintf(fscript, "#!/bin/sh\n\n");

         fprintf(fscript, "echo jobs/diffFit_%03d.sh\n\n", set-1);

         fprintf(fscript, "mkdir -p $1diffs\n");

         fprintf(fscript, "mHPXHdr %d $1hpx%02d_%03d.hdr\n", level+9, level+9, set-1);

         if(level_only)
            fprintf(fscript, "mDiffFitExec -d -n -l -p $1 $1diffs_%03d.tbl $1hpx%02d_%03d.hdr $1diffs $1fits_%03d.tbl\n", 
               set-1, level+9, set-1, set-1);
         else
            fprintf(fscript, "mDiffFitExec -d -n -p $1 $1diffs_%03d.tbl $1hpx%02d_%03d.hdr $1diffs $1fits_%03d.tbl\n", 
               set-1, level+9, set-1, set-1);

         fprintf(fscript, "rm -f $1hpx%02d_%03d.hdr\n", level+9, set-1);

         fflush(fscript);
         fclose(fscript);

         chmod(scriptfile, 0777);


         fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 %ssubmitDiffFit.bash %sjobs/diffFit_%03d.sh %s\n", 
            scriptdir, scriptdir, set-1, mosaicdir);
         fflush(fdriver);

         if(count == 0)
            fprintf(fcombo, "cat  $1/fits_%03d.tbl >  $1/fitcombo.tbl\n", set-1);
         else
            fprintf(fcombo, "grep -v \"|\" $1/fits_%03d.tbl >> $1/fitcombo.tbl\n", set-1);

         fflush(fcombo);

         ++count;
      }
   }

   ++count;

   fclose(fdriver);
   chmod(driverfile, 0777);

   fclose(fcombo);
   chmod(combofile, 0777);

   printf("[struct stat=\"OK\", module=\"mHPXDiffScripts\", nmatches=%d, njob=%d, nsubset=%d]\n",
      nmatches, njob, nsubset);
   fflush(stdout);
   exit(0);
} 


int makeCellList()
{
   int i, j, imin, imax;

   cellList.n = 0;

   for(j = 0; j<80; ++j)
   {
      if     (j < 16) { imin = 00; imax=32;}
      else if(j < 32) { imin = 00; imax=48;}
      else if(j < 48) { imin = 16; imax=64;}
      else if(j < 64) { imin = 32; imax=80;}
      else if(j < 80) { imin = 48; imax=80;}

      for(i=imin; i<imax; ++i)
      {
         cellList.x[cellList.n] = i;
         cellList.y[cellList.n] = j;

         ++cellList.n;
      }
   }
}
      


int makeCellPairs()
{
   int i, i1, i2, found, found2;

   cellPair.n = 0;

   for(i1=0; i1<cellList.n; ++i1)
   {
      for(i2=0; i2<cellList.n; ++i2)
      {
         if((cellList.x[i2] == cellList.x[i1]   && cellList.y[i2] == cellList.y[i1]-1)
         || (cellList.x[i2] == cellList.x[i1]   && cellList.y[i2] == cellList.y[i1]+1)
         || (cellList.x[i2] == cellList.x[i1]-1 && cellList.y[i2] == cellList.y[i1])
         || (cellList.x[i2] == cellList.x[i1]+1 && cellList.y[i2] == cellList.y[i1]))
         {
            found = 0;

            for(i=0; i<cellPair.n; ++i)
            {
               if(cellList.x[i1] == cellPair.x1[i]
               && cellList.y[i1] == cellPair.y1[i]
               && cellList.x[i2] == cellPair.x2[i]
               && cellList.y[i2] == cellPair.y2[i])
               {
                  found = 1;
                  break;
               }

               if(cellList.x[i1] == cellPair.x2[i]
               && cellList.y[i1] == cellPair.y2[i]
               && cellList.x[i2] == cellPair.x1[i]
               && cellList.y[i2] == cellPair.y1[i])
               {
                  found = 1;
                  break;
               }
            }

            if(!found)
            {
               cellPair.x1[cellPair.n] = cellList.x[i1];
               cellPair.y1[cellPair.n] = cellList.y[i1];
               cellPair.x2[cellPair.n] = cellList.x[i2];
               cellPair.y2[cellPair.n] = cellList.y[i2];

               sprintf(cellPair.plate1[cellPair.n], "plate_%02d_%02d.fits", 
                  cellList.x[i1], cellList.y[i1]);

               sprintf(cellPair.plate2[cellPair.n], "plate_%02d_%02d.fits", 
                  cellList.x[i2], cellList.y[i2]);

               found2 = 0;

               for(i=0; i<plates.n; ++i)
               {
                  if(strcmp(cellPair.plate1[cellPair.n], plates.fname[i]) == 0)
                  {
                     cellPair.cntr1[cellPair.n] = plates.cntr[i];
                     found2 = 1;
                  }
               }

               if(!found2)
                  break;

               found2 = 0;

               for(i=0; i<plates.n; ++i)
               {
                  if(strcmp(cellPair.plate2[cellPair.n], plates.fname[i]) == 0)
                  {
                     cellPair.cntr2[cellPair.n] = plates.cntr[i];
                     found2 = 1;
                  }
               }

               if(!found2)
                  break;

               if(found2)
                  ++cellPair.n;
            }
         }
      }
   }
}
