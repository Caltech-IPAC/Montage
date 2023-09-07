#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mtbl.h>

#include <montage.h>

#define MAXSTR 4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*******************************************************************/
/*                                                                 */
/*  mFlatten                                                       */
/*                                                                 */
/*  Run mFitplane and use the fit to subtract from the image using */
/*  mBackground.                                                   */
/*                                                                 */
/*******************************************************************/

int main(int argc, char **argv)
{
   int   ch, levelOnly, border;
   int   debug;

   char input_file [MAXSTR];
   char output_file[MAXSTR];

   struct mFlattenReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug     = 0;
   opterr    = 0;
   levelOnly = 0;
   border    = 0;

   montage_status = stdout;

   while ((ch = getopt(argc, argv, "dlb:s:")) != EOF) 
   {
      switch (ch) 
      {
         case 'd':
            debug = 1;
            break;

         case 'l':
            levelOnly = 1;
            break;

         case 'b':
            border = atoi(optarg);
            break;

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
	    printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s statusfile] in.fits out.fits\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 2) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-s statusfile] in.fits out.fits\"]\n", argv[0]);
      exit(1);
   }

   strcpy(input_file,  argv[optind]);
   strcpy(output_file, argv[optind + 1]);


   /********************************************/
   /* Call the mFlatten processing routine */
   /********************************************/

   returnStruct = mFlatten(input_file, output_file, levelOnly, border, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mFlatten\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
