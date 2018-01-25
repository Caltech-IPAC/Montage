#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <mPutHdr.h>
#include <montage.h>

#define MAXSTR  256

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);



/*************************************************************************/
/*                                                                       */
/*  mPutHdr                                                              */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mPutHdr, replaces the header of the input file with     */
/*  one supplied by the user (presumably a "corrected" version of the    */
/*  input).  Nothing else is changed.  The new header is in the form     */
/*  of a standard Montage template:  an ASCII file with the same content */
/*  as a FITS header but one card per line and no need to make the lines */
/*  80 characters long (i.e. an easily editable file).                   */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int       debug, hdu;

   char      input_file   [MAXSTR];
   char      output_file  [MAXSTR];
   char      template_file[MAXSTR];

   char     *end, c;

   struct mPutHdrReturn *returnStruct;

   FILE *montage_status;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug     = 0;
   opterr    = 0;
   hdu       = 0;

   montage_status = stdout;

   while ((c = getopt(argc, argv, "d:s:h:")) != EOF) 
   {
      switch (c) 
      {
         case 'd':
            debug = montage_debugCheck(optarg);

            if(debug < 0)
            {
                fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Invalid debug level.\"]\n");
                exit(1);
            }
            break;

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         case 'h':
            hdu = strtol(optarg, &end, 10);

            if(end < optarg + strlen(optarg) || hdu < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"HDU value (%s) must be a non-negative integer\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level][-s statusfile][-h hdu] in.fits out.fits hdr.template\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 3) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-d level][-s statusfile][-h hdu] in.fits out.fits hdr.template\"]\n", argv[0]);
      exit(1);
   }

   strcpy(input_file,    argv[optind]);
   strcpy(output_file,   argv[optind + 1]);
   strcpy(template_file, argv[optind + 2]);


   returnStruct = mPutHdr(input_file, output_file, template_file, hdu, debug);

   if(returnStruct->status == 1)
   {
       fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
       exit(1);
   }
   else
   {
       fprintf(montage_status, "[struct stat=\"OK\", module=\"mPutHdr\"]\n");
       exit(0);
   }
}
