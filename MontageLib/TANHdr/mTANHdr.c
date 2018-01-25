#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <coord.h>
#include <wcs.h>

#include <mTANHdr.h>
#include <montage.h>

#define MAXSTR  256

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);


/*************************************************************************/
/*                                                                       */
/*  mTANHdr                                                              */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  There are two reprojection modules, mProject and mProjectPP.         */
/*  The first can handle any projection transformation but is slow.  The */
/*  second can only handle transforms between tangent plane projections  */
/*  but is very fast.  In many cases, however, a non-tangent-plane       */
/*  projection can be approximated by a TAN projection with pixel-space  */
/*  distortions (in particular when the region covered is small, which   */
/*  is often the case in practice).                                      */
/*                                                                       */
/*  This module analyzes a template file and determines if there is      */
/*  an adequate equivalent distorted TAN projection that would be        */
/*  equivelent (i.e. location shifts less than, say, 0.01 pixels).       */
/*  mProjectPP can then be used to produce this distorted TAN image      */
/*  with the original non-TAN FITS header swapped in before writing      */
/*  to disk.                                                             */
/*                                                                       */
/*                                                                       */
/*  NOTE:  The "reverse" error is the important one for deciding whether */
/*  the new distorted-TAN header can be used in place of the original    */
/*  when reprojecting in mProjectPP since it is a measure of the         */
/*  process of going from distorted TAN to sky to original projection.   */
/*  Since the second part of this is exact, this error is all about how  */
/*  accurately the distorted-TAN maps to the right point of the sky.     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   char     origtmpl[MAXSTR];
   char     newtmpl [MAXSTR];

   int      c, maxiter, order, useOffscl, debug;
   double   tolerance;
   char    *end;

   struct mTANHdrReturn *returnStruct;

   FILE    *montage_status;


   /************************************/
   /* Read the command-line parameters */
   /************************************/

   opterr    =  0;
   debug     =  0;
   order     =  5;
   maxiter   = 50;
   tolerance =  0.01;
   useOffscl =  0;

   while ((c = getopt(argc, argv, "dui:o:t:s:")) != EOF) 
   {
      switch (c) 
      {
         case 'd':
            debug = 2;
            break;

         case 'u':
            useOffscl = 1;
            break;

         case 'i':
            maxiter = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"-i (iterations) argument \"%s\" not an integer\"]\n", optarg);
               fflush(stdout);
               exit(1);
            }

            if(maxiter < 1)
               maxiter = 1;
            break;

         case 'o':
            order = strtol(optarg, &end, 0);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"-o (order) argument \"%s\" not an integer\"]\n", optarg);
               fflush(stdout);
               exit(1);
            }

            if(order < 0)
               order = 0;
            break;

         case 't':
            tolerance = strtod(optarg, &end);

            if(end < optarg + strlen(optarg))
            {
               printf("[struct stat=\"ERROR\", msg=\"-t (tolerance) argument \"%s\" not a real number\"]\n", optarg);
               fflush(stdout);
               exit(1);
            }

            if(tolerance < 0)
               tolerance = 0;
            break;

         case 's':
            if((montage_status = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:
            fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-o order][-i maxiter][-t tolerance][-s statusfile] orig.hdr new.hdr (default: order = 5, maxiter = 50, tolerance = 0.01)\"]\n", 
               argv[0]);
            exit(1);
            break;
      }
   }

   if (argc - optind < 2) 
   {
      fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"Usage: %s [-d][-o order][-i maxiter][-t tolerance][-s statusfile] orig.hdr new.hdr (default: order = 5, maxiter = 50, tolerance = 0.01)\"]\n", 
            argv[0]);
      exit(1);
   }

   strcpy(origtmpl, argv[optind]);
   strcpy(newtmpl,  argv[optind + 1]);


   /***************************************/
   /* Call the mTANHdr processing routine */
   /***************************************/

   returnStruct = mTANHdr(origtmpl, newtmpl, order, maxiter, tolerance, useOffscl, debug);

   if(returnStruct->status == 1)
   {   
      fprintf(montage_status, "[struct stat=\"ERROR\", msg=\"%s\"]\n", returnStruct->msg);
      exit(1);
   }       
   else
   {   
      fprintf(montage_status, "[struct stat=\"OK\", module=\"mTANHdr\", %s]\n", returnStruct->msg);
      exit(0);
   }       
}
