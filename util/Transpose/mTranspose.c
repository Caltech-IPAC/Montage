/* Module: mTranspose.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.3      John Good        08Sep15  fits_read_pix() incorrect null value
1.2      John Good        02Sep15  Warning cleanup
1.1      John Good        27Jul15  Test results updates
1.0      John Good        30Jul14  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <fitsio.h>
#include <wcs.h>
#include <coord.h>

#include "montage.h"
#include "mNaN.h"

#define STRLEN  1024

extern char *optarg;
extern int optind, opterr;

char inputFile [STRLEN];
char outputFile[STRLEN];

char  *input_header;

int At[4][4];
int Bt[4];

int  order  [4];
int  reorder[4];


void  printFitsError  (int);
void  printError      (char *);
char *checkKeyword    (char *keyname, char *card, long naxis);
int   initTransform   (long *naxis, long *NAXIS);
void  transform       (int i, int j, int k, int l, int *it, int *jt, int *kt, int *lt);
int   analyzeCTYPE    (fitsfile *inFptr);

FILE *fstatus;

int  debug;


/******************************************************/
/*                                                    */
/*  mTranspose                                        */
/*                                                    */
/*  This module switches two of the axes for a 3D     */
/*  image.  This is mainly so we can get a cube       */
/*  rearranged so the spatial axes are the first two. */
/*                                                    */
/******************************************************/

