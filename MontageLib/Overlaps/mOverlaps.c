#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mOverlaps.h>
#include <montage.h>

#define MAXSTR 1024


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int main(int argc, char **argv)
{
   int    c;
   int    debug, quickmode;
   
   char   tblfile[MAXSTR];
   char   difftbl[MAXSTR];

   struct mOverlapsReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug  = 0;
   opterr = 0;

   montage_status = stdout;

   quickmode = 1;

   while ((c = getopt(argc, argv, "ed:s:")) != EOF) 
   {
      switch (c) 
      {
         case 'e':
            quickmode = 0;
            break;

         case 'd':
            debug = montage_debugCheck(optarg);

            if(debug < 0)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Invalid debug level.\"]\n");
                exit(1);
            }
            break;

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s", optarg);
               exit(1);
            }
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-e] [-d level] [-s statusfile] images.tbl diffs.tbl\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 2) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-e] [-d level] [-s statusfile] images.tbl diffs.tbl\"]\n", argv[0]);
      exit(1);
   }

   strcpy(tblfile, argv[optind]);
   strcpy(difftbl, argv[optind + 1]);

   returnStruct = mOverlaps(tblfile, difftbl, quickmode, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mOverlaps\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
