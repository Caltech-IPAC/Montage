#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mSubset.h>
#include <montage.h>

#define MAXSTR 256
#define MAXIMG 256


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int main(int argc, char **argv)
{
   int    c, debug, fastmode;

   char   tblfile [MAXSTR];
   char   template[MAXSTR];
   char   subtbl  [MAXSTR];

   struct mSubsetReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug  = 0;
   opterr = 0;

   fastmode = 0;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "fd:s:")) != EOF) 
   {
      switch (c) 
      {
         case 'f':
            fastmode = 1;
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
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
	    printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-f] [-s statusfile] images.tbl template.hdr subset.tbl\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-f] [-s statusfile] images.tbl template.hdr subset.tbl\"]\n", argv[0]);
      exit(1);
   }

   strcpy(tblfile,  argv[optind]);
   strcpy(template, argv[optind + 1]);
   strcpy(subtbl,   argv[optind + 2]);

   returnStruct = mSubset(tblfile, template, subtbl, fastmode, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mSubset\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
