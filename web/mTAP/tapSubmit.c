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
#include <curl/curl.h>

#define MAXSTR 100000

void   printError(char *errmsg);
size_t getLocation(void *ptr, size_t size, size_t nmemb, void *stream);

char locationStr[MAXSTR];

int debug = 0;

/*******************************************************/
/*                                                     */
/* Submit an ADQL query to the CDS VizieR TAP service. */
/*                                                     */
/*******************************************************/

int main(int argc, char **argv)
{
   char  url [MAXSTR];
   char  adql[MAXSTR];
   char  line[MAXSTR];

   CURL     *curl;
   CURLcode  res;


   if(argc > 1 && strcmp(argv[1], "-d") == 0)
      debug = 1;


   printf("[struct stat=\"OK\", msg=\"Enter query (terminated by semicolon)\"]\n");
   fflush(stdout);


   /* Because of the problems with quotes in     */
   /* script command arguments, we get the query */
   /* from stdin.                                */

   while(fgets(line, MAXSTR, stdin) != (char *)NULL)
   {
      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';

      strcat(adql, line);
      strcat(adql, " ");

      if(strstr(line, ";") != 0)
         break;

      printf("[struct stat=\"OK\", msg=\"Continue query (terminated by semicolon)\"]\n");
      fflush(stdout);
   }


   /* Construct a VizieR TAP URL */

   sprintf(url, "lang=adql&request=doQuery&PHASE=RUN&query=%s", url_encode(adql));

   if(debug)
   {
      printf("DEBUG> URL: [%s]\n", url);
      fflush(stdout);
   }


   /* Retrieve the URL using the cURL library POST. This is just the      */
   /* query submission so all we get back is a handle to poll for status. */

   curl_global_init(CURL_GLOBAL_ALL);

   curl = curl_easy_init();

   if(debug)
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

   curl_easy_setopt(curl, CURLOPT_URL, "http://tapvizier.u-strasbg.fr/TAPVizieR/tap/async");
   curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, getLocation);
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url);

   res = curl_easy_perform(curl);

   curl_easy_cleanup(curl);

   curl_global_cleanup();

   if(res != CURLE_OK)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", curl_easy_strerror(res));
      fflush(stdout);
      exit(0);
   }

   if(strlen(locationStr) == 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"No query handle returned.\"]\n");
      fflush(stdout);
      exit(0);
   }

   /* Finish up */

   printf("[struct stat=\"OK\", location=\"%s\"]\n", locationStr);
   fflush(stdout);

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


/**********************************************/
/* Callback for checking HTTP response header */
/**********************************************/

size_t getLocation(void *ptr, size_t size, size_t nmemb, void *stream)
{
   int   i;
   char  location[MAXSTR];
   char *begin, *end;
   
   for(i=0; i<nmemb; ++i)
      location[i] = *((char *)ptr+i);

   location[nmemb] = '\0';

   begin = strstr(location, "Location: ");

   if(begin != (char *)NULL)
   {
      begin += 10;

      while(*begin == ' ')
         ++begin;

      end = begin;

      while(*end != ' ' && *end != '\0' && *end != '\r' && *end != '\n')
         ++end;

      *end = '\0';

      strcpy(locationStr, begin);
   }

   return size * nmemb;
}
