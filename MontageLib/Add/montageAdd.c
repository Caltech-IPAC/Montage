/* mAdd.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
5.2      John Good        26Apr17  Added SUM 'averaging' mode (for X-ray observers)
5.3      John Good        08Sep15  fits_read_pix() incorrect null value
5.2      Daniel S. Katz   16Jul10  Small change for MPI with new fits library
5.1      John Good        09Jul06  Only show maxopen warning in debug mode
5.0      John Good        07Oct05  Added COUNT averaging mode           
4.7      John Good        14Jun05  Was losing the first row in every input
                                   file.
4.6      John Good        06Feb05  Fixed bug in allocation of pixel depth
                                   space. Was basing it on number of possible
                                   images when it should be number of pixel
                                   "fragments".
4.5      John Good        03Jan05  Updated offset calculation to work
                                   correctly with both pos and neg offsets
4.4      Daniel S. Katz   16Dec04  Parallelized file writes using libwcs
         Joe Jacob                 for optional MPI usage
4.3      John Good        06Dec04  Used define variables like MEAN instead
                                   of numeric equivalents
4.2      John Good        13Oct04  Changed format for printing time
4.1      John Good        03Aug04  Changed precision on updated keywords
4.0      John Good        29Jul04  Added code to generate wcs structures
                                   for the template and images and confirm
                                   that they match.
3.6      John Good        18Jul04  Added code to delete partial result
                                   files if there is a processing error
3.5      John Good        17Jul04  Linked-list code failing when count
                                   dropped to zero (gaps in coverage)
3.4      John Good        24Jun04  Added half pixel to offset calculation
                                   (the round back off) to allow for minor 
                                   variability in crpix values.
3.3      John Good        07Jun04  Modified FITS key updating precision
3.2      John Good        17May04  Changed median "area" value to be total
                                   area from all contributing pixels
3.1      John Good        09Apr04  Bug fix: wasn't checking input range
                                   to make sure lines weren't outside
                                   output image.
3.0      John Good        24Mar04  Reworked mechanism for keeping track
                                   of which files contribute to which
                                   output image lines using a linked
                                   list and sorted start and stop lines
                                   for those images.
2.11     John Good        24Mar04  Change MAXFILE (here and in CFITSIO
                                   library) so that it will be less
                                   likely that we start thrashing 
                                   opening and closing file repeatedly
2.10     John Good        09Mar04  Finessed fits_create_img() memory
                                   allocation by giving it naxis2 = 1
                                   to start, then switching back
2.9      John Good        07Mar04  Fixed bug in sort function
2.8      Anastasia Laity  04Feb04  Fixed typo in wrapping algorithm
2.7      Anastasia Laity  28Jan04  Added special check for
                                   Cartesian coordinates - will
                                   attempt to wrap around images that 
                                   mAdd_straddle 0 0.
2.6      Anastasia Laity  26Jan04  Get all header information from
                                   images.tbl instead of using
                                   fits library
2.5      Anastasia Laity  23Jan04  -Fixed bug in memory allocation
                                    for input_buffer
                                   -Fixed bug in calculation of open_files
2.4      Anastasia Laity  16Jan04  -Fixed bug in calculations
                                    of output crpix values 
                                    and input/output overlap lines
                                    for shrinking case.
                                   -No longer edits filenames 
                                    or builds area filenames in
                                    case of "no-area" flag 
                                    (allowing .fit extension)
                                   -Doesn't allocate memory
                                    for area files if "no-area"
                                    flag is on
2.3      John Good        13Jan04  Added -n flag to allow for
                                   coadding images where there is
                                   no corresponding "area" image
2.2      Anastasia Laity  06Jan04  Added -a flag for different
                                   types of averaging
2.1      Anastasia Laity  25Nov03  Added -e flag for exact fit to
                                   header template (doesn't shrink
                                   to fit data)
2.0      Anastasia Laity  29Oct03  Smaller memory version
1.6      John Good        10Oct03  Added alternate file column name processing
1.5      John Good        15Sep03  Updated fits_read_pix() call          
1.4      John Good        25Aug03  Added status file processing          
1.3      John Good        29May03  Check malloc() return values          
1.2      John Good        08Apr03  Also remove <CR> from template lines
1.1      John Good        14Mar03  Added filePath() processing,
                                   -dir argument, and getopt()
                                   argument processing and added 
                                   specific messages for missing flux
                                   and area files and for missing/invalid 
                                   images table. Check for valid template
                                   file.
1.0      John Good        29Jan03  Baseline code

*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>

#include <fitsio.h>
#include <wcs.h>
#include <mtbl.h>

#include <mAdd.h>
#include <montage.h>


#define MAXSTR    1024
#define MAXFILE     50
#define MAXFITS    200
#define MAXLIST    500
#define PIXDEPTH    50
#define HDRLEN   80000


/* Fractional area minumum for median-type averaging */

#define MINCOVERAGE 0.5


/***************************/
/* Define global variables */
/***************************/

static char ctype[MAXSTR];

static int maxfile = MAXFILE;

static char output_file      [MAXSTR];
static char output_area_file [MAXSTR];

static struct WorldCoor *imgWCS;
static struct WorldCoor *hdrWCS;

static int  mAdd_debug;
static int  status = 0;

static int  isCAR = 0;

static time_t currtime, start;


/*******************************************************/
/* Arrays to keep track of which files need to be open */
/*******************************************************/

static int *startline;
static int *startfile;
static int *endline;
static int *endfile;

static int open_files; 


/**************************************************/
/* Link list structure for keeping track of which */
/* input files are need for the current line      */
/**************************************************/

static struct ListElement
{
   int value;
   int used;
   int next;
   int prev;
}
**listElement;

static int nlistElement;

static int listFirst;
static int listMax;


/***************************************************/
/* structure to hold file information and pointers */
/***************************************************/

struct fileinfo
{
   int       isopen;
   fitsfile *fptr;
   int       start;
   int       offset;
   int       end;
};

static struct fileinfo *input, *input_area;
   

struct outfile
{
  fitsfile *fptr;
  long      naxes[2];
  double    crpix1, crpix2;
  double    crval1, crval2;
};

static struct outfile output, output_area;


static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mAdd                                                                 */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mAdd, reads sets of flux / area coverage images         */
/*  (the output of mProject) which have already been projected /         */
/*  resampled onto the same pixel space.  The fluxs, scaled by total     */
/*  input area, are then coadded into a single output composite.         */
/*                                                                       */
/*   char  *path           Directory containing files to be coadded      */
/*   char  *tblfile        Table file list of reprojected files to       */
/*                         coadd                                         */
/*   char  *template_file  FITS header file used to define the desired   */
/*                         output                                        */
/*   char  *outfile        Final mosaic FITS file                        */
/*                                                                       */
/*   int    shrink         Shrink-wrap to remove blank border areas      */
/*   int    haveAreas      Area files exist for weighting the coadd      */
/*   int    coadd          Image stacking: 0 (MEAN), 1 (MEDIAN)          */
/*                         2 (COUNT), 3 (SUM)                            */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/


