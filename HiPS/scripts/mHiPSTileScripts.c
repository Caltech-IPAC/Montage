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
/*  mHiPSTileScripts                                      */
/*                                                        */
/*  Create scripts that split the plates in the image     */
/*  list into their component HiPS tiles (still FITS      */
/*  image).                                               */
/*                                                        */
/*  Use the plate list determined originally for the      */
/*  mosaic building to once again normalize the I/O       */
/*  load (somewhat since only the iput images are         */
/*  distributed across storage devices; the output        */
/*  is put in a single tree).                             */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  i, ch, istat, ncols, iorder;
   int  count, order, single_threaded;

   char cwd       [MAXSTR];
   char imgtbl    [MAXSTR];
   char platedir  [MAXSTR];
   char hipsdir   [MAXSTR];
   char scriptdir [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];
   char tmpdir    [MAXSTR];
   char file      [MAXSTR];

   char *ptr;

   FILE *fscript;
   FILE *fdriver;

   int    iid;
   int    ifname;

   int    id;

   getcwd(cwd, MAXSTR);


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;
   order = -1;
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
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle_threaded)] order scriptdir platedir hipsdir images.tbl\"]\n", argv[0]);
                exit(1);
                break;
        }
   }

   if (argc - optind < 5)
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s(ingle_threaded)] order scriptdir platedir hipsdir images.tbl\"]\n", argv[0]);
      exit(1);
   }


   order = atoi(argv[optind]);

   strcpy(scriptdir, argv[optind + 1]);
   strcpy(platedir,  argv[optind + 2]);
   strcpy(hipsdir,   argv[optind + 3]);
   strcpy(imgtbl,    argv[optind + 4]);

   if(scriptdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, scriptdir);

      strcpy(scriptdir, tmpdir);
   }

   if(platedir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, platedir);

      strcpy(platedir, tmpdir);
   }

   if(hipsdir[0] != '/')
   {
      strcpy(tmpdir, cwd);
      strcat(tmpdir, "/");
      strcat(tmpdir, hipsdir);

      strcpy(hipsdir, tmpdir);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(hipsdir[strlen(hipsdir)-1] != '/')
      strcat(hipsdir, "/");

   if(debug)
   {
      printf("DEBUG> scriptdir:  [%s]\n", scriptdir);
      printf("DEBUG> platedir:   [%s]\n", platedir);
      printf("DEBUG> hipsdir:    [%s]\n", hipsdir);
      printf("DEBUG> imgtbl:     [%s]\n", imgtbl);
      fflush(stdout);
   }

   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%srunHiPSTiles.sh", scriptdir);

   fdriver = fopen(driverfile, "w+");

   if(fdriver == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output driver script file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fdriver, "#!/bin/sh\n\n");
   fflush(fdriver);


   /*******************************/
   /* Read through the image list */
   /*******************************/

   ncols = topen(imgtbl);

   ifname = tcol( "fname");

   if(debug)
   {
      printf("\nDEBUG> Image metdata table\n");
      printf("DEBUG> ifname = %d\n", ifname);
      fflush(stdout);
   }

   if(ifname < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need column fname in image list.\"]\n");
      fflush(stdout);
      exit(0);
   }

   count = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      strcpy(file, tval(ifname));

      if(debug)
      {
         printf("DEBUG> Record %5d: %s\n", count, file);
         fflush(stdout);
      }

      sprintf(scriptfile, "%sjobs/tiles%03d.sh", scriptdir, count);
      
      fscript = fopen(scriptfile, "w+");

     if(fscript == (FILE *)NULL)
     {
        printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s] for %s.\"]\n", scriptfile, file);
        fflush(stdout);
        exit(0);
     }

     fprintf(fscript, "#!/bin/sh\n\n");

     fprintf(fscript, "echo jobs/tiles%03d.sh\n\n", count);

     if(single_threaded)
     {
        for(iorder=order; iorder>=0; --iorder)
           fprintf(fscript, "mHiPSTiles $1order%d/%s $2\n", iorder, file);
     }
     else
     {
        for(iorder=order; iorder>=0; --iorder)
           fprintf(fscript, "mHiPSTiles $1order%d/%s $2\n", iorder, file);
     }

     fflush(fscript);
     fclose(fscript);

     chmod(scriptfile, 0777);

     if(single_threaded)
        fprintf(fdriver, "%sjobs/tiles%03d.sh %s %s\n", scriptdir, count, platedir, hipsdir, platedir);
     else
        fprintf(fdriver, "sbatch submitHiPSTiles.bash %sjobs/tiles%03d.sh %s %s\n", 
           scriptdir, count, platedir, hipsdir, platedir);
     fflush(fdriver);

      ++count;
   }

   fclose(fdriver);

   chmod(driverfile, 0777);


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", count=%d]\n", count);
   fflush(stdout);
   exit(0);
}
