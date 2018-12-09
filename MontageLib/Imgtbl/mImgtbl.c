#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <montage.h>
#include <mImgtbl.h>

#define MAXSTR 1024

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);



/*************************************************************************/
/*                                                                       */
/*  mImgtbl                                                              */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int   c;

   int   debug;
   int   showinfo;
   int   showbad;
   int   recursiveMode;
   int   processAreaFiles;
   int   noGZIP;
   int   showCorners;
   int   haveCubes;

   char  pathname     [MAXSTR];
   char  tblname      [MAXSTR];
   char  imgListFile  [MAXSTR];
   char  fieldListFile[MAXSTR];

   struct mImgtblReturn *returnStruct;

   FILE *montage_status;


   strcpy (pathname,     "");
   strcpy (tblname,      "");
   strcpy(fieldListFile, "");
   strcpy(imgListFile,   "");

   debug            = 0;
   showinfo         = 0;
   showbad          = 0;
   recursiveMode    = 0;
   processAreaFiles = 0;
   showCorners      = 0;
   noGZIP           = 0;
   haveCubes        = 1;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "rcCadibs:f:t:z")) != -1) 
   {
      switch (c) 
      {
         case 'r':
            recursiveMode = 1;
            break;

         case 'c':
            showCorners = 1;
            break;

         case 'C':
            haveCubes = 0;
            break;

         case 'a':
            processAreaFiles = 1;
            break;

         case 'z':
            noGZIP = 1;
            break;

         case 'i':
            showinfo = 1;
            break;

         case 'b':
            showbad = 1;
            break;

         case 'd':
            debug = 1;
            break;

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         case 'f':
            strcpy(fieldListFile, optarg);
            break;

         case 't':
            strcpy(imgListFile, optarg);
            break;


         default:
            fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Illegal argument: -%c\"]\n", c);
            exit(1);
            break;
      }
   }

   if (argc - optind < 2) 
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Usage: %s [-rcCaidbdz][-s statusfile][-f fieldlistfile][-t imglist] directory images.tbl\"]\n", argv[0]);
       exit(1);
   }

   strcpy(pathname, argv[optind]);
   strcpy(tblname,  argv[optind+1]);

   if(strlen(pathname) > 1
   && pathname[strlen(pathname)-1] == '/')
      pathname[strlen(pathname)-1]  = '\0';

   returnStruct = mImgtbl(pathname, tblname, recursiveMode, processAreaFiles, haveCubes,
                          noGZIP, showCorners, showinfo, showbad, imgListFile, fieldListFile, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mImgtbl\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
