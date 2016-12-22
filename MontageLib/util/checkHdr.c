/* Module: checkHdr.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
3.0      John Good        17Nov14  Cleanup to avoid compiler warnings, in preparation
                                   for new development cycle.
2.3      John Good        07Oct07  Add explicit hdrflag=2 check 
                                   (could be either FITS or HDR)
2.2      John Good        25Sep07  Added getWCS() call to return WCS structure pointer
2.1      John Good        28Apr05  Allowed realloc of mHeader string.
2.0      John Good        27Mar05  Added HDU support.
1.10     John Good        21Sep04  That last update was a mistake.  mHeader is
                                   sometimes needed externally (e.g. mCoverageCheck)
1.9      John Good        21Sep04  Free mHeader space when no longer needed.
1.8      John Good        11Nov03  Added file existence check if fits
                                   open fails.
1.7      John Good        25Aug03  Added status file processing
1.6      John Good        24May03  Added getHdr() call to return header
                                   "string". 
1.5.1    A. C. Laity      30Jun03  Added code documentation for fitsCheck
                                   and strAdd
1.5      John Good        29Apr03  Added check for failure when file is
                                   FITS format but fails FITS open
1.4      John Good        15Apr03  Added special check for DSS headers
                                   (partial; just look for PLTRAH)
1.3      John Good        08Apr03  Also remove <CR> from template lines
1.2      John Good        22Mar03  Renamed wcsCheck to checkWCS for
                                   consistency
1.1      John Good        19Mar03  Added limit on template file size
                                   (wcsinit() was choking).
1.0      John Good        13Mar03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <fitsio.h>
#include <wcs.h>

#include <montage.h>


#define MAXHDR 80000

#define FITS   0
#define HDR    1
#define EITHER 2

static int havePLTRAH;

static int haveSIMPLE;
static int haveBITPIX;
static int haveNAXIS;
static int haveNAXIS1;
static int haveNAXIS2;
static int haveCTYPE1;
static int haveCTYPE2;
static int haveCRPIX1;
static int haveCRPIX2;
static int haveCRVAL1;
static int haveCRVAL2;
static int haveCDELT1;
static int haveCDELT2;
static int haveCD1_1;
static int haveCD1_2;
static int haveCD2_1;
static int haveCD2_2;
static int haveBSCALE;
static int haveBZERO;
static int haveBLANK;
static int haveEPOCH;
static int haveEQUINOX;

static char ctype1[1024];
static char ctype2[1024];

static int CHdebug    = 0;

static char *mHeader = (char *)NULL;

static struct WorldCoor *hdrCheck_wcs = (struct WorldCoor *)NULL;

int  montage_fitsCheck  (char *keyword, char *value);
int  montage_strAdd     (char *header, char *card);
void montage_FITSerror  (int status);
void montage_errorOutput(char *msg);

struct WorldCoor *getWCS();
char             *getHdr();

static char montage_msgstr[1024];


static int hdrStringent = 0;

void montage_checkHdrExact(int stringent)
{
   hdrStringent = stringent;
}


/***********************************************/
/*                                             */
/*  checkHdr                                   */
/*                                             */
/*  Routine for checking header template files */
/*                                             */
/*  The hdrflag argument is used as follows:   */
/*                                             */
/*   0 - Assume we are checking a FITS file    */
/*   1 - Assume we are checking a hdr file     */
/*   2 - Could be either; check both ways      */
/*                                             */
/***********************************************/

