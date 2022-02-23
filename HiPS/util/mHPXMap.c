/* Module: mHPXMap.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

struct stat buf;

#define STRLEN 1024

/*************************************************************************/
/*                                                                       */
/*  mHPXMap                                                              */
/*                                                                       */
/*  We frequently have directories with a collection of HPX 'plate' PNGs */
/*  so this program generates an HTML file that uses a <table> to        */
/*  display them all so we can get an overview.                          */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int    i, j, nplate, pngsize;

   char   directory[STRLEN];
   char   htmlfile [STRLEN];
   char   pngfile  [STRLEN];
   char   cell     [STRLEN];

   FILE *fout;


   // Parse the command-line

   if(argc < 5)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mHPXMap directory htmlfile nplate pngsize\"]\n");
      exit(1);
   }

   strcpy(directory, argv[1]);
   strcpy(htmlfile,  argv[2]);

   nplate  = atoi(argv[3]);
   pngsize = atoi(argv[4]);

   if(pngsize < 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid PNG display size [%d].\"]\n", pngsize);
      exit(1);
   }


   fout = fopen(htmlfile, "w+");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open HTML file.\"]\n");
      exit(1);
   }


   // Loop over the space, checking for "plate_XX_XX.png" files and 
   // putting them in table cells if they exist.
   
   fprintf(fout, "<html>\n");

   fprintf(fout, "<head>\n");
   fprintf(fout, "   <style>\n");
   fprintf(fout, "      table {border-collapse: collapse; border-spacing: 0px;}\n");
   fprintf(fout, "      td {padding: 0px; margin: 0px;}\n");
   fprintf(fout, "   </style>\n");
   fprintf(fout, "</head>\n");

   fprintf(fout, "<body>\n");
   fprintf(fout, "<table>\n");

   for(j=0; j<nplate; ++j)
   {
      fprintf(fout, "   <tr>\n");

      for(i=0; i<nplate; ++i)
      {
         sprintf(pngfile, "%s/plate_%02d_%02d.png", directory, i, nplate-j-1);

         // printf("DEBUG> File: [%s]\n", pngfile);
         // fflush(stdout);

         if(stat(pngfile, &buf))
            sprintf(cell, "&nbsp;");
         else
            sprintf(cell, "<img src=\"plate_%02d_%02d.png\" width=\"%d\"/>",
               i, nplate-j-1, pngsize);

         fprintf(fout, "      <td>%s</td>\n", cell);
      }

      fprintf(fout, "   </tr>\n");
   }

   fprintf(fout, "</table>\n");
   fprintf(fout, "</body>\n");
   fprintf(fout, "</html>\n");

   fclose(fout);

   printf("[struct stat=\"OK\", module=\"mHPXMap\"]\n");
   fflush(stdout);
   exit(0);
}