struct mAddReturn *mAdd(char *inpath, char *tblfile, char *template_file, char *outfile,
                        int shrink, int haveAreas, int coadd, int debugin)
{
   int       i, j, ncols, namelen, imgcount;
   int       lineout, itemp, pixdepth, ipix, jcnt;
   int       inbuflen;
   int       currentstart, currentend;
   int       showwarning = 0;

   char     *checkHdr;

   double    tryval;

   char     *inputHeader;
   char     *ptr;
   int       wrap = 0;

   int       haveMinMax;
   int       avg_status = 0;

   long      fpixel[4], nelements;
   int       nullcnt;

   double    nominal_area = 0;

   double    imin, imax;
   double    jmin, jmax;

   double  **dataline;
   double  **arealine;
   int      *datacount;
   double   *input_buffer;
   double   *input_buffer_area;
   double   *outdataline;
   double   *outarealine;

   char      filename [MAXSTR];
   char      errstr   [MAXSTR];
   char      path     [MAXSTR];

   int       ifile, nfile;
   char    **inctype1, **inctype2;
   double   *incrpix1, *incrpix2;
   double   *incrval1, *incrval2;
   double   *incdelt1, *incdelt2;
   int      *innaxis1, *innaxis2;
   int      *cntr;
   char    **infile;
   char    **inarea;

   int       icntr;
   int       ifname;
   int       inaxis1;
   int       inaxis2;
   int       ictype1;
   int       ictype2;
   int       icrval1;
   int       icrval2;
   int       icrpix1;
   int       icrpix2;
   int       icdelt1;
   int       icdelt2;

   double    nom_crval1, nom_crval2;
   double    nom_cdelt1, nom_cdelt2;
   double    dtr;

   double    valOffset;

   struct mAddReturn *returnStruct;

   
   if(inpath == (char *)NULL)
      strcpy(path, ".");
   else
      strcpy(path, inpath);


   /*************************************************/
   /* Initialize output FITS basic image parameters */
   /*************************************************/

   int  bitpix = DOUBLE_IMG; 
   long naxis  = 2;  


   /************************************************/
   /* Make a NaN value to use setting blank pixels */
   /************************************************/

   union
   {
      double d;
      char   c[8];
   }
   value;

   double nan;

   for(i=0; i<8; ++i)
      value.c[i] = (char)255;

   nan = value.d;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mAddReturn *)malloc(sizeof(struct mAddReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /****************/
   /* Start timing */
   /****************/

   dtr = atan(1.)/45.;

   time(&currtime);
   start = currtime;

   mAdd_debug = debugin;


   /***********************************************/
   /* Check header and set up name of output file */
   /***********************************************/

   strcpy(output_file, outfile);

   checkHdr = montage_checkHdr(template_file, 1, 0);

   if(checkHdr)
   {
      strcpy(returnStruct->msg, checkHdr);
      return returnStruct;
   }

   if(strlen(output_file) > 5 &&
      strncmp(output_file+strlen(output_file)-5, ".fits", 5) == 0)
         output_file[strlen(output_file)-5] = '\0';

   if(strlen(output_file) > 5 &&
      strncmp(output_file+strlen(output_file)-5, ".FITS", 5) == 0)
         output_file[strlen(output_file)-5] = '\0';

   if(strlen(output_file) > 4 &&
      strncmp(output_file+strlen(output_file)-4, ".fit", 4) == 0)
         output_file[strlen(output_file)-4] = '\0';

   if(strlen(output_file) > 4 &&
      strncmp(output_file+strlen(output_file)-4, ".FIT", 4) == 0)
         output_file[strlen(output_file)-4] = '\0';

   strcpy(output_area_file, output_file);
   strcat(output_file,  ".fits");
   strcat(output_area_file, "_area.fits");

   if(mAdd_debug >= 1)
   {
      printf("image list       = [%s]\n", tblfile);
      printf("output_file      = [%s]\n", output_file);
      printf("output_area_file = [%s]\n", output_area_file);
      printf("template_file    = [%s]\n", template_file);
      fflush(stdout);
   }


   /*************************************************/ 
   /* Process the output header template to get the */ 
   /* image size, coordinate system and projection  */ 
   /*************************************************/ 

   if(mAdd_readTemplate(template_file) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   ptr = ctype + strlen(ctype) - 3;

   if (strcmp(ptr, "CAR") == 0)
      wrap=1;


   /*****************************/ 
   /* Open the image list table */
   /* to get metadata for input */
   /* files                     */
   /*****************************/ 

   ncols = topen(tblfile);

   if(ncols <= 0)
   {
      sprintf(returnStruct->msg, "Invalid image metadata file: %s", tblfile);
      return returnStruct;
   }


   /**************************/
   /* Get indices of columns */
   /**************************/

   icntr   = tcol("cntr");
   ifname  = tcol("fname");
   ictype1 = tcol("ctype1");
   ictype2 = tcol("ctype2");
   icdelt1 = tcol("cdelt1");
   icdelt2 = tcol("cdelt2");
   icrval1 = tcol("crval1");
   icrval2 = tcol("crval2");
   icrpix1 = tcol("crpix1");
   icrpix2 = tcol("crpix2");
   inaxis1 = tcol("naxis1");
   inaxis2 = tcol("naxis2");

   namelen = strlen(path) + tbl_rec[ifname].colwd + 16;


   /***********************************/
   /* Look for alternate column names */
   /***********************************/

   if(ifname < 0)
      ifname = tcol( "file");

   if (inaxis1 < 0)
     inaxis1 = tcol("ns");

   if (inaxis2 < 0)
     inaxis2 = tcol("nl");


   /**************************************/
   /* Were all required columns present? */
   /**************************************/

   if(icntr   < 0 || ifname  < 0 || icdelt1 < 0 || icdelt2 < 0 || icrpix1 < 0 
   || icrpix2 < 0 || inaxis1 < 0 || inaxis2 < 0 || icrval1 < 0 || icrval2 < 0
   || ictype1 < 0 || ictype2 < 0)
   {
      strcpy(returnStruct->msg, "Need columns: cntr,fname, crpix1, crpix2, cdelt1, cdelt2, naxis1, naxis2, crval1, crval2 ctype1, ctype2 in image list");
      return returnStruct;
   }


   /*************************************************/
   /* Allocate memory for file metadata table info  */
   /*************************************************/

   cntr     = (int *)    malloc(maxfile * sizeof(int)   );
   infile   = (char **)  malloc(maxfile * sizeof(char *));
   inarea   = (char **)  malloc(maxfile * sizeof(char *));
   inctype1 = (char **)  malloc(maxfile * sizeof(char *));
   inctype2 = (char **)  malloc(maxfile * sizeof(char *));
   incdelt1 = (double *) malloc(maxfile * sizeof(double));
   incdelt2 = (double *) malloc(maxfile * sizeof(double));
   incrval1 = (double *) malloc(maxfile * sizeof(double));
   incrval2 = (double *) malloc(maxfile * sizeof(double));
   incrpix1 = (double *) malloc(maxfile * sizeof(double));
   incrpix2 = (double *) malloc(maxfile * sizeof(double));
   innaxis1 = (int *)    malloc(maxfile * sizeof(int)   );
   innaxis2 = (int *)    malloc(maxfile * sizeof(int)   );


   for(ifile=0; ifile<maxfile; ++ifile)
   {
      infile[ifile] = (char *)malloc(namelen*sizeof(char));
      inarea[ifile] = (char *)malloc(namelen*sizeof(char));

      inctype1[ifile] = (char *)malloc(32*sizeof(char));
      inctype2[ifile] = (char *)malloc(32*sizeof(char));
   }

   if(mAdd_debug >= 1)
   {
      time(&currtime);
      printf("Memory allocated for file metadata table info [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /********************************************/
   /* Read the records and save the file info  */
   /********************************************/

   nfile      = 0;
   haveMinMax = 0;

   while(1)
   {
      status = tread();

      if(status < 0)
         break;

      cntr[nfile] = atoi(tval(icntr));

      incdelt1[nfile] = atof(tval(icdelt1));
      incdelt2[nfile] = atof(tval(icdelt2));
      incrval1[nfile] = atof(tval(icrval1));
      incrval2[nfile] = atof(tval(icrval2));
      incrpix1[nfile] = atof(tval(icrpix1));
      incrpix2[nfile] = atof(tval(icrpix2));
      innaxis1[nfile] = atoi(tval(inaxis1));
      innaxis2[nfile] = atoi(tval(inaxis2));

      strcpy(inctype1[nfile], tval(ictype1));
      strcpy(inctype2[nfile], tval(ictype2));


      /********************************/
      /* If dealing with cartesian    */
      /* images, look for wrap-around */
      /* cases and adjust crpix       */
      /* values by 360 degrees        */
      /********************************/

      if (wrap)
      {
        /* Try subtracting 360 degree increments first */

        while ((fabs(incrpix1[nfile] - output.crpix1)) > output.naxes[0])
        {
          tryval = incrpix1[nfile] - (360/fabs(incdelt1[nfile]));

          if (fabs(tryval - output.crpix1) > fabs(incrpix1[nfile] - output.crpix1))
          {
            /* We went the wrong direction:         */
            break;
          }
             /* Got closer to the header */
           incrpix1[nfile] = tryval;
        }

        /* Try adding increments of 360 degrees */

        while ((fabs(incrpix1[nfile] - output.crpix1)) > output.naxes[0])
        {
          tryval = incrpix1[nfile] + (360/fabs(incdelt1[nfile]));

          if (fabs(tryval - output.crpix1) > fabs(incrpix1[nfile] - output.crpix1))
          {
            /* We went the wrong direction:         */
            break;
          }
             /* Got closer to the header */
           incrpix1[nfile] = tryval;
        }
      }

      /* Look for maximum height/width */
      if (!haveMinMax)
      {
        imax = incrpix1[nfile];
        imin = incrpix1[nfile] - innaxis1[nfile]+1;
        jmax = incrpix2[nfile];
        jmin = incrpix2[nfile] - innaxis2[nfile]+1;

        haveMinMax = 1;
      }
      else
      {
        if (imax < incrpix1[nfile]) imax = incrpix1[nfile];

        if (imin > incrpix1[nfile] - innaxis1[nfile]+1) 
          imin = incrpix1[nfile] - innaxis1[nfile]+1;

        if (jmax < incrpix2[nfile]) jmax = incrpix2[nfile];

        if (jmin > incrpix2[nfile] - innaxis2[nfile]+1)
          jmin = incrpix2[nfile] - innaxis2[nfile]+1;
      }


      /* Get filename */

      strcpy(filename, montage_filePath(path, tval(ifname)));


      /* Need to build _area filenames if we have area images */

      if (haveAreas)
      {
        if(strlen(filename) > 5 &&
             strncmp(filename+strlen(filename)-5, ".fits", 5) == 0)
            filename[strlen(filename)-5] = '\0';
      }

      strcpy(infile[nfile], filename);

      if (haveAreas)
      {
        strcat(infile[nfile],  ".fits");
        strcpy(inarea[nfile], filename);
        strcat(inarea[nfile], "_area.fits");
      }


      ++nfile;

      if(nfile == maxfile)
      {
         /* Increase the default size of arrays */

         maxfile += MAXFILE;

         cntr     = (int *)    realloc(cntr,     maxfile * sizeof(int)   );
         infile   = (char **)  realloc(infile,   maxfile * sizeof(char *));
         inarea   = (char **)  realloc(inarea,   maxfile * sizeof(char *));
         inctype1 = (char **)  realloc(inctype1, maxfile * sizeof(char *));
         inctype2 = (char **)  realloc(inctype2, maxfile * sizeof(char *));
         incrval1 = (double *) realloc(incrval1, maxfile * sizeof(double));
         incrval2 = (double *) realloc(incrval2, maxfile * sizeof(double));
         incrpix1 = (double *) realloc(incrpix1, maxfile * sizeof(double));
         incrpix2 = (double *) realloc(incrpix2, maxfile * sizeof(double));
         innaxis1 = (int *)    realloc(innaxis1, maxfile * sizeof(int)   );
         innaxis2 = (int *)    realloc(innaxis2, maxfile * sizeof(int)   );
         incdelt1 = (double *) realloc(incdelt1, maxfile * sizeof(double));
         incdelt2 = (double *) realloc(incdelt2, maxfile * sizeof(double));

         for(ifile=nfile; ifile<maxfile; ++ifile)
         {
            infile[ifile] = (char *)malloc(namelen*sizeof(char));
            inarea[ifile] = (char *)malloc(namelen*sizeof(char));

            inctype1[ifile] = (char *)malloc(32*sizeof(char));
            inctype2[ifile] = (char *)malloc(32*sizeof(char));

            if(!inarea[ifile]) 
            { 
               mAdd_allocError("file info (realloc)");
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }
         }
      }
   }

   tclose();

   if(mAdd_debug >= 3)
   {
      printf("\n%d input files:\n\n", nfile);

      for(ifile=0; ifile<nfile; ++ifile)
      {
         if (haveAreas)
           printf("   [%s][%s]\n", infile[ifile], inarea[ifile]);
         else
           printf("   [%s]\n", infile[ifile]);
      }
      printf("\n");
      fflush(stdout);
   }

   if(mAdd_debug >= 1)
   {
      time(&currtime);
      printf("File metadata read [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /*****************************************************/
   /* Special handling for CAR projection, where we can */
   /* shift CRVAL/CRPIX by full pixels with impunity    */
   /*****************************************************/

   if(strstr(inctype1[0], "-CAR") != 0)
   {
      isCAR = 1;

      for(ifile=0; ifile<nfile; ++ifile)
      {
         valOffset = (output.crval1 - incrval1[ifile])/incdelt1[ifile];

         if(fabs(floor(valOffset+0.5) - valOffset) > 0.1)
         {
            sprintf(errstr, "CRVAL1 CAR pixel offset (%-g) not integer for image %s", valOffset, infile[ifile]);
            mAdd_printError(errstr);
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }

         incrpix1[ifile] = incrpix1[ifile] + valOffset;


         valOffset = (output.crval2 - incrval2[ifile])/incdelt2[ifile];

         if(fabs(floor(valOffset+0.5) - valOffset) > 0.1)
         {
            sprintf(errstr, "CRVAL2 CAR pixel offset (%.2f) not integer for image %s", valOffset, infile[ifile]);
            mAdd_printError(errstr);
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }

         incrpix2[ifile] = incrpix2[ifile] + valOffset;


         incrval1[ifile] = output.crval1;
         incrval2[ifile] = output.crval2;
      }
   }



   /*************************************************/
   /* Allocate memory for input fileinfo structures */
   /*************************************************/

   input = (struct fileinfo *)malloc(maxfile * sizeof(struct fileinfo));

   if(!input)
   {
      mAdd_allocError("file info structs");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if (haveAreas)
   {
      input_area = (struct fileinfo *)malloc(maxfile * sizeof(struct fileinfo));
      
      if(!input_area)
      {
         mAdd_allocError("area file info structs");
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }
   
   if(mAdd_debug >= 1)
   {
      time(&currtime);
      printf("Memory allocated for file info structures [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /*********************************************************/
   /* Is the output image smaller than the header template? */
   /* If so, change NAXIS UNLESS exact size flag was set    */
   /*********************************************************/

   if (shrink && (imax - imin + 1 < output.naxes[0]))
   {
     output.naxes[0] = imax - imin + 1;
     output.crpix1 = imax; /* left side of inputs */
   }

   if (shrink && (jmax - jmin + 1 < output.naxes[1]))
   {
     output.naxes[1] = jmax - jmin + 1;
     output.crpix2 = jmax; /* bottom side of inputs */
   }

   if (mAdd_debug >= 1)
   {
     printf("output.naxes[0] = %ld\n", output.naxes[0]);
     printf("output.naxes[1] = %ld\n", output.naxes[1]);
     printf("output.crpix1   = %lf\n", output.crpix1);
     printf("output.crpix2   = %lf\n", output.crpix2);
     fflush(stdout);
   }


   /*************************************/
   /* Allocate memory for input buffers */
   /*************************************/

   inbuflen = (int)(fabs(imax-imin)+0.5);

   if( output.naxes[0] > inbuflen)
      inbuflen = output.naxes[0];

   if (mAdd_debug >= 1)
   {
     printf("Input buffer length = %d\n", inbuflen);
     fflush(stdout);
   }

   input_buffer      = (double *)malloc(inbuflen * sizeof(double));
   input_buffer_area = (double *)malloc(inbuflen * sizeof(double));

   if(!input_buffer)     
   {
      mAdd_allocError("input buffer");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }
   if(!input_buffer_area)
   {
      mAdd_allocError("input area buffer");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAdd_debug >= 1)
   {
      time(&currtime);
      printf("Memory allocated for input buffers [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }
     

   /*****************************************************/
   /* Build array of fileinfo structures on input files */
   /*****************************************************/

   if(mAdd_debug >= 2)
   {
      printf("\nFILE RANGES\n");
      printf(" i   start   end   offset\n");
      printf("---- ------ ------ ------\n");
      fflush(stdout);
   }

   for (ifile = 0; ifile < nfile; ++ifile)
   {
      /*****************************************/
      /* Open file, get basic info from header */
      /*****************************************/

      if (ifile == 0)
      {
         nom_crval1 = incrval1[ifile];
         nom_crval2 = incrval2[ifile];
         nom_cdelt1 = incdelt1[ifile];
         nom_cdelt2 = incdelt2[ifile];
      }

      /****************************************************/
      /* Check that all files are in the same pixel space */
      /****************************************************/

      if(incrval1[ifile] != nom_crval1 
      || incrval2[ifile] != nom_crval2
      || incdelt1[ifile] != nom_cdelt1
      || incdelt2[ifile] != nom_cdelt2)
      {
         mAdd_printError("Images are not in same pixel space");
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }


      /************************************************/
      /* Sum up nominal areas of pixels in each image */
      /************************************************/

      nominal_area += incdelt1[ifile] * incdelt2[ifile];


      /**********************************/
      /* Find the output lines on which */
      /* this file starts/ends          */
      /**********************************/
 
      input[ifile].start = output.crpix2 - incrpix2[ifile] + 1;
      input[ifile].end   = input[ifile].start + innaxis2[ifile]-1;

      if(input[ifile].end > output.naxes[1])
         input[ifile].end = output.naxes[1];


      /***********************************/
      /* Find the output column on which */
      /* this file starts                */
      /***********************************/

      if(output.crpix1 > incrpix1[ifile])
         input[ifile].offset =  floor(output.crpix1 - incrpix1[ifile] + 0.5);
      else
         input[ifile].offset = -floor(incrpix1[ifile] - output.crpix1 + 0.5);

      if (mAdd_debug >= 2)
      {
         printf("%4d %6d %6d %6d\n", ifile, input[ifile].start, input[ifile].end, input[ifile].offset);
         fflush(stdout);
      }


      /**************************************/
      /* Initialize each "isopen" flag to 0 */
      /**************************************/

      input[ifile].isopen = 0;

      if (haveAreas)
         input_area[ifile].isopen = 0;
   }


   /**********************************************/
   /* Build a sorted list of starting and ending */
   /* lines for input images                     */
   /**********************************************/
   
   startline = (int *)malloc(maxfile * sizeof(int));
   startfile = (int *)malloc(maxfile * sizeof(int));
   endline   = (int *)malloc(maxfile * sizeof(int));
   endfile   = (int *)malloc(maxfile * sizeof(int));

   if(!endfile)
   {
      mAdd_allocError("start/end info");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   for(i=0; i<nfile; ++i)
   {
      startline[i] = input[i].start;
      startfile[i] = i;
      endline  [i] = input[i].end;
      endfile  [i] = i;
    }

    for(i=nfile-2; i>=0; --i)
    {
       for(j=0; j<=i; ++j)
       {
          if(startline[j] > startline[j+1])
          {
             itemp          = startline[j];
             startline[j]   = startline[j+1];
             startline[j+1] = itemp;

             itemp          = startfile[j];
             startfile[j]   = startfile[j+1];
             startfile[j+1] = itemp;
          }
       }
    }

    for(i=nfile-2; i>=0; --i)
    {
       for(j=0; j<=i; ++j)
       {
          if(endline[j] > endline[j+1])
          {
             itemp        = endline[j];
             endline[j]   = endline[j+1];
             endline[j+1] = itemp;

             itemp        = endfile[j];
             endfile[j]   = endfile[j+1];
             endfile[j+1] = itemp;
          }
       }
    }

    if(mAdd_debug >= 2)
    {
       printf("\nSTART LINES:\n");
       printf(" i   start   file \n");
       printf("---- ------ ------\n");

       for(i=0; i<nfile; ++i)
          printf("%4d %6d %6d\n", i, startline[i], startfile[i]);
       fflush(stdout);

       printf("\nEND LINES:\n");
       printf(" i    end    file \n");
       printf("---- ------ ------\n");

       for(i=0; i<nfile; ++i)
          printf("%4d %6d %6d\n", i, endline[i], endfile[i]);
       fflush(stdout);
    }

   if(mAdd_debug >= 1)
   {
      time(&currtime);
      printf("File start/end information organized [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /************************/
   /* Average nominal_area */
   /************************/

   nominal_area = fabs(nominal_area) * dtr * dtr / nfile;


   /*************************************************************/ 
   /* Allocate memory for pixdepth lines of output image pixels */ 
   /* We will modify pixel depth dynamically if need be         */ 
   /*************************************************************/ 

   pixdepth = PIXDEPTH;

   dataline = (double **)malloc(output.naxes[0] * sizeof(double *));

   if(!dataline)
   {
      mAdd_allocError("data line pointers");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   for (i = 0; i < output.naxes[0]; ++i)
   {
      dataline[i] = (double *)malloc(pixdepth * sizeof(double));

      if(!dataline[i])
      {
         mAdd_allocError("data line");
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }


   arealine = (double **)malloc(output.naxes[0] * sizeof(double *));

   if(!arealine)
   {
      mAdd_allocError("area line pointers");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   for (i = 0; i < output.naxes[0]; ++i)
   {
      arealine[i] = (double *)malloc(pixdepth * sizeof(double));

      if(!arealine[i])
      {
         mAdd_allocError("area line");
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }

   datacount = (int *)malloc(output.naxes[0] * sizeof(int));

   if(!datacount)
   {
      mAdd_allocError("data counts");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   if(mAdd_debug >= 1)
   {
      time(&currtime);
      printf("Memory allocated for input data buffer [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /***********************************/
   /* Allocate memory for output line */
   /***********************************/

   outdataline = (double *)malloc(output.naxes[0] * sizeof(double));
   outarealine = (double *)malloc(output.naxes[0] * sizeof(double));

   if(!outdataline)
   {
      mAdd_allocError("output data line");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }
   if(!outarealine)
   {
      mAdd_allocError("output area line");
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }
     
   if(mAdd_debug >= 1)
   {
      time(&currtime);
      printf("Memory allocated for output data buffers [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }

   /************************************/
   /* Delete pre-existing output files */
   /************************************/

   remove(output_file);               
   remove(output_area_file);               
 

   /***********************/
   /* Create output files */
   /***********************/

   status = 0;
   if(fits_create_file(&output.fptr, output_file, &status)) 
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_create_file(&output_area.fptr, output_area_file, &status)) 
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   open_files = 2;



   /*********************************************************/
   /* Create the FITS image.  All the required keywords are */
   /* handled automatically.                                */
   /*********************************************************/

   status = 0;
   if (fits_create_img(output.fptr, bitpix, naxis, output.naxes, &status))
   {
      mAdd_printFitsError(status);          
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAdd_debug >= 1)
   {
      printf("FITS data image created (not yet populated)\n"); 
      fflush(stdout);
   }

   status = 0;
   if (fits_create_img(output_area.fptr, bitpix, naxis, output_area.naxes, &status))
   {
      mAdd_printFitsError(status);          
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAdd_debug >= 1)
   {
      printf("FITS area image created (not yet populated)\n"); 
      fflush(stdout);
   }

   if(mAdd_debug >= 1)
   {
      time(&currtime);
      printf("Output FITS files created [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /****************************************/
   /* Set FITS header from a template file */
   /****************************************/

   status = 0;
   if(fits_write_key_template(output.fptr, template_file, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAdd_debug >= 1)
   {
      printf("Template keywords written to FITS data image\n"); 
      fflush(stdout);
   }

   status = 0;
   if(fits_write_key_template(output_area.fptr, template_file, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAdd_debug >= 1)
   {
      printf("Template keywords written to FITS area image\n"); 
      fflush(stdout);
   }


   /**********************************/
   /* Modify BITPIX to be DOUBLE_IMG */
   /**********************************/

   status = 0;
   if(fits_update_key_lng(output.fptr, "BITPIX", DOUBLE_IMG,
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_update_key_lng(output_area.fptr, "BITPIX", DOUBLE_IMG,
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /***************************/
   /* Update NAXIS keywords   */
   /***************************/

   status = 0;
   if(fits_update_key_lng(output.fptr, "NAXIS", 2,
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   status = 0;
   if(fits_update_key_lng(output.fptr, "NAXIS1", output.naxes[0],
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_update_key_lng(output.fptr, "NAXIS2", output.naxes[1],
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_update_key_lng(output_area.fptr, "NAXIS", 2,
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_update_key_lng(output_area.fptr, "NAXIS1", output.naxes[0],
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_update_key_lng(output_area.fptr, "NAXIS2", output.naxes[1],
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*************************/
   /* Update CRPIX keywords */
   /*************************/

   status = 0;
   if(fits_update_key_dbl(output.fptr, "CRPIX1", output.crpix1, -14,
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_update_key_dbl(output.fptr, "CRPIX2", output.crpix2, -14,
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   status = 0;
   if(fits_update_key_dbl(output_area.fptr, "CRPIX1", output.crpix1, -14,
                                  (char *)NULL, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_update_key_dbl(output_area.fptr, "CRPIX2", output.crpix2, -14,
                                  (char *)NULL, &status))
   {
     mAdd_printFitsError(status);           
     strcpy(returnStruct->msg, montage_msgstr);
     return returnStruct;
   }

   if(mAdd_debug >= 1)
   {
      time(&currtime);
      printf("Output FITS headers updated [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /********************************************/
   /* Build/write one line of output at a time */
   /********************************************/

   haveMinMax = 0;

   currentstart = 0;
   currentend   = 0;

   if(mAdd_listInit() > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   for (lineout=1; lineout<=output.naxes[1]; ++lineout)
   {
      if (mAdd_debug >= 2)
      {
        printf("\nOUTPUT LINE %d\n",lineout);
        fflush(stdout);
      }

      if (mAdd_debug == 1)
      {
         printf("\r Processing line: %d", lineout);
         fflush(stdout);
      }

      for(i=0; i<output.naxes[0]; ++i)
         datacount[i] = 0;


      /*********************************/
      /* Update the "contributor" list */
      /*********************************/

      while(1)
      {
         if(currentstart >= nfile)
            break;

         if(startline[currentstart] > lineout)
            break;

         if(mAdd_listAdd(startfile[currentstart]) > 0)
         {
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }

         ++currentstart;
      }
      
      while(1)
      {
         if(currentend >= nfile)
            break;

         if(endline[currentend] > lineout - 1)
            break;

         ifile = endfile[currentend];

         mAdd_listDelete(ifile);

         if(input[ifile].isopen)
         {
            status = 0;
            if(fits_close_file(input[ifile].fptr, &status))
            {
               mAdd_printFitsError(status);           
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            input[ifile].isopen = 0;

            --open_files;
         }
        
         if(haveAreas
         && input_area[ifile].isopen)
         {
            status = 0;
            if(fits_close_file(input_area[ifile].fptr, &status))
            {
               mAdd_printFitsError(status);           
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            input_area[ifile].isopen = 0;

             --open_files;
         }

         ++currentend;
      }

      imgcount = mAdd_listCount();


      /******************************************/
      /* Read from files that overlap this line */
      /******************************************/
     
      if (mAdd_debug >= 2) 
      {
         printf("\nContributing files (%d):\n\n", imgcount);
         printf(" i   isopen   open/max      infile[i]       \n");
         printf("---- ------ ------------ -------------------\n");
         fflush(stdout);
      }

      for(j=0; j<imgcount; ++j)
      {
         ifile = mAdd_listIndex(j);

         if(mAdd_debug >= 2)
         {
            printf("%4d %4d %6d/%6d %s\n",
               ifile, input[ifile].isopen, open_files, MAXFITS, infile[ifile]);
            fflush(stdout);
         }

         if (input[ifile].isopen == 0)
         {
            /* Open files that aren't already open */

            ++open_files;
 
            if (open_files > MAXFITS)
            {
               sprintf(returnStruct->msg, "Too many open files");
               return returnStruct;
            }
 
            status = 0;
            if(fits_open_file(&input[ifile].fptr, infile[ifile], READONLY, &status))
            {
               sprintf(errstr, "Image file %s missing or invalid FITS", infile[ifile]);
                
               mAdd_printError(errstr);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            if(mAdd_debug >= 2)
            {
               printf("Open:  %4d\n", ifile); 
               fflush(stdout);
            }


            input[ifile].isopen = 1;
 
            if(haveAreas)
            {
               ++open_files;

               if (open_files > MAXFITS)
               {
                  sprintf(returnStruct->msg, "Too many open files");
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }

               status = 0;
               if(fits_open_file(&input_area[ifile].fptr, inarea[ifile], READONLY, &status))
               {
                  sprintf(errstr, "Area file %s missing or invalid FITS", inarea[ifile]);
                  mAdd_printError(errstr);
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }

               input_area[ifile].isopen = 1;
            }


            /* Get the WCS and check it against */
            /* the one for the header template  */

            status = 0;
            if(fits_get_image_wcs_keys(input[ifile].fptr, &inputHeader, &status))
            {
               mAdd_printFitsError(status);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            if(mAdd_debug >= 3)
            {
               printf("Input header to wcsinit() [imgWCS]:\n%s\n", inputHeader);
               fflush(stdout);
            }

            imgWCS = wcsinit(inputHeader);

            if(imgWCS == (struct WorldCoor *)NULL)
            {
               sprintf(returnStruct->msg, "Input wcsinit() failed.");
               return returnStruct;
            }

            if(strcmp(imgWCS->c1type, hdrWCS->c1type) != 0)
            {
               sprintf(errstr, "Image %s header CTYPE1 does not match template", infile[ifile]);
               mAdd_printError(errstr);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            if(strcmp(imgWCS->c2type, hdrWCS->c2type) != 0)
            {
               sprintf(errstr, "Image %s header CTYPE2 does not match template", infile[ifile]);
               mAdd_printError(errstr);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            if(!isCAR)
            {
               if(fabs(imgWCS->xref - hdrWCS->xref) > 1.e-8)
               {
                  sprintf(errstr, "Image %s header CRVAL1 does not match template", infile[ifile]);
                  mAdd_printError(errstr);
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }

               if(fabs(imgWCS->yref - hdrWCS->yref) > 1.e-8)
               {
                  sprintf(errstr, "Image %s header CRVAL2 does not match template", infile[ifile]);
                  mAdd_printError(errstr);
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }
            }

            if(fabs(imgWCS->cd[0] - hdrWCS->cd[0]) > 1.e-8)
            {
               sprintf(errstr, "Image %s header CD/CDELT1 does not match template (%.8f vs %.8f)", infile[ifile], imgWCS->cd[0], hdrWCS->cd[0]);
               mAdd_printError(errstr);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            if(fabs(imgWCS->cd[1] - hdrWCS->cd[1]) > 1.e-8)
            {
               sprintf(errstr, "Image %s header CD/CDELT2 does not match template (%.8f vs %.8f)", infile[ifile], imgWCS->cd[1], hdrWCS->cd[1]);
               mAdd_printError(errstr);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            if(imgWCS->equinox != hdrWCS->equinox)
            {
               sprintf(errstr, "Image %s header EQUINOX does not match template", infile[ifile]);
               mAdd_printError(errstr);
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }
         } 


         /**************************************************************/ 
         /* For line from input file corresponding to this output line */
         /**************************************************************/ 
 
         fpixel[0] = 1;
         fpixel[1] = (lineout - input[ifile].start) + 1;
         fpixel[2] = 1;
         fpixel[3] = 1;

         nelements = innaxis1[ifile];

         if (mAdd_debug >= 3)
         {
            printf("Reading line from %d:\n", ifile);
            printf("fpixel[1] = %ld\n", fpixel[1]);
            printf("nelements = %ld\n", nelements);
            fflush(stdout);
         }

         if(fpixel[1] >= 1
         && fpixel[1] <= innaxis2[ifile])
         {
            /*****************/
            /* Read the line */
            /*****************/

            status = 0;
            if(fits_read_pix(input[ifile].fptr, TDOUBLE, fpixel, nelements, &nan,
                               input_buffer, &nullcnt, &status))
            {
              mAdd_printFitsError(status);
              strcpy(returnStruct->msg, montage_msgstr);
              return returnStruct;
            }

            if(haveAreas)
            {
               status = 0;
               if(fits_read_pix(input_area[ifile].fptr, TDOUBLE, fpixel, nelements, &nan,
                                  input_buffer_area, &nullcnt, &status))
               {
                 mAdd_printFitsError(status);
                 strcpy(returnStruct->msg, montage_msgstr);
                 return returnStruct;
               }
            }
            else
            {
               for(i=0; i<nelements; ++i)
                  input_buffer_area[i] = 1.000;
            }


            /**********************/
            /* Process the pixels */
            /**********************/

            for (i = 0; i<nelements; ++i)
            {
               /***********************************/
               /* If there's not a value here, we */
               /* won't add anything to dataline  */
               /***********************************/
    
               if (mNaN(input_buffer[i]) || input_buffer_area[i] <= 0.)
                  continue;
             
               /* Are we off the image? */
              
               ipix = i + input[ifile].offset;

               if (ipix <               0 ) continue;
               if (ipix >= output.naxes[0]) continue;
             

               /****************************************************/
               /* Not off the image, and not NaNs; add to dataline */
               /* corresponding to ifile                           */
               /****************************************************/
             
               jcnt = datacount[ipix];

               if(jcnt >= pixdepth)
               {
                  pixdepth += PIXDEPTH;

                  if(mAdd_debug >= 1)
                  {
                     printf("\nReallocating input data buffers; new depth = %d\n",
                        pixdepth);
                     fflush(stdout);
                  }

                  for (i=0; i<output.naxes[0]; ++i)
                  {
                     dataline[i] = (double *)realloc(dataline[i],
                        pixdepth * sizeof(double));

                     if(dataline[i] == (double *)NULL)
                     {
                        mAdd_allocError("data line (realloc)");
                        strcpy(returnStruct->msg, montage_msgstr);
                        return returnStruct;
                     }

                     arealine[i] = (double *)realloc(arealine[i],
                        pixdepth * sizeof(double));

                     if(arealine[i] == (double *)NULL)
                     {
                        mAdd_allocError("area line (realloc)");
                        strcpy(returnStruct->msg, montage_msgstr);
                        return returnStruct;
                     }
                  }

                  if(mAdd_debug >= 1)
                  {
                     printf("Memory reallocation complete\n");
                     fflush(stdout);
                  }
               }
               dataline[ipix][jcnt] = input_buffer[i];

               arealine[ipix][jcnt] = input_buffer_area[i];

               ++datacount[ipix];
            }
         }
         else
         {
            if (mAdd_debug >= 3)
            {
               printf("Nothing read: outside image bounds\n");
               fflush(stdout);
            }
         }


         /*****************************************/
         /* Done adding pixels to dataline stacks */
         /*                                       */
         /* Is it time to close this file?        */
         /* Either because we're at the           */
         /* bottom of it, or because we're        */
         /* running out of available file         */
         /* pointers?                             */
         /*****************************************/

         if (!showwarning && open_files >= MAXFITS) 
         {
            showwarning = 1;

            if(mAdd_debug >= 1)
            {
               printf("\nWARNING: Opening and closing files to avoid too many open FITS\n\n");
               fflush(stdout);
            }
         }

         if (open_files >= MAXFITS) 
         {
            status = 0;
            if(fits_close_file(input[ifile].fptr, &status))
            {
               mAdd_printFitsError(status);           
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }

            if(mAdd_debug >= 2)
            {
               printf("Close: %4d\n", ifile); 
               fflush(stdout);
            }

            input[ifile].isopen = 0;

            --open_files;
           
            if(haveAreas)
            {
               status = 0;
               if(fits_close_file(input_area[ifile].fptr, &status))
               {
                  mAdd_printFitsError(status);           
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }

               input_area[ifile].isopen = 0;

               --open_files;
            }
         }
      } 

     
      /***************************************************************/
      /* Done reading all the files that overlap this line of output */
      /*                                                             */
      /* Now to average each pixel and prepare the output pixels:    */
      /***************************************************************/

      for (i = 0; i<output.naxes[0]; ++i)
      {
         outdataline[i] = 0;
         outarealine[i] = 0;

         avg_status=0;


         /**********************************/
         /* Average this "stack" of pixels */
         /* according to the user-chosen   */
         /* averaging method               */
         /**********************************/

         if(datacount[i] > 0)
         {
            if (coadd == MEAN)
               avg_status = mAdd_avg_mean(dataline[i], arealine[i], 
                  &outdataline[i], &outarealine[i], datacount[i]);

            else if (coadd == MEDIAN)
               avg_status = mAdd_avg_median(dataline[i], arealine[i], 
                  &outdataline[i], &outarealine[i], datacount[i], nominal_area);

            else if (coadd == COUNT)
               avg_status = mAdd_avg_count(dataline[i], arealine[i], 
                  &outdataline[i], &outarealine[i], datacount[i]);

            else if (coadd == SUM)
               avg_status = mAdd_avg_sum(dataline[i], arealine[i], 
                  &outdataline[i], &outarealine[i], datacount[i]);

            if (avg_status)
            {
               outdataline[i] = nan;
               outarealine[i] = 0;
            }
         }
         else
         {
            outdataline[i] = nan;
            outarealine[i] = 0;
         }
      }


      /****************************************/
      /* Every input value for this pixel has */
      /* been averaged, and set to NaN if     */
      /* nothing overlapped it.               */
      /* Write this line to output FITS files */   
      /****************************************/
    
      fpixel[0] = 1;
      fpixel[1] = lineout; 
      fpixel[2] = 1;
      fpixel[3] = 1;
      nelements = output.naxes[0];

      status = 0;
      if (fits_write_pix(output.fptr, TDOUBLE, fpixel, nelements,
                         (void *)(&outdataline[0]), &status))
      {
         mAdd_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      status = 0;
      if (fits_write_pix(output_area.fptr, TDOUBLE, fpixel, nelements,
                         (void *)(&outarealine[0]), &status))
      {
         mAdd_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }


   /*******************************/
   /* Close the output FITS files */
   /*******************************/

   status = 0;
   if(fits_close_file(output.fptr, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   status = 0;
   if(fits_close_file(output_area.fptr, &status))
   {
      mAdd_printFitsError(status);           
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mAdd_debug >= 1)
   {
      printf("FITS images finalized\n"); 
      fflush(stdout);
   }

   time(&currtime);

   returnStruct->status = 0;

   sprintf(returnStruct->msg,    "time=%.0f",    (double)(currtime - start)); 
   sprintf(returnStruct->json, "{\"time\":%.1f}", (double)(currtime - start));

   returnStruct->time = (double)(currtime - start);

   return returnStruct;
}


/**************************************************/
/*                                                */
/*  Read the output header template file.         */
/*  Specifically extract the image size info.     */
/*                                                */
/**************************************************/

int mAdd_readTemplate(char *filename)
{
   int       i, j;
   FILE     *fp;
   char      line     [MAXSTR];
   char      headerStr[HDRLEN];


   /********************************************************/
   /* Open the template file, read and parse all the lines */
   /********************************************************/

   fp = fopen(filename, "r");

   if(fp == (FILE *)NULL)
   {
      mAdd_printError("Template file not found.");
      return 1;
   }

   strcpy(headerStr, "");

   for(j=0; j<1000; ++j)
   {
      if(fgets(line, MAXSTR, fp) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';
      
      if(line[strlen(line)-1] == '\r')
         line[strlen(line)-1]  = '\0';

      if(mAdd_debug >= 3)
      {
         printf("Template line: [%s]\n", line);
         fflush(stdout);
      }

      for(i=strlen(line); i<80; ++i)
         line[i] = ' ';
      
      line[80] = '\0';

      mAdd_stradd(headerStr, line);

      mAdd_parseLine(line);
   }

   fclose(fp);

   hdrWCS = wcsinit(headerStr);

   if(hdrWCS == (struct WorldCoor *)NULL)
   {
      sprintf(montage_msgstr, "Bad WCS in header template.");
      return 1;
   }

   return 0;
}



/**************************************************/
/*                                                */
/*  Parse header lines from the template,         */
/*  looking for NAXIS1, NAXIS2, CRPIX1 and CRPIX2 */
/*                                                */
/**************************************************/

void mAdd_parseLine(char *line)
{
   char *keyword;
   char *value;
   char *end;

   int   len;

   len = strlen(line);

   keyword = line;

   while(*keyword == ' ' && keyword < line+len)
      ++keyword;
   
   end = keyword;

   while(*end != ' ' && *end != '=' && end < line+len)
      ++end;

   value = end;

   while((*value == '=' || *value == ' ' || *value == '\'')
         && value < line+len)
      ++value;
   
   *end = '\0';
   end = value;

   if(*end == '\'')
      ++end;

   while(*end != ' ' && *end != '\'' && end < line+len)
      ++end;
   
   *end = '\0';

   if(mAdd_debug >= 2)
   {
      printf("keyword [%s] = value [%s]\n", keyword, value);
      fflush(stdout);
   }

   if(strcmp(keyword, "CTYPE1") == 0)
     strcpy(ctype, value);

   if(strcmp(keyword, "NAXIS1") == 0)
   {
      output.naxes[0] = atoi(value);
      output_area.naxes[0] = atoi(value);
   }

   if(strcmp(keyword, "NAXIS2") == 0)
   {
      output.naxes[1] = atoi(value);
      output_area.naxes[1] = atoi(value);
   }

   if(strcmp(keyword, "CRPIX1") == 0)
   {
      output.crpix1 = atof(value);
      output_area.crpix1 = atof(value);
   }

   if(strcmp(keyword, "CRPIX2") == 0)
   {
      output.crpix2 = atof(value);
      output_area.crpix2 = atof(value);
   }

   if(strcmp(keyword, "CRVAL1") == 0)
   {
      output.crval1 = atof(value);
      output_area.crval1 = atof(value);
   }

   if(strcmp(keyword, "CRVAL2") == 0)
   {
      output.crval2 = atof(value);
      output_area.crval2 = atof(value);
   }
}


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mAdd_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}



/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void mAdd_printError(char *msg)
{
   strcpy(montage_msgstr, msg);
   return;
}


/*******************************************************/
/* STRADD adds the string "card" to a header line, and */
/* pads the header out to 80 characters.               */
/*******************************************************/

int mAdd_stradd(char *header, char *card)
{
   int i;

   int hlen = strlen(header);
   int clen = strlen(card);

   for(i=0; i<clen; ++i)
      header[hlen+i] = card[i];

   if(clen < 80)
      for(i=clen; i<80; ++i)
         header[hlen+i] = ' ';

   header[hlen+80] = '\0';

   return(strlen(header));
}


/**************************************/
/* Find the mean of a stack of pixels */
/**************************************/

int mAdd_avg_mean(double data[], double area[], double *outdata, double *outarea, int count)
{
  int i;
  int isCovered = 0;

  *outdata = 0.;
  *outarea = 0.;

  for (i = 0; i < count; ++i)
  {
    /* Add up total flux from each file's contribution: */
    if (area[i] > 0.)
    {
      *outdata += data[i] * area[i];
      *outarea += area[i];
      isCovered = 1;
    }
  }
  if (!isCovered)
  {
    /* No area actually covered this pixel; */
    return 1;
  }      
  else
  {
    /* Normalize pixel using total area */
    *outdata /= *outarea;
    return 0;
  }
}


/**********************************************************/
/* Find the median data/area values from stacks of pixels */
/**********************************************************/

int mAdd_avg_median(double data[], double area[], double *outdata, double *outarea, int n, double nom_area)
{
  static int nalloc = 0;

  static double *sorted;
  static double *sortedarea;

  int i, nsort;

  if(nalloc == 0)
  {
     nalloc = 1024;

     sorted     = (double *)malloc(nalloc * sizeof(double));
     sortedarea = (double *)malloc(nalloc * sizeof(double));

     if(!sorted)
     {
        mAdd_allocError("median array");
        return 1;
      }
  }
  if(nalloc < 2*n)
  {
     nalloc = 2*n;

     sorted     = (double *)realloc(sorted,     nalloc * sizeof(double));
     sortedarea = (double *)realloc(sortedarea, nalloc * sizeof(double));

     if(!sorted)
     {
        mAdd_allocError("median array (realloc)");
        return 1;
      }
  }


  /**********************************************/
  /* Pick out the pixels that cover the defined */
  /* fraction of the nominal pixel area         */
  /**********************************************/

  nsort = 0;

  *outdata = 0.;
  *outarea = 0.;

  for (i = 0; i < n; ++i)
  {
    if (area[i] > MINCOVERAGE*nom_area)
    {
      sorted[nsort]     = data[i];
      sortedarea[nsort] = area[i];
      ++nsort;

      *outarea += area[i];
    }
  }

  if (nsort == 0)
  {
    /* No values covered enough area */
    return 1;
  }

  /* Sort the pixel values */
  
  mAdd_sort(sorted, sortedarea, nsort);

  if (nsort%2 != 0)
  {
    /* Odd number of values; use the one in the middle */

    *outdata = sorted[nsort/2];
  }

  else
  {
    /* Even number of values; average the two middle values     */
    /* unless exactly two, in which case take the lower value   */

    if(nsort == 2)
        *outdata = sorted[0];
    else
       *outdata = (sorted[nsort/2] + sorted[nsort/2-1]) / 2.;
  }

  return 0;
}


/*******************************************/
/* Find the count of pixels that have data */
/*******************************************/

int mAdd_avg_count(double data[], double area[], double *outdata, double *outarea, int count)
{
  int i;

  double value;

  *outdata = 0.;
  *outarea = 1.;

  if(count <= 0)
     return 1;

  value = 0.;

  for (i=0; i<count; ++i)
  {
    if(area[i] > 0. && data[i] > 0.)
      value += 1.;
  }

  *outdata = value;

  return 0;
}


/*************************************/
/* Find the sum of a stack of pixels */
/*************************************/

int mAdd_avg_sum(double data[], double area[], double *outdata, double *outarea, int count)
{
  int i;
  int isCovered = 0;

  *outdata = 0.;
  *outarea = 0.;

  for (i = 0; i < count; ++i)
  {
    /* Add up total flux from each file's contribution: */
    if (area[i] > 0.)
    {
      *outdata += data[i];
      *outarea += area[i];
      isCovered = 1;
    }
  }
  if (!isCovered)
  {
    /* No area actually covered this pixel; */
    return 1;
  }      
  else
  {
    /* Normalize pixel using total area */
    return 0;
  }
}


/**********************************************/
/*                                            */
/*  Sort a set of pixel values.  Carry the    */
/*  area along as well.                       */
/*                                            */
/**********************************************/

void mAdd_sort(double *data, double *area, int n)
{
  unsigned long i, j;
  double tmp, tmp2;

  for (i = 1; i < n; ++i)
  {
    for (j = i; j > 0 && (data[j-1] > data[j]); j--)
    {
      tmp = data[j];
      tmp2 = area[j];
      data[j] = data[j-1];
      area[j] = area[j-1];
      data[j-1] = tmp;
      area[j-1] = tmp2;
    }
  }
}


/**********************************************/
/*                                            */
/*  Routines for maintaining linked lists for */
/*  keeping track of files that need to be    */
/*  included in the current output line       */
/*                                            */
/**********************************************/


int mAdd_listInit()
{
   int i;

   nlistElement = MAXLIST;

   listElement = (struct ListElement **)
      malloc(nlistElement * sizeof(struct ListElement *));

   for(i=0; i<nlistElement; ++i)
   {
      listElement[i] = (struct ListElement *)
         malloc(sizeof(struct ListElement));

      if(!listElement[i])
      {
         mAdd_allocError("linked list structs");
         return 1;
      }

      listElement[i]->used  =  0;
      listElement[i]->value = -1;
      listElement[i]->next  = -1;
      listElement[i]->prev  = -1;
   }

   listFirst = 0;
   listMax   = 0;

   return(0);
} 


int mAdd_listCount()
{
   return listMax;
}


int mAdd_listAdd(int value)
{
   int i, j, current, prev;

   current = listFirst;

   if(listMax == 0)
   {
      listElement[0]->value = value;

      listElement[0]->used  = 1;
      listElement[0]->next  = 1;
      ++listMax;

      return 0;
   }

   for(i=0; i<listMax; ++i)
   {
      prev    = current;
      current = listElement[current]->next;
   }

   listElement[current]->value = value;
   listElement[current]->used  = 1;
   listElement[current]->prev  = prev;

   for(i=0; i<nlistElement; ++i)
   {
      if(listElement[i]->used == 0)
         break;
   }

   if(i == nlistElement)
   {
      listElement = (struct ListElement **)
         realloc(listElement, 
         (nlistElement+MAXLIST) * sizeof(struct ListElement *));

      for(j=nlistElement; j<nlistElement+MAXLIST; ++j)
      {
         listElement[j] = (struct ListElement *)
            malloc(sizeof(struct ListElement));

         if(!listElement[j]) 
         {
            mAdd_allocError("linked list structs (additions)");
            return 1;
         }

         listElement[j]->used  =  0;
         listElement[j]->value = -1;
         listElement[j]->next  = -1;
         listElement[j]->prev  = -1;
      }

      nlistElement += MAXLIST;
   }

   listElement[current]->next = i;

   ++listMax;

   return 0;
}


int mAdd_listDelete(int value)
{
   int i, current, prev, next;

   current = listFirst;

   while(1)
   {
      if(!listElement[current]->used)
         break;

      if(listElement[current]->value == value)
      {
         --listMax;

         next = listElement[current]->next;
         prev = listElement[current]->prev;

         if(current == listFirst)
         {
            listFirst = next;
            
            if(!listElement[listFirst]->used)
            {
               for(i=0; i<nlistElement; ++i)
               {
                  listElement[i]->used  =  0;
                  listElement[i]->value = -1;
                  listElement[i]->next  = -1;
                  listElement[i]->prev  = -1;
               }

               listFirst = 0;
               listMax   = 0;

               return 0;
            }
         }
            
         listElement[current]->value = -1;
         listElement[current]->used  =  0;
         listElement[current]->next  = -1;
         listElement[current]->prev  = -1;

         if(prev == -1)
            listElement[next]->prev = prev;

         else if(next == -1)
            listElement[prev]->next = next;

         else
         {
            listElement[next]->prev = prev;
            listElement[prev]->next = next;
         }

         break;
      }

      current = listElement[current]->next;

      if(current == -1)
         break;
   }

   return 0;
}


int mAdd_listIndex(int index)
{
   int i;
   int current;

   i       = 0;
   current = listFirst;

   while(1)
   {
      if(!(listElement[current]->used))
         return(-1);

      if(i == index)
         return(listElement[current]->value);

      ++i;

      current = listElement[current]->next;

      if(current == -1)
         break;
   }

   return -1;
}
   
   
int mAdd_allocError(char *label)
{
   sprintf(montage_msgstr, "Allocation failed for %s.", label);
   return 0;
}
