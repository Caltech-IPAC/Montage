/* Module: mAdd.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.1      John Good        08Sep15  fits_read_pix() incorrect null value
1.0      John Good        08May15  Baseline code, based on mAdd.c of this date.

*/

int allocError(char *);


/*************************************************************************/
/*                                                                       */
/*  mCubeAdd                                                             */
/*                                                                       */
/*  This module, mCubeAdd, reads sets of flux / area coverage datacubes  */
/*  (the output of mCubeProj) which have already been projected /        */
/*  resampled onto the same pixel space.  The fluxs, scaled by total     */
/*  input area, are then coadded into a single output composite.         */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>

#include <fitsio.h>
#include <wcs.h>
#include <mtbl.h>

#include "montage.h"
#include "mNaN.h"

#define MAXSTR     256
#define MAXFILE     50
#define MAXFITS    200
#define MAXLIST    500
#define PIXDEPTH    50
#define HDRLEN   80000

#define MEAN   1
#define MEDIAN 2
#define MIDAVG 3
#define COUNT  4

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int debugCheck(char *debugStr);
int checkHdr(char *infile, int hdrflag, int hdu);
int listInit();
int listAdd(int value);
int listDelete(int value);
int listCount();
int listIndex(int index);
int stradd(char *header, char *card);


/* Fractional area minumum for median-type averaging */

#define MINCOVERAGE 0.5


/*********************/
/* Define prototypes */
/*********************/

void parseLine             (char *line);
void sort                  (double *data, double *area, int n);
void readTemplate          (char *filename);
int  avg_count             (double data[], double area[], double *outdata, 
                            double *outarea, int count);
int  avg_mean              (double data[], double area[], double *outdata, 
                            double *outarea, int count);
int  avg_median            (double data[], double area[], double *outdata, 
                            double *outarea, int n, double nom_area);
char *filePath             (char *path, char *fname);
void printFitsError        (int);
void printError            (char *);


/***************************/
/* Define global variables */
/***************************/

char ctype[MAXSTR];

static int maxfile = MAXFILE;

char output_file      [MAXSTR];
char output_area_file [MAXSTR];

struct WorldCoor *imgWCS;
struct WorldCoor *hdrWCS;

int  debug;
int  haveAreas = 0;
int  status    = 0;
int  haveAxis4 = 0;

static time_t currtime, start;


/*******************************************************/
/* Arrays to keep track of which files need to be open */
/*******************************************************/

int *startline;
int *startfile;
int *endline;
int *endfile;

int open_files; 


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
   int       j3start;
   int       j3offset;
   int       j3end;
}
*input, *input_area;
   

struct outfile
{
  fitsfile *fptr;
  int       naxis;
  long      naxes[4];
  double    crpix1, crpix2, crpix3, crpix4;
}
output, output_area;



/***********************/
/*  mAdd main routine  */
/***********************/

