#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mtbl.h>

#define STRLEN 1024


/****************************************************************************************/
/*                                                                                      */
/*  mHPXGapCombine                                                                      */
/*  --------------                                                                      */
/* For special projections, we sometimes have other pair-ups of plates besides          */
/* the overlaps.  For instance, the HPX projection is mostly Cylindrical Equal Area,    */
/* except at high latitudes where it "splits open" from the pole down some ~40 degrees. */
/* Plates on either side of this gap are not only separated by a considerable number    */
/* of (blank) pixels the matching edges are at right angles to each other.              */
/*                                                                                      */
/* Similarly, the far left and right of the projection is a "wrap-around" (-180/+180),  */
/* contiguous on the real sky.                                                          */
/*                                                                                      */
/* The way we analyze these matches, there is one "fit" file for each of them in a      */
/* "gap" directory and a summary table ("gap.tbl") listing them.                        */
/*                                                                                      */
/*  This routine parses through the gap background comparison data and creates          */
/*  an output table with the same structure as the standard DiffFit analysis            */
/*  based on plate overlaps.  To this is added a set of records tying together          */
/*  the plates that duplicate the same area on either end of the HPX map due to         */
/*  -180 / +180 wraparound.  These "differences" are always zero (it's the same         */
/*  data) and serve to tie the sky together globally.                                   */
/*                                                                                      */
/*  The arguments are:                                                                  */
/*                                                                                      */
/*     platelist.tbl  -  List of plates.                                                */
/*                                                                                      */
/*     gap.tbl        -  List of gap diff/fit matches.  This file just names the        */
/*                       set of gap_XXX.diff files where the real data resides.         */
/*                                                                                      */
/*     wrap.tbl       -  List of wraparound plate matches.                              */
/*                                                                                      */
/*     gapfit.tbl     -  Output summary in DiffFit format.                              */
/*                                                                                      */
/****************************************************************************************/

