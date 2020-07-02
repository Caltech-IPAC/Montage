#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <mProjExec.h>
#include <montage.h>

#define MAXSTR 4096

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*******************************************************************/
/*                                                                 */
/*  mProjExec                                                      */
/*                                                                 */
/*  Runs mProject on a set of images, given the final mosaic       */
/*  header file, a list of images, and a location to put the       */
/*  projected data.                                                */
/*                                                                 */
/*******************************************************************/

int main(int argc, char **argv)
{
   int    debug, c, exact;
   int    expand, restart, quickMode;

   int    energyMode = 0;

   char   path     [MAXSTR];
   char   tblfile  [MAXSTR];
   char   template [MAXSTR];
   char   projdir  [MAXSTR];
   char   stats    [MAXSTR];
   char   border   [MAXSTR];
   char   scaleCol [MAXSTR];
   char   weightCol[MAXSTR];

   char  *end;

   double drizzle;

   struct mProjExecReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug       = 0;
   exact       = 0;
   restart     = 0;
   quickMode   = 0;
   energyMode  = 0;
   expand      = 0;

   drizzle     = 1.;

   strcpy(path,      ".");
   strcpy(border,    "");
   strcpy(scaleCol,  "");
   strcpy(weightCol, "");

   opterr = 0;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "p:dqeb:s:r:W:x:Xfz:")) != EOF) 
   {
      switch (c) 
      {
         case 'p':
            strcpy(path, optarg);

            if(montage_checkFile(path) != 2)
            {
               printf("[struct stat=\"ERROR\", msg=\"Path (%s) is not a directory\"]\n", path);
               exit(1);
            }

            break;

         case 'd':
            debug = 1;
            break;

         case 'q':
            quickMode = 1;
            exact     = 1;
            break;

         case 'e':
            exact = 1;
            break;

         case 'X':
            expand = 1;
            break;

         case 'b':
            strcpy(border, optarg);
            break;

         case 'x':
            strcpy(scaleCol, optarg);
            break;

         case 'W':
            strcpy(weightCol, optarg);
            break;

         case 'f':
            energyMode = 1;
            break;

         case 'r':
            restart = strtol(optarg, &end, 10);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Restart index value string (%s) cannot be interpreted as an integer\"]\n",
                  optarg);
               exit(1);
            }

            if(restart < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Restart index value (%d) must be greater than or equal to zero\"]\n",
                  restart);
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

         case 'z':
            drizzle = strtod(optarg, &end);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Drizzle factor string (%s) cannot be interpreted as a real number\"]\n"    ,
                  optarg);
               exit(1);
            }

            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-q(uick-mode)][-p rawdir] [-d] [-e(xact)] [-X(whole image)] [-b border] [-r restartrec] [-s statusfile] [-W weightColumn] [-x scaleColumn] images.tbl template.hdr projdir stats.tbl\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 4) 
   {
      fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Usage: %s [-q(uick-mode)][-p rawdir] [-d] [-e(xact)] [-X(whole image)] [-b border] [-r restartrec] [-s statusfile] [-W weightColumn] [-x scaleColumn] images.tbl template.hdr projdir stats.tbl\"]\n", argv[0]);
      exit(1);
   }

   strcpy(tblfile,  argv[optind]);
   strcpy(template, argv[optind + 1]);
   strcpy(projdir,  argv[optind + 2]);
   strcpy(stats,    argv[optind + 3]);


   /*****************************************/
   /* Call the mProjExec processing routine */
   /*****************************************/

   returnStruct = mProjExec(path, tblfile, template, projdir, quickMode, exact, expand, energyMode, drizzle,
                            border, scaleCol, weightCol, restart, stats, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mProjExec\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
