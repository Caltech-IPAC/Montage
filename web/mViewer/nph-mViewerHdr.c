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
void  printRetval(char *msg);

int debug = 2;

FILE *fdebug;

int writeFitshdrHtml (char *hdrpath, char *htmlpath, char *fileBase);


/*******************************************************************/
/*                                                                 */
/*  This service is a utility for getting/formatting FITS header   */
/*  records for display in association with the mViewer web app.   */
/*                                                                 */
/*******************************************************************/


int main(int argc, char *argv[], char *envp[])
{
   int   nkey;
   int   pid;
   int   istatus;

   char  cmd         [STRLEN];
   char  wspace      [STRLEN];
   char  fileName    [STRLEN];
   char  filePath    [STRLEN];
   char  fileBase    [STRLEN];
   char  directory   [STRLEN];
   char  baseURL     [STRLEN];
   char  workDir     [STRLEN];
   char  hdrFile     [STRLEN];
   char  htmlFile    [STRLEN];
   char  status      [STRLEN];
   char  tmpstr      [STRLEN];
   char  url         [1024];

   char *ptr;

   
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

      sprintf (tmpstr, "/tmp/icePlotterHdr.debug_%d", pid);
      
      sprintf (tmpstr, 
          "/koa/cm/ws/mihseh/montage/web/mViewer/mviewerhdr.debug");
      fdebug = fopen (tmpstr, "w+");
   }


/*
   if(debug)
      svc_debug(fdebug);
*/

   if(keyword_exists("workspace"))
      strcpy(wspace, keyword_value("workspace"));
   else
      printError("No workspace.");

   strcpy(fileName, "");
   if(keyword_exists("file"))
      strcpy(fileName, keyword_value("file"));

   
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
      


   ptr = fileName + strlen(fileName);

   while(ptr > fileName && *ptr != '/')
      --ptr;

   if(*ptr == '/')
      ++ptr;

   strcpy(fileBase, ptr);


    strcpy (tmpstr, fileName);
    ptr = (char *)NULL;
    ptr = strrchr (tmpstr, '/');

    if (ptr != (char *)NULL) {
        
        strcpy (filePath, fileName);
        strcpy (fileName, ptr+1);
    }
    else {
        sprintf (filePath, "%s/%s", directory, fileName);
    }

   if(debug)
   {
      fprintf(fdebug, "DEBUG> Config parameters:");

      fprintf(fdebug, "DEBUG> ISIS_WORKDIR = [%s]\n", workDir);
      fprintf(fdebug, "DEBUG> ISIS_WORKURL = [%s]\n", baseURL);
      fprintf(fdebug, "DEBUG> wspace       = [%s]\n", wspace);
      fprintf(fdebug, "DEBUG> fileName     = [%s]\n", fileName);
      fprintf(fdebug, "DEBUG> filePath     = [%s]\n", filePath);
      fprintf(fdebug, "DEBUG> fileBase     = [%s]\n", fileBase);
      fflush(fdebug);
   }


   /************************/
   /* Retrieve information */
   /************************/

   sprintf(cmd, "mGetHdr %s %s/%s.hdr", filePath, directory, fileBase);
   if(debug)
   {
      fprintf(fdebug, "DEBUG> \ncmd= [%s]\n", cmd);
      fflush(fdebug);
   }
   
   svc_run(cmd);

   strcpy(status, svc_value("stat"));

   if(strcmp(status, "OK") != 0)
      printError("Cannot extract FITS header from file.");

   sprintf(hdrFile, "%s/%s.hdr", directory, fileBase);

   sprintf(htmlFile, "%s/%s.html", directory, fileBase);

   if(debug) {
      fprintf(fdebug, "DEBUG> call writeFitshdrHtml\n");
      fprintf(fdebug, "DEBUG> hdrFile= [%s]\n", hdrFile);
      fprintf(fdebug, "DEBUG> htmlFile= [%s]\n", htmlFile);
      fprintf(fdebug, "DEBUG> fileBase= [%s]\n", fileBase);
      fflush(fdebug);
   }
   
   istatus = writeFitshdrHtml (hdrFile, htmlFile, fileBase);

   if(debug) {
      fprintf(fdebug, "DEBUG> returned writeFitshdrHtml: istatus= [%d]\n",
          istatus);
      fflush(fdebug);
   }
   


   sprintf (url, "%s/%s.html", baseURL, fileBase);
   
   
   if(debug) {
       fprintf(fdebug, "url= [%s]\n", url);
       fprintf (fdebug, "{\"url\" : \"%s\"}", url);
       fflush(fdebug);
   }
   
   sprintf (url, "{\"url\" : \"%s/%s.html\"}", baseURL, fileBase);
   
   if(debug) {
      fprintf(fdebug, "url= [%s]\n", url);
      fflush(fdebug);
   }
   
   printRetval (url);

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