int main(int argc, char **argv)
{
   int        i, j, k, l;
   int        it, jt, kt, lt;
   int        bitpix, first;
   int        nullcnt, status, nfound, keynum;
   long       fpixel[4];

   long       naxis;
   long       nAxisIn [4];
   long       nAxisOut[4];

   double    *indata;
   double ****outdata;

   char      *end;

   char       card    [STRLEN];
   char       newcard [STRLEN];
   char       keyname [STRLEN];
   char       value   [STRLEN];
   char       comment [STRLEN];
   char       errstr  [STRLEN];
   char       statfile[STRLEN];

   double     mindata, maxdata;

   fitsfile  *inFptr;
   fitsfile  *outFptr;


   /************************************************/
   /* Make a NaN value to use setting blank pixels */
   /************************************************/

   union
   {
      double d;
      char   c[8];
   }
   nvalue;

   double nan;

   for(i=0; i<8; ++i)
      nvalue.c[i] = 255;

   nan = nvalue.d;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug    = 0;
   fstatus  = stdout;

   strcpy(statfile, "");

   for(i=0; i<argc; ++i)
   {
      if(strcmp(argv[i], "-s") == 0)
      {
         if(i+1 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"No status file name given\"]\n");
            exit(1);
         }

         strcpy(statfile, argv[i+1]);

         argv += 2;
         argc -= 2;
      }

      if(strcmp(argv[i], "-d") == 0)
      {
         if(i+1 >= argc)
         {
            printf("[struct stat=\"ERROR\", msg=\"No debug level given\"]\n");
            exit(1);
         }

         debug = strtol(argv[i+1], &end, 0);

         if(end - argv[i+1] < strlen(argv[i+1]))
         {
            printf("[struct stat=\"ERROR\", msg=\"No debug level given\"]\n");
            exit(1);
         }

         if(debug < 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"Debug level value cannot be negative\"]\n");
            exit(1);
         }

         if(debug > 3)
         {
            printf("[struct stat=\"ERROR\", msg=\"Debug level range is 0 to 3\"]\n");
            exit(1);
         }

         argv += 2;
         argc -= 2;
      }
   }
   
   if (argc >= 3 && argc < 5) 
   {
      printf ("[struct stat=\"ERROR\", msg=\"You must give input/output files the output axis order list (which will always be at least two integers).\"]\n");
      exit(1);
   }

   if (argc < 5) 
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: mTranspose [-d level] [-s statusfile] in.fits out.fits outaxis1 outaxis2 [outaxis3 [outaxis4]]\"]\n");
      exit(1);
   }

   strcpy(inputFile, argv[1]);

   if(inputFile[0] == '-')
   {
      printf ("[struct stat=\"ERROR\", msg=\"Invalid input file '%s'\"]\n", inputFile);
      exit(1);
   }

   strcpy(outputFile, argv[2]);

   if(outputFile[0] == '-')
   {
      printf ("[struct stat=\"ERROR\", msg=\"Invalid output file '%s'\"]\n", outputFile);
      exit(1);
   }


   /************************/
   /* Open the input image */
   /************************/
     
   status = 0;
   if(fits_open_file(&inFptr, inputFile, READONLY, &status))
   {
      sprintf(errstr, "Input image file %s missing or invalid FITS", inputFile);
      printError(errstr);
   }

   analyzeCTYPE(inFptr);
   
   status = 0;
   if(fits_read_keys_lng(inFptr, "NAXIS", 1, 4, nAxisIn, &nfound, &status))
      printFitsError(status);

   if(nfound < 4)
      nAxisIn[3] = 1;

   if(nfound < 3)
      nAxisIn[2] = 1;

   naxis = nfound;

   if(debug)
   {
      printf("naxis       =  %ld\n",   naxis);
      printf("nAxisIn[0]  =  %ld\n",   nAxisIn[0]);
      printf("nAxisIn[1]  =  %ld\n",   nAxisIn[1]);
      printf("nAxisIn[2]  =  %ld\n",   nAxisIn[2]);
      printf("nAxisIn[3]  =  %ld\n",   nAxisIn[3]);
      printf("\n");
      fflush(stdout);
   }


   /****************************************************/
   /* We analyzed the image for lat/lot and initialize */
   /* the transpose order.  Here we optionally set it  */
   /* manually with command-line arguments.            */
   /****************************************************/

   if(argc < naxis + 3)
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Image has %ld dimensions.  You must list the output order for all of them.\"]\n", naxis);
      exit(1);
   }
      
   if(strlen(statfile) > 0)
   {
      if((fstatus = fopen(statfile, "w+")) == (FILE *)NULL)
      {
         printf ("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
            statfile);
         exit(1);
      }
   }

   if(argc > 3)
   {
      order[0] = strtol(argv[3], &end, 0);

      if(end < argv[3] + (int)strlen(argv[3]))
      {
         fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Axis ID 1 cannot be interpreted as an integer.\"]\n");
         exit(1);
      }

      if(order[0] < 1 || order[0] > naxis)
      {
         fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Axis ID 1 must be between 1 and %ld.\"]\n", naxis);
         exit(1);
      }

      if(argc > 4)
      {
         order[1] = strtol(argv[4], &end, 0);

         if(end < argv[4] + (int)strlen(argv[4]))
         {
            fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Axis ID 2 cannot be interpreted as an integer.\"]\n");
            exit(1);
         }

         if(order[1] < 1 || order[1] > naxis)
         {
            fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Axis ID 2 must be between 1 and %ld.\"]\n", naxis);
            exit(1);
         }

         if(argc > 5)
         {
            order[2] = strtol(argv[5], &end, 0);

            if(end < argv[5] + (int)strlen(argv[5]))
            {
               fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Axis ID 3 cannot be interpreted as an integer.\"]\n");
               exit(1);
            }

            if(order[2] < 1 || order[2] > naxis)
            {
               fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Axis ID 3 must be between 1 and %ld.\"]\n", naxis);
               exit(1);
            }

            if(argc > 6)
            {
               order[3] = strtol(argv[6], &end, 0);

               if(end < argv[6] + (int)strlen(argv[6]))
               {
                  fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Axis ID 4 cannot be interpreted as an integer.\"]\n");
                  exit(1);
               }

               if(order[3] < 1 || order[3] > naxis)
               {
                  fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Axis ID 4 must be between 1 and %ld.\"]\n", naxis);
                  exit(1);
               }
            }
         }
      }
   }

   for(i=0; i<naxis; ++i)
   {
      for(j=0; j<i; ++j)
      {
         if(order[j] == order[i])
         {
            fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Output axis %d is the same as axis %d. They must be unique.\"]\n", i+1, j+1);
            exit(1);
         }
      }
   }

   if(debug >= 1)
   {
      printf("\n");
      printf("debug       = %d\n",   debug);
      printf("\n");
      printf("inputFile   = [%s]\n", inputFile);
      printf("outputFile  = [%s]\n", outputFile);
      printf("\n");
      printf("order[0]    = [%d]\n", order[0]);
      printf("order[1]    = [%d]\n", order[1]);
      printf("order[2]    = [%d]\n", order[2]);
      printf("order[3]    = [%d]\n", order[3]);
      printf("\n");

      fflush(stdout);
   }


   /***********************************/
   /* Initialize the transform matrix */
   /***********************************/

   initTransform(nAxisIn, nAxisOut);

   if(debug)
   {
      printf("nAxisOut[0] =  %ld\n", nAxisOut[0]);
      printf("nAxisOut[1] =  %ld\n", nAxisOut[1]);
      printf("nAxisOut[2] =  %ld\n", nAxisOut[2]);
      printf("nAxisOut[3] =  %ld\n", nAxisOut[3]);
      printf("\n");

      printf("*it = %d*i + %d*j + %d*k + %d*l + %d\n", At[0][0], At[0][1], At[0][2], At[0][3], Bt[0]);
      printf("*jt = %d*i + %d*j + %d*k + %d*l + %d\n", At[1][0], At[1][1], At[1][2], At[0][3], Bt[1]);
      printf("*kt = %d*i + %d*j + %d*k + %d*l + %d\n", At[2][0], At[2][1], At[2][2], At[0][3], Bt[2]);
      printf("*lt = %d*i + %d*j + %d*k + %d*l + %d\n", At[3][0], At[3][1], At[3][2], At[0][3], Bt[3]);
      printf("\n");

      printf("reorder[0]  =  %d\n", reorder[0]);
      printf("reorder[1]  =  %d\n", reorder[1]);
      printf("reorder[2]  =  %d\n", reorder[2]);
      printf("reorder[3]  =  %d\n", reorder[3]);
      printf("\n");
      fflush(stdout);
   }


   /**********************************************/ 
   /* Allocate memory for the input image pixels */ 
   /* We don't need the whole array, just one    */ 
   /* line which we read in and immediately      */ 
   /* redistribute to the output array.          */ 
   /**********************************************/ 

   indata = (double *)malloc(nAxisIn[0] * sizeof(double));


   /***********************************************/ 
   /* Allocate memory for the output image pixels */ 
   /***********************************************/ 

   outdata = (double ****)malloc(nAxisOut[3] * sizeof(double ***));

   for(l=0; l<nAxisOut[3]; ++l)
   {
      if(debug && l == 0)
      {
         printf("%ld (double **) allocated %ld times\n", nAxisOut[2], nAxisOut[3]);
         fflush(stdout);
      }

      outdata[l] = (double ***)malloc(nAxisOut[2] * sizeof(double **));

      for(k=0; k<nAxisOut[2]; ++k)
      {
         if(debug && l == 0 && k == 0)
         {
            printf("%ld (double *) allocated %ldx%ld times\n", nAxisOut[1], nAxisOut[2], nAxisOut[3]);
            fflush(stdout);
         }

         outdata[l][k] = (double **)malloc(nAxisOut[1] * sizeof(double *));

         for(j=0; j<nAxisOut[1]; ++j)
         {
            if(debug && l == 0 && k == 0 && j == 0)
            {
               printf("%ld (double) allocated %ldx%ldx%ld times\n", nAxisOut[0], nAxisOut[1], nAxisOut[2], nAxisOut[3]);
               fflush(stdout);
            }

            outdata[l][k][j] = (double *)malloc(nAxisOut[0] * sizeof(double));

            for(i=0; i<nAxisOut[0]; ++i)
            {
               if(debug && l == 0 && k == 0 && j == 0 && i == 0)
               {
                  printf("%ld doubles zeroed %ldx%ldx%ld times\n\n", nAxisOut[0], nAxisOut[1], nAxisOut[2], nAxisOut[3]);
                  fflush(stdout);
               }

               outdata[l][k][j][i] = 0;
            }
         }
      }
   }

   if(debug >= 1)
   {
      printf("%ld bytes allocated for input image pixels\n\n", 
         nAxisOut[0] * nAxisOut[1] * nAxisOut[2] * nAxisOut[3] * sizeof(double));
      fflush(stdout);
   }


   /*************************/
   /* Read / write the data */
   /*************************/

   fpixel[0] = 1;

   status = 0;
   first  = 1;

   for (l=0; l<nAxisIn[3]; ++l)
   {
      // For each 4D plane

      fpixel[3] = l+1;

      for (k=0; k<nAxisIn[2]; ++k)
      {
         // For each plane 

         fpixel[2] = k+1;

         for (j=0; j<nAxisIn[1]; ++j)
         {
            // Read lines from the input file

            fpixel[1] = j+1;

            if(debug >= 2)
            {
               printf("Reading input 4plane/plane/row %5d/%5d/%5d\n", l, k, j);
               fflush(stdout);
            }

            status = 0;

            if(fits_read_pix(inFptr, TDOUBLE, fpixel, nAxisIn[0], &nan,
                             (void *)indata, &nullcnt, &status))
               printFitsError(status);


            // And copy the data to the correct 
            // pixels in the output array

            if(debug >= 3)
            {
               printf("\n");
               printf("%5s %5s %5s %5s -> %5s %5s %5s %5s\n", "l", "k", "j", "i", "lt", "kt", "jt", "it");
               fflush(stdout);
            }

            for(i=0; i<nAxisIn[0]; ++i)
            {
               transform(i, j, k, l, &it, &jt, &kt, &lt);

               if(debug >= 3)
               {
                  printf("%5d %5d %5d %5d -> %5d %5d %5d %5d [%-g]\n",
                     l, k, j, i, lt, kt, jt, it, indata[i]);
                  fflush(stdout);
               }

               if(mNaN(indata[i]))
                  outdata[lt][kt][jt][it] = nan;
               else
               {
                  outdata[lt][kt][jt][it] = indata[i];

                  if(first)
                  {
                     mindata = indata[i];
                     maxdata = indata[i];
                     first = 0;
                  }
                  else
                  {
                     if(indata[i] < mindata) mindata = indata[i];
                     if(indata[i] > maxdata) maxdata = indata[i];
                  }
               }
            }
         }
      }
   }

   if(debug >= 1)
   {
      printf("Input image read complete.\n\n");
      fflush(stdout);
   }


   /********************************/
   /* Create the output FITS files */
   /********************************/

   remove(outputFile);               

   if(fits_create_file(&outFptr, outputFile, &status)) 
      printFitsError(status);           

   if(debug >= 1)
   {
      printf("\nFITS output files created (not yet populated)\n"); 
      fflush(stdout);
   }

   bitpix = -64;

   if (fits_create_img(outFptr, bitpix, naxis, nAxisOut, &status))
         printFitsError(status);


   /**************************************************************/
   /* Copy all the header keywords from the input to the output. */
   /* Update the ones that change because of axes swapping and   */
   /* ignore BITPIX/NAXIS values.                                */
   /**************************************************************/

   keynum = 1;

   while(1)
   { 
      status = 0;
      fits_read_record(inFptr, keynum, card, &status);

      status = 0;
      fits_read_keyn(inFptr, keynum, keyname, value, comment, &status);

      if(status)
         break;

      strcpy(newcard, checkKeyword(keyname, card, naxis));

      if(strlen(newcard) > 0)
      {
         if(debug >= 1)
         {
            printf("Header keyword %d: [%s][%s][%s]\n", keynum, keyname, value, comment);
            printf("               --> [%s]\n", newcard);
            fflush(stdout);
         }

         status = 0;
         if(fits_write_record(outFptr, newcard, &status))
         {
            sprintf(errstr, "Error writing card %d.", keynum);
            printError(errstr);
         }
      }

      ++keynum;
   }


   // Now we switch the lower case keywords back to upper case

   keynum = 1;

   while(1)
   { 
      status = 0;
      fits_read_keyn(outFptr, keynum, keyname, value, comment, &status);

      if(status)
         break;

      // printf("Header keyword %d: [%s][%s][%s]\n", keynum, keyname, value, comment);

      ++keynum;
   }

   if(debug >= 1)
   {
      printf("Header keywords copied to FITS output file with axes modifications\n"); 
      fflush(stdout);
   }

   status = 0;
   if(fits_close_file(inFptr, &status))
      printFitsError(status);


   /************************/
   /* Write the image data */
   /************************/

   fpixel[0] = 1;

   status = 0;

   for (l=0; l<nAxisOut[3]; ++l)
   {
      // For each 4D plane

      fpixel[3] = l+1;

      for (k=0; k<nAxisOut[2]; ++k)
      {
         // For each plane 

         fpixel[2] = k+1;

         for (j=0; j<nAxisOut[1]; ++j)
         {
            // Write lines to the output file

            fpixel[1] = j+1;

            status = 0;

            if (fits_write_pix(outFptr, TDOUBLE, fpixel, nAxisOut[0], 
                               (void *)outdata[l][k][j], &status))
               printFitsError(status);

         }
      }
   }

   if(debug >= 1)
   {
      printf("Data written to FITS data image\n"); 
      fflush(stdout);
   }


   /******************************/
   /* Close the output FITS file */
   /******************************/

   if(fits_close_file(outFptr, &status))
      printFitsError(status);           

   if(debug >= 1)
   {
      printf("FITS data image finalized\n"); 
      fflush(stdout);
   }

   fprintf(fstatus, "[struct stat=\"OK\", mindata=%-g, maxdata=%-g]\n", mindata, maxdata);
   fflush(fstatus);

   exit(0);
}



