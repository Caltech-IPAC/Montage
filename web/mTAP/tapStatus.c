#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <svc.h>
#include <www.h>
#include <xmlinfo.h>
#include <curl/curl.h>

#define MAXSTR 32768

void   printError(char *errmsg);
size_t getLocation(void *ptr, size_t size, size_t nmemb, void *stream);

char locationStr[MAXSTR];

int debug = 0;

/*******************************************************/
/*                                                     */
/* Check the status of a CDS VizieR TAP service query. */
/*                                                     */
/*******************************************************/

int main(int argc, char **argv)
{
   char  url    [MAXSTR];
   char  ref    [MAXSTR];
   char  status [MAXSTR];
   char  xmlfile[MAXSTR];

   FILE     *fout;

   CURL     *curl;
   CURLcode  res;

   if (argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: tapStatus <ref>\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(strcmp(argv[1], "-d") == 0)
   {
      debug = 1;

      --argc;
      ++argv;
   }

   if (argc < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: tapStatus <ref>\"]\n");
      fflush(stdout);
      exit(0);
   }


   /* One argument: the query ID */

   strcpy(ref, argv[1]);



   /* Construct a VizieR TAP URL */

   sprintf(url, "http://tapvizier.u-strasbg.fr/TAPVizieR/tap/async/%s", ref);

   if(debug)
   {
      printf("DEBUG> URL: [%s]\n", url);
      fflush(stdout);
   }


   /* And a temporary file for the XML returned. */

   strcpy(xmlfile, "/tmp/TAPStatusXXXXXX");
   mkstemp(xmlfile);

   fout = fopen(xmlfile, "w+");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Error opening output data file\"]\n");
      fflush(stdout);
      exit(0);
   }


   /* Retrieve the URL using the cURL library POST. This is just the      */
   /* query submission so all we get back is a handle to poll for status. */

   curl_global_init(CURL_GLOBAL_ALL);

   curl = curl_easy_init();

   if(debug)
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

   curl_easy_setopt(curl, CURLOPT_URL, url);

   curl_easy_setopt(curl, CURLOPT_WRITEDATA, fout);

   res = curl_easy_perform(curl);

   curl_easy_cleanup(curl);

   curl_global_cleanup();

   fclose(fout);

   if(res != CURLE_OK)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", curl_easy_strerror(res));
      fflush(stdout);
      exit(0);
   }


   /* Initialize the library by opening and */
   /* parsing the URL return file           */

   strcpy(status, "");

   if(xmlinfo_init(xmlfile) == 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", xmlinfo_error());
      fflush(stdout);
      exit(0);
   }


   /* Extract the pertinent return information */

   if(!xmlinfo_value("uws:job.uws:phase"))
   {
      printf("[struct stat=\"ERROR\", msg=\"No such job.\"]\n");
      fflush(stdout);
      exit(0);
   }

   strcpy(status, xmlinfo_value("uws:job.uws:phase"));

   if(!debug)
      unlink(xmlfile);


   /* Finish up */

   printf("[struct stat=\"OK\", status=\"%s\"]\n", status);
   fflush(stdout);
   exit(0);
}


/****************/
/* Error return */
/****************/

void printError(char *errmsg)
{
   printf ("[struct stat=\"ERROR\", msg=\"%s\"]\n", errmsg);
   fflush (stdout);
   exit(0);
}