int main(int argc, char **argv)
{
   int    ncols, ifname, iplatename, icntr, stat, nplate, maxplate;
   int    i, cntr1, cntr2, iplus, iminus;

   char   platelist [STRLEN];
   char   gaptbl    [STRLEN];
   char   wraptbl   [STRLEN];
   char   gapfits   [STRLEN];
   char   tmpstr    [STRLEN];
   char   fname     [STRLEN];
   char   line      [STRLEN];

   char **platename;
   int   *cntr;

   char  *ptr;

   FILE  *fgap;
   FILE  *fout;

   char   plus_file [STRLEN];
   char   minus_file[STRLEN];
   double A, B, C;
   double C0;
   double crpix1, crpix2;
   int    xmin, xmax;
   int    ymin, ymax;
   double xcenter, ycenter;
   double npix;
   double rms;
   double boxx, boxy;
   double boxwidth, boxheight, boxang;

   double size;


   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXGapCombine platelist.tbl gap.tbl wrap.tbl gapfit.tbl\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(platelist, argv[1]);
   strcpy(gaptbl,    argv[2]);
   strcpy(wraptbl,   argv[3]);
   strcpy(gapfits,   argv[4]);
  

   // Open output file

   fout = fopen(gapfits, "w+");

   if(fout  == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open output table.\"]\n");
      fflush(stdout);
      exit(0);
   }

   fprintf(fout, "|   plus  |  minus  |         a      |        b       |        c       |    crpix1    |    crpix2    |   xmin   |   xmax   |   ymin   |   ymax   |   xcenter   |   ycenter   |    npixel   |      rms       |      boxx      |      boxy      |    boxwidth    |   boxheight    |     boxang     |\n");
   fflush(fout);


   // Read the plate list

   ncols = topen(platelist);

   if(ncols < 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open platelist.tbl file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   icntr      = tcol("id");
   iplatename = tcol("plate");

   if(icntr < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"gap.tbl file in gap directory does not have 'id' column.\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(iplatename < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"gap.tbl file in gap directory does not have 'id' column.\"]\n");
      fflush(stdout);
      exit(0);
   }

   maxplate = 1024;

   platename = (char **)malloc(maxplate * sizeof(char *));
   cntr      = (int   *)malloc(maxplate * sizeof(int));

   for(i=0; i<maxplate; ++i)
      platename[i] = (char *)malloc(STRLEN * sizeof(char));

   nplate = 0;

   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      strcpy(platename[nplate], tval(iplatename));
      strcat(platename[nplate], ".fits");

      cntr[nplate] = atoi(tval(icntr));

      ++nplate;

      if(nplate >= maxplate)
      {
         maxplate += 1024;

         platename = (char **)realloc(platename, maxplate * sizeof(char *));
         cntr      = (int   *)realloc(cntr,      maxplate * sizeof(int));

         for(i=maxplate-1024; i<maxplate; ++i)
            platename[i] = (char *)malloc(STRLEN * sizeof(char));
      }
   }


   // Read through list of gap fits

   ncols = topen(gaptbl);

   if(ncols < 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open gap.tbl file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   ifname = tcol("file");

   if(ifname < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"gap.tbl file in gap directory does not have 'file' column.\"]\n");
      fflush(stdout);
      exit(0);
   }


   size = 0.;

   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      strcpy(fname, tval(ifname));

      strcpy(tmpstr, fname);

      fgap = fopen(tmpstr, "r");

      if(fgap == (FILE *)NULL)
      {
         tclose();

         printf("[struct stat=\"ERROR\", msg=\"Cannot open gap file.\"]\n");
         fflush(stdout);
         exit(0);
      }


      // Read through the diff file for this special pair.
      // These file, for now, are rigorously structured.

      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%s",          plus_file);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%s",          minus_file);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf %lf", &A, &B, &C);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf",         &C0);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &crpix1, &crpix2);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%d %d",       &xmin, &xmax);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%d %d",       &ymin, &ymax);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &xcenter, &ycenter);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &npix, &rms);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf",     &boxx, &boxy);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%lf %lf %lf", &boxwidth, &boxheight, &boxang);
      fgets(line, 1024, fgap); ptr = line + 28; sscanf(ptr, "%s",          tmpstr);

      if(boxwidth > size)
         size = boxwidth;

      if(boxheight > size)
         size = boxheight;


      // Look up the cntrs for the two files

      cntr1 = -1;

      for(i=0; i<nplate; ++i)
      {
         if(strcmp(plus_file, platename[i]) == 0)
         {
            cntr1 = cntr[i];
            break;
         }
      }


      cntr2 = -1;

      for(i=0; i<nplate; ++i)
      {
         if(strcmp(minus_file, platename[i]) == 0)
         {
            cntr2 = cntr[i];
            break;
         }
      }


      // Print out the diff record
      
      fprintf(fout, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13.0f %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f \n",
            cntr1, cntr2, A, B, C, crpix1, crpix2, xmin, xmax, ymin, ymax,   
            xcenter, ycenter, npix, rms, boxx, boxy, boxwidth, boxheight, boxang);
      fflush(fout); 
   }


   /**********************/
   /* Wraparound matches */
   /**********************/

   ncols = topen(wraptbl);

   if(ncols < 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open wraps.tbl file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   iplus  = tcol("plus");
   iminus = tcol("minus");

   if(iplus < 0 || iminus < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"wrap.tbl file does not have 'plus' and 'minus' columns.\"]\n");
      fflush(stdout);
      exit(0);
   }


   A         = 0.;
   B         = 0.;
   C         = 0.;
   crpix1    = 0.;
   crpix2    = 0.;
   xmin      = 0.;
   xmax      = 0.;
   ymin      = 0.;
   ymax      = 0.;
   xcenter   = 0.;
   ycenter   = 0.;
   npix      = 0;
   rms       = 0.;
   boxx      = 0.;
   boxy      = 0.;
   boxwidth  = 0.;
   boxheight = 0.;
   boxang    = 0.;

   while(1)
   {
      stat = tread();

      if(stat < 0)
         break;

      strcpy(plus_file,  tval(iplus));
      strcpy(minus_file, tval(iminus));


      // Look up the cntrs for the two files

      cntr1 = -1;

      for(i=0; i<nplate; ++i)
      {
         if(strcmp(plus_file, platename[i]) == 0)
         {
            cntr1 = cntr[i];
            break;
         }
      }


      cntr2 = -1;

      for(i=0; i<nplate; ++i)
      {
         if(strcmp(minus_file, platename[i]) == 0)
         {
            cntr2 = cntr[i];
            break;
         }
      }

      boxwidth  = size;
      boxheight = size;
      xmax      = size;
      ymax      = size;
      npix      = size * size;


      // Print out the diff record
      
      fprintf(fout, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13.0f %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f \n",
            cntr1, cntr2, A, B, C, crpix1, crpix2, xmin, xmax, ymin, ymax,   
            xcenter, ycenter, npix, rms, boxx, boxy, boxwidth, boxheight, boxang);
      fflush(fout); 
   }

   tclose();


   printf("[struct stat=\"OK\"]\n");
   fflush(stdout);
   exit(0);
}
