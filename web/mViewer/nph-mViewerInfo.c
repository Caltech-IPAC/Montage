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

#define STRLEN 4096

void  printError(char *errmsg);

int debug = 0;

FILE *fdebug;


/*******************************************************************/
/*                                                                 */
/*  This service is a utility for getting/formatting FITS header,  */
/*  table column lists and optional label aliases returned for use */
/*  by the IcePlotter GUI.                                         */
/*                                                                 */
/*******************************************************************/


int main(int argc, char *argv[], char *envp[])
{
   int   i, j, nkey, ncols, pid, first, firstCat, ncat;
   int   iname, ilabel, ioffset, itable, icolumn, update;

   char  wspace      [STRLEN];
   char  prefix      [STRLEN];
   char  file        [STRLEN];
   char  directory   [STRLEN];
   char  workDir     [STRLEN];
   char  baseFile    [STRLEN];
   char  fileName    [STRLEN];
   char  colname     [STRLEN];
   char  prevname    [STRLEN];
   char  label       [STRLEN];
   char  offset      [STRLEN];
   char  tmpstr      [STRLEN];

   char  catName[256][STRLEN];


   /********************/
   /* Config variables */
   /********************/

   config_init((char *)NULL);

   if(config_exists("ISIS_WORKDIR"))
      strcpy(workDir, config_value("ISIS_WORKDIR"));
   else
      printError("No workspace directory.");


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

      sprintf (tmpstr, "/tmp/icePlotterInfo.debug_%d", pid);

      fdebug = fopen (tmpstr, "w+");
   }
 
   if(keyword_exists("ws"))
      strcpy(wspace, keyword_value("ws"));
   else
      printError("No workspace.");

   strcpy(prefix, "");
   if(keyword_exists("prefix"))
      strcpy(prefix, keyword_value("prefix"));

   update = 0;
   if(keyword_exists("update"))
      update = atoi(keyword_value("update"));

   if(debug)
   {
      fprintf(fdebug, "DEBUG> Config parameters:");

      fprintf(fdebug, "DEBUG> ISIS_WORKDIR = [%s]\n", workDir);
      fprintf(fdebug, "DEBUG> wspace       = [%s]\n", wspace);
      fprintf(fdebug, "DEBUG> prefix       = [%s]\n", prefix);
      fprintf(fdebug, "DEBUG> update       =  %d \n", update);
      fflush(fdebug);
   }



   /************************/
   /* Build directory name */
   /************************/

   strcpy(directory, workDir);
   strcat(directory, "/");
   strcat(directory, wspace);

   strcpy(baseFile, directory);
   strcat(baseFile, "/");
   strcat(baseFile, file);

   strcpy(fileName, baseFile);

   if(debug)
   {
      fprintf(fdebug, "DEBUG> directory = %s\n", directory);
      fprintf(fdebug, "DEBUG> baseFile  = %s\n", baseFile);
      fprintf(fdebug, "DEBUG> fileName  = %s\n", fileName);
      fflush(fdebug);
   }
      


   /************************/
   /* Retrieve information */
   /************************/


   // Special case: in update mode we just look for XXX_update.tbl

   if(update)
   {
      if(strlen(prefix) == 0)
         sprintf(fileName, "%s/updates.tbl", directory);
      else
         sprintf(fileName, "%s/%s_updates.tbl", directory, prefix);

      if(debug)
      {
         fprintf(fdebug, "DEBUG> updates   = %s\n", fileName);
         fflush(fdebug);
      }

      ncols = topen(fileName);

      if(ncols < 0)
         printError("Cannot open updates file.");

      itable  = tcol("table");
      icolumn = tcol("column");
      ilabel  = tcol("label");

      if(ilabel < 0)
         ilabel = icolumn;

      printf("HTTP/1.1 200 OK\r\n");
      printf("Content-type: text/plain\r\n\r\n");
      fflush(stdout);

      printf("[");
      fflush(stdout);

      first = 1;

      while(1)
      {
         if(tread() < 0)
            break;

         if(first)
         {
            printf(" {\"table\" : \"%s\", \"column\" : \"%s\", \"label\" : \"%s\"}", 
               tval(itable), tval(icolumn), tval(ilabel));
            
            first = 0;
         }
         else
         {
            printf(", {\"table\" : \"%s\", \"column\" : \"%s\", \"label\" : \"%s\"}", 
               tval(itable), tval(icolumn), tval(ilabel));
         }

         fflush(stdout);
      }

      printf("]\n");
      fflush(stdout);

      tclose();

      exit(0);
   }


   // The table list

   if(strlen(prefix) == 0)
      sprintf(fileName, "%s/tables.tbl", directory);
   else
      sprintf(fileName, "%s/%s_tables.tbl", directory, prefix);

   if(debug)
   {
      fprintf(fdebug, "DEBUG> tables    = %s\n", fileName);
      fflush(fdebug);
   }

   ncols = topen(fileName);

   if(ncols < 0)
      printError("Cannot open table list file.");

   iname = tcol("file");

   if(iname < 0)
      printError("Cannot find 'file' column in table list.");

   ilabel = tcol("label");

   if(ilabel < 0)
      ilabel = iname;


   printf("HTTP/1.1 200 OK\r\n");
   printf("Content-type: text/plain\r\n\r\n");
   fflush(stdout);

   printf("{");
   fflush(stdout);

   first = 1;

   ncat = 0;

   while(1)
   {   
      if(tread() < 0)
         break;

      strcpy(tmpstr, tval(iname));

      if(strlen(tmpstr) > 0)
      {
         if(first)
         {
            printf("\"tables\":[{\"name\":\"%s\", \"label\":\"%s\"}", 
               tval(iname), tval(ilabel));
            fflush(stdout);

            first = 0;
         }
         else
         {
            printf(",{\"name\":\"%s\", \"label\":\"%s\"}", 
               tval(iname), tval(ilabel));
            fflush(stdout);
         }

         strcpy(catName[ncat], tval(iname));
         ++ncat;
      }
   }

   if(ncat > 0)
   {
      printf("], ");
      fflush(stdout);
   }

   tclose();


   // If they exist, the column label strings

   if(strlen(prefix) == 0)
      sprintf(fileName, "%s/labels.tbl", directory);
   else
      sprintf(fileName, "%s/%s_labels.tbl", directory, prefix);

   if(debug)
   {
      fprintf(fdebug, "DEBUG> labels    = %s\n", fileName);
      fflush(fdebug);
   }

   ncols = topen(fileName);

   if(ncols > 0)
   {
      iname = tcol("name");

      if(iname < 0)
         printError("Cannot find 'name' column in columns list.");

      ilabel = tcol("label");

      if(ilabel < 0)
         ilabel = iname;

      ioffset = tcol("offsetlabel");


      // label strings

      strcpy(prevname, "");

      first = 1;

      while(1)
      {   
         if(tread() < 0)
            break;

         strcpy(colname, tval(iname));
         strcpy(label,   tval(ilabel));

         if(strlen(colname) == 0)
            continue;

         if(strlen(label) > 5 && strcmp(label+strlen(label)-5, "(val)") == 0)
                  *(label+strlen(label)-5) = '\0';

         if(strlen(colname) > 8 && strcmp(colname+strlen(colname)-8, "_display") == 0)
            *(colname+strlen(colname)-8) = '\0';
         
         else if(strlen(colname) > 4 && strcmp(colname+strlen(colname)-4, "_str") == 0)
            *(colname+strlen(colname)-4) = '\0';
         
         else if(strlen(colname) > 3 && strcmp(colname+strlen(colname)-3, "str") == 0)
            *(colname+strlen(colname)-3) = '\0';
         

         if(strcmp(colname, prevname) == 0)
            continue;

         if(first)
         {
            printf("\"labels\":{\"%s\":\"%s\"", colname, label);
            fflush(stdout);

            first = 0;
         }
         else
         {
            printf(",\"%s\":\"%s\"", colname, label);
            fflush(stdout);
         }

         strcpy(prevname, colname);
      }

      printf("}, ");
      fflush(stdout);


      // offset label control

      if(ioffset >= 0)
      {
         tseek(0);

         strcpy(prevname, "");

         first = 1;

         while(1)
         {   
            if(tread() < 0)
               break;

            strcpy(colname, tval(iname));
            strcpy(offset,  tval(ioffset));

            if(strlen(colname) == 0)
               continue;

            if(strlen(colname) > 8 && strcmp(colname+strlen(colname)-8, "_display") == 0)
               *(colname+strlen(colname)-8) = '\0';
            
            else if(strlen(colname) > 4 && strcmp(colname+strlen(colname)-4, "_str") == 0)
               *(colname+strlen(colname)-4) = '\0';
            
            else if(strlen(colname) > 3 && strcmp(colname+strlen(colname)-3, "str") == 0)
               *(colname+strlen(colname)-3) = '\0';
            

            if(strcmp(colname, prevname) == 0)
               continue;

            if(first)
            {
               printf("\"offset\":{\"%s\":\"%s\"", colname, offset);
               fflush(stdout);

               first = 0;
            }
            else
            {
               printf(",\"%s\":\"%s\"", colname, offset);
               fflush(stdout);
            }

            strcpy(prevname, colname);
         }

         printf("}, ");
         fflush(stdout);
      }

      tclose();
   }



   // And finally for each catalog, the column list

   if(ncat > 0)
   {
      printf("\"columns\":{");
      fflush(stdout);

      firstCat = 1;

      for(i=0; i<ncat; ++i)
      {
         sprintf(baseFile, "%s/%s", directory, catName[i]);

         if(debug)
         {
            fprintf(fdebug, "\nDEBUG> catalog header: [%s][%s]\n", catName[i], baseFile);
            fflush(stdout);
         }

         if(firstCat)
         {
            printf("\"%s\":", catName[i]);
            fflush(stdout);

            firstCat = 0;
         }
         else
         {
            printf(",\"%s\":", catName[i]);
            fflush(stdout);
         }

         ncols = topen(baseFile);

         if(debug)
         {
            fprintf(fdebug, "DEBUG> file      = %s\n", fileName);
            fprintf(fdebug, "\nDEBUG> ncols     = %d\n", ncols);
            fflush(stdout);
         }

         first = 1;

         if(ncols<= 0)
         {
            printf("[]");
            fflush(stdout);
            continue;
         }

         printf("[");
         fflush(stdout);

         for(j=0; j<ncols; ++j)
         {
            if(debug)
            {
               fprintf(fdebug, "\nDEBUG> cols[%d] = [%s](%s)\n", j, tbl_rec[j].name, tbl_rec[j].type);
               fflush(stdout);
            }

            if(first)
            {
               printf("[\"%s\",\"%s\"]", tbl_rec[j].name, tbl_rec[j].type);
               first = 0;
            }
            else
               printf(",[\"%s\",\"%s\"]", tbl_rec[j].name, tbl_rec[j].type);

            fflush(stdout);
         }

         printf("]");
         fflush(stdout);

         tclose();
      }

      printf("}");
      fflush(stdout);
   }

   printf("}");
   fflush(stdout);

   exit(0);
}


/**********************/
/* HTML Error message */
/**********************/

void printError(char *errmsg)
{
    printf("HTTP/1.1 200 OK\n");
    printf("Content-type: text/xml\r\n");
    printf ("\r\n");

    printf ("<error>%s</error>\n", errmsg);
    fflush (stdout);

   exit(0);
}