char *montage_checkHdr(char *infile, int hdrflag, int hdu)
{
   int       i, len, ncard, morekeys;

   int       status = 0;

   char     *keyword;
   char     *value;

   char      fitskeyword[80];
   char      fitsvalue  [80];
   char      fitscomment[80];
   char      tmpstr     [80];

   char     *end;

   char     *checkWCS;

   char      line  [1024];
   char      pline [1024];

   char     *ptr1;
   char     *ptr2;

   FILE     *fp;
   fitsfile *infptr;

   static int maxhdr;

   if(!mHeader)
   {
      mHeader = malloc(MAXHDR);
      maxhdr = MAXHDR;
   }

   havePLTRAH  = 0;

   haveSIMPLE  = 0;
   haveBITPIX  = 0;
   haveNAXIS   = 0;
   haveNAXIS1  = 0;
   haveNAXIS2  = 0;
   haveCTYPE1  = 0;
   haveCTYPE2  = 0;
   haveCRPIX1  = 0;
   haveCRPIX2  = 0;
   haveCDELT1  = 0;
   haveCDELT2  = 0;
   haveCD1_1   = 0;
   haveCD1_2   = 0;
   haveCD2_1   = 0;
   haveCD2_2   = 0;
   haveCRVAL1  = 0;
   haveCRVAL2  = 0;
   haveBSCALE  = 0;
   haveBZERO   = 0;
   haveBLANK   = 0;
   haveEPOCH   = 0;
   haveEQUINOX = 0;


   /****************************************/
   /* Initialize the WCS transform library */
   /* and find the pixel location of the   */
   /* sky coordinate specified             */
   /****************************************/

   strcpy(mHeader, "");

   if(fits_open_file(&infptr, infile, READONLY, &status) == 0)
   {
      if(CHdebug)
      {
         printf("\nFITS file\n");
         fflush(stdout);
      }

      if(hdrflag == HDR)
      {
         sprintf(montage_msgstr, "FITS file (%s) cannot be used as a header template", infile);
         return montage_msgstr;
      }

      if(hdu > 0)
      {
         if(fits_movabs_hdu(infptr, hdu+1, NULL, &status))
         {
            montage_FITSerror(status);
            return montage_msgstr;
         }
      }

      if(fits_get_hdrspace (infptr, &ncard, &morekeys, &status))
      {
         montage_FITSerror(status);
         return montage_msgstr;
      }
      
      if(ncard > 1000)
         mHeader = realloc(mHeader, ncard * 80 + 1024);

      if(CHdebug)
      {
         printf("ncard = %d\n", ncard);
         fflush(stdout);
      }

      for (i=1; i<=ncard; i++)
      {
         if(fits_read_keyn (infptr, i, fitskeyword, fitsvalue, fitscomment, &status))
         {
            montage_FITSerror(status);
            return montage_msgstr;
         }

         if(fitsvalue[0] == '\'')
         {
            strcpy(tmpstr, fitsvalue+1);

            if(tmpstr[strlen(tmpstr)-1] == '\'')
               tmpstr[strlen(tmpstr)-1] =  '\0';
         }
         else
            strcpy(tmpstr, fitsvalue);

         montage_fitsCheck(fitskeyword, tmpstr);

         sprintf(line, "%-8s= %20s", fitskeyword, fitsvalue);

         if(strncmp(line, "COMMENT", 7) != 0)
            montage_strAdd(mHeader, line);
      }

      montage_strAdd(mHeader, "END");

      if(fits_close_file(infptr, &status))
      {
         montage_FITSerror(status);
         return montage_msgstr;
      }
   }
   else
   {
      if(CHdebug)
      {
         printf("\nTemplate file\n");
         fflush(stdout);
      }

      if(hdrflag == FITS)
      {
         fp = fopen(infile, "r");

         if(fp == (FILE *)NULL)
         {
            sprintf(montage_msgstr, "File %s not found.", infile);
            return montage_msgstr;
         }

         fclose(fp);

         sprintf(montage_msgstr, "File (%s) is not a FITS image", infile);
         return montage_msgstr;
      }

      fp = fopen(infile, "r");

      if(fp == (FILE *)NULL)
      {
         sprintf(montage_msgstr, "File %s not found.", infile);
         return montage_msgstr;
      }

      while(1)
      {
         if(fgets(line, 1024, fp) == (char *)NULL)
            break;

         if(line[(int)strlen(line)-1] == '\n')
            line[(int)strlen(line)-1]  = '\0';
         
         if(line[(int)strlen(line)-1] == '\r')
            line[(int)strlen(line)-1]  = '\0';
         
         strcpy(pline, line);

         if((int)strlen(line) > 80)
         {
            sprintf(montage_msgstr, "FITS header lines cannot be greater than 80 characters.");
            return montage_msgstr;
         }

         len = (int)strlen(pline);

         keyword = pline;

         while(*keyword == ' ' && keyword < pline+len)
            ++keyword;

         end = keyword;

         while(*end != ' ' && *end != '=' && end < pline+len)
            ++end;

         value = end;

         while((*value == '=' || *value == ' ' || *value == '\'')
               && value < pline+len)
            ++value;

         *end = '\0';
         end = value;

         if(*end == '\'')
            ++end;

         while(*end != ' ' && *end != '\'' && end < pline+len)
            ++end;

         *end = '\0';

         montage_fitsCheck(keyword, value);

         montage_strAdd(mHeader, line);
         
         if((int)strlen(mHeader) + 160 > maxhdr)
         {
            maxhdr += MAXHDR;
            mHeader = realloc(mHeader, maxhdr);
         }
      }

      fclose(fp);
   }


   /********************************************************/
   /*                                                      */
   /* Check to see if we have the minimum FITS header info */
   /*                                                      */
   /********************************************************/

   if(!haveBITPIX)
   {
      montage_errorOutput("No BITPIX keyword in FITS header");
      return montage_msgstr;
   }

   if(!haveNAXIS)
   {
      montage_errorOutput("No NAXIS keyword in FITS header");
      return montage_msgstr;
   }

   if(!haveNAXIS1)
   {
      montage_errorOutput("No NAXIS1 keyword in FITS header");
      return montage_msgstr;
   }

   if(!haveNAXIS2)
   {
      montage_errorOutput("No NAXIS2 keyword in FITS header");
      return montage_msgstr;
   }

   if(havePLTRAH)
   {
      /* If we have this parameter, we'll assume this is a DSS header  */
      /* the WCS checking routine should be able to verify if it isn't */

      return (char *)NULL;
   }

   if(!haveCTYPE1)
   {
      montage_errorOutput("No CTYPE1 keyword in FITS header");
      return montage_msgstr;
   }

   if(!haveCTYPE2)
   {
      montage_errorOutput("No CTYPE2 keyword in FITS header");
      return montage_msgstr;
   }

   if(!haveCRPIX1)
   {
      montage_errorOutput("No CRPIX1 keyword in FITS header");
      return montage_msgstr;
   }

   if(!haveCRPIX2)
   {
      montage_errorOutput("No CRPIX2 keyword in FITS header");
      return montage_msgstr;
   }

   if(!haveCRVAL1)
   {
      montage_errorOutput("No CRVAL1 keyword in FITS header");
      return montage_msgstr;
   }

   if(!haveCRVAL2)
   {
      montage_errorOutput("No CRVAL2 keyword in FITS header");
      return montage_msgstr;
   }

   if(!haveCD1_1 
   && !haveCD1_2 
   && !haveCD2_1 
   && !haveCD2_2)
   {
      if(!haveCDELT1)
      {
         montage_errorOutput("No CDELT1 keyword (or incomplete CD matrix) in FITS header");
         return montage_msgstr;
      }
      else if(!haveCDELT2)
      {
         montage_errorOutput("No CDELT2 keyword (or incomplete CD matrix) in FITS header");
         return montage_msgstr;
      }
   }

   if(strlen(ctype1) < 8)
   {
      montage_errorOutput("CTYPE1 must be at least 8 characters");
      return montage_msgstr;
   }

   if(strlen(ctype2) < 8)
   {
      montage_errorOutput("CTYPE2 must be at least 8 characters");
      return montage_msgstr;
   }

   ptr1 = ctype1;

   while(*ptr1 != '-' && *ptr1 != '\0') ++ptr1;
   while(*ptr1 == '-' && *ptr1 != '\0') ++ptr1;

   ptr2 = ctype2;

   while(*ptr2 != '-' && *ptr2 != '\0') ++ptr2;
   while(*ptr2 == '-' && *ptr2 != '\0') ++ptr2;

   if(strlen(ptr1) == 0
   || strlen(ptr2) == 0)
   {
      montage_errorOutput("Invalid CTYPE1 or CTYPE2 projection information");
      return montage_msgstr;
   }

   if(strcmp(ptr1, ptr2) != 0)
   {
      montage_errorOutput("CTYPE1, CTYPE2 projection information mismatch");
      return montage_msgstr;
   }

   if(hdrStringent)
   {
      if(strlen(ptr1) != 3)
      {
         montage_errorOutput("Invalid CTYPE1 projection information");
         return montage_msgstr;
      }

      if(strlen(ptr2) != 3)
      {
         montage_errorOutput("Invalid CTYPE2 projection information");
         return montage_msgstr;
      }
   }


   /****************************************/
   /* Initialize the WCS transform library */
   /* and find the pixel location of the   */
   /* sky coordinate specified             */
   /****************************************/

   /*
   if(CHdebug)
   {
      printf("header = \n%s\n", mHeader);
      fflush(stdout);
   }
   */

   hdrCheck_wcs = wcsinit(mHeader);

   checkWCS = montage_checkWCS(hdrCheck_wcs);

   if(checkWCS)
      return checkWCS;

   return (char *)NULL;
}



