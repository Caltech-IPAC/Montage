#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#include <fitsio.h>

#include <mHPXGap.h>

#define MAXSTR  256

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);



/*************************************************************************/
/*                                                                       */
/*  mHPXGapDiff                                                          */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mHPXGapDiff, is part of the HiPS package.  Our HPX      */
/*  plates or organized so that some plate that are adjacent on the sky  */
/*  are separated by a gap in projected space and rotated by 90 degrees  */
/*  relative to each other.  So we can't do a normal overlap difference  */
/*  between them.                                                        */
/*                                                                       */
/*  Instead, we fit a plane to the backgrounds in what would be the      */
/*  adjacent regions along their edges and use this to infer what an     */
/*  approximate overlap difference would look like. Since these plates   */
/*  are near the north Galactic poles, this approach is fairly stable.   */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int   c, levelOnly, pad, width, debug;

   char  gappath   [MAXSTR];
   char  plus_file [MAXSTR];
   char  minus_file[MAXSTR];
   char  edge_str  [MAXSTR];

   int   plus_edge, minus_edge;

   char *end;

   struct mHPXGapDiffReturn *returnStruct;

   FILE *montage_status;
   

   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug     = 0;
   width     = 4096;
   pad       = 0;
   levelOnly = 0;

   opterr    = 0;

   strcpy(gappath, "");

   montage_status = stdout;

   while ((c = getopt(argc, argv, "dp:w:g:ls:")) != EOF) 
   {
      switch (c) 
      {
         case 'd':
            debug = 1;
            break;

         case 'g':
            strcpy(gappath, optarg);

            if(strcmp(gappath, ".") == 0)
               strcpy(gappath, "");

            break;

         case 'p':
            pad = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -p (%s) cannot be interpreted as an integer\"]\n", 
                  optarg);
               exit(1);
            }

            if(pad < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -p (%s) must be a positive integer\"]\n", 
                  optarg);
               exit(1);
            }

            break;

         case 'w':
            width = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) cannot be interpreted as an integer\"]\n", 
                  optarg);
               exit(1);
            }

            if(width < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Argument to -w (%s) must be a positive integer\"]\n", 
                  optarg);
               exit(1);
            }

            break;

         case 'l':
            levelOnly = 1;
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
            printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXGapDiff [-d(ebug)] [-g gappath] [-p pad] [-w width] [-s statusfile] [-l(evel-only)] plus_file plus_edge minus_file minus_edge\"]\n");
            exit(1);
            break;
      }
   }

   if (argc - optind < 4) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXGapDiff [-d(ebug)] [-g gappath] [-p pad] [-w width] [-s statusfile] [-l(evel-only)] plus_file plus_edge minus_file minus_edge\"]\n");
      exit(1);
   }

   plus_edge  = TOP;
   minus_edge = LEFT;

   strcpy(plus_file, argv[optind]);

   strcpy(edge_str, argv[optind+1]);

   if(edge_str[0] == 't' || edge_str[0] == 'T' || edge_str[0] == '0') plus_edge = TOP;
   if(edge_str[0] == 'l' || edge_str[0] == 'L' || edge_str[0] == '1') plus_edge = LEFT;
   if(edge_str[0] == 'r' || edge_str[0] == 'R' || edge_str[0] == '2') plus_edge = RIGHT;
   if(edge_str[0] == 'b' || edge_str[0] == 'B' || edge_str[0] == '3') plus_edge = BOTTOM;

   strcpy(minus_file, argv[optind+2]);

   strcpy(edge_str, argv[optind+3]);

   if(edge_str[0] == 't' || edge_str[0] == 'T' || edge_str[0] == '0') minus_edge = TOP;
   if(edge_str[0] == 'l' || edge_str[0] == 'L' || edge_str[0] == '1') minus_edge = LEFT;
   if(edge_str[0] == 'r' || edge_str[0] == 'R' || edge_str[0] == '2') minus_edge = RIGHT;
   if(edge_str[0] == 'b' || edge_str[0] == 'B' || edge_str[0] == '3') minus_edge = BOTTOM;

   if(debug)
   {
      printf("DEBUG> plus_file  = [%s]\n", plus_file);
      printf("DEBUG> minus_file = [%s]\n", minus_file);
      printf("DEBUG> plus_edge  =  %d \n", plus_edge);
      printf("DEBUG> minus_edge =  %d \n", minus_edge);
      fflush(stdout);
   }

   returnStruct = mHPXGapDiff(plus_file, plus_edge, minus_file, minus_edge, gappath, levelOnly, 
                              pad, width, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mHPXGapDiff\", %s]\n",
             returnStruct->msg);
       exit(0);
   }
}