/*******************************************/
/*                                         */
/* Analyze the CTYPE keywords to determine */
/* the default reordering.                 */
/*                                         */
/*******************************************/

int analyzeCTYPE(fitsfile *inFptr)
{
   int  i, status, lonaxis, lataxis;

   char ctype[4][16];

   status = 0;
   fits_read_key(inFptr, TSTRING, "CTYPE1", ctype[0], (char *)NULL, &status);

   if(status) strcpy(ctype[0], "NONE");

   status = 0;
   fits_read_key(inFptr, TSTRING, "CTYPE2", ctype[1], (char *)NULL, &status);

   if(status) strcpy(ctype[1], "NONE");

   status = 0;
   fits_read_key(inFptr, TSTRING, "CTYPE3", ctype[2], (char *)NULL, &status);

   if(status) strcpy(ctype[2], "NONE");

   status = 0;
   fits_read_key(inFptr, TSTRING, "CTYPE4", ctype[3], (char *)NULL, &status);

   if(status) strcpy(ctype[3], "NONE");


   for(i=0; i<4; ++i)
      order[i] = -1;

   lonaxis = -1;
   lataxis = -1;

   for(i=0; i<4; ++i)
   {
      if(strncmp(ctype[i], "RA--", 4) == 0
      || strncmp(ctype[i], "GLON", 4) == 0
      || strncmp(ctype[i], "ELON", 4) == 0
      || strncmp(ctype[i], "LON-", 4) == 0)
      {
         if(lonaxis == -1)
            lonaxis = i;
         else
            printError("Multiple 'longitude' axes.");
      }

      if(strncmp(ctype[i], "DEC-", 4) == 0
      || strncmp(ctype[i], "GLAT", 4) == 0
      || strncmp(ctype[i], "ELAT", 4) == 0
      || strncmp(ctype[i], "LAT-", 4) == 0)
      {
         if(lataxis == -1)
            lataxis = i;
         else
            printError("Multiple 'latitude' axes.");
      }
   }

   if(lonaxis == -1 || lataxis == -1)
      printError("Need both longitude and latitude axes.");

   order[0] = lonaxis;
   order[1] = lataxis;

   for(i=0; i<4; ++i)
   {
      if(i != lonaxis && i != lataxis)
      {
         if(order[2] == -1)
            order[2] = i;
         else
            order[3] = i;
      }
   }
      
   for(i=0; i<4; ++i)
      ++order[i];

   return 0;
}


