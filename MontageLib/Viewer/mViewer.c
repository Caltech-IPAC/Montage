#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fitsio.h>
#include <wcs.h>

#include <mViewer.h>
#include <montage.h>

#define MAXSTR 32768
#define STRLEN 1024


/***************************/
/*  mViewer driver routine */
/***************************/

int main(int argc, char **argv)
{
   int i, debug, len;

   char cmdstr  [MAXSTR];
   char line    [STRLEN];
   char outFmt  [STRLEN];
   char outFile [STRLEN];
   char jsonFile[STRLEN];
   char jsonStr [MAXSTR];
   char fontFile[MAXSTR];

   char *rstat;

   FILE *fin;

   struct mViewerReturn *returnStruct;


   /*****************************************************/
   /* Scan through the command line parameters to pull  */
   /* out a few parameters (debug, output file, output  */
   /* format).  In a second pass, we process JSON stuff */
   /* specially but if we don't find JSON, we send all  */
   /* the command line as a command string.             */
   /*****************************************************/

   strcpy(outFmt,   "png");
   strcpy(outFile,  "");
   strcpy(fontFile, "");
   strcpy(jsonStr,  "");
   strcpy(line,     "");

   debug = 0;

   for(i=0; i<argc; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
         debug = 1;

      if(strcmp(argv[i], "-out") == 0
      || strcmp(argv[i], "-png") == 0)
      {
         if(i > argc-2)
         {
            printf("[struct stat=\"ERROR\", msg=\"No PNG output file argument.\"]\n");
            fflush(stdout);
            exit(0);
         }

         strcpy(outFmt, "png");
         strcpy(outFile, argv[i+1]);

         ++i;
      }

      if(strcmp(argv[i], "-jpeg") == 0)
      {
         if(i > argc-2)
         {
            printf("[struct stat=\"ERROR\", msg=\"No JPEG output file argument.\"]\n");
            fflush(stdout);
            exit(0);
         }

         strcpy(outFmt, "jpeg");
         strcpy(outFile, argv[i+1]);

         ++i;
      }

      if(strcmp(argv[i], "-json") == 0)
      {
         if(i > argc-2)
         {
            printf("[struct stat=\"ERROR\", msg=\"No input JSON data argument.\"]\n");
            fflush(stdout);
            exit(0);
         }

         strcpy(jsonStr, argv[i+1]);

         ++i;
      }

      if(strcmp(argv[i], "-jfile") == 0)
      {
         if(i > argc-2)
         {
            printf("[struct stat=\"ERROR\", msg=\"No input JSON file argument.\"]\n");
            fflush(stdout);
            exit(0);
         }

         strcpy(jsonFile, argv[i+1]);

         ++i;
      }

      if(strcmp(argv[i], "-fontfile") == 0)
      {
         if(i > argc-2)
         {
            printf("[struct stat=\"ERROR\", msg=\"No alternate font file argument.\"]\n");
            fflush(stdout);
            exit(0);
         }

         strcpy(fontFile, argv[i+1]);

         ++i;
      }
   }


   /*************/
   /* JSON mode */
   /*************/

   if(strlen(jsonStr) > 0)
   {
      returnStruct = mViewer(jsonStr, outFile, JSONMODE, outFmt, fontFile, debug);

      if(returnStruct->status == 1)
      {
         printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
         exit(1);
      }
      else
      {
         printf("[struct stat=\"OK\", module=\"mViewer\", %s]\n", returnStruct->msg);
         exit(0);
      }
   }


   /******************/
   /* JSON file mode */
   /******************/

   if(strlen(jsonFile) > 0)
   {
      fin = fopen(jsonFile, "r");
      
      if(fin == (FILE *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Failed to open JSON file.\"]\n");
         exit(1);
      }
      
      strcpy(jsonStr, "");

      while(1)
      {
         rstat = fgets(line, STRLEN, fin);

         if(rstat == (char *)NULL)
            break;

         len = strlen(line) - 1;

         while(1)
         {
            if(line[len] == '\n' || line[len] == '\r')
            {
               line[len]  = '\0';
               --len;

               if(len < 0)
                  break;
            }
            else
               break;
         }

         strcat(jsonStr, line);
         strcat(jsonStr, " ");
      }

      returnStruct = mViewer(jsonStr, outFile, JSONMODE, outFmt, fontFile, debug);

      if(returnStruct->status == 1)
      {
         printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
         fflush(stdout);
         exit(1);
      }
      else
      {
         printf("[struct stat=\"OK\", module=\"mViewer\", %s]\n", returnStruct->msg);
         fflush(stdout);
         exit(0);
      }
   }


   /************************************************/
   /* Finally, assume the arguments are individual */
   /* on the command line.  Put the command-line   */
   /* arguments back together into a single string */
   /************************************************/

   strcpy(cmdstr, "");

   for(i=1; i<argc; ++i)
   {
      if(i > 1)
         strcat(cmdstr, " ");

      strcat(cmdstr, "\"");
      strcat(cmdstr, argv[i]);
      strcat(cmdstr, "\"");
   }


   /***************************************/
   /* Call the mViewer processing routine */
   /***************************************/

   returnStruct = mViewer(cmdstr, outFile, CMDMODE, outFmt, fontFile, debug);

   if(returnStruct->status == 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
      exit(1);
   }
   else
   {
      printf("[struct stat=\"OK\", module=\"mViewer\", %s]\n", returnStruct->msg);
      exit(0);
   }
}
