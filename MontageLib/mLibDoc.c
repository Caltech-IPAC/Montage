#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
   int  found;

   char line   [1024];
   char module [1024];
   char pattern[1024];
   char fileIn [1024];
   char fileOut[1024];

   FILE *fin, *fout;


   
   // Get the module name

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mLibDoc <module>  (module is name like 'Add' or 'ProjectPP').  Must be run from MontageLib directory.\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(module, argv[1]);


   // Open the output file

   sprintf(fileOut, "%s/m%sLib.html", module, module);

   fout = fopen(fileOut, "w+");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open [%s].\"]\n", fileOut);
      fflush(stdout);
      exit(0);
   }

   fprintf(fout, "<html>\n");
   fprintf(fout, "<body>\n");
   fprintf(fout, "<h2>m%s Function Call</h2>\n", module);
   fprintf(fout, "\n");
   fprintf(fout, "<p><b>Function Arguments</b></p>\n");
   fprintf(fout, "<pre>\n");
   fflush(fout);


   // Process the source code file first

   sprintf(fileIn, "%s/montage%s.c", module, module);

   fin = fopen(fileIn, "r");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open [%s].\"]\n", fileOut);
      fflush(stdout);
      exit(0);
   }


   found = 0;

   while(1)
   {
      if(fgets(line, 1024, fin) == (char *)NULL)
         break;

      
      // The comment block has this special structure in the first line

      if(strstr(line, "/*-*") != 0)
      {
         found = 1;

         fprintf(fout, "%s", line);
         fflush(fout);

         while(1)
         {
            if(fgets(line, 1024, fin) == (char *)NULL)
               break;

            if(line[0] == '{')
               break;

            fprintf(fout, "%s", line);
            fflush(fout);
         }

         break;
      }
   }

   if(found == 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Could not find documentation block in [%s]\"]\n", fileIn);
      fflush(stdout);
      exit(0);
   }

   fclose(fin);


   // Now go get the return structure from montage.h

   fprintf(fout, "</pre>\n");
   fprintf(fout, "<p><b>Return Structure</b></p>\n");
   fprintf(fout, "<pre>\n");
   fflush(fout);

   strcpy(fileIn, "montage.h");

   sprintf(pattern, "struct m%sReturn", module);

   fin = fopen(fileIn, "r");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open [%s].\"]\n", fileOut);
      fflush(fout);
      exit(0);
   }


   found = 0;

   while(1)
   {
      if(fgets(line, 1024, fin) == (char *)NULL)
         break;

      
      // The comment block has this special structure in the first line

      if(strstr(line, pattern) != 0)
      {
         found = 1;

         fprintf(fout, "%s", line);
         fflush(fout);

         while(1)
         {
            if(fgets(line, 1024, fin) == (char *)NULL)
               break;

            if(strncmp(line, "};", 2) == 0)
            {
               fprintf(fout, "%s", line);
               break;
            }

            fprintf(fout, "%s", line);
            fflush(fout);
         }

         break;
      }
   }

   if(found == 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Could not find [%s] in montage.h\"]\n", pattern);
      fflush(stdout);
      exit(0);
   }

   fprintf(fout, "</pre>\n");
   fprintf(fout, "</body>\n");
   fprintf(fout, "</html>\n");
   fflush(fout);

   fclose(fout);
   fclose(fin);

   printf("[struct stat=\"OK\"]\n");
   fflush(stdout);
   exit(0);
}