/*****************************************************/
/*                                                   */
/*  Check which keywords we know need to be switched */
/*                                                   */
/*****************************************************/

char *wcs[9] = { "NAXISn", "CRVALn", "CRPIXn",
                 "CTYPEn", "CDELTn", "CDn_n", 
                 "CROTAn", "CUNITn", "PCn_n" };
int nwcs = 9;


char *checkKeyword(char *keyname, char *card, long naxis)
{
   int i, j, match, index, newindex;

   static char retstr[STRLEN];

   char wcskey[STRLEN];

   // The axes counts and BITPIX were handle
   // when we created the image, so skip these

   if(strcmp(keyname, "SIMPLE") == 0
   || strcmp(keyname, "NAXIS" ) == 0
   || strcmp(keyname, "NAXIS1") == 0
   || strcmp(keyname, "NAXIS2") == 0
   || strcmp(keyname, "NAXIS3") == 0
   || strcmp(keyname, "NAXIS4") == 0
   || strcmp(keyname, "BITPIX") == 0)
   {
      strcpy(retstr, "");
      return retstr;
   }


   // Check whether this is one of the keywords
   // we know need to be transformed

   for(i=0; i<nwcs; ++i)
   {
      // NOTE: Its a bit of a kludge but in the case where 
      // naxis=2 it is safer to not switch CROTA2 to CROTA1.
      // CROTA2 has become a standard shorthand for the overall
      // image rotation; a lot of software probably never thinks
      // to check CROTA1

      if(strncmp(wcs[i], "CROTA", 5) && naxis == 2)
         continue;

      // Check keyword for match and convert as
      // we go.  Abandon if bad match.

      strcpy(retstr, card);

      strcpy(wcskey, wcs[i]);

      if(strlen(keyname) != strlen(wcskey))
         continue;

      match = 1;

      for(j=0; j<strlen(keyname); ++j)
      {
         if(wcskey[j] == 'n')
         {
            index = keyname[j] - '1';

            newindex = reorder[index];

            retstr[j] = '1' + newindex;
         }
         else if(keyname[j] != wcskey[j])
         {
            match = 0;
            break;
         }
      }

      if(match)
         return retstr;
   }

   strcpy(retstr, card);
   return retstr;
}



