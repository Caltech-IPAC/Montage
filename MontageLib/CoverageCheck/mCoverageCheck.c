#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <mCoverageCheck.h>
#include <montage.h>


/*************************************************************************/
/*                                                                       */
/*  mCoverageCheck                                                       */
/*                                                                       */
/*  There are several ways to define the region of interest              */
/*  (polygon, box, circle, point) and defaults for that can be used      */
/*  to simplify some of them.  The following examples should illustrate  */
/*  this:                                                                */
/*                                                                       */
/*  mCoverageCheck swire.tbl region.tbl -points 2. 4. 4. 4. 4. 6. 2. 6.  */
/*  mCoverageCheck swire.tbl region.tbl -box 33. 25. 2. 2. 45.           */
/*  mCoverageCheck swire.tbl region.tbl -box 33. 25. 2. 2.               */
/*  mCoverageCheck swire.tbl region.tbl -box 33. 25. 2.                  */
/*  mCoverageCheck swire.tbl region.tbl -circle 33. 25. 2.8              */
/*  mCoverageCheck swire.tbl region.tbl -point 33. 25.                   */
/*  mCoverageCheck swire.tbl region.tbl -header region.hdr               */
/*  mCoverageCheck swire.tbl region.tbl -cutout 33. 25. 2.               */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv) 
{
   int    debug;

   int    i, imode;

   char   infile  [1024];
   char   outfile [1024];
   char   hdrfile [1024];
   char   mode    [1024];
   char   path    [1024];

   int    narray;
   double array[1024];

   char  *end;

   struct mCoverageCheckReturn *returnStruct;

   FILE *montage_status;


   montage_status = stdout;

   debug = 0;


   /* Process basic command-line arguments */

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-s statusfile] in.tbl out.tbl -<mode> <parameters> [where mode can be 'points', 'box', 'circle', 'header', 'point' or 'cutout'\"]\n", argv[0]);

      exit(0);
   }

   for(i=1; i<argc; ++i)
   {
      if(argv[i][0] != '-')
         break;

      if(strncmp(argv[i], "-s", 2) == 0)
      {
         if(argc > i+1 && (montage_status = fopen(argv[i+1], "w+")) == (FILE *)NULL)
         {
            printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file.\"]\n");
            exit(1);
         }
      }

      if(strncmp(argv[i], "-p", 2) == 0)
      {
         if(argc > i+1)
         strcpy(path, argv[i+1]);
      }

      if(strncmp(argv[i], "-d", 2) == 0)
         debug = 1; 
   }

   if(montage_status != stdout)
   {
      argv += 2;
      argc -= 2;
   }

   if(strlen(path) > 0)
   {
      argv += 2;
      argc -= 2;
   }

   if(debug)
   {
      argv += 1;
      argc -= 1;
   }

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-s statusfile] in.tbl out.tbl -<mode> <parameters> [where mode can be 'points', 'box', 'circle', 'header', 'point' or 'cutout'\"]\n", argv[0]);

      exit(0);
   }

   strcpy(infile,  argv[1]);
   strcpy(outfile, argv[2]);
   strcpy(mode,    argv[3]);

        if(strcmp(mode, "-points" ) == 0)  imode = POINTS;
   else if(strcmp(mode, "-box"    ) == 0)  imode = BOX;
   else if(strcmp(mode, "-circle" ) == 0)  imode = CIRCLE;
   else if(strcmp(mode, "-point"  ) == 0)  imode = POINT;
   else if(strcmp(mode, "-header" ) == 0)  imode = HEADER;
   else if(strcmp(mode, "-cutout" ) == 0)  imode = CUTOUT;
   else
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid region definition mode: \"%s\"]\"]\n", 
         mode);
      fflush(stdout);
      exit(0);
   }

   narray = 0;

   if(imode == HEADER)
   {
      if(argc < 5)
      {
         printf("[struct stat=\"ERROR\", msg=\"Must give header file name\"]\n");
         exit(1);
      }

      strcpy(hdrfile, argv[4]);
   }
   else
   {
      for(i=4; i<argc; ++i)
      {
         array[narray] = strtod(argv[i], &end);

         if(end < argv[i] + strlen(argv[i]))
         {
            printf("[struct stat=\"ERROR\", msg=\"Input %d (%s) cannot be interpreted as a real number\"]\n", 
               narray+1, argv[i]);
            exit(1);
         }

         ++narray;
      }
   }


   returnStruct = mCoverageCheck(path, infile, outfile, imode, hdrfile, narray, array, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", %s]\n", returnStruct->msg);
       exit(0);
   }
}
