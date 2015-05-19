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

#define STRLEN 4096

#define NORMAL  0
#define COM     1
#define WCS     2

void  printError(char *errmsg);

int debug = 0;

FILE *fdebug;


/*******************************************************************/
/*                                                                 */
/*  This service is a utility for getting/formatting FITS header   */
/*  records for display in association with the mViewer web app.   */
/*                                                                 */
/*******************************************************************/


int main(int argc, char *argv[], char *envp[])
{
   int   nkey, odd, class, pid;

   char  cmd         [STRLEN];
   char  line        [STRLEN];
   char  wspace      [STRLEN];
   char  fileName    [STRLEN];
   char  fileBase    [STRLEN];
   char  directory   [STRLEN];
   char  baseURL     [STRLEN];
   char  workDir     [STRLEN];
   char  hdrFile     [STRLEN];
   char  keyword     [STRLEN];
   char  status      [STRLEN];
   char  tmpstr      [STRLEN];

   char *ptr;

   FILE *fhdr;


   /********************/
   /* Config variables */
   /********************/

   config_init((char *)NULL);

   if(config_exists("ISIS_WORKDIR"))
      strcpy(workDir, config_value("ISIS_WORKDIR"));
   else
      printError("No workspace directory.");


   if(config_exists("ISIS_WORKURL"))
      strcpy(baseURL, config_value("ISIS_WORKURL"));
   else
      printError("No workspace URL.");


   /************/
   /* Keywords */
   /************/

   nkey = keyword_init(argc, argv);

   if(nkey <= 0)
      printError("No keywords found.");

   if(!debug && keyword_exists("debug"))
      debug = atoi(keyword_value("debug"));

   fdebug = stdout;

   if(debug > 1)
   {
      pid = getpid();

      sprintf (tmpstr, "/tmp/mViewerHdr.debug_%d", pid);

      fdebug = fopen (tmpstr, "w+");
   }

   if(debug)
      svc_debug(fdebug);
 
   if(keyword_exists("ws"))
      strcpy(wspace, keyword_value("ws"));
   else
      printError("No workspace.");

   strcpy(fileName, "");
   if(keyword_exists("file"))
      strcpy(fileName, keyword_value("file"));

   ptr = fileName + strlen(fileName);

   while(ptr > fileName && *ptr != '/')
      --ptr;

   if(*ptr == '/')
      ++ptr;

   strcpy(fileBase, ptr);

   if(debug)
   {
      fprintf(fdebug, "DEBUG> Config parameters:");

      fprintf(fdebug, "DEBUG> ISIS_WORKDIR = [%s]\n", workDir);
      fprintf(fdebug, "DEBUG> ISIS_WORKURL = [%s]\n", baseURL);
      fprintf(fdebug, "DEBUG> wspace       = [%s]\n", wspace);
      fprintf(fdebug, "DEBUG> fileName     = [%s]\n", fileName);
      fprintf(fdebug, "DEBUG> fileBase     = [%s]\n", fileBase);
      fflush(fdebug);
   }



   /************************/
   /* Build directory name */
   /************************/

   strcpy(directory, workDir);
   strcat(directory, "/");
   strcat(directory, wspace);

   strcat(baseURL,   "/");
   strcat(baseURL,   wspace);

   if(debug)
   {
      fprintf(fdebug, "DEBUG> directory = %s\n", directory);
      fprintf(fdebug, "DEBUG> baseURL      = %s\n", baseURL);
      fflush(fdebug);
   }
      


   /************************/
   /* Retrieve information */
   /************************/

   sprintf(cmd, "mGetHdr %s/%s %s/%s.hdr", directory, fileBase, directory, fileBase);
   svc_run(cmd);

   strcpy(status, svc_value("stat"));

   if(strcmp(status, "OK") != 0)
      printError("Cannot extract FITS header from file.");


   sprintf(hdrFile, "%s/%s.hdr", directory, fileBase);

   fhdr = fopen(hdrFile, "r");

   printf("HTTP/1.1 200 OK\r\n");
   printf("Content-type: text/plain\r\n\r\n");
   fflush(stdout);

   odd = 0;

   while(1)
   {   
      if(fgets(line, STRLEN, fhdr) == (char *)NULL)
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
            printf("<pre class=\"fitsOddWCS\" > %s </pre>\n", html_encode(line));

         else if(class == COM)
            printf("<pre class=\"fitsOddCOM\" > %s </pre>\n", html_encode(line));

         else
            printf("<pre class=\"fitsOdd\"    > %s </pre>\n", html_encode(line));
      }
      else
      {
         if(class == WCS)
            printf("<pre class=\"fitsEvenWCS\"> %s </pre>\n", html_encode(line));

         else if(class == COM)
            printf("<pre class=\"fitsEvenCOM\"> %s </pre>\n", html_encode(line));

         else
            printf("<pre class=\"fitsEven\"   > %s </pre>\n", html_encode(line));
      }

      odd = (odd+1)%2;
   }

   fflush(stdout);

   fclose(fhdr);

   exit(0);
}


/**********************/
/* HTML Error message */
/**********************/

void printError(char *errmsg)
{
   printf("HTTP/1.1 200 OK\n");
   printf("Content-type: text/plain\r\n");
   printf ("\r\n");

   printf ("{\"error\" : \"%s\"}", errmsg);
   fflush (stdout);

   exit(0);
}
