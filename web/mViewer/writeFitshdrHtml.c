/*
    This subroutine rewrites the FITS header extracted from mGetHdr program
    into nicely formatted records for diaplay as an HTML page in association 
    with web applications.
*/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#include <www.h>
#include <password.h>
#include <config.h>
#include <mtbl.h>
#include <svc.h>

#define NORMAL  0
#define COM     1
#define WCS     2


extern FILE *fdebug;

int writeFitshdrHtml (char *hdrFile, char *htmlPath, char *fileBase)
{
   int   odd, class;

   char  line        [1024];
   char  keyword     [1024];

   char *ptr;

   FILE *fhdr;
   FILE *fout;

   
   fhdr = fopen (hdrFile, "r");

   fout = fopen (htmlPath, "w+");

/*
   fprintf (fout, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");

   fprintf (fout, "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n");
*/

   fprintf(fout, "<html>\n");
   
   fprintf(fout, "<head>\n");
   fprintf(fout, "<style>\n");
   fprintf(fout, "   .fitsBody {\n");
   fprintf(fout, "      background-color:#D8D8D8;\n");
   fprintf(fout, "      font-size: 20px;\n");
   fprintf(fout, "      font-style: normal;\n");
   fprintf(fout, "      font-weight: normal;\n");
   fprintf(fout, "      font-family: \"Times New Roman\", Times, serif;\n");
   fprintf(fout, "   }\n");
   fprintf(fout, "\n");
   fprintf(fout, "   .fitsHdr {\n");
   fprintf(fout, "      display: inline-block;\n");
   fprintf(fout, "      height: 500px;\n");
   fprintf(fout, "      overflow-y: scroll;\n");
   fprintf(fout, "      padding: 5px;\n");
   fprintf(fout, "      border: 1px solid black;\n");
   fprintf(fout, "   }\n");
   fprintf(fout, "\n");
   fprintf(fout, "   .fitsOdd {\n");
   fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
   fprintf(fout, "      font: 12px courier,sans-serif;\n");
   fprintf(fout, "      background-color:#F0F8FF;\n");
   fprintf(fout, "   }\n");
   fprintf(fout, "\n");
   fprintf(fout, "   .fitsEven {\n");
   fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
   fprintf(fout, "      font: 12px courier,sans-serif;\n");
   fprintf(fout, "      background-color:#FFFFFF;\n");
   fprintf(fout, "   }\n");
   fprintf(fout, "\n");
   fprintf(fout, "   .fitsOddWCS {\n");
   fprintf(fout, "      color: #0000A0;\n");
   fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
   fprintf(fout, "      font: 12px courier,sans-serif;\n");
   fprintf(fout, "      background-color:#F0F8E8;\n");
   fprintf(fout, "   }\n");
   fprintf(fout, "\n");
   fprintf(fout, "   .fitsEvenWCS {\n");
   fprintf(fout, "      color: #0000A0;\n");
   fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
   fprintf(fout, "      font: 12px courier,sans-serif;\n");
   fprintf(fout, "      background-color:#FFFFE8;\n");
   fprintf(fout, "   }\n");
   fprintf(fout, "\n");
   fprintf(fout, "   .fitsOddCOM {\n");
   fprintf(fout, "      color: #A00000;\n");
   fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
   fprintf(fout, "      font: 12px courier,sans-serif;\n");
   fprintf(fout, "      background-color:#F0F8FF;\n");
   fprintf(fout, "   }\n");
   fprintf(fout, "\n");
   fprintf(fout, "   .fitsEvenCOM {\n");
   fprintf(fout, "      color: #A00000;\n");
   fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
   fprintf(fout, "      font: 12px courier,sans-serif;\n");
   fprintf(fout, "      background-color:#FFFFFF;\n");
   fprintf(fout, "   }\n");
   fprintf(fout, "</style>\n");
   fprintf(fout, "</head>\n");
   fprintf(fout, "\n");
   fprintf(fout, "<body class=\"fitsBody\">\n");
   fprintf(fout, "<b><br/>&nbsp;&nbsp;%s</b><br/>\n", fileBase);
   fprintf(fout, "\n");
   fprintf(fout, "<div class=\"fitsHdr\">\n");


   odd = 0;

   while(1)
   {   
      if(fgets(line, 1024, fhdr) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';

      class = NORMAL;
      
      if(strncmp(line, "COMMENT", 7) == 0)
         class = COM;

      else
      {
         strcpy(keyword, line);

         ptr = strstr(keyword, "=");

         if(ptr)
            *ptr = '\0';

         ptr = keyword + strlen(keyword);

         while(ptr > keyword && (*ptr == '\0' || *ptr == ' ' || *ptr == '\n'))
         {
            *ptr = '\0';
            --ptr;
         }

         if(strncmp(keyword, "NAXIS", 5) == 0
         || strncmp(keyword, "CTYPE", 5) == 0
         || strncmp(keyword, "CRVAL", 5) == 0
         || strncmp(keyword, "CRPIX", 5) == 0
         || strncmp(keyword, "CDELT", 5) == 0
         || strncmp(keyword, "CROTA", 5) == 0
         || strncmp(keyword, "CD",    2) == 0
         || strncmp(keyword, "PC",    2) == 0)
            class = WCS;
      }

      if(odd)
      {
         if(class == WCS)
            fprintf(fout, "<pre class=\"fitsOddWCS\" > %s </pre>\n", html_encode(line));

         else if(class == COM)
            fprintf(fout, "<pre class=\"fitsOddCOM\" > %s </pre>\n", html_encode(line));

         else
            fprintf(fout, "<pre class=\"fitsOdd\"    > %s </pre>\n", html_encode(line));
      }
      else
      {
         if(class == WCS)
            fprintf(fout, "<pre class=\"fitsEvenWCS\"> %s </pre>\n", html_encode(line));

         else if(class == COM)
            fprintf(fout, "<pre class=\"fitsEvenCOM\"> %s </pre>\n", html_encode(line));

         else
            fprintf(fout, "<pre class=\"fitsEven\"   > %s </pre>\n", html_encode(line));
      }

      odd = (odd+1)%2;
   }

   fprintf(fout, "</div>\n");
   fprintf(fout, "</body>\n");
   fprintf(fout, "</html>\n");

   fclose(fhdr);
   fclose(fout);


   return (0);
}

