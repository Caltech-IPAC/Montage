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

#define MAXSTR 32768

size_t getLocation(void *ptr, size_t size, size_t nmemb, void *stream);

char locationStr[MAXSTR];

int debug = 0;

/*******************************************************/
/*                                                     */
/* Retrieve the results of a VizieR TAP service query. */
/*                                                     */
/*******************************************************/

int main(int argc, char **argv)
{
   char  url    [MAXSTR];
   char  ref    [MAXSTR];
   char  outfile[MAXSTR];

   FILE     *fout;

   CURL     *curl;
   CURLcode  res;


   if (argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: tapResults <ref> <outfile>\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(strcmp(argv[1], "-d") == 0)
   {
      debug = 1;

      --argc;
      ++argv;
   }

   if (argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: tapResults <ref> <outfile>\"]\n");
      fflush(stdout);
      exit(0);
   }


   /* Two arguments: the query ID and output file */

   strcpy(ref,     argv[1]);
   strcpy(outfile, argv[2]);



   /* Construct a VizieR TAP URL */

   sprintf(url, "http://tapvizier.u-strasbg.fr/TAPVizieR/tap/async/%s/results/result", ref);

   if(debug)
   {
      printf("DEBUG> URL: [%s]\n", url);
      fflush(stdout);
   }


   /* Open the output file */

   fout = fopen(outfile, "w+");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Error opening output data file\"]\n");
      fflush(stdout);
      exit(0);
   }


   /* Retrieve the query results table */

   curl_global_init(CURL_GLOBAL_ALL);

   curl = curl_easy_init();

   if(debug)
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

   curl_easy_setopt(curl, CURLOPT_URL, url);

   curl_easy_setopt(curl, CURLOPT_WRITEDATA, fout);

   res = curl_easy_perform(curl);

   curl_easy_cleanup(curl);

   curl_global_cleanup();

   if(res != CURLE_OK)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", curl_easy_strerror(res));
      fflush(stdout);
      exit(0);
   }

   fclose(fout);


   /* Finish up */

   printf("[struct stat=\"OK\"]\n");
   fflush(stdout);
   exit(0);
}
