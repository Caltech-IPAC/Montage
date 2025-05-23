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
/*  mHPXCoaddScripts                                      */
/*                                                        */
/*  Simple scripts:  Run mAdd in each of the project      */
/*  plate subdirectories.                                 */
/*                                                        */
/*  Use the plate list determined originally for the      */
/*  projection building to once again normalize the I/O   */
/*  load.                                                 */
/*                                                        */
/**********************************************************/

int main(int argc, char **argv)
{
   int  ch, noAreas, ncols, ijob, order;

   char cwd       [MAXSTR];
   char platelist [MAXSTR];
   char platedir  [MAXSTR];
   char projectdir[MAXSTR];
   char scriptdir [MAXSTR];
   char tmpstr    [MAXSTR];
   char scriptfile[MAXSTR];
   char driverfile[MAXSTR];

   FILE *fscript;
   FILE *fdriver;

   int    iid;
   int    iplate;

   int    id;
   char   plate[MAXSTR];


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
            printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-n(o-areas)] scriptdir projectdir platelist.tbl platedir order\"]\n", argv[0]);
                exit(1);
                break;
        }
   }

   if (argc - optind < 5)
   {
        printf ("[struct stat=\"ERROR\", msg=\"Usage: %s [-d] [-n(o-areas)] scriptdir projectdir platelist.tbl platedir order\"]\n", argv[0]);
        exit(1);
   }

   strcpy(scriptdir,  argv[optind]);
   strcpy(projectdir, argv[optind + 1]);
   strcpy(platelist,  argv[optind + 2]);
   strcpy(platedir,   argv[optind + 3]);

   order = atoi(argv[optind + 4]);

   if(scriptdir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, scriptdir);

      strcpy(scriptdir, tmpstr);
   }

   if(projectdir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, projectdir);

      strcpy(projectdir, tmpstr);
   }

   if(platelist[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, platelist);

      strcpy(platelist, tmpstr);
   }

   if(platedir[0] != '/')
   {
      strcpy(tmpstr, cwd);
      strcat(tmpstr, "/");
      strcat(tmpstr, platedir);

      strcpy(platedir, tmpstr);
   }

   if(scriptdir[strlen(scriptdir)-1] != '/')
      strcat(scriptdir, "/");

   if(projectdir[strlen(projectdir)-1] != '/')
      strcat(projectdir, "/");

   if(platedir[strlen(platedir)-1] != '/')
      strcat(platedir, "/");

   if(debug)
   {
      printf("scriptdir:  [%s]\n", scriptdir);
      printf("projectdir: [%s]\n", projectdir);
      printf("platelist:  [%s]\n", platelist);
      printf("platedir:   [%s]\n", platedir);
      printf("order      = %d \n", order);
      fflush(stdout);
   }


   /*****************************/ 
   /* Open the plate list file. */
   /*****************************/ 

   ncols = topen(platelist);

   if(ncols <= 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid image metadata file: %s\"]\n", platelist);
      fflush(stdout);
      exit(0);
   }

   iid    = tcol("id");
   iplate = tcol("plate");

   if(debug)
   {
      printf("\nPlate list:\n");
      printf("iid    = %d\n", iid);
      printf("iplate = %d\n", iplate);
      fflush(stdout);
   }

   if(iid    < 0
   || iplate < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Need columns: id and plate in plate list.\"]\n");
      fflush(stdout);
      exit(0);
   }


   /*******************************/
   /* Open the driver script file */
   /*******************************/

   sprintf(driverfile, "%srunCoadds.sh", scriptdir);

   fdriver = fopen(driverfile, "w+");

   if(debug)
   {
      printf("Driver file: [%s]\n", driverfile);
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



   /*******************************************/ 
   /* Iterate over the plates. Create an mAdd */
   /* script for each one.                    */
   /*******************************************/ 

   ijob = 0;

   while(1)
   {
      if(tread() < 0)
         break;

      id = atoi(tval(iid));

      strcpy(plate, tval(iplate));

      if(debug)
      {
         printf("Make script for plate [%s] (%d)\n", plate, ijob);
         fflush(stdout);
      }

      sprintf(scriptfile, "%sjobs/coadd_%04d.sh", scriptdir, ijob);
      
      fscript = fopen(scriptfile, "w+");

      if(debug)
      {
         printf("Script file: [%s]\n", scriptfile);
         fflush(stdout);
      }
      if(fscript == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Cannot open output script file [%s].\"]\n", scriptfile);
         fflush(stdout);
         exit(0);
      }

      fprintf(fscript, "#!/bin/sh\n\n");

      fprintf(fscript, "echo jobs/coadd_%04d.sh\n", ijob);

      fprintf(fscript, "mImgtbl $1/corrected $1/cimages.tbl\n");

      if(noAreas) 
         fprintf(fscript, "mAdd -n -p $1/corrected $1/cimages.tbl $1/region.hdr $2order$3/$4.fits\n");
      else
         fprintf(fscript, "mAdd -p $1/corrected $1/cimages.tbl $1/region.hdr $2order$3/$4.fits\n");

      fflush(fscript);
      fclose(fscript);

      chmod(scriptfile, 0777);

      fprintf(fdriver, "sbatch --mem=8192 --mincpus=1 %sslurmCoadd.bash %sjobs/coadd_%04d.sh %s%s %s %d %s\n", 
         scriptdir, scriptdir, ijob, projectdir, plate, platedir, order, plate);
      fflush(fdriver);

      ++ijob;
   }

   tclose(); 

   fclose(fdriver);

   chmod(driverfile, 0777);


   /*************/
   /* Finish up */
   /*************/

   printf("[struct stat=\"OK\", count=%d]\n", ijob);
   fflush(stdout);
   exit(0);
}
