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
/*  This service runs mExamine on an image to get file and region  */
/*  region statistics.                                             */
/*                                                                 */
/*******************************************************************/


int main(int argc, char *argv[], char *envp[])
{
   int   nkey, pid;

   char  cmd         [STRLEN];
   char  wspace      [STRLEN];
   char  fileName    [STRLEN];
   char  x           [STRLEN];
   char  y           [STRLEN];
   char  radius      [STRLEN];
   char  fileBase    [STRLEN];
   char  directory   [STRLEN];
   char  baseURL     [STRLEN];
   char  workDir     [STRLEN];
   char  status      [STRLEN];
   char  tmpstr      [STRLEN];

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

      sprintf (tmpstr, "/tmp/mViewerStats.debug_%d", pid);

      fdebug = fopen (tmpstr, "w+");
   }

   if(debug)
      svc_debug(fdebug);
 
   if(keyword_exists("ws"))
      strcpy(wspace, keyword_value("ws"));
   else
      printError("No workspace.");

   if(keyword_exists("file"))
      strcpy(fileName, keyword_value("file"));
   else
      printError("No file specified.");

   ptr = fileName + strlen(fileName);

   while(ptr > fileName && *ptr != '/')
      --ptr;

   if(*ptr == '/')
      ++ptr;

   strcpy(fileBase, ptr);

   if(keyword_exists("x"))
      strcpy(x, keyword_value("x"));
   else
      printError("No x location specified.");

   if(keyword_exists("y"))
      strcpy(y, keyword_value("y"));
   else
      printError("No y location specified.");

   if(keyword_exists("radius"))
      strcpy(radius, keyword_value("radius"));
   else
      printError("No radius specified.");

   if(debug)
   {
      fprintf(fdebug, "DEBUG> Config parameters:");

      fprintf(fdebug, "DEBUG> ISIS_WORKDIR = [%s]\n", workDir);
      fprintf(fdebug, "DEBUG> ISIS_WORKURL = [%s]\n", baseURL);
      fprintf(fdebug, "DEBUG> wspace       = [%s]\n", wspace);
      fprintf(fdebug, "DEBUG> fileName     = [%s]\n", fileName);
      fprintf(fdebug, "DEBUG> fileBase     = [%s]\n", fileBase);
      fprintf(fdebug, "DEBUG> x            = [%s]\n", x);
      fprintf(fdebug, "DEBUG> y            = [%s]\n", y);
      fprintf(fdebug, "DEBUG> radius       = [%s]\n", radius);
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

   sprintf(cmd, "mExamine -p %sp %sp %sp %s/%s", x, y, radius, directory, fileBase);
   svc_run(cmd);

   strcpy(status, svc_value("stat"));

   if(strcmp(status, "OK") != 0)
      printError("Cannot get info from file file.");

   printf("HTTP/1.1 200 OK\r\n");
   printf("Content-type: application/json\r\n\r\n");
   fflush(stdout);

   printf("{\"proj\": \"%s\", ",    svc_value("proj"));
   printf("\"csys\": \"%s\", ",     svc_value("csys"));
   printf("\"equinox\": \"%s\", ",  svc_value("equinox"));
   printf("\"naxis1\": \"%s\", ",   svc_value("naxis1"));
   printf("\"naxis2\": \"%s\", ",   svc_value("naxis2"));
   printf("\"crval1\": \"%s\", ",   svc_value("crval1"));
   printf("\"crval2\": \"%s\", ",   svc_value("crval2"));
   printf("\"crpix1\": \"%s\", ",   svc_value("crpix1"));
   printf("\"crpix2\": \"%s\", ",   svc_value("crpix2"));
   printf("\"cdelt1\": \"%s\", ",   svc_value("cdelt1"));
   printf("\"cdelt2\": \"%s\", ",   svc_value("cdelt2"));
   printf("\"crota2\": \"%s\", ",   svc_value("crota2"));
   printf("\"lonc\": \"%s\", ",     svc_value("lonc"));
   printf("\"latc\": \"%s\", ",     svc_value("latc"));
   printf("\"ximgsize\": \"%s\", ", svc_value("ximgsize"));
   printf("\"yimgsize\": \"%s\", ", svc_value("yimgsize"));
   printf("\"rotequ\": \"%s\", ",   svc_value("rotequ"));
   printf("\"rac\": \"%s\", ",      svc_value("rac"));
   printf("\"decc\": \"%s\", ",     svc_value("decc"));
   printf("\"ra1\": \"%s\", ",      svc_value("ra1"));
   printf("\"dec1\": \"%s\", ",     svc_value("dec1"));
   printf("\"ra2\": \"%s\", ",      svc_value("ra2"));
   printf("\"dec2\": \"%s\", ",     svc_value("dec2"));
   printf("\"ra3\": \"%s\", ",      svc_value("ra3"));
   printf("\"dec3\": \"%s\", ",     svc_value("dec3"));
   printf("\"ra4\": \"%s\", ",      svc_value("ra4"));
   printf("\"dec4\": \"%s\", ",     svc_value("dec4"));
   printf("\"radius\": \"%s\", ",   svc_value("radius"));
   printf("\"radpix\": \"%s\", ",   svc_value("radpix"));
   printf("\"npixel\": \"%s\", ",   svc_value("npixel"));
   printf("\"nnull\": \"%s\", ",    svc_value("nnull"));
   printf("\"aveflux\": \"%s\", ",  svc_value("aveflux"));
   printf("\"rmsflux\": \"%s\", ",  svc_value("rmsflux"));
   printf("\"fluxref\": \"%s\", ",  svc_value("fluxref"));
   printf("\"sigmaref\": \"%s\", ", svc_value("sigmaref"));
   printf("\"xref\": \"%s\", ",     svc_value("xref"));
   printf("\"yref\": \"%s\", ",     svc_value("yref"));
   printf("\"raref\": \"%s\", ",    svc_value("raref"));
   printf("\"decref\": \"%s\", ",   svc_value("decref"));
   printf("\"fluxmin\": \"%s\", ",  svc_value("fluxmin"));
   printf("\"sigmamin\": \"%s\", ", svc_value("sigmamin"));
   printf("\"xmin\": \"%s\", ",     svc_value("xmin"));
   printf("\"ymin\": \"%s\", ",     svc_value("ymin"));
   printf("\"ramin\": \"%s\", ",    svc_value("ramin"));
   printf("\"decmin\": \"%s\", ",   svc_value("decmin"));
   printf("\"fluxmax\": \"%s\", ",  svc_value("fluxmax"));
   printf("\"sigmamax\": \"%s\", ", svc_value("sigmamax"));
   printf("\"xmax\": \"%s\", ",     svc_value("xmax"));
   printf("\"ymax\": \"%s\", ",     svc_value("ymax"));
   printf("\"ramax\": \"%s\", ",    svc_value("ramax"));
   printf("\"decmax\": \"%s\"}",    svc_value("decmax"));

   fflush(stdout);

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
