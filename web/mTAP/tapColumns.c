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

#define MAXSTR 1024


void printError(char *errmsg);
int  isblank   (int c);

int debug = 0;

/******************************************************/
/*                                                    */
/* Use the CDS VizieR TAP server to retrieve a list   */
/* of the columns for a given catalog.                */
/*                                                    */
/******************************************************/

int main(int argc, char **argv)
{
   int   i, j, k, n, ntables, ncol, nflag;

   char  status     [16];
   char  xmlfile    [MAXSTR];
   char  url        [MAXSTR];
   char  tmpname    [MAXSTR];
   char  catname    [MAXSTR];
   char  filename   [MAXSTR];
   char  directory  [MAXSTR];
   char  cmd        [MAXSTR];
   char  tag        [MAXSTR];
   char  tblname    [MAXSTR];
   char  tabfile    [MAXSTR];
   char  tblfile    [MAXSTR];
   char  flagval    [MAXSTR];
   char  name       [MAXSTR];
   char  description[MAXSTR];
   char  unit       [MAXSTR];
   char  utype      [MAXSTR];
   char  ucd        [MAXSTR];
   char  dataType   [MAXSTR];
   char  indexed    [MAXSTR];
   char  primary    [MAXSTR];
   char  table      [MAXSTR][132];

   FILE *fout;
   FILE *ftab;

   CURL     *curl;
   CURLcode  res;


   if (argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: tapColumns <catalog name> <ddfile name>\"]\n");
      fflush(stdout);
      exit(0);
   }

   if(strcmp(argv[1], "-d") == 0)
   {
      debug = 1;

      svc_debug(stdout);

      --argc;
      ++argv;
   }

   if (argc < 3)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: tapColumns <catalog name> <ddfile dir>\"]\n");
      fflush(stdout);
      exit(0);
   }


   /* Two arguments: the catalog name and the output file name */

   strcpy(tmpname,   argv[1]);
   strcpy(directory, argv[2]);


   if(mkdir(directory, 0775) < 0)
   {
      if(errno != EEXIST)
         printError("Cannot create UID workspace subdirectory.");
   }

   n = 0;

   for(i=0; i<strlen(tmpname); ++i)
   {
      if(!isblank(tmpname[i]))
      {
         catname[n] = tmpname[i];

         filename[n] = tmpname[i];
         
         if(filename[n] == '/')
            filename[n]  = '_';
         
         ++n;
      }
   }

   catname[n] = '\0';

   if(strlen(catname) == 0)
         printError("No catalog name given.");


   /* Construct a VizieR TAP URL */

   sprintf(url, "http://tapvizier.u-strasbg.fr/TAPVizieR/tap/tables/%s", catname);

   if(debug)
   {
      printf("DEBUG> URL: [%s]\n", url);
      fflush(stdout);
   }


   /* and a temporary XML file (based on the output file) */

   strcpy(xmlfile, directory);
   strcat(xmlfile, "/");
   strcat(xmlfile, filename);
   strcat(xmlfile, ".xml");
   
   if(debug)
   {
      printf("DEBUG> catname:   [%s]\n", catname);
      printf("DEBUG> directory: [%s]\n", directory);
      fflush(stdout);
   }


   /* Open the output file */

   fout = fopen(xmlfile, "w+");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Error opening output data file\"]\n");
      fflush(stdout);
      exit(0);
   }


   /* Retrieve the catalog column XML table */

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


   /* Initialize the library by opening and */
   /* parsing the URL return file           */

   ntables = 0;
   ncol = 0;

   if(xmlinfo_init(xmlfile) == 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", xmlinfo_error());
      fflush(stdout);
      exit(0);
   }


   /* Extract the pertinent return information */

   ntables = xmlinfo_count("tableset.schema.table");

   if(debug)
   {
      printf("DEBUG> ntables:    %d \n", ntables);
      fflush(stdout);
   }

   for(i=0; i<ntables; ++i)
   {
      sprintf(tag, "tableset.schema.table[%d].name", i);

      strcpy(tblname, xmlinfo_value(tag));

      strcpy(tmpname, tblname);

      for(j=0; j<strlen(tmpname); ++j)
         if(tmpname[j] == '/')
            tmpname[j]  = '_';

      sprintf(tabfile, "%s/%s.tab", directory, tmpname);
      sprintf(tblfile, "%s/%s.tbl", directory, tmpname);

      sprintf(table[i], "%s.tbl", tmpname);

      if(debug)
      {
         printf("DEBUG> ntable[%d]: [%s] -> file: [%s]/[%s]\n", 
            i, tblname, tabfile, tblfile);
         fflush(stdout);
      }

      sprintf(tag, "tableset.schema.table[%d].column", i);

      ncol = xmlinfo_count(tag);

      if(debug)
      {
         printf("DEBUG> ncol:      %d\n", ncol);
         fflush(stdout);
      }

      ftab = fopen(tabfile, "w+");

      fprintf(ftab, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
         "name", "description", "unit", "utype", "ucd", "dataType", "indexed", "primary");

      for(j=0; j<ncol; ++j)
      {
         strcpy(name,        "");
         strcpy(description, "");
         strcpy(unit,        "");
         strcpy(utype,       "");
         strcpy(ucd,         "");
         strcpy(dataType,    "");
         strcpy(indexed,     "");
         strcpy(primary,     "");

         sprintf(tag, "tableset.schema.table[%d].column[%d].name", i, j);
         
         if(xmlinfo_value(tag))
            strcpy(name, xmlinfo_value(tag));


         sprintf(tag, "tableset.schema.table[%d].column[%d].description", i, j);
         
         if(xmlinfo_value(tag))
            strcpy(description, xmlinfo_value(tag));

         sprintf(tag, "tableset.schema.table[%d].column[%d].unit", i, j);
         
         if(xmlinfo_value(tag))
            strcpy(unit, xmlinfo_value(tag));

         sprintf(tag, "tableset.schema.table[%d].column[%d].utype", i, j);
         
         if(xmlinfo_value(tag))
            strcpy(utype, xmlinfo_value(tag));

         sprintf(tag, "tableset.schema.table[%d].column[%d].ucd", i, j);
         
         if(xmlinfo_value(tag))
            strcpy(ucd, xmlinfo_value(tag));

         sprintf(tag, "tableset.schema.table[%d].column[%d].dataType", i, j);
         
         if(xmlinfo_value(tag))
            strcpy(dataType, xmlinfo_value(tag));

         sprintf(tag, "tableset.schema.table[%d].column[%d].flag", i, j);
         
         nflag = xmlinfo_count(tag);

         strcpy(indexed, "false");
         strcpy(primary, "false");

         for(k=0; k<nflag; ++k)
         {
            sprintf(tag, "tableset.schema.table[%d].column[%d].flag[%d]", i, j, k);

            strcpy(flagval, "");
            if(xmlinfo_value(tag))
               strcpy(flagval, xmlinfo_value(tag));

            if(strcmp(flagval, "indexed") == 0)
               strcpy(indexed, "true");

            if(strcmp(flagval, "primary") == 0)
               strcpy(primary, "true");
         }

         fprintf(ftab, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
            name, description, unit, utype, ucd, dataType, indexed, primary);
         fflush(ftab);
      }
   }

   xmlinfo_close();


   /* Convert tab file to a table file */

   sprintf(cmd, "tab2tbl -h 1 %s %s", tabfile, tblfile);

   if(debug)
   {
      printf("DEBUG> cmd: [%s]\n", cmd);
      fflush(stdout);
   }

   svc_run(cmd);

   strcpy( status, svc_value( "stat" ));

   if(strcmp( status, "ERROR") == 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", svc_value("msg"));
      fflush(stdout);
      exit(0);
   }

   unlink(tabfile);


   /* Finish up */

   printf("[struct stat=\"OK\", ntbl=%d, tables=[array ", ntables);

   for(i=0; i<ntables; ++i)
   {
      if(i == 0)
         printf("\"%s\"", table[i]);
      else
         printf(", \"%s\"", table[i]);
   }

   printf("]]\n");

   fflush(stdout);
   exit(0);
}


void printError(char *errmsg)
{
   printf ("[struct stat=\"ERROR\", msg=\"%s\"]\n", errmsg);
   fflush (stdout);
   exit(0);
}
