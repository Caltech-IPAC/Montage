/* Module: mCalExec.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        09Dec15  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mtbl.h>

#include "montage.h"

#define MAXSTR 4096

char *svc_value();
char *svc_run  (char *cmd);
int   svc_debug(FILE *stream);

char *filePath (char *path, char *fname);
char *fileName (char *filename);


extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int debug;


/*******************************************************************/
/*                                                                 */
/*  mCalExec                                                       */
/*                                                                 */
/*  This executive "calibrates" a set of images.  That is, it runs */
/*  mCalibrate on each one.  It adds a scale column to the input   */
/*  table, creating a new table that can then be used by           */
/*  mProjExec (by adding "-x scale" to the call).                  */
/*                                                                 */
/*******************************************************************/

int main(int argc, char **argv)
{
   int    ch, istat, ncols, count, failed;
   int    levelOnly, warning, noAreas;

   int    ifname;

   char   fname   [MAXSTR];

   char   path    [MAXSTR];
   char   tblfile [MAXSTR];
   char   calfile [MAXSTR];

   char   cmd     [MAXSTR];
   char   msg     [MAXSTR];
   char   status  [32];

   double scale, offset;

   static time_t currtime, start;

   struct stat type;

   FILE  *fout;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   strcpy(path, "");

   opterr = 0;

   fstatus = stdout;

   while ((ch = getopt(argc, argv, "p:ds:")) != EOF) 
   {
      switch (ch) 
      {
         case 'p':
            strcpy(path, optarg);
            break;

         case 'd':
            debug = 1;
            svc_debug(stdout);
            break;

         case 's':
            if((fstatus = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-p imgdir] [-d] orig.tbl cal.tbl\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 2) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-p imgdir] [-d] orig.tbl cal.tbl\"]\n", argv[0]);
      exit(1);
   }

   strcpy(tblfile, argv[optind]);
   strcpy(calfile, argv[optind+1]);



   /**********************************/ 
   /* Open the image list table file */
   /**********************************/ 

   ncols = topen(tblfile);

   if(ncols <= 0)
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Invalid image metadata file: %s\"]\n",
         tblfile);
      exit(1);
   }

   ifname = tcol("fname");

   if(ifname < 0)
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Need column 'file' in image metadata file %s\"]\n",
         tblfile);
      exit(1);
   }


   /*************************/ 
   /* Open the output table */
   /*************************/ 

   fout = fopen(calfile, "w+");

   if(fout == (FILE *)NULL)
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Cannot create output file (%s)\"]\n",
         calfile);
      exit(1);
   }

   fprintf(fout, "|%14s %s\n", "scale", tbl_hdr_string);

   if(haveType)
      fprintf(fout, "|%14s %s\n", "double", tbl_typ_string);

   if(haveUnit)
      fprintf(fout, "|%14s %s\n", "", tbl_uni_string);

   if(haveNull)
      fprintf(fout, "|%14s %s\n", "", tbl_nul_string);

   fflush(fout);


   /****************************************/ 
   /* Read the records and call mCalibrate */
   /****************************************/ 

   count   = 0;
   failed  = 0;
   warning = 0;

   time(&currtime);
   start = currtime;

   offset = 0.;

   while(1)
   {
      istat = tread();

      if(istat < 0)
         break;

      strcpy(fname, filePath(path, tval(ifname)));

      sprintf(cmd, "mCalibrate %s", fname);
      svc_run(cmd);

      strcpy( status, svc_value( "stat" ));

      if(strcmp( status, "ABORT") == 0)
      {
         strcpy( msg, svc_value( "msg" ));

         fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
         fflush(fstatus);

         exit(1);
      }

      ++count;

      if(strcmp( status, "ERROR") == 0)
      {
         fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Calibration failed for image %s\"]\n", fname);
         fflush(fstatus);

         exit(1);
      }

      scale = atof(svc_value("scale"));

      if(offset == 0.)
      {
         offset = log10(scale);

         offset = floor(offset + 0.5);

         offset = pow(10., offset);
      }

      fprintf(fout, " %14.7e %s\n", scale/offset, tbl_rec_string);
      fflush(fout);

      time(&currtime);

      if(debug)
      {
         printf("%s (%-g): %.0f seconds\n", 
            fname, scale/offset, (double)(currtime - start));
         fflush(stdout);
         
         start = currtime;
      }
   }

   fprintf(fstatus, "[struct stat=\"OK\", count=%d]\n", count);
   fflush(fstatus);

   exit(0);
}
