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
/*  mHPXBgScripts                                         */
/*                                                        */
/*  Create scripts that take the background correction    */
/*  determined for a set of images and subtract it using  */
/*  mBackground.                                          */
/*                                                        */
/*  Use the plate list determined originally for the      */
/*  mosaic building the once again normalize the I/O      */
/*  load.                                                 */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  i, ch, istat, ncols, order, njob, ijob;
   int  count, nocorrection, failed, noAreas;
   int  found, nskip, nset, iset, irec, ndone;

   char cwd       [MAXSTR];
   char platelist [MAXSTR];
   char corrfile  [MAXSTR];
   char template  [MAXSTR];
   char mosaicdir [MAXSTR];
   char platedir  [MAXSTR];
   char scriptdir [MAXSTR];
   char tmpstr    [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];

   char cmd       [MAXSTR];
   char msg       [MAXSTR];

   char *ptr;

   struct stat type;

   FILE *fscript;
   FILE *fdriver;

   int    iid;
   int    ifname;

   int    id;
   char   file [MAXSTR];

   int    icntr;
   int    ia;
   int    ib;
   int    ic;

   int    cntr, maxcntr;
   double *a;
   double *b;
   double *c;
   int    *have;

   char  **fnames;


   getcwd(cwd, MAXSTR);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug   = 0;
   noAreas = 0;

   opterr = 0;

   while ((ch = getopt(argc, argv, "nd")) != EOF) 
   {
        switch (ch) 
        {
           case 'd':
                debug = 1;
                break;

           case 'n':
                noAreas = 1;
                break;

           default:
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-n(o-areas)] order scriptdir mosaicdir platedir images.tbl corrections.tbl [njob]\"]\n", argv[0]);
                exit(1);
                break;
        }
   }

   if (argc - optind < 6)
   {
        printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-n(o-areas)] order scriptdir mosaicdir platedir images.tbl corrections.tbl [njob]\"]\n", argv[0]);
        exit(1);
   }

   order = atoi(argv[optind]);

   strcpy(scriptdir, argv[optind + 1]);
   strcpy(mosaicdir, argv[optind + 2]);
   strcpy(platedir,  argv[optind + 3]);
   strcpy(platelist, argv[optind + 4]);
   strcpy(corrfile,  argv[optind + 5]);

   njob = 0;
   if(argc - optind > 6)
      njob = atoi(argv[optind + 6]);

   if(njob <= 0)
      njob = 1;

   if(scriptdir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, scriptdir);

      strcpy(scriptdir, tmpstr);
   }

   if(mosaicdir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, mosaicdir);

      strcpy(mosaicdir, tmpstr);
   }

   if(platedir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, platedir);

      strcpy(platedir, tmpstr);
   }

   if(platelist[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, platelist);

      strcpy(platelist, tmpstr);
   }

   if(corrfile[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, corrfile);

      strcpy(corrfile, tmpstr);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(mosaicdir[strlen(mosaicdir)-1] != '/')
      strcat(mosaicdir, "/");

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");




   /*********************************/ 
   /* Open the image metadata file  */
   /* The first time is really just */
   /* to get max id value.          */
   /*********************************/ 

   ncols = topen(platelist);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid image metadata file: %s\"]\n", platelist);
      fflush(stdout);
      exit(0);
   }

   icntr  = tcol( "cntr");
   ifname = tcol( "fname");

   if(debug)
   {
      printf("\nImage metdata table\n");
      printf("icntr   = %d\n", icntr);
      printf("ifname  = %d\n", ifname);
      fflush(stdout);
   }

   if(icntr  < 0
   || ifname < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns: cntr and fname in image list.\"]\n");
      fflush(stdout);
      exit(0);
   }

   maxcntr = 0;
   count   = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      cntr = atoi(tval(icntr));

      if(cntr > maxcntr)
         maxcntr = cntr;

      ++count;
   }

   tclose(); 

   ++maxcntr;
   ++count;

   nset = count / njob;


   /**************************************/ 
   /* Allocate space for the corrections */
   /**************************************/ 

   if(debug)
   {
      printf("mallocking %d\n", maxcntr);
      fflush(stdout);
   }

   a = (double *)malloc(maxcntr * sizeof(double));
   b = (double *)malloc(maxcntr * sizeof(double));
   c = (double *)malloc(maxcntr * sizeof(double));

   have = (int *)malloc(maxcntr * sizeof(int));

   for(i=0; i<maxcntr; ++i)
   {
      a[i]    = 0.;
      b[i]    = 0.;
      c[i]    = 0.;

      have[i] = 0;
   }


   /*************************/
   /* And for the file info */
   /*************************/

   fnames = (char **)malloc(maxcntr * sizeof(char *));

   for(i=0; i<maxcntr; ++i)
   {
      fnames[i] = (char *)malloc(128 * sizeof(char));

      strcpy(fnames[i], "--------------------------------------------------------");
   }


   /******************************************/ 
   /* Open the corrections table file        */
   /******************************************/ 

   ncols = topen(corrfile);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid corrections file: %s\"]\n", corrfile);
      fflush(stdout);
      exit(0);
   }

   iid = tcol( "id");
   ia  = tcol( "a");
   ib  = tcol( "b");
   ic  = tcol( "c");

   if(debug)
   {
      printf("\nCorrections table\n");
      printf("iid = %d\n", iid);
      printf("ia  = %d\n", ia);
      printf("ib  = %d\n", ib);
      printf("ic  = %d\n", ic);
      printf("\n");
      fflush(stdout);
   }

   if(iid < 0
   || ia  < 0
   || ib  < 0
   || ic  < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns: id,a,b,c in corrections file\"]\n");
      fflush(stdout);
      exit(0);
   }


   /********************************/
   /* And read in the corrections. */
   /********************************/

   while(1)
   {
      if(tread() < 0)
         break;

      id = atoi(tval(iid));

      if(id >= maxcntr)
      {
         printf("[struct stat=\"ERROR\", msg=\"Correction ID out of range (%d vs. %d)\"]\n",
            id, maxcntr-1);
         fflush(stdout);
         exit(0);
      }

      a[id] = atof(tval(ia));
      b[id] = atof(tval(ib));
      c[id] = atof(tval(ic));

      have[id] = 1;
   }

   tclose();

   if(debug)
   {
      printf("Corrections read\n");
      fflush(stdout);
   }


   /**************************/
   /* Reread the image list  */
   /**************************/

   ncols = topen(platelist);

   while(1)
   {
      if(tread() < 0)
         break;

      cntr = atoi(tval(icntr));

      strcpy(fnames[cntr], tval(ifname));
   }

   if(debug)
   {
      printf("Image names read\n");
      fflush(stdout);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%srunBgCorrections.sh", scriptdir);

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
   /* Iterate over the files. Collect sets of         */
   /* mBackground commands together in scripts.       */
   /*                                                 */
   /* If there is no correction for an image file,    */
   /* increment 'nocorrection' and copy it unchanged. */
   /* Then run mBackground to create the corrected    */
   /* image.  If there is an image in the list for    */
   /* which we don't actually have a projected file   */
   /* (can happen if the list was created from the    */
   /* 'raw' set), increment the 'nocorrection' count. */
   /***************************************************/ 

   count        = 0;
   nocorrection = 0;

   iset = 0;
   ijob = 0;

   for(irec=0; irec<maxcntr; ++irec)
   {
      if(debug)
      {
         printf("Record %5d (%d:%d): %s -> Plane %-g %-g %-g (have: %d)\n", irec, ijob, iset, fnames[irec], a[irec], b[irec], c[irec], have[irec]);
         fflush(stdout);
      }

      if(fnames[irec][0] == '-')
         continue;

      if(iset == 0)
      {
         sprintf(scriptfile, "%sjobs/background_%04d.sh", scriptdir, ijob);
         
         fscript = fopen(scriptfile, "w+");

         if(fscript == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s].\"]\n", scriptfile);
            fflush(stdout);
            exit(0);
         }

         fprintf(fscript, "#!/bin/sh\n\n");

         fprintf(fscript, "echo jobs/background_%04d.sh\n", ijob);
      }

      if(!have[irec])
      {
         ++nocorrection;
         fprintf(fscript, "cp $1%s $2%s\n", fnames[irec], fnames[irec]);
      }
      else
      {
         if(noAreas) 
            fprintf(fscript, "mBackground -n $1%s $2order%d/%s %-g %-g %-g\n", fnames[irec], order, fnames[irec], a[irec], b[irec], c[irec]);
         else
            fprintf(fscript, "mBackground $1%s $2order%d/%s %-g %-g %-g\n", fnames[irec], order, fnames[irec], a[irec], b[irec], c[irec]);
      }
 
      if(iset == nset-1)
      {
         fflush(fscript);
         fclose(fscript);

         chmod(scriptfile, 0777);

         fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 %ssubmitBackground.bash %sjobs/background_%04d.sh %s %s\n", 
            scriptdir, scriptdir, ijob, mosaicdir, platedir);
         fflush(fdriver);

         ++ijob;

         iset = 0;
      }
      else
         ++iset;

      ++count;
   }


   // The last set may not have been closed yet
   
   if(iset < nset-1)
   {
      fflush(fscript);
      fclose(fscript);

      chmod(scriptfile, 0777);

      fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 %ssubmitBackground.bash %sjobs/background_%04d.sh %s %s\n", 
         scriptdir, scriptdir, ijob, mosaicdir, platedir);
      fflush(fdriver);
   }

   chmod(driverfile, 0777);

   if(debug)
   {
      printf("\nFreeing a,b,c,have arrays\n");
      fflush(stdout);
   }

   free(a);
   free(b);
   free(c);
   free(have);

   for(i=0; i<maxcntr; ++i)
      free(fnames[i]);

   free(fnames);


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", count=%d, nocorrection=%d]\n", count, nocorrection);
   fflush(stdout);
   exit(0);
}
