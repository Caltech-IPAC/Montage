#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <mAddCube.h>
#include <montage.h>

#define MAXSTR    1024


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/***************************/
/*  mAddCube main routine  */
/***************************/

int main(int argc, char **argv)
{
   int  c;

   int  debug     = 0;
   int  shrink    = 1;
   int  haveAreas = 1;
   int  coadd     = MEAN;

   char path    [MAXSTR];
   char tblfile [MAXSTR];
   char template[MAXSTR];
   char imgfile [MAXSTR];
   char argument[MAXSTR];

   struct mAddCubeReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   strcpy(path, ".");

   montage_status = stdout;

   while ((c = getopt(argc, argv, "enp:s:d:a:")) != EOF) 
   {
      switch (c) 
      {
        /***********************/
        /* Find averaging type */
        /***********************/

         case 'a':

            strcpy(argument, optarg);

                 if (strcmp(argument, "mean"  ) == 0) coadd = MEAN;
            else if (strcmp(argument, "median") == 0) coadd = MEDIAN;
            else if (strcmp(argument, "count" ) == 0) coadd = COUNT;
            else
            {
               printf("[struct stat=\"ERROR\", msg=\"Invalid argument for -a flag\"]\n");
               fflush(stdout);
               exit(1);
            }
            break;
 

         /************************/
         /* Look for debug level */
         /************************/

         case 'd':

            debug = montage_debugCheck(optarg);

            if(debug < 0)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Invalid debug level.\"]\n");
                exit(1);
            }
            break;


         /*****************************/
         /* Is 'exact-size" flag set? */
         /*****************************/

         case 'e':

            shrink = 0;
            break;


         /****************************/
         /* We don't have area files */
         /****************************/

         case 'n':

            haveAreas = 0;
            break;


         /*****************************/
         /* Get path to image dir     */
         /*****************************/

         case 'p':

            strcpy(path, optarg);
            break;


         /************************/
         /* Look for status file */
         /************************/

         case 's':

            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n", optarg);
               exit(1);
            }
            break;


         /*********************************************/
         /* Unknown directive: Usage message and exit */
         /*********************************************/

         default:

            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-p imgdir] [-n(o-areas)] [-a mean|median|count] [-e(xact-size)] [-d level] [-s statusfile] images.tbl template.hdr out.fits\"]\n", argv[0]);
            exit(1);
            break;
      }
   }


   /*****************************/
   /* Get required arguments    */
   /*****************************/

   if (argc - optind < 3) 
   {
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-p imgdir] [-n(o-areas)] [-a mean|median|count] [-e(xact-size)] [-d level] [-s statusfile] images.tbl template.hdr out.fits\"]\n", argv[0]);
      exit(1);
   }

   strcpy(tblfile,  argv[optind]);
   strcpy(template, argv[optind + 1]);
   strcpy(imgfile,  argv[optind + 2]);
   

   /****************************************/
   /* Call the mAddCube processing routine */
   /****************************************/

   returnStruct = mAddCube(path, tblfile, template, imgfile, shrink, haveAreas, coadd, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mAddCube\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