char *montage_getHdr()
{
   return mHeader;
}



struct WorldCoor *montage_getWCS()
{
   return hdrCheck_wcs;
}



/* fitsCheck checks the value of a given keyword to make */
/* sure it is a valid entry in the FITS header.          */

int montage_fitsCheck(char *keyword, char *value)
{
   char  *end;
   int    ival;
   double dval;

   if(CHdebug)
   {
      printf("fitsCheck() [%s] = [%s]\n", keyword, value);
      fflush(stdout);
   }

   if(strcmp(keyword, "SIMPLE") == 0)
   {
      haveSIMPLE = 1;

      if(strcmp(value, "T") != 0
      && strcmp(value, "F") != 0)
      {
         montage_errorOutput("SIMPLE keyword must be T or F");
         return 1;
      }
   }

   else if(strcmp(keyword, "BITPIX") == 0)
   {
      haveBITPIX = 1;

      ival = strtol(value, &end, 0);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("BITPIX keyword in FITS header not an integer");
         return 1;
      }

      if(ival != 8
      && ival != 16
      && ival != 32
      && ival != 64
      && ival != -32
      && ival != -64)
      {
         montage_errorOutput("Invalid BITPIX in FITS header (must be 8,16,32,64,-32 or -64)");
         return 1;
      }
   }

   else if(strcmp(keyword, "NAXIS") == 0)
   {
      haveNAXIS = 1;

      ival = strtol(value, &end, 0);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("NAXIS keyword in FITS header not an integer");
         return 1;
      }

      if(ival < 2)
      {
         montage_errorOutput("NAXIS keyword in FITS header must be >= 2");
         return 1;
      }
   }

   else if(strcmp(keyword, "NAXIS1") == 0)
   {
      haveNAXIS1 = 1;

      ival = strtol(value, &end, 0);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("NAXIS1 keyword in FITS header not an integer");
         return 1;
      }

      if(ival < 0)
      {
         montage_errorOutput("NAXIS1 keyword in FITS header must be > 0");
         return 1;
      }
   }

   else if(strcmp(keyword, "NAXIS2") == 0)
   {
      haveNAXIS2 = 1;

      ival = strtol(value, &end, 0);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("NAXIS2 keyword in FITS header not an integer");
         return 1;
      }

      if(ival < 0)
      {
         montage_errorOutput("NAXIS2 keyword in FITS header must be > 0");
         return 1;
      }
   }

   else if(strcmp(keyword, "PLTRAH") == 0)
      havePLTRAH = 1;

   else if(strcmp(keyword, "CTYPE1") == 0)
   {
      haveCTYPE1 = 1;
      strcpy(ctype1, value);
   }

   else if(strcmp(keyword, "CTYPE2") == 0)
   {
      haveCTYPE2 = 1;
      strcpy(ctype2, value);
   }

   else if(strcmp(keyword, "CRPIX1") == 0)
   {
      haveCRPIX1 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CRPIX1 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CRPIX2") == 0)
   {
      haveCRPIX2 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CRPIX2 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CRVAL1") == 0)
   {
      haveCRVAL1 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CRVAL1 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CRVAL2") == 0)
   {
      haveCRVAL2 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CRVAL2 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CDELT1") == 0)
   {
      haveCDELT1 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CDELT1 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CDELT2") == 0)
   {
      haveCDELT2 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CDELT2 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CROTA2") == 0)
   {
      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CROTA2 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CD1_1") == 0)
   {
      haveCD1_1 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CD1_1 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CD1_2") == 0)
   {
      haveCD1_2 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CD1_2 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CD2_1") == 0)
   {
      haveCD2_1 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CD1_2 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "CD2_2") == 0)
   {
      haveCD2_2 = 1;

      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("CD2_2 keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "BSCALE") == 0)
   {
      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("BSCALE keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "BZERO") == 0)
   {
      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("BZERO keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "BLANK") == 0)
   {
      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("BLANK keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "EPOCH") == 0)
   {
      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         montage_errorOutput("EPOCH keyword in FITS header not a real number");
         return 1;
      }
   }

   else if(strcmp(keyword, "EQUINOX") == 0)
   {
      dval = strtod(value, &end);

      if(end < value + (int)strlen(value))
      {
         if(dval < 1900. || dval > 2050.)
         {
            montage_errorOutput("EQUINOX keyword in FITS header not a real number");
            return 1;
         }
      }
   }
   
   return 0;
}


/* Adds the string "card" to a header line, and */
/* pads the header out to 80 characters.        */

int montage_strAdd(char *header, char *card)
{
   int i;

   int hlen = (int)strlen(header);
   int clen = (int)strlen(card);

   for(i=0; i<clen; ++i)
      header[hlen+i] = card[i];

   if(clen < 80)
      for(i=clen; i<80; ++i)
         header[hlen+i] = ' ';

   header[hlen+80] = '\0';

   return((int)strlen(header));
}



/*************************/
/* General error routine */
/*************************/

void montage_errorOutput(char *msg)
{
   strcpy(montage_msgstr, msg);
   return;
}


/***********************************/
/*  Print out FITS library errors  */
/***********************************/

void montage_FITSerror(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);

   return;
}
