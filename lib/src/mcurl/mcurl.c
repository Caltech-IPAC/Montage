#include <mcurl.h>

int mcurl(char *url, char *outFile, int timeout, double *size, char *msg)
{
   long      responseCode;
   FILE     *fdata;

   CURL     *curl;
   CURLcode  res;

   strcpy(msg, "Successful download.");


   /*********************************************/
   /* Initialize the CURL HTTP/HTTPS connection */
   /*********************************************/

   curl_global_init(CURL_GLOBAL_ALL);

   curl = curl_easy_init();
 
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT,       (long)timeout);


   /*****************************/
   /* Open the output data file */
   /*****************************/

   fdata = fopen(outFile, "w+");

   if(fdata == (FILE *)NULL)
   {
      strcpy(msg, "Error opening output file.");

      fflush(fdata);
      fclose(fdata);

      return 1;
   }

   curl_easy_setopt(curl, CURLOPT_WRITEDATA, fdata);


   /***************************/
   /* Tell CURL about the URL */
   /***************************/

   curl_easy_setopt(curl, CURLOPT_URL, url);


   /*************************/
   /* And perform the query */
   /*************************/

   res = curl_easy_perform(curl);

   if (res != CURLE_OK) 
   {
      if (res == CURLE_OPERATION_TIMEDOUT)
         strcpy(msg, "Timeout retrieving URL.");
      else
         strcpy(msg, "Error retrieving URL.");

      fflush(fdata);
      fclose(fdata);

      return 1;
   }


   /************************/
   /* Get info on response */
   /************************/

   curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD,  size);
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);


   /*****************/
   /* And finish up */
   /*****************/

   curl_easy_cleanup(curl);

   fflush(fdata);
   fclose(fdata);

   return 0;
}
