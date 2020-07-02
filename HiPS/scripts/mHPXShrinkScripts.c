#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <mtbl.h>

#define MAXSTR  4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int debug;


/**********************************************************/
/*                                                        */
/*  mHPXShrinkScripts                                     */
/*                                                        */
/*  Create scripts that turn a set of plates for one      */
/*  HiPS order into plates for all the lower orders by    */
/*  successively shrinking by factors of two.             */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  i, ch, istat, ncols, order, iorder;
   int  count, cntr, maxcntr, nfile;
   int  subset, maxsubset, nset, found, nskip;
   int  iset, irec, ndone, single_threaded;
   int  pixlev, nsidePix;
   
   double pixscale;

   char platedir  [MAXSTR];
   char platelist [MAXSTR];
   char scriptdir [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];
   char tmpdir    [MAXSTR];
   char file      [MAXSTR];
   char cwd       [MAXSTR];

   char *ptr;

   FILE *fscript;
   FILE *fdriver;

   int    ifname;

   char  **fnames;
   int    *sets;

   int    *setrec;
   int    *done;

   getcwd(cwd, 1024);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   single_threaded = 0;

   opterr = 0;

   while ((ch = getopt(argc, argv, "ds")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 's':
            single_threaded = 1;
            break;

         default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle-threaded)] order scriptdir platedir images.tbl\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle-threaded)] order scriptdir platedir images.tbl\"]\n", argv[0]);
      exit(1);
   }

   order = atoi(argv[optind]);

   strcpy(scriptdir, argv[optind + 1]);
   strcpy(platedir,  argv[optind + 2]);
   strcpy(platelist, argv[optind + 3]);

   if(platedir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platedir);

      strcpy(platedir, tmpdir);
   }

   if(scriptdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, scriptdir);

      strcpy(scriptdir, tmpdir);
   }

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");



   /*********************************/ 
   /* Open the image metadata file  */
   /* The first time is really just */
   /* to get the count.             */
   /*********************************/ 

   ncols = topen(platelist);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid image metadata file: %s\"]\n", platelist);
      fflush(stdout);
      exit(0);
   }

   ifname = tcol("fname");

   if(debug)
   {
      printf("\nPlate list\n");
      printf("ifname  = %d\n", ifname);
      fflush(stdout);
   }

   if(ifname < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need column 'fname' in plate list.\"]\n");
      fflush(stdout);
      exit(0);
   }
   
   cntr      = 0;
   maxcntr   = 0;
   maxsubset = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      ++cntr;

      strcpy(file, tval(ifname));

      ptr = strstr(file, "subset");

      if(ptr != (char *)NULL)
      {
         ptr += 6;

         subset = atoi(ptr);

         if(subset > maxsubset)
            maxsubset = subset;
      }
   }

   tclose();

   nfile = cntr+1;

   nset = maxsubset + 1;

   setrec = (int *)malloc(nset * sizeof(int));
   done   = (int *)malloc(nset * sizeof(int));

   for(i=0; i<nset; ++i)
   {
      setrec[i] = -1;
      done  [i] =  0;
   }

   if(debug)
   {
      printf("nfile   = %d\n", nfile);
      printf("nset    = %d\n", nset);
      fflush(stdout);
   }


   /************************************/
   /* Allocate space for the file info */
   /************************************/

   fnames = (char **)malloc(nfile * sizeof(char *));
   sets   = (int   *)malloc(nfile * sizeof(int));

   for(i=0; i<nfile; ++i)
      fnames[i] = (char *)malloc(128 * sizeof(char));


   /**************************/
   /* Reread the image list  */
   /**************************/

   ncols = topen(platelist);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid image metadata file: %s\"]\n", platelist);
      fflush(stdout);
      exit(0);
   }

   cntr = -1;

   while(1)
   {
      if(tread() < 0)
         break;

      ++cntr;

      strcpy(file, tval(ifname));

      subset = 0;

      if(nset > 1)
      {
         ptr = strstr(file, "subset");

         ptr += 6;

         subset = atoi(ptr);

         if(debug)
         {
            printf("Read in plate [%s]: sets[%d] -> %d\n", file, cntr, subset);
            fflush(stdout);
         }
      }

      strcpy(fnames[cntr], file);

      sets[cntr] = subset;
   }

   if(debug)
   {
      printf("\nplate list read in.\n");
      fflush(stdout);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%srunShrink.sh", scriptdir);

   fdriver = fopen(driverfile, "w+");

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);



   /***************************************************/ 
   /* Iterate over the sets, identifying the next     */
   /* file for each one round-robin.  This will       */
   /* spread the processing over the storage units    */
   /* evenly in time.                                 */
   /***************************************************/ 

   count = 0;

   iset = -1;

   while(1)
   {
      iset = (iset + 1) % nset;

      found = 0;
      nskip = 0;

      for(irec=setrec[iset]+1; irec<nfile; ++irec)
      {
         if(sets[irec] == iset)
         {
            found = 1;
            break;
         }

         ++nskip;
      }

      if(found && strlen(fnames[irec]) > 0)
      {
         if(debug)
         {
            printf("File: [%s]  subset %d record -> %d\n", fnames[irec], iset, irec);
            fflush(stdout);
         }

         setrec[iset] = irec;

         sprintf(scriptfile, "%s/jobs/shrink%03d.sh", scriptdir, count);
         
         fscript = fopen(scriptfile, "w+");

        if(fscript == (FILE *)NULL)
        {
           printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] to shrink image %d.\"]\n",
              scriptfile, count);
           fflush(stdout);
           exit(0);
        }

        fprintf(fscript, "#!/bin/sh\n\n");

        fprintf(fscript, "echo jobs/shrink%03d.sh\n\n", count);

        for(iorder=order; iorder>=1; --iorder)
        {
           pixlev = iorder - 1 + 9;
  
           nsidePix = pow(2., (double)pixlev);
  
           pixscale  = 90.0 / nsidePix / sqrt(2.0);

           fprintf(fscript, "mShrink -p %.8f $1order%d/%s $1order%d/%s 2\n", 
              pixscale, iorder, fnames[irec], iorder-1, fnames[irec]);
        }

        fflush(fscript);
        fclose(fscript);

        chmod(scriptfile, 0777);

        if(single_threaded)
        {
           fprintf(fdriver, "%sjobs/shrink%03d.sh %s\n", scriptdir, count, platedir);
           fflush(fdriver);
        }
        else
        {
           fprintf(fdriver, "sbatch submitShrink.bash %sjobs/shrink%03d.sh %s\n", 
              scriptdir, count, platedir);
           fflush(fdriver);
        }

         ++count;
      }
      else
         done[iset] = 1;

      ndone = 0;
      for(i=0; i<nset; ++i)
         if(done[i] == 1)
            ++ndone; 

      if(ndone >= nset)
         break;
   }

   chmod(driverfile, 0777);

   free(setrec);
   free(done);
   free(sets);

   for(i=0; i<nfile; ++i)
      free(fnames[i]);

   free(fnames);


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", count=%d]\n", count);
   fflush(stdout);
   exit(0);
}
