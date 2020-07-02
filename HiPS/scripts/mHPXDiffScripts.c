#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mtbl.h>

int NFILE   = 1024;
int MAXFILE = 1024;

int STRLEN  = 1024;

/***************************************************************************/
/*                                                                         */
/*  Split a diffs table up unto subsets.  We will run mDiffFitExec on      */
/*  each one and then concatenate the fits.tbl files that result.          */
/*                                                                         */
/***************************************************************************/

int main(int argc, char **argv)
{
   int  i, level, ncore, nsubset, count, ncols, set, nread;
   int  j, xbase, ybase, xref, yref, namelen, nmatches;

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


   // Command-line arguments

   if(argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXDiffScripts [-d] scriptdir mosaicdir images.tbl order ncore\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(strcmp(argv[1], "-d") == 0)
   {
      debug = 1;
      ++argv;
      --argc;
   }

   if(argc < 6)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXDiffScripts [-d] scriptdir mosaicdir images.tbl order ncore\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(scriptdir, argv[1]);
   strcpy(mosaicdir, argv[2]);
   strcpy(imgfile,   argv[3]);

   level = atoi(argv[4]);
   ncore = atoi(argv[5]);

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(mosaicdir[strlen(mosaicdir)-1] != '/')
      strcat(mosaicdir, "/");


   // The plates follow a regular pattern so we can define
   // the differences that exist algorithmically.  Each tile
   // can have up to eight neighbors, though we need to check
   // for file existence in each case.
   
   // First read in the list of files we actually have.

   ncols = topen(imgfile);

   icntr = tcol("cntr");
   ifile = tcol("fname");

   if(icntr < 0 || ifile < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need 'cntr' and 'fname' columns in image metadata file.\n");
      fflush(stdout);
      exit(0);
   }

   cntr   = (int   *)malloc(MAXFILE * sizeof(int));
   xindex = (int   *)malloc(MAXFILE * sizeof(int));
   yindex = (int   *)malloc(MAXFILE * sizeof(int));
   fname  = (char **)malloc(MAXFILE * sizeof(char *));

   for(i=0; i<MAXFILE; ++i)
      fname[i] = (char *)malloc(STRLEN * sizeof(char));

   nfile   = 0;
   namelen = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      cntr[nfile] = atoi(tval(icntr));

      strcpy(fname[nfile], tval(ifile));

      if(strlen(fname[nfile]) > namelen)
         namelen = strlen(fname[nfile]);

      ptr = fname[nfile] + strlen(fname[nfile]);

      while(*ptr != '_' && ptr > fname[nfile])
         --ptr;

      if(ptr > fname[nfile])
         --ptr;

      while(*ptr != '_' && ptr > fname[nfile])
         --ptr;

      ++ptr;

      xindex[nfile] = atoi(ptr);

      while(*ptr != '_' && ptr < fname[nfile] + strlen(fname[nfile]))
         ++ptr;

      ++ptr;

      yindex[nfile] = atoi(ptr);

      ++nfile;

      if(nfile >= MAXFILE)
      {
         MAXFILE += NFILE;

         cntr   = (int   *)realloc(cntr,   MAXFILE * sizeof(int));
         xindex = (int   *)realloc(xindex, MAXFILE * sizeof(int));
         yindex = (int   *)realloc(yindex, MAXFILE * sizeof(int));
         fname  = (char **)realloc(fname,  MAXFILE * sizeof(char *));

         for(i=MAXFILE-NFILE; i<MAXFILE; ++i)
            fname[i] = (char *)malloc(STRLEN * sizeof(char));
      }
   }


   // Now we check for overlaps with each image.  We only
   // need to check above right and above-right. All others
   // will get found as we work our way through the other
   // images.
   
   sprintf(difffile, "%s/diffs.tbl", mosaicdir);

   fout = fopen(difffile, "w+");

   if(fout == (FILE *)NULL)
   {
      for(i=0; i<nfile; ++i)
         free(fname[i]);

      free(fname);
      free(xindex);
      free(yindex);
      free(cntr);

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

   nmatches = 0;

   for(i=0; i<nfile; ++i)
   {
      xbase = xindex[i];
      ybase = yindex[i];

      for(j=0; j<nfile; ++j)
      {
         xref = xindex[j];
         yref = yindex[j];

         if((xref == xbase   && yref == ybase+1)
         || (xref == xbase+1 && yref == ybase+1)
         || (xref == xbase+1 && yref == ybase))
         {
            ++nmatches;

            fprintf(fout, fmt, cntr[i], cntr[j], fname[i], fname[j], cntr[i], cntr[j]);
            fflush(fout);
         }
      }
   }

   fclose(fout);

   nsubset = nmatches / ncore;

   if(ncore * nsubset < nmatches)
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
      for(i=0; i<nfile; ++i)
         free(fname[i]);

      free(fname);
      free(xindex);
      free(yindex);
      free(cntr);

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
      for(i=0; i<nfile; ++i)
         free(fname[i]);

      free(fname);
      free(xindex);
      free(yindex);
      free(cntr);

      printf("[struct stat=\"ERROR\", msg=\"Cannot open fit combo script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fcombo, "#!/bin/sh\n\n");
   fflush(fcombo);


   // Create the scripts. We double check that the plate intersects with
   // at least a part of the sky
   
   count = 0;

   ncols = topen(difffile);

   if(ncols < 0)
   {
      for(i=0; i<nfile; ++i)
         free(fname[i]);

      free(fname);
      free(xindex);
      free(yindex);
      free(cntr);

      printf("[struct stat=\"ERROR\", msg=\"Error opening diffs.tbl file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   set = 0;

   sprintf(outfile, "%sdiffs_%d.tbl", mosaicdir, set);

   if(debug)
   {
      printf("DEBUG> output file %d: [%s]\n", set, outfile);
      fflush(stdout);
   }

   fout = fopen(outfile, "w+");

   if(fout == (FILE *)NULL)
   {
      for(i=0; i<nfile; ++i)
         free(fname[i]);

      free(fname);
      free(xindex);
      free(yindex);
      free(cntr);

      printf("[struct stat=\"ERROR\", msg=\"Error opening diffs_%d.tbl file.\"]\n", set);
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

         sprintf(outfile, "%sdiffs_%d.tbl", mosaicdir, set);

         if(debug)
         {
            printf("DEBUG> output file %d: [%s]\n", set, outfile);
            fflush(stdout);
         }

         fout = fopen(outfile, "w+");

         if(fout == (FILE *)NULL)
         {
            for(i=0; i<nfile; ++i)
               free(fname[i]);

            free(fname);
            free(xindex);
            free(yindex);
            free(cntr);

            printf("[struct stat=\"ERROR\", msg=\"Error opening diffs_%d.tbl file.\"]\n", set);
            fflush(stdout);
            exit(0);
         }

         fprintf(fout, "%s\n", tbl_hdr_string);


         // Create a script to run mDiffFitExec on the set we just created

         sprintf(scriptfile, "%sjobs/diffFit%0d.sh", scriptdir, set-1);

         if(debug)
         {
            printf("DEBUG> scriptfile: [%s]\n", scriptfile);
            fflush(stdout);
         }

         fscript = fopen(scriptfile, "w+");

         if(fscript == (FILE *)NULL)
         {
            for(i=0; i<nfile; ++i)
               free(fname[i]);

            free(fname);
            free(xindex);
            free(yindex);
            free(cntr);

            printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file for diff subset %d.\"]\n", set-1);
            fflush(stdout);
            exit(0);
         }

         fprintf(fscript, "#!/bin/sh\n\n");

         fprintf(fscript, "echo jobs/diffFit%03d.sh\n\n", set-1);

         fprintf(fscript, "mHPXHdr %d $1hpx%d_%d.hdr\n", level+9, level+9, set-1);

         fprintf(fscript, "mDiffFitExec -d -n -p $1 $1diffs_%d.tbl $1hpx%d_%d.hdr $1diffs $1fits%d.tbl\n", 
            set-1, level+9, set-1, set-1);

         fprintf(fscript, "rm -f $1hpx%d_%d.hdr\n", level+9, set-1);

         fflush(fscript);
         fclose(fscript);

         chmod(scriptfile, 0777);


         fprintf(fdriver, "sbatch submitDiffFit.bash %sjobs/diffFit_%d.sh %s\n", 
            scriptdir, set-1, mosaicdir);
         fflush(fdriver);

         if(count == 0)
            fprintf(fcombo, "cat  $1/fits%d.tbl >  $1/fitcombo.tbl\n", set-1);
         else
            fprintf(fcombo, "grep -v \"|\" $1/fits%d.tbl >> $1/fitcombo.tbl\n", set-1);

         fflush(fcombo);

         ++count;
      }
   }

   ++count;

   fclose(fdriver);
   chmod(driverfile, 0777);

   fclose(fcombo);
   chmod(combofile, 0777);

   for(i=0; i<nfile; ++i)
      free(fname[i]);

   free(fname);
   free(xindex);
   free(yindex);
   free(cntr);

   printf("[struct stat=\"OK\", module=\"mHPXDiffScripts\", nmatches=%d, ncore=%d, nsubset=%d]\n",
      nmatches, ncore, nsubset);
   fflush(stdout);
   exit(0);
} 
