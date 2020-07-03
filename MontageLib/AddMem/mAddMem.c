#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <mAddMem.h>
#include <montage.h>

#define MAXSTR  256


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


int main(int argc, char **argv)
{
   int    c, debug;
   int    noAreas;

   char   path         [MAXSTR];
   char   table_file   [MAXSTR];
   char   template_file[MAXSTR];
   char   output_file  [MAXSTR];

   struct mAddMemReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug   = 0;
   noAreas = 0;

   opterr =  0;

   montage_status = stdout;

   strcpy(path, ".");

   while ((c = getopt(argc, argv, "nd:s:p:")) != EOF) 
   {
      switch (c) 
      {
         case 'n':
            noAreas = 1;
            break;

         case 'd':
            debug = montage_debugCheck(optarg);

            if(debug < 0)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Invalid debug level.\"]\n");
                exit(1);
            }
            break;

         case 'p':
    
            strcpy(path, optarg);
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
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-n(o-areas)] [-s statusfile] [-p path] images.tbl hdr.template out.fits\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level] [-n(o-areas)] [-s statusfile] [-p path] images.tbl hdr.template out.fits\"]\n", argv[0]);
      exit(1);
   }

   strcpy(table_file,    argv[optind]);
   strcpy(template_file, argv[optind + 1]);
   strcpy(output_file,   argv[optind + 2]);

   returnStruct = mAddMem(path, table_file, template_file, output_file, noAreas, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mAddMem\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