int main(int argc, char **argv)
{
   int       i, j, j3, j4, c, ncols, namelen, imgcount;
   int       lineout, itemp, pixdepth, ipix, jcnt;
   int       inbuflen;
   int       currentstart, currentend;
   int       showwarning = 0;

   double    try;

   char     *inputHeader;
   char     *ptr;
   int       wrap = 0;

   int       haveMinMax;
   int       shrink = 1;    
   int       avg_status = 0;

   long      fpixel[4], nelements;
   int       nullcnt;

   double    nominal_area = 0;

   int       coadd = MEAN;

   double    imin, imax;
   double    jmin, jmax;

   double  **dataline;
   double  **arealine;
   int      *datacount;
   double   *input_buffer;
   double   *input_buffer_area;
   double   *outdataline;
   double   *outarealine;

   char      argument     [MAXSTR];
   char      template_file[MAXSTR];
   char      filename     [MAXSTR];
   char      path         [MAXSTR];
   char      errstr       [MAXSTR];

   int       ifile, nfile;
   double   *incrpix1, *incrpix2, *incrpix3, *incrpix4;
   double   *incrval1, *incrval2, *incrval3, *incrval4;
   double   *incdelt1, *incdelt2, *incdelt3, *incdelt4;
   int      *innaxis1, *innaxis2, *innaxis3, *innaxis4;
   int      *cntr;
   char    **infile;
   char    **inarea;

   char      tblfile [MAXSTR];
   int       icntr;
   int       ifname;
   int       inaxis1;
   int       inaxis2;
   int       inaxis3;
   int       inaxis4;
   int       icrval1;
   int       icrval2;
   int       icrval3;
   int       icrval4;
   int       icrpix1;
   int       icrpix2;
   int       icrpix3;
   int       icrpix4;
   int       icdelt1;
   int       icdelt2;
   int       icdelt3;
   int       icdelt4;

   double nom_crval1, nom_crval2, nom_crval3, nom_crval4;
   double nom_cdelt1, nom_cdelt2, nom_cdelt3, nom_cdelt4;
   double dtr;


   /*************************************************/
   /* Initialize output FITS basic image parameters */
   /*************************************************/

   long naxis      = 4;  
   long naxis_area = 2;  


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
      value.c[i] = 255;

   nan = value.d;


   /****************/
   /* Start timing */
   /****************/

   time(&currtime);
   start = currtime;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   dtr = atan(1.)/45.;

   strcpy(path, "");
   debug     = 0;
   opterr    = 0;
   haveAreas = 1;

   fstatus = stdout;

   while ((c = getopt(argc, argv, "enp:s:d:a:")) != EOF) 
   {
      switch (c) 
      {
         case 'a':

           /***********************/
           /* Find averaging type */
           /***********************/

           strcpy(argument, optarg);
           if (strcmp(argument, "mean") == 0)
             coadd = MEAN;
           else if (strcmp(argument, "median") == 0)
             coadd = MEDIAN;
           else if (strcmp(argument, "count") == 0)
             coadd = COUNT;
           else
           {
             printf("[struct stat=\"ERROR\", msg=\"Invalid argument for -a flag\"]\n");
             fflush(stdout);
             exit(1);
           }
           break;
 
         case 'e':

           /*****************************/
           /* Is 'exact-size" flag set? */
           /*****************************/

           shrink = 0;
           break;

         case 'p':

           /*****************************/
           /* Get path to image dir     */
           /*****************************/

            strcpy(path, optarg);
            break;

         case 'd':

           /************************/
           /* Look for debug level */
           /************************/

            debug = debugCheck(optarg);
            break;

         case 'n':

           /****************************/
           /* We don't have area files */
           /****************************/

            haveAreas = 0;
            break;

         case 's':

           /************************/
           /* Look for status file */
           /************************/

            if((fstatus = fopen(optarg, "w+")) == (FILE *)NULL)
            {
               printf("[struct stat=\"ERROR\", msg=\"Cannot open status file: %s\"]\n",
                  optarg);
               exit(1);
            }
            break;

         default:

           /************************/
           /* Print usage message  */
           /************************/

            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-p imgdir] [-n(o-areas)] [-a mean|median|count] [-e(xact-size)] [-d level] [-s statusfile] images.tbl template.hdr out.fits\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   /*****************************/
   /* Get required arguments    */
   /*****************************/

   if (argc - optind < 3) 
   {
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-p imgdir] [-n(o-areas)] [-a mean|median|count] [-e(xact-size)] [-d level] [-s statusfile] images.tbl template.hdr out.fits\"]\n", argv[0]);
      exit(1);
   }

   strcpy(tblfile,       argv[optind]);
   strcpy(template_file, argv[optind + 1]);
   strcpy(output_file,   argv[optind + 2]);
   
   if(debug >= 1)
   {
      time(&currtime);
      printf("Command line arguments processed [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /***********************************************/
   /* Check header and set up name of output file */
   /***********************************************/

   checkHdr(template_file, 1, 0);

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

   if(debug >= 1)
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

   readTemplate(template_file);

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
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Invalid or missing image metadata file: %s\"]\n",
         tblfile);
      exit(1);
   }


   /**************************/
   /* Get indices of columns */
   /**************************/

   icntr   = tcol("cntr");
   ifname  = tcol("fname");
   icdelt1 = tcol("cdelt1");
   icdelt2 = tcol("cdelt2");
   icdelt3 = tcol("CDELT3");
   icdelt4 = tcol("CDELT4");
   icrval1 = tcol("crval1");
   icrval2 = tcol("crval2");
   icrval3 = tcol("CRVAL3");
   icrval4 = tcol("CRVAL4");
   icrpix1 = tcol("crpix1");
   icrpix2 = tcol("crpix2");
   icrpix3 = tcol("CRPIX3");
   icrpix4 = tcol("CRPIX4");
   inaxis1 = tcol("naxis1");
   inaxis2 = tcol("naxis2");
   inaxis3 = tcol("NAXIS3");
   inaxis4 = tcol("NAXIS4");

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

   if(    icntr  < 0 ||  ifname < 0 || icdelt1 < 0 || icdelt2 < 0 || icrpix1 < 0 
      || icrpix2 < 0 || inaxis1 < 0 || inaxis2 < 0 || icrval1 < 0 || icrval2 < 0
      || inaxis3 < 0)
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Need columns: cntr,fname, crpix1, crpix2, cdelt1, cdelt2, naxis1, naxis2, crval1, crval2, naxis3 in image list\"]\n");
      exit(1);
   }


   /*************************************************/
   /* Allocate memory for file metadata table info  */
   /*************************************************/

   cntr     = (int *)    malloc(maxfile * sizeof(int)   );
   infile   = (char **)  malloc(maxfile * sizeof(char *));
   inarea   = (char **)  malloc(maxfile * sizeof(char *));
   incdelt1 = (double *) malloc(maxfile * sizeof(double));
   incdelt2 = (double *) malloc(maxfile * sizeof(double));
   incdelt3 = (double *) malloc(maxfile * sizeof(double));
   incdelt4 = (double *) malloc(maxfile * sizeof(double));
   incrval1 = (double *) malloc(maxfile * sizeof(double));
   incrval2 = (double *) malloc(maxfile * sizeof(double));
   incrval3 = (double *) malloc(maxfile * sizeof(double));
   incrval4 = (double *) malloc(maxfile * sizeof(double));
   incrpix1 = (double *) malloc(maxfile * sizeof(double));
   incrpix2 = (double *) malloc(maxfile * sizeof(double));
   incrpix3 = (double *) malloc(maxfile * sizeof(double));
   incrpix4 = (double *) malloc(maxfile * sizeof(double));
   innaxis1 = (int *)    malloc(maxfile * sizeof(int)   );
   innaxis2 = (int *)    malloc(maxfile * sizeof(int)   );
   innaxis3 = (int *)    malloc(maxfile * sizeof(int)   );
   innaxis4 = (int *)    malloc(maxfile * sizeof(int)   );

   for(ifile=0; ifile<maxfile; ++ifile)
   {
      infile[ifile] = (char *)malloc(namelen*sizeof(char));
      inarea[ifile] = (char *)malloc(namelen*sizeof(char));
   }

   if(debug >= 1)
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
      incdelt3[nfile] = atof(tval(icdelt3));
      incdelt4[nfile] = atof(tval(icdelt4));
      incrval1[nfile] = atof(tval(icrval1));
      incrval2[nfile] = atof(tval(icrval2));
      incrval3[nfile] = atof(tval(icrval3));
      incrval4[nfile] = atof(tval(icrval4));
      incrpix1[nfile] = atof(tval(icrpix1));
      incrpix2[nfile] = atof(tval(icrpix2));
      incrpix3[nfile] = atof(tval(icrpix3));
      incrpix4[nfile] = atof(tval(icrpix4));
      innaxis1[nfile] = atoi(tval(inaxis1));
      innaxis2[nfile] = atoi(tval(inaxis2));
      innaxis3[nfile] = atoi(tval(inaxis3));
      innaxis4[nfile] = atoi(tval(inaxis4));

      if(innaxis4[nfile] == 0)
         innaxis4[nfile] = 1;


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
          try = incrpix1[nfile] - (360/fabs(incdelt1[nfile]));

          if (fabs(try - output.crpix1) > fabs(incrpix1[nfile] - output.crpix1))
          {
            /* We went the wrong direction:         */
            break;
          }
             /* Got closer to the header */
           incrpix1[nfile] = try;
        }

        /* Try adding increments of 360 degrees */

        while ((fabs(incrpix1[nfile] - output.crpix1)) > output.naxes[0])
        {
          try = incrpix1[nfile] + (360/fabs(incdelt1[nfile]));

          if (fabs(try - output.crpix1) > fabs(incrpix1[nfile] - output.crpix1))
          {
            /* We went the wrong direction:         */
            break;
          }
             /* Got closer to the header */
           incrpix1[nfile] = try;
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
        if(imax < incrpix1[nfile]) imax = incrpix1[nfile];

        if(imin > incrpix1[nfile] - innaxis1[nfile]+1) 
           imin = incrpix1[nfile] - innaxis1[nfile]+1;

        if(jmax < incrpix2[nfile]) jmax = incrpix2[nfile];

        if(jmin > incrpix2[nfile] - innaxis2[nfile]+1)
           jmin = incrpix2[nfile] - innaxis2[nfile]+1;
      }


      /* Get filename */

      strcpy(filename, filePath(path, tval(ifname)));


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
         incrval1 = (double *) realloc(incrval1, maxfile * sizeof(double));
         incrval2 = (double *) realloc(incrval2, maxfile * sizeof(double));
         incrval3 = (double *) realloc(incrval3, maxfile * sizeof(double));
         incrval4 = (double *) realloc(incrval4, maxfile * sizeof(double));
         incrpix1 = (double *) realloc(incrpix1, maxfile * sizeof(double));
         incrpix2 = (double *) realloc(incrpix2, maxfile * sizeof(double));
         incrpix3 = (double *) realloc(incrpix3, maxfile * sizeof(double));
         incrpix4 = (double *) realloc(incrpix4, maxfile * sizeof(double));
         innaxis1 = (int *)    realloc(innaxis1, maxfile * sizeof(int)   );
         innaxis2 = (int *)    realloc(innaxis2, maxfile * sizeof(int)   );
         innaxis3 = (int *)    realloc(innaxis3, maxfile * sizeof(int)   );
         innaxis4 = (int *)    realloc(innaxis4, maxfile * sizeof(int)   );
         incdelt1 = (double *) realloc(incdelt1, maxfile * sizeof(double));
         incdelt2 = (double *) realloc(incdelt2, maxfile * sizeof(double));
         incdelt3 = (double *) realloc(incdelt3, maxfile * sizeof(double));
         incdelt4 = (double *) realloc(incdelt4, maxfile * sizeof(double));

         for(ifile=nfile; ifile<maxfile; ++ifile)
         {
            infile[ifile] = (char *)malloc(namelen*sizeof(char));
            inarea[ifile] = (char *)malloc(namelen*sizeof(char));

            if(!inarea[ifile]) allocError("file info (realloc)");
         }
      }
   }

   tclose();

   if(debug >= 3)
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

   if(debug >= 1)
   {
      time(&currtime);
      printf("File metadata read [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /*************************************************/
   /* Allocate memory for input fileinfo structures */
   /*************************************************/

   input = (struct fileinfo *)malloc(maxfile * sizeof(struct fileinfo));

   if(!input) allocError("file info structs");

   if (haveAreas)
   {
      input_area = (struct fileinfo *)malloc(maxfile * sizeof(struct fileinfo));
      
      if(!input_area) allocError("area file info structs");
   }
   
   if(debug >= 1)
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

   if (debug >= 1)
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

   inbuflen = labs(imax-imin);

   if( output.naxes[0] > inbuflen)
      inbuflen = output.naxes[0];

   if (debug >= 1)
   {
     printf("Input buffer length = %d\n", inbuflen);
     fflush(stdout);
   }

   input_buffer      = (double *)malloc(inbuflen * sizeof(double));
   input_buffer_area = (double *)malloc(inbuflen * sizeof(double));

   if(!input_buffer)      allocError("input buffer");
   if(!input_buffer_area) allocError("input area buffer");

   if(debug >= 1)
   {
      time(&currtime);
      printf("Memory allocated for input buffers [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }
     

   /*****************************************************/
   /* Build array of fileinfo structures on input files */
   /*****************************************************/

   if(debug >= 2)
   {
      printf("\nFILE RANGES\n");
      printf(" i   start   end   offset j3start   j3end  \n");
      printf("---- ------ ------ ------ -------- ---------\n");
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
         nom_crval3 = incrval3[ifile];
         nom_crval4 = incrval4[ifile];
         nom_cdelt1 = incdelt1[ifile];
         nom_cdelt2 = incdelt2[ifile];
         nom_cdelt3 = incdelt3[ifile];
         nom_cdelt4 = incdelt4[ifile];
      }

      /****************************************************/
      /* Check that all files are in the same pixel space */
      /****************************************************/

       if(incrval1[ifile] != nom_crval1 
       || incrval2[ifile] != nom_crval2
       || incrval3[ifile] != nom_crval3
       || incrval4[ifile] != nom_crval4
       || incdelt1[ifile] != nom_cdelt1
       || incdelt2[ifile] != nom_cdelt2
       || incdelt3[ifile] != nom_cdelt3
       || incdelt4[ifile] != nom_cdelt4)
          printError("Images are not in same pixel space");


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


      /****************************/
      /* Same for third dimension */
      /****************************/
 
      input[ifile].j3start = output.crpix3 - incrpix3[ifile] + 1;
      input[ifile].j3end   = input[ifile].j3start + innaxis3[ifile]-1;

      if(input[ifile].j3end > output.naxes[2])
         input[ifile].j3end = output.naxes[2];


      /***********************************/
      /* Find the output column on which */
      /* this file starts                */
      /***********************************/

      if(output.crpix1 > incrpix1[ifile])
         input[ifile].offset =  floor(output.crpix1 - incrpix1[ifile] + 0.5);
      else
         input[ifile].offset = -floor(incrpix1[ifile] - output.crpix1 + 0.5);

      if (debug >= 2)
      {
         printf("%4d %6d %6d %6d %8d %8d\n", 
            ifile, input[ifile].start, input[ifile].end, input[ifile].offset,
            input[ifile].j3start, input[ifile].j3end);
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

   if(!endfile) allocError("start/end info");

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

    if(debug >= 2)
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

   if(debug >= 1)
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

   if(!dataline) allocError("data line pointers");

   for (i = 0; i < output.naxes[0]; ++i)
   {
      dataline[i] = (double *)malloc(pixdepth * sizeof(double));

      if(!dataline[i]) allocError("data line");
   }


   arealine = (double **)malloc(output.naxes[0] * sizeof(double *));

   if(!arealine) allocError("area line pointers");

   for (i = 0; i < output.naxes[0]; ++i)
   {
      arealine[i] = (double *)malloc(pixdepth * sizeof(double));

      if(!arealine[i]) allocError("area line");
   }

   datacount = (int *)malloc(output.naxes[0] * sizeof(int));

   if(!datacount) allocError("data counts");


   if(debug >= 1)
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

   if(!outdataline) allocError("output data line");
   if(!outarealine) allocError("output area line");
     
   if(debug >= 1)
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
       printFitsError(status);           

   status = 0;
   if(fits_create_file(&output_area.fptr, output_area_file, &status)) 
       printFitsError(status);           

   open_files = 2;



   /*********************************************************/
   /* Create the FITS image.  All the required keywords are */
   /* handled automatically.                                */
   /*********************************************************/

   if(haveAxis4 == 0)
      --naxis;
   
   status = 0;
   if (fits_create_img(output.fptr, DOUBLE_IMG, naxis, output.naxes, &status))
      printFitsError(status);          

   if(debug >= 1)
   {
      printf("FITS data image created (not yet populated)\n"); 
      fflush(stdout);
   }

   status = 0;
   if (fits_create_img(output_area.fptr, DOUBLE_IMG, naxis_area, output_area.naxes, &status))
      printFitsError(status);          

   if(debug >= 1)
   {
      printf("FITS area image created (not yet populated)\n"); 
      fflush(stdout);
   }

   if(debug >= 1)
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
      printFitsError(status);           

   if(debug >= 1)
   {
      printf("Template keywords written to FITS data image\n"); 
      fflush(stdout);
   }

   status = 0;
   if(fits_write_key_template(output_area.fptr, template_file, &status))
      printFitsError(status);           

   if(debug >= 1)
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
      printFitsError(status);           

   status = 0;
   if(fits_update_key_lng(output_area.fptr, "BITPIX", DOUBLE_IMG,
                                  (char *)NULL, &status))
      printFitsError(status);           


   /***************************/
   /* Update NAXIS keywords   */
   /***************************/

   status = 0;
   if(fits_update_key_lng(output.fptr, "NAXIS", naxis,
                                  (char *)NULL, &status))
       printFitsError(status);           

   status = 0;
   if(fits_update_key_lng(output.fptr, "NAXIS1", output.naxes[0],
                                  (char *)NULL, &status))
       printFitsError(status);           

   status = 0;
   if(fits_update_key_lng(output.fptr, "NAXIS2", output.naxes[1],
                                  (char *)NULL, &status))
       printFitsError(status);           

   status = 0;
   if(fits_update_key_lng(output.fptr, "NAXIS3", output.naxes[2],
                                  (char *)NULL, &status))
       printFitsError(status);           

   if(haveAxis4)
   {
      status = 0;
      if(fits_update_key_lng(output.fptr, "NAXIS4", output.naxes[3],
                                     (char *)NULL, &status))
          printFitsError(status);           
   }

   status = 0;
   if(fits_update_key_lng(output_area.fptr, "NAXIS", naxis_area,
                                  (char *)NULL, &status))
       printFitsError(status);           

   status = 0;
   if(fits_update_key_lng(output_area.fptr, "NAXIS1", output.naxes[0],
                                  (char *)NULL, &status))
       printFitsError(status);           

   status = 0;
   if(fits_update_key_lng(output_area.fptr, "NAXIS2", output.naxes[1],
                                  (char *)NULL, &status))
       printFitsError(status);           


   /*************************/
   /* Update CRPIX keywords */
   /*************************/

   status = 0;
   if(fits_update_key_dbl(output.fptr, "CRPIX1", output.crpix1, -14,
                                  (char *)NULL, &status))
       printFitsError(status);           

   status = 0;
   if(fits_update_key_dbl(output.fptr, "CRPIX2", output.crpix2, -14,
                                  (char *)NULL, &status))
       printFitsError(status);           


   status = 0;
   if(fits_update_key_dbl(output_area.fptr, "CRPIX1", output.crpix1, -14,
                                  (char *)NULL, &status))
       printFitsError(status);           

   status = 0;
   if(fits_update_key_dbl(output_area.fptr, "CRPIX2", output.crpix2, -14,
                                  (char *)NULL, &status))
     printFitsError(status);           

   if(debug >= 1)
   {
      time(&currtime);
      printf("Output FITS headers updated [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /*******************************************************************************/
   /* Build/write one line of output at a time.                                   */
   /* In this cube case, that 'line' refers to a horizontal line (e.g. Dec-like)  */
   /* in the output.  Once we have figured out which input images are involved,   */
   /* we will invoke a loop over the third dimension (or loops over the third and */
   /* fourth) to create that same line for each plane and write it out.           */
   /*******************************************************************************/

   haveMinMax = 0;

   currentstart = 0;
   currentend   = 0;

   listInit();

   for (lineout=1; lineout<=output.naxes[1]; ++lineout)
   {
      if (debug >= 2)
      {
        printf("\nOUTPUT LINE %d\n",lineout);
        fflush(stdout);
      }

      if (debug == 1)
      {
         printf("\r Processing line: %d", lineout);
         fflush(stdout);
      }


      /*********************************/
      /* Update the "contributor" list */
      /*********************************/

      while(1)
      {
         if(currentstart >= nfile)
            break;

         if(startline[currentstart] > lineout)
            break;

         listAdd(startfile[currentstart]);

         ++currentstart;
      }
      
      while(1)
      {
         if(currentend >= nfile)
            break;

         if(endline[currentend] > lineout - 1)
            break;

         ifile = endfile[currentend];

         listDelete(ifile);

         if(input[ifile].isopen)
         {
            status = 0;
            if(fits_close_file(input[ifile].fptr, &status))
               printFitsError(status);           

            input[ifile].isopen = 0;

            --open_files;
         }
        
         if(haveAreas
         && input_area[ifile].isopen)
         {
             status = 0;
             if(fits_close_file(input_area[ifile].fptr, &status))
                printFitsError(status);           

             input_area[ifile].isopen = 0;

             --open_files;
         }

         ++currentend;
      }


      imgcount = listCount();


      /******************************************/
      /* Check the files that overlap this line */
      /******************************************/
     
      if (debug >= 2) 
      {
         printf("\nContributing files (%d):\n\n", imgcount);
         printf(" i   isopen   open/max      infile[i]       \n");
         printf("---- ------ ------------ -------------------\n");
         fflush(stdout);
      }

      for(j=0; j<imgcount; ++j)
      {
         ifile = listIndex(j);

         if(debug >= 2)
         {
            printf("%4d %4d %6d/%6d %s  ",
               ifile, input[ifile].isopen, open_files, MAXFITS, infile[ifile]);
            fflush(stdout);
         }

         if (input[ifile].isopen == 0)
         {
            /* Open files that aren't already open */

            ++open_files;
 
            if (open_files > MAXFITS)
            {
               printf("[struct stat=\"ERROR\", msg=\"Too many open files\"]\n");
               fflush(stdout);

               remove(output_file);               
               remove(output_area_file);               

               exit(1);
            }
 
            if(fits_open_file(&input[ifile].fptr, infile[ifile], READONLY, &status))
            {
               sprintf(errstr, "Image file %s missing or invalid FITS", infile[ifile]);
                
               printError(errstr);
            }

            if(debug >= 2)
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
                  printf("[struct stat=\"ERROR\", msg=\"Too many open files\"]\n");
                  fflush(stdout);

                  remove(output_file);               
                  remove(output_area_file);               

                  exit(1);
               }

               if(fits_open_file(&input_area[ifile].fptr, inarea[ifile], READONLY, &status))
               {
                  sprintf(errstr, "Area file %s missing or invalid FITS", inarea[ifile]);
                  printError(errstr);
               }

               input_area[ifile].isopen = 1;
            }


            /* Get the WCS and check it against */
            /* the one for the header template  */

            if(fits_get_image_wcs_keys(input[ifile].fptr, &inputHeader, &status))
               printFitsError(status);

            if(debug >= 3)
            {
               printf("Input header to wcsinit() [imgWCS]:\n%s\n", inputHeader);
               fflush(stdout);
            }

            imgWCS = wcsinit(inputHeader);

            if(imgWCS == (struct WorldCoor *)NULL)
            {
               fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Input wcsinit() failed.\"]\n");
               exit(1);
            }

            if(strcmp(imgWCS->c1type, hdrWCS->c1type) != 0)
            {
               sprintf(errstr, "Image %s header CTYPE1 does not match template", infile[ifile]);
               printError(errstr);
            }

            if(strcmp(imgWCS->c2type, hdrWCS->c2type) != 0)
            {
               sprintf(errstr, "Image %s header CTYPE2 does not match template", infile[ifile]);
               printError(errstr);
            }

            if(fabs(imgWCS->xref - hdrWCS->xref) > 1.e-8)
            {
               sprintf(errstr, "Image %s header CRVAL1 does not match template", infile[ifile]);
               printError(errstr);
            }

            if(fabs(imgWCS->yref - hdrWCS->yref) > 1.e-8)
            {
               sprintf(errstr, "Image %s header CRVAL2 does not match template", infile[ifile]);
               printError(errstr);
            }

            if(fabs(imgWCS->cd[0] - hdrWCS->cd[0]) > 1.e-8
            || fabs(imgWCS->cd[1] - hdrWCS->cd[1]) > 1.e-8
            || fabs(imgWCS->cd[2] - hdrWCS->cd[2]) > 1.e-8
            || fabs(imgWCS->cd[3] - hdrWCS->cd[3]) > 1.e-8)
            {
               sprintf(errstr, "Image %s header CD/CDELT does not match template", infile[ifile]);
               printError(errstr);
            }

            if(imgWCS->equinox != hdrWCS->equinox)
            {
               sprintf(errstr, "Image %s header EQUINOX does not match template", infile[ifile]);
               printError(errstr);
            }
         } 
         else
         {
            if(debug >= 2)
            {
               printf("Already open\n"); 
               fflush(stdout);
            }
         }
      } 


      /*****************************************************************/ 
      /* For lines from input files corresponding to this output line, */
      /* looping over the third (and fourth) dimensions of the cube    */
      /*****************************************************************/ 

      for(j4=1; j4<output.naxes[3]+1; ++j4)
      {
         for(j3=1; j3<output.naxes[2]+1; ++j3)
         {
            for(i=0; i<output.naxes[0]; ++i)
               datacount[i] = 0;

            for(j=0; j<imgcount; ++j)
            {
               ifile = listIndex(j);

               fpixel[0] = 1;
               fpixel[1] = (lineout - input[ifile].start) + 1;
               fpixel[2] = j3;
               fpixel[3] = j4;

               nelements = innaxis1[ifile];

               if (debug >= 3)
               {
                  printf("Reading line from %d:\n", ifile);
                  printf("fpixel[1] = %ld\n", fpixel[1]);
                  printf("nelements = %ld\n", nelements);
                  fflush(stdout);
               }

               if(fpixel[3] >= 1 && fpixel[3] <= innaxis4[ifile]
               && fpixel[2] >= 1 && fpixel[2] <= innaxis3[ifile]
               && fpixel[1] >= 1 && fpixel[1] <= innaxis2[ifile])
               {
                  /*****************/
                  /* Read the line */
                  /*****************/

                  status = 0;

                  if(fits_read_pix(input[ifile].fptr, TDOUBLE, fpixel, nelements, &nan,
                                     input_buffer, &nullcnt, &status))
                    printFitsError(status);

                  if(haveAreas)
                  {
                     if(fits_read_pix(input_area[ifile].fptr, TDOUBLE, fpixel, nelements, &nan,
                                        input_buffer_area, &nullcnt, &status))
                       printFitsError(status);
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

                        if(debug >= 1)
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
                              remove(output_file);               
                              remove(output_area_file);               

                              allocError("data line (realloc)");
                           }

                           arealine[i] = (double *)realloc(arealine[i],
                              pixdepth * sizeof(double));

                           if(arealine[i] == (double *)NULL)
                           {
                              remove(output_file);               
                              remove(output_area_file);               

                              allocError("area line (realloc)");
                           }
                        }

                        if(debug >= 1)
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
                  if (debug >= 3)
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

                  if(debug >= 1)
                  {
                     printf("\nWARNING: Opening and closing files to avoid too many open FITS\n\n");
                     fflush(stdout);
                  }
               }

               if (open_files >= MAXFITS) 
               {
                  if(fits_close_file(input[ifile].fptr, &status))
                     printFitsError(status);           

                  if(debug >= 2)
                  {
                     printf("Close: %4d\n", ifile); 
                     fflush(stdout);
                  }

                  input[ifile].isopen = 0;

                  --open_files;
                 
                  if(haveAreas)
                  {
                      if(fits_close_file(input_area[ifile].fptr, &status))
                         printFitsError(status);           

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
                     avg_status = avg_mean(dataline[i], arealine[i], 
                        &outdataline[i], &outarealine[i], datacount[i]);

                  else if (coadd == MEDIAN)
                     avg_status = avg_median(dataline[i], arealine[i], 
                        &outdataline[i], &outarealine[i], datacount[i], nominal_area);

                  else if (coadd == COUNT)
                     avg_status = avg_count(dataline[i], arealine[i], 
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
            fpixel[2] = j3;
            fpixel[3] = j4;
            nelements = output.naxes[0];

            if (fits_write_pix(output.fptr, TDOUBLE, fpixel, nelements,
                               (void *)(&outdataline[0]), &status))
               printFitsError(status);
         }
      }

      fpixel[2] = 1;
      fpixel[3] = 1;

      if (fits_write_pix(output_area.fptr, TDOUBLE, fpixel, nelements,
                         (void *)(&outarealine[0]), &status))
         printFitsError(status);
   }

   if(debug >= 1)
   {
      time(&currtime);
      printf("\nOutput FITS files completed [time: %.0f]\n", 
         (double)(currtime - start));
      fflush(stdout);
   }


   /*******************************/
   /* Close the output FITS files */
   /*******************************/

   if(fits_close_file(output.fptr, &status))
     printFitsError(status);           

   if(fits_close_file(output_area.fptr, &status))
       printFitsError(status);           

   if(debug >= 1)
   {
      printf("FITS images finalized\n"); 
      fflush(stdout);
   }


   time(&currtime);
   fprintf(fstatus, "[struct stat=\"OK\", time=%.0f]\n", 
      (double)(currtime - start));
   fflush(stdout);

   exit(0);
}


/**************************************************/
/*                                                */
/*  Read the output header template file.         */
/*  Specifically extract the image size info.     */
/*                                                */
/**************************************************/

void readTemplate(char *filename)
{
   int       i, j;
   FILE     *fp;
   char      line     [MAXSTR];
   char      headerStr[HDRLEN];

   output.naxes[2]      = 1;
   output_area.naxes[2] = 1;

   output.naxes[3]      = 1;
   output_area.naxes[3] = 1;

   /********************************************************/
   /* Open the template file, read and parse all the lines */
   /********************************************************/

   fp = fopen(filename, "r");

   if(fp == (FILE *)NULL)
      printError("Template file not found.");

   strcpy(headerStr, "");

   for(j=0; j<1000; ++j)
   {
      if(fgets(line, MAXSTR, fp) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';
      
      if(line[strlen(line)-1] == '\r')
         line[strlen(line)-1]  = '\0';

      if(debug >= 3)
      {
         printf("Template line: [%s]\n", line);
         fflush(stdout);
      }

      for(i=strlen(line); i<80; ++i)
         line[i] = ' ';
      
      line[80] = '\0';

      stradd(headerStr, line);

      parseLine(line);
   }

   fclose(fp);

   hdrWCS = wcsinit(headerStr);

   if(hdrWCS == (struct WorldCoor *)NULL)
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Bad WCS in header template.\"]\n");
      exit(1);
   }

   return;
}



/**************************************************/
/*                                                */
/*  Parse header lines from the template,         */
/*  looking for NAXIS1, NAXIS2, CRPIX1 and CRPIX2 */
/*                                                */
/**************************************************/

void parseLine(char *line)
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

   if(debug >= 2)
   {
      printf("keyword [%s] = value [%s]\n", keyword, value);
      fflush(stdout);
   }

   if(strcmp(keyword, "CTYPE1") == 0)
     strcpy(ctype, value);

   if(strcmp(keyword, "NAXIS1") == 0)
   {
      output.naxes[0]      = atoi(value);
      output_area.naxes[0] = atoi(value);
   }

   if(strcmp(keyword, "NAXIS2") == 0)
   {
      output.naxes[1]      = atoi(value);
      output_area.naxes[1] = atoi(value);
   }

   if(strcmp(keyword, "NAXIS3") == 0)
   {
      output.naxes[2]      = atoi(value);
      output_area.naxes[2] = atoi(value);

      if(output.naxes[2] == 0)
      {
         output.naxes[2]      = 1;
         output_area.naxes[2] = 1;
      }
   }

   if(strcmp(keyword, "NAXIS4") == 0)
   {
      haveAxis4 = 1;

      output.naxes[3]      = atoi(value);
      output_area.naxes[3] = atoi(value);

      if(output.naxes[3] == 0)
      {
         output.naxes[3]      = 1;
         output_area.naxes[3] = 1;
      }
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

   if(strcmp(keyword, "CRPIX3") == 0)
   {
      output.crpix3 = atof(value);
      output_area.crpix3 = atof(value);
   }

   if(strcmp(keyword, "CRPIX4") == 0)
   {
      output.crpix4 = atof(value);
      output_area.crpix4 = atof(value);
   }
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

   printf("[struct stat=\"ERROR\", status=%d, msg=\"%s\"]\n", status, status_str);

   remove(output_file);               
   remove(output_area_file);               

   exit(status);
}



/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void printError(char *msg)
{
   fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);

   remove(output_file);               
   remove(output_area_file);               

   exit(1);
}


/*******************************************************/
/* STRADD adds the string "card" to a header line, and */
/* pads the header out to 80 characters.               */
/*******************************************************/

int stradd(char *header, char *card)
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

int avg_mean(double data[], double area[], double *outdata, double *outarea, int count)
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

int avg_median(double data[], double area[], double *outdata, double *outarea, int n, double nom_area)
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

     if(!sorted) allocError("median array");
  }
  if(nalloc < 2*n)
  {
     nalloc = 2*n;

     sorted     = (double *)realloc(sorted,     nalloc * sizeof(double));
     sortedarea = (double *)realloc(sortedarea, nalloc * sizeof(double));

     if(!sorted) allocError("median array (realloc)");
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
  
  sort(sorted, sortedarea, nsort);

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

int avg_count(double data[], double area[], double *outdata, double *outarea, int count)
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


/**********************************************/
/*                                            */
/*  Sort a set of pixel values.  Carry the    */
/*  area along as well.                       */
/*                                            */
/**********************************************/

void sort(double *data, double *area, int n)
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


int listInit()
{
   int i;

   nlistElement = MAXLIST;

   listElement = (struct ListElement **)
      malloc(nlistElement * sizeof(struct ListElement *));

   for(i=0; i<nlistElement; ++i)
   {
      listElement[i] = (struct ListElement *)
         malloc(sizeof(struct ListElement));

      if(!listElement[i]) allocError("linked list structs");

      listElement[i]->used  =  0;
      listElement[i]->value = -1;
      listElement[i]->next  = -1;
      listElement[i]->prev  = -1;
   }

   listFirst = 0;
   listMax   = 0;

   return(0);
} 


int listCount()
{
   return listMax;
}


int listAdd(int value)
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

         if(!listElement[j]) allocError("linked list structs (additions)");

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


int listDelete(int value)
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


int listIndex(int index)
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
   
   
int allocError(char *label)
{
   fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Allocation failed for %s.\"]\n", label);
   fflush(fstatus);
   exit(1);
}
