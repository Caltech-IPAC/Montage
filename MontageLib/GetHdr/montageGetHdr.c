/* Module: mGetHdr.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.5      John Good        25Nov03  Added extern optarg references
1.4      John Good        25Aug03  Added status file processing
1.3      John Good        30Apr03  Added checkFile() reference for infile  
1.2      John Good        14Mar03  Error: was checking for a FITS
                                   header in the wrong file
1.1      John Good        14Mar03  Modified command-line processing
                                   to use getopt() library
1.0      John Good        07Feb03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#include <www.h>
#include <fitsio.h>

#include <mGetHdr.h>
#include <montage.h>

#define NORMAL  0
#define COM     1
#define WCS     2

static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mGetHdr                                                              */
/*                                                                       */
/*  This program extracts the FITS header from an image into a text file */
/*                                                                       */
/*   char  *infile         Input FITS file                               */
/*   int    hdu            Optional HDU offset for input file            */
/*   char  *hdrfile        ASCII header stripped from the input, with    */
/*                         newlines added for readability                */
/*                                                                       */
/*   int    htmlMode       Write output as an HTML file                  */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mGetHdrReturn *mGetHdr(char *infile, int hdu, char *hdrfile, int htmlMode, int debug)
{
   int i, j, odd, class, morekeys, status;

   char fileBase[1024];
   char line    [1024];
   char keyword [1024];

   char *ptr;

   fitsfile *infptr;

   int       ncard;
   char      card[256];

   FILE     *fout;

   status = 0;

   struct mGetHdrReturn *returnStruct;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mGetHdrReturn *)malloc(sizeof(struct mGetHdrReturn));

   bzero((void *)returnStruct, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /****************************/
   /* Check for the input file */
   /****************************/

   if(montage_checkFile(infile) != 0)
   {
      sprintf(returnStruct->msg, "Image file (%s) does not exist", infile);
      return returnStruct;
   }

   /**********************/
   /* Open the FITS file */
   /**********************/

   fout = fopen(hdrfile, "w+");

   if(fout == (FILE *)NULL)
   {
      sprintf(returnStruct->msg, "Can't open output header file.");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /******************************************/
   /* Open the FITS file and read the header */
   /******************************************/

   if(fits_open_file(&infptr, infile, READONLY, &status))
   {
      mGetHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(hdu > 0)
   {
      if(fits_movabs_hdu(infptr, hdu+1, NULL, &status))
      {
         mGetHdr_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }

   if(fits_get_hdrspace(infptr, &ncard, &morekeys, &status))
   {
      mGetHdr_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(debug)
   {
      printf("DEBUG> %d cards\n", ncard);
      fflush(stdout);
   }

   if(htmlMode == 1)
   {
      fprintf(fout, "<html>\n");

      fprintf(fout, "<head>\n");
      fprintf(fout, "<style>\n");
      fprintf(fout, "   .fitsBody {\n");
      fprintf(fout, "      background-color:#D8D8D8;\n");
      fprintf(fout, "      font-size: 20px;\n");
      fprintf(fout, "      font-style: normal;\n");
      fprintf(fout, "      font-weight: normal;\n");
      fprintf(fout, "      font-family: \"Times New Roman\", Times, serif;\n");
      fprintf(fout, "   }\n");
      fprintf(fout, "\n");
      fprintf(fout, "   .fitsHdr {\n");
      fprintf(fout, "      display: inline-block;\n");
      fprintf(fout, "      height: 500px;\n");
      fprintf(fout, "      overflow-y: scroll;\n");
      fprintf(fout, "      padding: 5px;\n");
      fprintf(fout, "      border: 1px solid black;\n");
      fprintf(fout, "   }\n");
      fprintf(fout, "\n");
      fprintf(fout, "   .fitsOdd {\n");
      fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
      fprintf(fout, "      font: 12px courier,sans-serif;\n");
      fprintf(fout, "      background-color:#F0F8FF;\n");
      fprintf(fout, "   }\n");
      fprintf(fout, "\n");
      fprintf(fout, "   .fitsEven {\n");
      fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
      fprintf(fout, "      font: 12px courier,sans-serif;\n");
      fprintf(fout, "      background-color:#FFFFFF;\n");
      fprintf(fout, "   }\n");
      fprintf(fout, "\n");
      fprintf(fout, "   .fitsOddWCS {\n");
      fprintf(fout, "      color: #0000A0;\n");
      fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
      fprintf(fout, "      font: 12px courier,sans-serif;\n");
      fprintf(fout, "      background-color:#F0F8E8;\n");
      fprintf(fout, "   }\n");
      fprintf(fout, "\n");
      fprintf(fout, "   .fitsEvenWCS {\n");
      fprintf(fout, "      color: #0000A0;\n");
      fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
      fprintf(fout, "      font: 12px courier,sans-serif;\n");
      fprintf(fout, "      background-color:#FFFFE8;\n");
      fprintf(fout, "   }\n");
      fprintf(fout, "\n");
      fprintf(fout, "   .fitsOddCOM {\n");
      fprintf(fout, "      color: #A00000;\n");
      fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
      fprintf(fout, "      font: 12px courier,sans-serif;\n");
      fprintf(fout, "      background-color:#F0F8FF;\n");
      fprintf(fout, "   }\n");
      fprintf(fout, "\n");
      fprintf(fout, "   .fitsEvenCOM {\n");
      fprintf(fout, "      color: #A00000;\n");
      fprintf(fout, "      margin: 0px 0px 0px 0px;\n");
      fprintf(fout, "      font: 12px courier,sans-serif;\n");
      fprintf(fout, "      background-color:#FFFFFF;\n");
      fprintf(fout, "   }\n");
      fprintf(fout, "</style>\n");
      fprintf(fout, "</head>\n");
      fprintf(fout, "\n");
      fprintf(fout, "<body class=\"fitsBody\">\n");
      fprintf(fout, "<b><br/>&nbsp;&nbsp;%s</b><br/>\n", fileBase);
      fprintf(fout, "\n");
      fprintf(fout, "<div class=\"fitsHdr\">\n");
   }



   odd = 0;

   for(i=1; i<=ncard; ++i)
   {
      fits_read_record(infptr, i, card, &status);

      for(j=(int)strlen(card)-1; j>=0; --j)
      {
         if(card[j] != ' ')
            break;

         card[j] = '\0';
      }


      // If in HTML mode, we need some formatting

      if(htmlMode)
      {
         strcpy(line, card);

         class = NORMAL;

         if(strncmp(line, "COMMENT", 7) == 0)
            class = COM;

         else
         {
            strcpy(keyword, line);

            ptr = strstr(keyword, "=");

            if(ptr)
               *ptr = '\0';

            ptr = keyword + strlen(keyword);

            while(ptr > keyword && (*ptr == '\0' || *ptr == ' ' || *ptr == '\n'))
            {
               *ptr = '\0';
               --ptr;
            }

            if(strncmp(keyword, "NAXIS", 5) == 0
            || strncmp(keyword, "CTYPE", 5) == 0
            || strncmp(keyword, "CRVAL", 5) == 0
            || strncmp(keyword, "CRPIX", 5) == 0
            || strncmp(keyword, "CDELT", 5) == 0
            || strncmp(keyword, "CROTA", 5) == 0
            || strncmp(keyword, "CD",    2) == 0
            || strncmp(keyword, "PC",    2) == 0)
               class = WCS;
         }

         if(odd)
         {
            if(class == WCS)
               fprintf(fout, "<pre class=\"fitsOddWCS\" > %s </pre>\n", html_encode(line));

            else if(class == COM)
               fprintf(fout, "<pre class=\"fitsOddCOM\" > %s </pre>\n", html_encode(line));

            else
               fprintf(fout, "<pre class=\"fitsOdd\"    > %s </pre>\n", html_encode(line));
         }
         else
         {
            if(class == WCS)
               fprintf(fout, "<pre class=\"fitsEvenWCS\"> %s </pre>\n", html_encode(line));

            else if(class == COM)
               fprintf(fout, "<pre class=\"fitsEvenCOM\"> %s </pre>\n", html_encode(line));

            else
               fprintf(fout, "<pre class=\"fitsEven\"   > %s </pre>\n", html_encode(line));
         }

         odd = (odd+1)%2;
      }
      else
         fprintf(fout, "%s\n", card);

      fflush(fout);

      if(debug)
      {
         printf("card %3d: [%s]\n", i, card);
         fflush(stdout);
      }
   }


   // Closing up

   if(htmlMode)
   {
      if(odd)
         fprintf(fout, "<pre class=\"fitsOdd\"    > END </pre>\n");
      else
         fprintf(fout, "<pre class=\"fitsEven\"   > END </pre>\n");

      if(htmlMode == 1)
      {
         fprintf(fout, "</div>\n");
         fprintf(fout, "</body>\n");
         fprintf(fout, "</html>\n");
      }
   }
   else
      fprintf(fout, "END\n");

   fflush(fout);
   fclose(fout);

   fits_close_file(infptr, &status);
   
   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "ncard=%d", ncard);
   sprintf(returnStruct->json, "{\"ncard\":%d}", ncard);

   returnStruct->ncard = ncard;

   return returnStruct;
}



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mGetHdr_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);

   return;
}
