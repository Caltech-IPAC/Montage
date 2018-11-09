#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <svc.h>

int main(int argc, char **argv)
{
   int  found;

   char template[100000];

   char code   [1024];
   char module [1024];
   char pattern[1024];
   char fileIn [1024];
   char fileOut[1024];
   char cmd    [1024];
   char message[1024];

   char *ptr1, *ptr2;

   FILE *ftemplate, *fcode, *fout;

   
   // Get the module name

   if(argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mJupyter <module>  (module is name like 'Add' or 'ProjectPP').  Must be run from MontageLib directory.\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(module, argv[1]);


   // Open the output file

   sprintf(fileOut, "%s/m%s.ipynb", module, module);

   fout = fopen(fileOut, "w+");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open [%s].\"]\n", fileOut);
      fflush(stdout);
      exit(0);
   }


   // We are going to generate a generic Jupyter page for the module.
   //
   //   The first section is boilerplate.
   //
   //   The second imports the python module and runs help().
   //
   //   The next few are an example or examples of running the module
   //   but here we just create a placeholder.
   //
   //   At the end we have two sections that illustrate the stand-alone
   //   program calling syntax (retrived by running the stand-alone program
   //   and capturing the "Usage" return) followed by the subroutine documentation
   //   and return structure extracted from the code.
   //
   // To do this, we use a simple template, looking for and replacing the strings
   // MODULE, "EXAMPLE", USAGE, and "ARGUMENTS" (the quotes are there so we can 
   // view the template directly as a Jupyter notebook file.1

   ftemplate = fopen("data/mJupyter.ipynb", "r");

   if(ftemplate == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Cannot open template file.\"]\n");
      fflush(stdout);
      exit(0);
   }

   while(1)
   {
      if(fgets(template, 100000, ftemplate) == (char *)NULL)
         break;

      if(template[strlen(template)-1] == '\n')
         template[strlen(template)-1]  = '\0';


      // MODULE

      if(strstr(template, "MODULE"))
      {
         ptr1 = strstr(template, "MODULE");
         ptr2 = ptr1 + 6;

         *ptr1 = '\0';

         fprintf(fout, "%sm%s%s\n", template, module, ptr2);
         fflush(fout);
      }


      // EXAMPLE

      else if(strstr(template, "\"EXAMPLE\""))
      {
         fprintf(fout, "%s\n", template);
         fflush(fout);
      }


      // USAGE

      else if(strstr(template, "USAGE"))
      {
         strcpy(message, "Usage: unknown");

         ptr2 = strstr(message, "Usage: " + 7);

         sprintf(cmd, "m%s", module);

         svc_run(cmd);

         if(svc_value("msg"))
         {
            strcpy(message, svc_value("msg"));

            if(strstr(message, "Usage: "))
               ptr2 = strstr(message, "Usage: ") + 7;
         }

         ptr1 = strstr(template, "USAGE");

         *ptr1 = '\0';

         fprintf(fout, "%s%s\\n\",\n", template, ptr2);
         fflush(fout);
      }


      // ARGUMENTS

      else if(strstr(template, "\"ARGUMENTS\""))
      {
         // Process the source code file first

         sprintf(fileIn, "%s/montage%s.c", module, module);

         fcode = fopen(fileIn, "r");

         if(fcode == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open [%s].\"]\n", fileOut);
            fflush(stdout);
            exit(0);
         }

         found = 0;

         fprintf(fout, "    \"<pre>\\n\",\n");
         fflush(fout);

         while(1)
         {
            if(fgets(code, 1024, fcode) == (char *)NULL)
               break;

            if(code[strlen(code)-1] == '\n')
               code[strlen(code)-1]  = '\0';

            
            // The comment block has this special structure 
            // in the first code line we want to keep

            if(strstr(code, "/*-*") != 0)
            {
               found = 1;

               fprintf(fout, "    \"%s\\n\",\n", code);
               fflush(fout);

               while(1)
               {
                  if(fgets(code, 1024, fcode) == (char *)NULL)
                     break;

                  if(code[strlen(code)-1] == '\n')
                     code[strlen(code)-1]  = '\0';

                  if(code[0] == '{')
                     break;

                  fprintf(fout, "    \"%s\\n\",\n", code);
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

         fclose(fcode);


         // Now go get the return structure from montage.h

         fprintf(fout, "    \"</pre>\\n\",\n");
         fprintf(fout, "    \"<p><b>Return Structure</b></p>\\n\",\n");
         fprintf(fout, "    \"<pre>\\n\",\n");
         fflush(fout);

         strcpy(fileIn, "montage.h");

         sprintf(pattern, "struct m%sReturn", module);

         fcode = fopen(fileIn, "r");

         if(fout == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Cannot open [%s].\"]\n", fileOut);
            fflush(fout);
            exit(0);
         }


         found = 0;

         while(1)
         {
            if(fgets(code, 1024, fcode) == (char *)NULL)
               break;

            if(code[strlen(code)-1] == '\n')
               code[strlen(code)-1]  = '\0';

            
            // The comment block has this special structure in the first line

            if(strstr(code, pattern) != 0)
            {
               found = 1;

               fprintf(fout, "    \"%s\\n\",\n", code);
               fflush(fout);

               while(1)
               {
                  if(fgets(code, 1024, fcode) == (char *)NULL)
                     break;

                  if(code[strlen(code)-1] == '\n')
                     code[strlen(code)-1]  = '\0';

                  if(strncmp(code, "};", 2) == 0)
                  {
                     fprintf(fout, "    \"%s\\n\",\n", code);
                     break;
                  }

                  fprintf(fout, "    \"%s\\n\",\n", code);
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

         fprintf(fout, "    \"</pre>\"\n");
         fflush(fout);
      }


      // TEMPLATE

      else
      {
         fprintf(fout, "%s\n", template);
         fflush(fout);
      }
   }

   fclose(fout);
   fclose(fcode);

   printf("[struct stat=\"OK\"]\n");
   fflush(stdout);
   exit(0);
}