/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void printError(char *msg)
{
   fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
   exit(1);
}




/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   fprintf(fstatus, "[struct stat=\"ERROR\", status=%d, msg=\"%s\"]\n", status, status_str);

   exit(1);
}



/*******************/
/*                 */
/*  Axes transform */
/*                 */
/*******************/

int initTransform(long *naxis, long *NAXIS)
{
   int i, j;
   int index;
   int dir;

   for(i=0; i<4; ++i)
   {
      reorder[order[i]-1] = i;

      for(j=0; j<4; ++j)
         At[i][i] = 0;

      Bt[i] = 0;

      index = fabs(order[i]-1);
      dir   = 1;

      if(order[i] < 0)
      {
         Bt[index] = naxis[i];
         dir = -1;
      }

      At[i][index] = dir;

      NAXIS[i] = naxis[index];
   }

   return 0;
}

void transform(int  i,  int  j,  int  k,  int  l,
               int *it, int *jt, int *kt, int *lt)
{
   *it = At[0][0]*i + At[0][1]*j + At[0][2]*k + At[0][3]*l + Bt[0];
   *jt = At[1][0]*i + At[1][1]*j + At[1][2]*k + At[0][3]*l + Bt[1];
   *kt = At[2][0]*i + At[2][1]*j + At[2][2]*k + At[0][3]*l + Bt[2];
   *lt = At[3][0]*i + At[3][1]*j + At[3][2]*k + At[0][3]*l + Bt[3];

   return;
}
