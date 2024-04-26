#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fitsio.h>
#include <wcs.h>

#include <mMakeImg.h>
#include <montage.h>

#define MAXSTR 32768


/***************************/
/* mMakeImg driver routine */
/***************************/

int main(int argc, char **argv)
{
   int i, debug, len, position;

   char cmdstr  [MAXSTR];
   char line    [STRLEN];
   char template[STRLEN];
   char outFile [STRLEN];
   char jsonFile[STRLEN];
   char jsonStr [MAXSTR];

   char *rstat;

   FILE *fin;

   struct mMakeImgReturn *returnStruct;


   /*****************************************************/
   /* Scan through the command line parameters to pull  */
   /* out a few parameters (debug, output file, output  */
   /* format).  In a second pass, we process JSON stuff */
   /* specially but if we don't find JSON, we send all  */
   /* the command line as a command string.             */
   /*****************************************************/

   strcpy(outFile,  "");
   strcpy(jsonStr,  "");
   strcpy(line,     "");

   debug = 0;

   for(i=1; i<argc-2; ++i)
   {
      if(strcmp(argv[i], "-d") == 0)
      {
         debug = atoi(argv[i+1]);
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
   }

   if(i > argc-1)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: mMakeImg [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tblfile col width csys epoch refval mag/flux flat/gaussian] [-i imagetbl refval] [-a array.txt] template.hdr out.fits (-t and -i args can be repeated)\"]\n");
      fflush(stdout);
      exit(0);
   }

   position = argc - 2;

   strcpy(template, argv[position]);
   strcpy(outFile,  argv[position+1]);


   /*************/
   /* JSON mode */
   /*************/

   if(strlen(jsonStr) > 0)
   {
      returnStruct = mMakeImg(template, outFile, jsonStr, JSONMODE, debug);

      if(returnStruct->status == 1)
      {
         printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
         exit(1);
      }
      else
      {
         printf("[struct stat=\"OK\", module=\"mMakeImg\", %s]\n", returnStruct->msg);
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
      
      returnStruct = mMakeImg(template, outFile, jsonStr, JSONMODE, debug);

      if(returnStruct->status == 1)
      {
         printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
         fflush(stdout);
         exit(1);
      }
      else
      {
         printf("[struct stat=\"OK\", module=\"mMakeImg\", %s]\n", returnStruct->msg);
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

   for(i=1; i<position; ++i)
   {
      if(i > 1)
         strcat(cmdstr, " ");

      strcat(cmdstr, "\"");
      strcat(cmdstr, argv[i]);
      strcat(cmdstr, "\"");
   }


   /***************************************/
   /* Call the mMakeImg processing routine */
   /***************************************/

   returnStruct = mMakeImg(template, outFile, cmdstr, CMDMODE, debug);

   if(returnStruct->status == 1)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
      exit(1);
   }
   else
   {
      printf("[struct stat=\"OK\", module=\"mMakeImg\", %s]\n", returnStruct->msg);
      exit(0);
   }
}
