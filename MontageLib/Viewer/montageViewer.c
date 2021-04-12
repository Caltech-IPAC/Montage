/* mViewer.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
2.1      John Good        10Oct15  Add font scaling to coord grid, labels
2.0      John Good        01Sep15  Organize layer info into structures
1.3      John Good        09Sep14  Add compass "rose" capability as a variant of "mark"
1.2      John Good        15Mar11  Fixed drawing bug; didn't flip plot in Y if wcs->imflip=1
1.1      John Good        13Mar11  Use 1.e-9 for "zero" brightness overlay and 0. for "not drawn"
1.0      John Good        23Dec10  Baseline code (see mJPEG for previous history)

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <errno.h>

#include <lodepng.h>
#include <jpeglib.h>
#include <fitsio.h>
#include <wcs.h>
#include <coord.h>
#include <mtbl.h>
#include <json.h>
#include <cmd.h>

#include <mViewer.h>
#include <montage.h>

#define  MAXSTR    1024
#define  MAXGRID     16
#define  MAXCAT     128
#define  MAXDRAW    128
#define  MAXMARK   1024
#define  MAXLABEL  1024
#define  MAXDIM   65500
#define  MAXJSON 800000

#define  VALUE       0
#define  PERCENTILE  1
#define  SIGMA       2

#define  POWER       0
#define  GAUSSIAN    1
#define  GAUSSIANLOG 2
#define  ASINH       3

#define  FRACTIONAL  0
#define  SECONDS     1
#define  MINUTES     2
#define  DEGREES     3
#define  PIXELS      4

#define  NLAB      256
#define  NLIN     1024

#define  FLUX        0
#define  MAG         1
#define  LOGFLUX     2

#define  PNG         0
#define  JPEG        1

#define FOURCORNERS  0
#define WCS          1



static char fontfile[1024];

static int color_table[256][3];

static int hdu, redhdu, greenOff;
static int grayPlaneCount;
static int redPlaneCount;
static int greenPlaneCount;
static int bluePlaneCount;

static int grayPlanes[256];
static int redPlanes[256];
static int greenPlanes[256];
static int bluePlanes[256];

static int naxis1, naxis2;
static int outType;
static int noflip;
static int flipX;
static int flipY;

static double crpix1, crpix2;
static double crval1, crval2;
static double cdelt1, cdelt2;
static double crota2;
static double cd11,   cd12;
static double cd21,   cd22;
static double xinc,   yinc;

static double xcorrection;
static double ycorrection;

static double saturationValue;

static struct WorldCoor *wcs;


// We'll handle PNG/JPEG variables separately
// rather than trying to share for clarity.
// Only one set will actually be used.     

// JPEG

static JSAMPROW *jpegData;
static JSAMPROW *jpegOvly;

// PNG

static unsigned char *pngData;
static unsigned char *pngOvly;

// Drawing overlay

static double **ovlyweight;
static int    **ovlylock;

double *fitsbuf;
double *rfitsbuf;
double *gfitsbuf;
double *bfitsbuf;


static unsigned int ii, jj;
static unsigned int nx;
static unsigned int ny;
static unsigned int membytes;

int hpx, hpxPix, hpxLevel;

int isRGB;

static double mynan;

static double dtr;

static char montage_msgstr[1024];

static int mViewer_debug;


/*-***************************************************************************/
/*                                                                           */
/*  mViewer                                                                  */
/*                                                                           */
/*  This program generates a JPEG image file from a FITS file (or a set      */
/*  of three FITS files in color).  A data range for each image can          */
/*  be defined and the data can be stretch by any power of the log()         */
/*  function (including zero: linear).  Pseudo-color color tables can        */
/*  be applied in single-image mode.                                         */
/*                                                                           */
/*  Scaled symbols (individually or for a catalog table), image outlines,    */
/*  and labels (individually or from a table) and coordinate grids can be    */
/*  overlaid on the image.  The details of the command syntax for this are   */
/*  too involved to document here.  See the mViewer documentation on-line    */
/*  or delivered with the Montage source.                                    */
/*                                                                           */
/*   char  *params         The parameters string (arguments or JSON) or JSON */
/*                         file.                                             */
/*                                                                           */
/*   char  *outFile        Output PNG/JPEG                                   */
/*                                                                           */
/*   int    mode           Type of the command string: 0 for a JSON file,    */
/*                         1 for a JSON string, and 2 for a command string.  */
/*                                                                           */
/*   char  *fontFile       Font file (overrides default)                     */
/*                                                                           */
/*   char  *outFmt         'png' or 'jpeg'                                   */
/*                                                                           */
/*   int    debug          Debugging output level                            */
/*                                                                           */
/*****************************************************************************/


struct mViewerReturn *mViewer(char *params, char *outFile, int mode, char *outFmt, char *fontFile, int debugin)
{
   int       argc;
   char     *argv[4096];

   char      keystr  [MAXSTR];
   char      valstr  [MAXSTR];
   char      ovlyType[MAXSTR];
   char      ovlyVis [MAXSTR];
   char      line    [MAXJSON];
   char      layout  [MAXJSON];

   char     *checkHdr;

   int       i, j, k, ipix, index, itemp, nowcs, ref, istatus;
   int       istart, iend;
   int       jstart, jend, jinc;
   int       nullcnt;
   int       keysexist, keynum;
   int       datatype;
   int       len;

   double    x, y;
   double    ix, iy;
   double    xpos, ypos;

   int       grayType;
   int       redType;
   int       greenType;
   int       blueType;

   int       noverlay, quality, offscl;
   int       csysimg;
   double    epochimg;

   JSON     *sv;



   // COORDINATE GRID LAYERS

   int ngrid;

   struct gridInfo
   {
      double fontscale;               // Multiplicative factor on default label size
      double linewidth;               // Line thickness for grid lines
      int    csys;                    // Coordinate system (default EQUJ)
      double epoch;                   // Coordinate epoch (default 2000.)
      double red, green, blue;        // Grid color
      double alpha;                   // Grid overlay alpha
   };

   struct gridInfo grid[MAXGRID];



   // CATALOG (AND IMGINFO) LAYERS
   
   int ncat;

   struct catInfo
   {
      int    isImgInfo;               // Is this a location catalog or image metadata
      char   file[MAXSTR];            // File name

      int    csys;                    // Coordinate system (default EQUJ)
      double epoch;                   // Coordinate epoch (default 2000.)

      double red, green, blue;        // Global symbol/outline/label color
      char   colorColumn[MAXSTR];     // Override color column (content e.g. 'red' or "ff00a0")
      double alpha;                   // Global grid overlay alpha

      double symSize;                 // Symbol reference size (e.g. 2.5)
      int    symUnits;                // Size units (e.g. arcsec)
      int    symNPnt;                 // Symbol 'polygon' number of sides
      int    symNMax;                 // Special cutoff for symbols like 'el'
      int    symType;                 // Polygon, starred, skeletal
      double symRotAngle;             // Symbol rotation (e.g. 45 degrees)
      char   symSizeColumn [MAXSTR];  // Override symbol column (content e.g. '20s diamond')
      char   symShapeColumn[MAXSTR];  // Override symbol column (content e.g. '20s diamond')

      double scaleVal;                // Data value corresponding to symbol of reference size
      int    scaleType;               // Scaling rule: linear, log, magnitude
      char   scaleColumn[MAXSTR];     // Column for data-scaled symbols

      char   labelColumn[MAXSTR];     // Column containing label string
      double fontscale;               // Multiplicative factor on default label size
      double linewidth;               // Line thickness for drawing    
   };

   struct catInfo cat[MAXCAT];



   // DRAWING LAYERS
   
   int ndrawing;

   struct drawingInfo
   {
      char file[MAXSTR];

      double alpha;
   };

   struct drawingInfo drawing[MAXDRAW];
   

   // INDIVIDUAL LABEL LAYERS

   int nlabel;

   struct labelInfo
   {
      char   text[MAXSTR];            // Label text
      double fontscale;               // Multiplicative factor on default label size

      double x, y;                    // Label location
      int    inpix;                   // Label location is pixel, not sky, coordinates
      double red, green, blue;        // Label color
      double alpha;                   // Mark overlay alpha
   };

   struct labelInfo label[MAXLABEL];


   // INDIVIDUAL MARKER LAYERS

   int nmark;

   struct markInfo
   {
      double ra, dec;                 // Mark location
      int    inpix;                   // Mark location is pixel, not sky, coordinates
      int    csys;                    // Coordinate system (default EQUJ)
      double epoch;                   // Coordinate epoch (default 2000.)

      double symSize;                 // Symbol size (e.g. 2.5)
      int    symUnits;                // Size units (e.g. arcsec)
      int    symNPnt;                 // Symbol 'polygon' number of sides
      int    symNMax;                 // Special cutoff for symbols like 'el'
      int    symType;                 // Polygon, starred, skeletal
      double symRotAngle;             // Symbol rotation (e.g. 45 degrees)
      double linewidth;               // Line thickness for drawing    

      double red, green, blue;        // Mark color
      double alpha;                   // Mark overlay alpha
   };

   struct markInfo mark[MAXMARK];



   // "Sticky" value; set in a command like -symbol, used when
   // needed thereafter until reset or unset.

   int       csys, symUnits, symNPnt, symNMax, symType, scaleType, tzero;
   double    epoch, symSize, symRotAngle, scaleVal, fontScale, fontSize;
   double    lineWidth, alpha, imgalpha;

   char      symSizeColumn [MAXSTR];
   char      symShapeColumn[MAXSTR];
   char      scaleColumn   [MAXSTR];
   char      labelColumn   [MAXSTR];
   char      colorColumn   [MAXSTR];



   double    charHeight, pixScale;

   int       ncol, ira, idec, stat;
   int       iscale, ilabel, icolor, isymsize, isymshape;
   double    ra, dec, flux;

   double    ira1, idec1;
   double    ira2, idec2;
   double    ira3, idec3;
   double    ira4, idec4;

   int       ictype1;
   int       ictype2;
   int       iequinox;
   int       inl;
   int       ins;
   int       icrval1;
   int       icrval2;
   int       icrpix1;
   int       icrpix2;
   int       icdelt1;
   int       icdelt2;
   int       icrota2;

   double    ra1, dec1;
   double    ra2, dec2;
   double    ra3, dec3;
   double    ra4, dec4;

   struct WorldCoor *im_wcs;

   int       nimages;

   long      naxisl;

   int       im_sys;
   int       im_equinox;
   double    im_epoch;
   char      im_ctype1[16];
   char      im_ctype2[16];
   int       im_naxis1;
   int       im_naxis2;
   double    im_crpix1;
   double    im_crpix2;
   double    im_crval1;
   double    im_crval2;
   double    im_cdelt1;
   double    im_cdelt2;
   double    im_crota2;

   char       im_header[1600];
   char       temp[80];

   double    ovlyred, ovlygreen, ovlyblue;

   double    median, sigma;

   double    graydiff, reddiff, greendiff, bluediff;
   double    grayval, redval, greenval, blueval;
   double    grayImVal, redImVal, greenImVal, blueImVal, maxImVal;

   double    redxoff, greenxoff, bluexoff;
   double    redyoff, greenyoff, blueyoff;
   double    redxmin, greenxmin, bluexmin;
   double    redxmax, greenxmax, bluexmax;
   double    redymin, greenymin, blueymin;
   double    redymax, greenymax, blueymax;

   int       rednaxis1,   rednaxis2;
   int       greennaxis1, greennaxis2;
   int       bluenaxis1,  bluenaxis2;

   double    xmin, xmax;
   double    ymin, ymax;

   double    xpix, ypix;

   double    truecolor;

   int       status;

   int       colortable;
   int       pngError;

   char      statusfile   [1024];
   char      grayfile     [1024];
   char      redfile      [1024];
   char      greenfile    [1024];
   char      bluefile     [1024];
   char      jpegfile     [1024];
   char      pngfile      [1024];

   char      grayhistfile [1024];
   char      redhistfile  [1024];
   char      greenhistfile[1024];
   char      bluehistfile [1024];


   char      grayminstr   [256];
   char      graymaxstr   [256];
   char      graybetastr  [256];
   char      redminstr    [256];
   char      redmaxstr    [256];
   char      redbetastr   [256];
   char      greenminstr  [256];
   char      greenmaxstr  [256];
   char      greenbetastr [256];
   char      blueminstr   [256];
   char      bluemaxstr   [256];
   char      bluebetastr  [256];
   char      colorstr     [256];
   char      symbolstr    [256];
   char      labelstr     [256];

   double    grayminval;
   double    graymaxval;
   double    grayminpercent;
   double    graymaxpercent;
   double    grayminsigma;
   double    graymaxsigma;
   double    graybetaval;
   int       graylogpower;
   double    graydataval[256];

   double    graydatamin;
   double    graydatamax;

   double    redminval;
   double    redmaxval;
   double    redminpercent;
   double    redmaxpercent;
   double    redminsigma;
   double    redmaxsigma;
   double    redbetaval;
   int       redlogpower;
   double    reddataval[256];

   double    rdatamin;
   double    rdatamax;

   double    greenminval;
   double    greenmaxval;
   double    greenminpercent;
   double    greenmaxpercent;
   double    greenminsigma;
   double    greenmaxsigma;
   double    greenbetaval;
   int       greenlogpower;
   double    greendataval[256];

   double    gdatamin;
   double    gdatamax;

   double    blueminval;
   double    bluemaxval;
   double    blueminpercent;
   double    bluemaxpercent;
   double    blueminsigma;
   double    bluemaxsigma;
   double    bluebetaval;
   int       bluelogpower;
   double    bluedataval[256];

   double    bdatamin;
   double    bdatamax;

   fitsfile *grayfptr;
   fitsfile *redfptr;
   fitsfile *greenfptr;
   fitsfile *bluefptr;

   long      fpixelGray [4];
   long      fpixelRed  [4];
   long      fpixelGreen[4];
   long      fpixelBlue [4];
   long      nelements;

   char      bunit[256];

   char     *end;

   char     *header;

   char     *ptr;

   FILE     *jpegfp;
   FILE     *fjson;

   JSAMPARRAY  jpegptr;

   struct jpeg_compress_struct cinfo;

   struct jpeg_error_mgr jerr;

   static struct mViewerReturn rtnStruct;
   struct mViewerReturn *returnStruct;


   /************************************************/
   /* Make a NaN value to use setting blank pixels */
   /************************************************/

   union
   {
      double d;
      char   c[8];
   }
   value;

   for(i=0; i<8; ++i)
      value.c[i] = (char)255;

   mynan = value.d;


   /********************************/
   /* Initialize various variables */
   /********************************/

   returnStruct = &rtnStruct;

   outType = PNG;

   grayType  = 0;
   redType   = 0;
   greenType = 0;
   blueType  = 0;

   status = 0;

   colortable = 0;

   grayminval      = 0.;
   graymaxval      = 0.;
   grayminpercent  = 0.;
   graymaxpercent  = 0.;
   grayminsigma    = 0.;
   graymaxsigma    = 0.;
   graybetaval     = 0.;
   graylogpower    = 0;

   redminval       = 0.;
   redmaxval       = 0.;
   redminpercent   = 0.;
   redmaxpercent   = 0.;
   redminsigma     = 0.;
   redmaxsigma     = 0.;
   redbetaval      = 0.;
   redlogpower     = 0;

   greenminval     = 0.;
   greenmaxval     = 0.;
   greenminpercent = 0.;
   greenmaxpercent = 0.;
   greenminsigma   = 0.;
   greenmaxsigma   = 0.;
   greenbetaval    = 0.;
   greenlogpower   = 0;

   blueminval      = 0.;
   bluemaxval      = 0.;
   blueminpercent  = 0.;
   bluemaxpercent  = 0.;
   blueminsigma    = 0.;
   bluemaxsigma    = 0.;
   bluebetaval     = 0.;
   bluelogpower    = 0;

   grayfptr  = (fitsfile *)NULL;
   redfptr   = (fitsfile *)NULL;
   greenfptr = (fitsfile *)NULL;
   bluefptr  = (fitsfile *)NULL;

   header = (char *)NULL;

   hpx    = 0;
   hpxPix = 0;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /*******************************************************************************************/
   /* Process the input parameters.  A "command-line" argument is turned into argc, argv form */
   /* whice we process later like we would if this were a main program (from which this code  */
   /* derives).  JSON input is restructured into the same form first. This may seem odd, but  */
   /* the argc,argv handling already existed and there didn't seem to be much point in        */
   /* restructuring all that when there was no real benefit in doing so.                      */
   /*******************************************************************************************/

   mViewer_debug = debugin;


   /* Find the font to be used for text rendering */

   if(strlen(fontFile) > 0)
      strcpy(fontfile, fontFile);

   else
   {
      strcpy(fontfile, FONT_DIR);

      if(getenv("MONTAGE_FONT_DIR") != (char *)NULL)
         strcpy(fontfile, getenv("MONTAGE_FONT_DIR"));

      if(fontfile[strlen(fontfile)-1] != '/')
         strcat(fontfile, "/");

      strcat(fontfile, "FreeSans.ttf");
   }


   /* Initial values for "sticky" parameters */

   dtr = atan(1.)/45.;

   csys  = EQUJ;
   epoch = 2000.;

   symNPnt     = 3;
   symNMax     = 0;
   symType     = 0;
   symUnits    = FRACTIONAL;
   symSize     = 0.5;
   symRotAngle = 0.0;

   scaleVal    = 1.;
   scaleType   = FLUX;

   fontScale   = 1.;
   lineWidth   = 1.;
   alpha       = 1.;
   imgalpha    = 1.;

   tzero = 1;

   strcpy(symSizeColumn,  "");
   strcpy(symShapeColumn, "");
   strcpy(scaleColumn,    "");
   strcpy(labelColumn,    "");
   strcpy(colorColumn,    "");



   /* Command-line arguments */

   truecolor   = 0.;
   nowcs       = 0;

   saturationValue = 0.;

   strcpy(statusfile,     "");
   strcpy(grayfile,       "");
   strcpy(redfile,        "");
   strcpy(greenfile,      "");
   strcpy(bluefile,       "");
   strcpy(pngfile,        "");
   strcpy(jpegfile,       "");
   strcpy(grayhistfile,   "");
   strcpy(redhistfile,    "");
   strcpy(greenhistfile,  "");
   strcpy(bluehistfile,   "");

   ovlyred   = 0.5;
   ovlygreen = 0.5;
   ovlyblue  = 0.5;

   ngrid    = 0;
   ncat     = 0;
   ndrawing = 0;
   nlabel   = 0;
   nmark    = 0;

   noflip   = 0;
   flipX    = 0;
   flipY    = 0;

   if(strcmp(outFmt, "png") == 0)
      strcpy(pngfile, outFile);

   if(strcmp(outFmt, "jpeg") == 0)
      strcpy(jpegfile, outFile);

   if(mViewer_debug)
   {
      printf("DEBUG> mode = %d\n", mode);
      printf("DEBUG> params = [%s]\n", params);
      fflush(stdout);
   }


   if(mode == JSONMODE   // We were passed a JSON string rather than a set of arguments
   || mode == JSONFILE)  // so we mine that for display information
   {
      /* If it is a JSON file, read it to make the command string */

      if(mode == JSONFILE)
      {
         if(mViewer_debug)
         {
            printf("DEBUG> JSONFILE mode\n");
            fflush(stdout);
         }

         fjson = fopen(params, "r");

         if(fjson == (FILE *)NULL)
         {
            strcpy(returnStruct->msg, "Cannot open input JSON file.");
            return returnStruct;
         }

         strcpy(layout, "");

         while(1)
         {
            if(fgets(line, MAXSTR, fjson) == (char *)NULL)
               break;

            strcat(layout, line);
         }

         fclose(fjson);
      }
      else
         strcpy(layout, params);

         
      /* Parse the JSON (set up the structures to mine) */

      if((sv = json_struct(layout)) == (JSON *)NULL)
      {
         strcpy(returnStruct->msg, "Invalid JSON structure.");
         return returnStruct;
      }


      /* Sticky "default" parameters */

      /* Ignore WCS */

      if(json_val(layout, "no_wcs", valstr))
      {
         nowcs = strtol(valstr, &end, 0);

         if(end < valstr+strlen(valstr))
         {
            strcpy(returnStruct->msg, "Font scale parameter must an integer.");
            return returnStruct;
         }
      }


      /* COLOR */

      if(json_val(layout, "color", valstr))
      {
         if(mViewer_colorLookup(valstr, &ovlyred, &ovlygreen, &ovlyblue))
         {
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }


      /* FONT SCALE */

      if(json_val(layout, "font_scale", valstr))
      {
         fontScale = strtod(valstr, &end);

         if(fontScale <= 0.  || end < valstr+strlen(valstr))
         {
            strcpy(returnStruct->msg, "Font scale parameter must a number greater than zero.");
            return returnStruct;
         }
      }


      /* LINE WIDTH */

      if(json_val(layout, "line_width", valstr))
      {
         lineWidth = strtod(valstr, &end);

         if(lineWidth <= 0.  || end < valstr+strlen(valstr))
         {
            strcpy(returnStruct->msg, "Line width`parameter must a number greater than zero.");
            return returnStruct;
         }
      }


      /* ALPHA */

      if(json_val(layout, "alpha", valstr))
      {
         alpha = strtod(valstr, &end);

         if(alpha < 0. || alpha > 1. || end < valstr+strlen(valstr))
         {
            strcpy(returnStruct->msg, "Alpha parameter must a number between zero and one.");
            return returnStruct;
         }
      }


      /* IMGALPHA */

      if(json_val(layout, "imgalpha", valstr))
      {
         imgalpha = strtod(valstr, &end);

         if(imgalpha < 0. || imgalpha > 1. || end < valstr+strlen(valstr))
         {
            strcpy(returnStruct->msg, "Alpha parameter must a number between zero and one.");
            return returnStruct;
         }
      }


      /* TZERO */

      if(json_val(layout, "tzero", valstr))
      {
         tzero = strtol(valstr, &end, 0);

         if(end < valstr+strlen(valstr))
         {
            strcpy(returnStruct->msg, "Tzero parameter must be an integer (nominally zero or one for false/true).");
            return returnStruct;
         }
      }


      /* COLOR TABLE (for grayscale) */

      if(json_val(layout, "color_table", valstr))
      {
         colortable = strtol(valstr, &end, 10);

         if(colortable < 0 || colortable > 12 || end < valstr+strlen(valstr))
         {
            strcpy(returnStruct->msg, "Color table index must be a number between 0 and 12");
            return returnStruct;
         }
      }


      /* TRUE COLOR (for full color) */

      if(json_val(layout, "true_color", valstr))
      {
         truecolor = strtod(valstr, &end);

         if(truecolor <= 1. || truecolor > 4. || end < valstr+strlen(valstr))
         {
            strcpy(returnStruct->msg, "Color enhancement parameter must be a number between 1. and 4.");
            return returnStruct;
         }
      }


      /* COORDINATE SYSTEM */

      if(json_val(layout, "coord_sys", valstr))
      {
         mViewer_parseCoordStr(valstr, &csys, &epoch);
      }


      /* SYMBOL */

      if(json_val(layout, "symbol", valstr))
      {
         if(mViewer_parseSymbol(valstr, &symNPnt, &symNMax, &symType, &symRotAngle))
         {
            strcpy(returnStruct->msg, "Invalid symbol attribute");
            return returnStruct;
         }
      }


      /* Gray, red, green, blue FITS files */

      if(json_val(layout, "gray_file", valstr))  // GRAY FILE
      {
         if(json_val(layout, "gray_file.fits_file", grayfile) == (char *)NULL)
         {
            strcpy(returnStruct->msg, "Gray file has no 'fits_file' attribute.");
            return returnStruct;
         }

         if(strlen(grayfile) > 0)
         {
            grayPlaneCount = mViewer_getPlanes(grayfile, grayPlanes);

            if(grayPlaneCount > 0)
               hdu = grayPlanes[0];
            else
               hdu = 0;

            if(!nowcs)
            {
               checkHdr = montage_checkHdr(grayfile, 0, hdu);

               if(checkHdr)
               {
                  strcpy(returnStruct->msg, checkHdr);
                  return returnStruct;
               }
            }


            if(json_val(layout, "gray_file.color_table", valstr))
            {
               colortable = strtol(valstr, &end, 10);

               if(colortable < 0 || colortable > 12 || end < valstr+strlen(valstr))
               {
                  strcpy(returnStruct->msg, "Color table index must be a number between 0 and 12");
                  return returnStruct;
               }
            }

            json_val(layout, "gray_file.stretch_min", grayminstr);
            json_val(layout, "gray_file.stretch_max", graymaxstr);

            if(json_val(layout, "gray_file.stretch_mode", valstr))
            {
               grayType = POWER;

               if(valstr[0] == 'g')
               {
                  grayType = GAUSSIAN;

                  if(strlen(valstr) > 1 
                        && (   valstr[strlen(valstr)-1] == 'g'
                           || valstr[strlen(valstr)-1] == 'l'))
                     grayType = GAUSSIANLOG;

                  i += 1;
               }

               else if(valstr[0] == 'a')
               {
                  grayType = ASINH;

                  strcpy(graybetastr, "2s");

                  json_val(layout, "gray_file.stretch_beta", graybetastr);
               }

               else if(strncmp(valstr, "lin", 3) == 0)
                  graylogpower = 0;

               else if(strcmp(valstr, "log") == 0)
                  graylogpower = 1;

               else if(strcmp(valstr, "loglog") == 0)
                  graylogpower = 2;

               else
               {
                  graylogpower = strtol(valstr, &end, 10);

                  if(graylogpower < 0  || end < valstr + strlen(valstr))
                     graylogpower = 0;
               }
            }


            if(fits_open_file(&grayfptr, grayfile, READONLY, &status))
            {
               sprintf(returnStruct->msg, "Image file %s invalid FITS", grayfile);
               return returnStruct;
            }

            if(hdu > 0)
            {
               if(fits_movabs_hdu(grayfptr, hdu+1, NULL, &status))
               {
                  sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
                  return returnStruct;
               }
            }
         }
      }


      if(json_val(layout, "red_file", valstr))  // RED FILE
      {
         if(json_val(layout, "red_file.fits_file", redfile) == (char *)NULL)
         {
            strcpy(returnStruct->msg, "Red file has no 'fits_file' attribute.");
            return returnStruct;
         }

         if(strlen(redfile) > 0)
         {
            redPlaneCount = mViewer_getPlanes(redfile, redPlanes);

            if(redPlaneCount > 0)
               hdu = redPlanes[0];
            else
               hdu = 0;

            if(!nowcs)
            {
               checkHdr = montage_checkHdr(redfile, 0, hdu);

               if(checkHdr)
               {
                  strcpy(returnStruct->msg, checkHdr);
                  return returnStruct;
               }
            }

            json_val(layout, "red_file.stretch_min", redminstr);
            json_val(layout, "red_file.stretch_max", redmaxstr);

            if(json_val(layout, "red_file.stretch_mode", valstr))
            {
               redType = POWER;

               if(valstr[0] == 'g')
               {
                  redType = GAUSSIAN;

                  if(strlen(valstr) > 1 
                        && (   valstr[strlen(valstr)-1] == 'g'
                           || valstr[strlen(valstr)-1] == 'l'))
                     redType = GAUSSIANLOG;

                  i += 1;
               }

               else if(valstr[0] == 'a')
               {
                  redType = ASINH;

                  strcpy(redbetastr, "2s");

                  json_val(layout, "red_file.stretch_beta", redbetastr);
               }

               else if(strncmp(valstr, "lin", 3) == 0)
                  redlogpower = 0;

               else if(strcmp(valstr, "log") == 0)
                  redlogpower = 1;

               else if(strcmp(valstr, "loglog") == 0)
                  redlogpower = 2;

               else
               {
                  redlogpower = strtol(valstr, &end, 10);

                  if(redlogpower < 0  || end < valstr + strlen(valstr))
                     redlogpower = 0;
               }
            }


            if(fits_open_file(&redfptr, redfile, READONLY, &status))
            {
               sprintf(returnStruct->msg, "Image file %s invalid FITS", redfile);
               return returnStruct;
            }

            if(hdu > 0)
            {
               if(fits_movabs_hdu(redfptr, hdu+1, NULL, &status))
               {
                  sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
                  return returnStruct;
               }
            }
         }
      }


      if(json_val(layout, "green_file", valstr))  // GREEN FILE
      {
         if(json_val(layout, "green_file.fits_file", greenfile) == (char *)NULL)
         {
            strcpy(returnStruct->msg, "Green file has no 'fits_file' attribute.");
            return returnStruct;
         }

         if(strlen(greenfile) > 0)
         {
            greenPlaneCount = mViewer_getPlanes(greenfile, greenPlanes);

            if(greenPlaneCount > 0)
               hdu = greenPlanes[0];
            else
               hdu = 0;

            if(!nowcs)
            {
               checkHdr = montage_checkHdr(greenfile, 0, hdu);

               if(checkHdr)
               {
                  strcpy(returnStruct->msg, checkHdr);
                  return returnStruct;
               }
            }

            json_val(layout, "green_file.stretch_min", greenminstr);
            json_val(layout, "green_file.stretch_max", greenmaxstr);

            if(json_val(layout, "green_file.stretch_mode", valstr))
            {
               greenType = POWER;

               if(valstr[0] == 'g')
               {
                  greenType = GAUSSIAN;

                  if(strlen(valstr) > 1 
                        && (   valstr[strlen(valstr)-1] == 'g'
                           || valstr[strlen(valstr)-1] == 'l'))
                     greenType = GAUSSIANLOG;

                  i += 1;
               }

               else if(valstr[0] == 'a')
               {
                  greenType = ASINH;

                  strcpy(greenbetastr, "2s");

                  json_val(layout, "green_file.stretch_beta", greenbetastr);
               }

               else if(strncmp(valstr, "lin", 3) == 0)
                  greenlogpower = 0;

               else if(strcmp(valstr, "log") == 0)
                  greenlogpower = 1;

               else if(strcmp(valstr, "loglog") == 0)
                  greenlogpower = 2;

               else
               {
                  greenlogpower = strtol(valstr, &end, 10);

                  if(greenlogpower < 0  || end < valstr + strlen(valstr))
                     greenlogpower = 0;
               }
            }


            if(fits_open_file(&greenfptr, greenfile, READONLY, &status))
            {
               sprintf(returnStruct->msg, "Image file %s invalid FITS", greenfile);
               return returnStruct;
            }

            if(hdu > 0)
            {
               if(fits_movabs_hdu(greenfptr, hdu+1, NULL, &status))
               {
                  sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
                  return returnStruct;
               }
            }
         }
      }


      if(json_val(layout, "blue_file", valstr))  // BLUE FILE
      {
         if(json_val(layout, "blue_file.fits_file", bluefile) == (char *)NULL)
         {
            strcpy(returnStruct->msg, "Blue file has no 'fits_file' attribute.");
            return returnStruct;
         }

         if(strlen(bluefile) > 0)
         {
            bluePlaneCount = mViewer_getPlanes(bluefile, bluePlanes);

            if(bluePlaneCount > 0)
               hdu = bluePlanes[0];
            else
               hdu = 0;

            if(!nowcs)
            {
               checkHdr = montage_checkHdr(bluefile, 0, hdu);

               if(checkHdr)
               {
                  strcpy(returnStruct->msg, checkHdr);
                  return returnStruct;
               }
            }

            json_val(layout, "blue_file.stretch_min", blueminstr);
            json_val(layout, "blue_file.stretch_max", bluemaxstr);

            if(json_val(layout, "blue_file.stretch_mode", valstr))
            {
               blueType = POWER;

               if(valstr[0] == 'g')
               {
                  blueType = GAUSSIAN;

                  if(strlen(valstr) > 1 
                        && (   valstr[strlen(valstr)-1] == 'g'
                           || valstr[strlen(valstr)-1] == 'l'))
                     blueType = GAUSSIANLOG;

                  i += 1;
               }

               else if(valstr[0] == 'a')
               {
                  blueType = ASINH;

                  strcpy(bluebetastr, "2s");

                  json_val(layout, "blue_file.stretch_beta", bluebetastr);
               }

               else if(strncmp(valstr, "lin", 3) == 0)
                  bluelogpower = 0;

               else if(strcmp(valstr, "log") == 0)
                  bluelogpower = 1;

               else if(strcmp(valstr, "loglog") == 0)
                  bluelogpower = 2;

               else
               {
                  bluelogpower = strtol(valstr, &end, 10);

                  if(bluelogpower < 0  || end < valstr + strlen(valstr))
                     bluelogpower = 0;
               }
            }


            if(fits_open_file(&bluefptr, bluefile, READONLY, &status))
            {
               sprintf(returnStruct->msg, "Image file %s invalid FITS", bluefile);
               return returnStruct;
            }

            if(hdu > 0)
            {
               if(fits_movabs_hdu(bluefptr, hdu+1, NULL, &status))
               {
                  sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
                  return returnStruct;
               }
            }
         }
      }


      /* OVERLAYS */

      noverlay = 0;

      while(1)
      {
         sprintf(keystr, "overlays[%d]", noverlay);

         if(json_val(layout, keystr, valstr) == (char *)NULL)
            break;

         sprintf(keystr, "overlays[%d].visible", noverlay);

         if(json_val(layout, keystr, ovlyVis) != (char *)NULL)
         {
            if(strcasecmp(ovlyVis, "false")  == 0
            || strcasecmp(ovlyVis, "no"   )  == 0
            || strcasecmp(ovlyVis, "0"    )  == 0)
            {
               ++noverlay;
               continue;
            }
         }

         sprintf(keystr, "overlays[%d].type", noverlay);

         if(json_val(layout, keystr, ovlyType) == (char *)NULL)
         {
            sprintf(returnStruct->msg, "Overlay %d has no 'type' attribute.", noverlay);
            return returnStruct;
         }

         //-----------

         if(strcmp(ovlyType, "grid") == 0)     // GRID OVERLAY
         {
            sprintf(keystr, "overlays[%d].color", noverlay);  // Check for color

            if(json_val(layout, keystr, valstr))
            {
               if(mViewer_colorLookup(valstr, &(grid[ngrid].red), &(grid[ngrid].green), &(grid[ngrid].blue)))
               {
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }
            }
            else
            {
               grid[ngrid].red   = ovlyred;
               grid[ngrid].green = ovlygreen;
               grid[ngrid].blue  = ovlyblue;
            }


            sprintf(keystr, "overlays[%d].font_scale", noverlay);  // Check for font scale

            if(json_val(layout, keystr, valstr))
            {
                grid[ngrid].fontscale = strtod(valstr, &end);

               if(grid[ngrid].fontscale <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Font scale (overlay %d) parameter must a number greater than zero.", noverlay);
                  return returnStruct;
               }
            }
            else
            {
               grid[ngrid].fontscale = fontScale;
            }


            sprintf(keystr, "overlays[%d].line_width", noverlay);  // Check for line width

            if(json_val(layout, keystr, valstr))
            {
                grid[ngrid].linewidth = strtod(valstr, &end);

               if(grid[ngrid].linewidth <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Line width (overlay %d) parameter must a number greater than zero.", noverlay);
                  return returnStruct;
               }
            }
            else
               grid[ngrid].linewidth = lineWidth;


            sprintf(keystr, "overlays[%d].alpha", noverlay);  // Check for alpha

            if(json_val(layout, keystr, valstr))
            {
                grid[ngrid].alpha = strtod(valstr, &end);

               if(grid[ngrid].alpha <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Alpha (overlay %d) parameter must a number between zero and one.", noverlay);
                  return returnStruct;
               }
            }
            else
               grid[ngrid].alpha = alpha;


            sprintf(keystr, "overlays[%d].coord_sys", noverlay);  // Require coordinate system (don't use default)

            if(json_val(layout, keystr, valstr) == (char *)NULL)
            {
               sprintf(returnStruct->msg, "Overlay %d has no 'coord_sys' attribute.", noverlay);
               return returnStruct;
            }

            mViewer_parseCoordStr(valstr, &(grid[ngrid].csys), &(grid[ngrid].epoch));

            ++ngrid;
         }

         //-----------

         if(strcmp(ovlyType, "catalog") == 0)  // CATALOG OVERLAY
         {
            cat[ncat].isImgInfo = 0;

            sprintf(keystr, "overlays[%d].color", noverlay);  // Check for color 

            if(json_val(layout, keystr, valstr))
            {
               if(mViewer_colorLookup(valstr, &(cat[ncat].red), &(cat[ncat].green), &(cat[ncat].blue)))
               {
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }
            }
            else
            {
               cat[ncat].red   = ovlyred;
               cat[ncat].green = ovlygreen;
               cat[ncat].blue  = ovlyblue;
            }


            sprintf(keystr, "overlays[%d].coord_sys", noverlay);  // Check for coordinate system

            if(json_val(layout, keystr, valstr))
            {
               mViewer_parseCoordStr(valstr, &(cat[ncat].csys), &(cat[ncat].epoch));
            }
            else
            {
               cat[ncat].csys  = csys;
               cat[ncat].epoch = epoch;
            }


            sprintf(keystr, "overlays[%d].data_file", noverlay);  // Require catalog file name

            if(json_val(layout, keystr, valstr) == (char *)NULL)
            {
               sprintf(returnStruct->msg, "Overlay %d has no 'data_file' attribute.", noverlay);
               return returnStruct;
            }

            strcpy(cat[ncat].file, valstr);


            sprintf(keystr, "overlays[%d].data_column", noverlay);  // Check for mag/flux column for scaling

            strcpy(cat[ncat].scaleColumn, "");
            if(json_val(layout, keystr, valstr))
               strcpy(cat[ncat].scaleColumn, valstr);


            sprintf(keystr, "overlays[%d].data_ref", noverlay);  // Check for scaling data reference value

            cat[ncat].scaleVal = 1.;
            if(json_val(layout, keystr, valstr))
               cat[ncat].scaleVal = atof(valstr);


            sprintf(keystr, "overlays[%d].data_type", noverlay);  // Check for type of scaling data column 

            if(json_val(layout, keystr, valstr))
            {
               cat[ncat].scaleType = FLUX;

               if(strncasecmp(valstr, "mag", 3) == 0) cat[ncat].scaleType = MAG;
               if(strncasecmp(valstr, "log", 3) == 0) cat[ncat].scaleType = LOGFLUX;
            }


            sprintf(keystr, "overlays[%d].symbol", noverlay);  // Check for symbol shape info

            if(json_val(layout, keystr, valstr))
            {
               if(mViewer_parseSymbol(valstr, &(cat[ncat].symNPnt), &(cat[ncat].symNMax), &(cat[ncat].symType), &(cat[ncat].symRotAngle)))
               {
                  sprintf(returnStruct->msg, "Invalid symbol attribute for overlay %d.", noverlay);
                  return returnStruct;
               }
            }
            else
            {
               cat[ncat].symUnits    = symUnits;
               cat[ncat].symNPnt     = symNPnt;
               cat[ncat].symNMax     = symNMax;
               cat[ncat].symType     = symType;
               cat[ncat].symRotAngle = symRotAngle;
            }

            sprintf(keystr, "overlays[%d].sym_size", noverlay);  // Check for symbol size

            if(json_val(layout, keystr, valstr))
            {
               ptr = valstr + strlen(valstr) - 1;

               cat[ncat].symUnits = FRACTIONAL;

               if(*ptr == 's')
                  cat[ncat].symUnits = SECONDS;

               else if(*ptr == 'm')
                  cat[ncat].symUnits = MINUTES;

               else if(*ptr == 'd')
                  cat[ncat].symUnits = DEGREES;

               else if(*ptr == 'p')
                  cat[ncat].symUnits = PIXELS;

               if(symUnits != FRACTIONAL)
                  *ptr = '\0';

               cat[ncat].symSize = strtod(valstr, &end);

               if(end < (valstr + (int)strlen(valstr)))
               {
                  sprintf(returnStruct->msg, "Invalid symbol size in overlay %d.", noverlay);
                  return returnStruct;
               }
            }
            else
            {
               cat[ncat].symUnits = symUnits;
               cat[ncat].symSize  = symSize;
            }


            sprintf(keystr, "overlays[%d].line_width", noverlay);  // Check for line width

            if(json_val(layout, keystr, valstr))
            {
                cat[ncat].linewidth = strtod(valstr, &end);

               if(cat[ncat].linewidth <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Line width (overlay %d) parameter must a number greater than zero.", noverlay);
                  return returnStruct;
               }
            }
            else
               cat[ncat].linewidth = lineWidth;


            sprintf(keystr, "overlays[%d].alpha", noverlay);  // Check for alpha

            if(json_val(layout, keystr, valstr))
            {
                cat[ncat].alpha = strtod(valstr, &end);

               if(cat[ncat].alpha <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Alpha (overlay %d) parameter must a number between zero and one.", noverlay);
                  return returnStruct;
               }
            }
            else
               cat[ncat].alpha = alpha;


            // Columns can be used instead of some fixed values, mixing both 
            // for the same table would result in indeterminant behavior, so be careful

            sprintf(keystr, "overlays[%d].size_column", noverlay);  // Check for explicit size column

            strcpy(cat[ncat].symSizeColumn, "");
            if(json_val(layout, keystr, valstr))
               strcpy(cat[ncat].symSizeColumn, valstr);


            sprintf(keystr, "overlays[%d].shape_column", noverlay);  // Check for shape column

            strcpy(cat[ncat].symShapeColumn, "");
            if(json_val(layout, keystr, valstr))
               strcpy(cat[ncat].symShapeColumn, valstr);


            sprintf(keystr, "overlays[%d].label_column", noverlay);  // Check for label column

            strcpy(cat[ncat].labelColumn, "");
            if(json_val(layout, keystr, valstr))
               strcpy(cat[ncat].labelColumn, valstr);


            sprintf(keystr, "overlays[%d].color_column", noverlay);  // Check for color column

            strcpy(cat[ncat].colorColumn, "");
            if(json_val(layout, keystr, valstr))
               strcpy(cat[ncat].colorColumn, valstr);


            ++ncat;
         }

         //-----------

         if(strcmp(ovlyType, "imginfo") == 0)  // IMGINFO OVERLAY
         {
            cat[ncat].isImgInfo = 1;

            sprintf(keystr, "overlays[%d].color", noverlay);  // Check for color 

            if(json_val(layout, keystr, valstr))
            {
               if(mViewer_colorLookup(valstr, &(cat[ncat].red), &(cat[ncat].green), &(cat[ncat].blue)))
               {
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }
            }
            else
            {
               cat[ncat].red   = ovlyred;
               cat[ncat].green = ovlygreen;
               cat[ncat].blue  = ovlyblue;
            }


            sprintf(keystr, "overlays[%d].coord_sys", noverlay);  // Check for coordinate system

            if(json_val(layout, keystr, valstr))
            {
               mViewer_parseCoordStr(valstr, &(cat[ncat].csys), &(cat[ncat].epoch));
            }
            else
            {
               cat[ncat].csys  = csys;
               cat[ncat].epoch = epoch;
            }


            sprintf(keystr, "overlays[%d].data_file", noverlay);  // Require image metadata file name

            if(json_val(layout, keystr, valstr) == (char *)NULL)
            {
               sprintf(returnStruct->msg, "Overlay %d has no 'data_file' attribute.", noverlay);
               return returnStruct;
            }

            strcpy(cat[ncat].file, valstr);


            sprintf(keystr, "overlays[%d].line_width", noverlay);  // Check for line width

            if(json_val(layout, keystr, valstr))
            {
                cat[ncat].linewidth = strtod(valstr, &end);

               if(cat[ncat].linewidth <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Line width (overlay %d) parameter must a number greater than zero.", noverlay);
                  return returnStruct;
               }
            }
            else
               cat[ncat].linewidth = lineWidth;


            sprintf(keystr, "overlays[%d].alpha", noverlay);  // Check for alpha

            if(json_val(layout, keystr, valstr))
            {
                cat[ncat].alpha = strtod(valstr, &end);

               if(cat[ncat].alpha <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Alpha (overlay %d) parameter must a number between zero and one.", noverlay);
                  return returnStruct;
               }
            }
            else
               cat[ncat].alpha = alpha;


            ++ncat;
         }

         //-----------

         if(strcmp(ovlyType, "mark") == 0)     // MARKER OVERLAY
         {
            sprintf(keystr, "overlays[%d].color", noverlay);  // Check for color 

            if(json_val(layout, keystr, valstr))
            {
               if(mViewer_colorLookup(valstr, &(mark[nmark].red), &(mark[nmark].green), &(mark[nmark].blue)))
               {
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }
            }
            else
            {
               mark[nmark].red   = ovlyred;
               mark[nmark].green = ovlygreen;
               mark[nmark].blue  = ovlyblue;
            }


            sprintf(keystr, "overlays[%d].coord_sys", noverlay);  // Check for coordinate system

            if(json_val(layout, keystr, valstr))
            {
               mViewer_parseCoordStr(valstr, &(mark[nmark].csys), &(mark[nmark].epoch));
            }
            else
            {
               mark[nmark].csys  = csys;
               mark[nmark].epoch = epoch;
            }


            sprintf(keystr, "overlays[%d].lon", noverlay);  // Require longitude

            if(json_val(layout, keystr, valstr) == (char *)NULL)
            {
               sprintf(returnStruct->msg, "Overlay %d has no 'lon' attribute.", noverlay);
               return returnStruct;
            }

            if(strstr(valstr, "p") != (char *)NULL)
               mark[nmark].inpix = 1;

            mark[nmark].ra = atof(valstr);


            sprintf(keystr, "overlays[%d].lat", noverlay);  // Require latitude

            if(json_val(layout, keystr, valstr) == (char *)NULL)
            {
               sprintf(returnStruct->msg, "Overlay %d has no 'lat' attribute.", noverlay);
               return returnStruct;
            }

            if(strstr(valstr, "p") != (char *)NULL)
               mark[nmark].inpix = 1;

            mark[nmark].dec = atof(valstr);


            sprintf(keystr, "overlays[%d].symbol", noverlay);  // Check for symbol shape info

            if(json_val(layout, keystr, valstr))
            {
               if(mViewer_parseSymbol(valstr, &(mark[nmark].symNPnt), &(mark[nmark].symNMax), &(mark[nmark].symType), &(mark[nmark].symRotAngle)))
               {
                  sprintf(returnStruct->msg, "Invalid symbol attribute for overlay %d.", noverlay);
                  return returnStruct;
               }
            }
            else
            {
               mark[nmark].symUnits    = symUnits;
               mark[nmark].symNPnt     = symNPnt;
               mark[nmark].symNMax     = symNMax;
               mark[nmark].symType     = symType;
               mark[nmark].symRotAngle = symRotAngle;
            }

            sprintf(keystr, "overlays[%d].sym_size", noverlay);  // Check for symbol size

            if(json_val(layout, keystr, valstr))
            {
               ptr = valstr + strlen(valstr) - 1;

               mark[nmark].symUnits = FRACTIONAL;

               if(*ptr == 's')
                  mark[nmark].symUnits = SECONDS;

               else if(*ptr == 'm')
                  mark[nmark].symUnits = MINUTES;

               else if(*ptr == 'd')
                  mark[nmark].symUnits = DEGREES;

               else if(*ptr == 'p')
                  mark[nmark].symUnits = PIXELS;

               if(symUnits != FRACTIONAL)
                  *ptr = '\0';

               mark[nmark].symSize = strtod(valstr, &end);

               if(end < (valstr + (int)strlen(valstr)))
               {
                  sprintf(returnStruct->msg, "Invalid symbol size in overlay %d.", noverlay);
                  return returnStruct;
               }
            }
            else
            {
               mark[nmark].symUnits = symUnits;
               mark[nmark].symSize  = symSize;
            }

            
            sprintf(keystr, "overlays[%d].line_width", noverlay);  // Check for line width

            if(json_val(layout, keystr, valstr))
            {
                mark[nmark].linewidth = strtod(valstr, &end);

               if(mark[nmark].linewidth <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Line width (overlay %d) parameter must a number greater than zero.", noverlay);
                  return returnStruct;
               }
            }
            else
               mark[nmark].linewidth = lineWidth;


            sprintf(keystr, "overlays[%d].alpha", noverlay);  // Check for alpha

            if(json_val(layout, keystr, valstr))
            {
                mark[nmark].alpha = strtod(valstr, &end);

               if(mark[nmark].alpha <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Alpha (overlay %d) parameter must a number between zero and one.", noverlay);
                  return returnStruct;
               }
            }
            else
               mark[nmark].alpha = alpha;


            ++nmark;
         }

         //-----------

         if(strcmp(ovlyType, "label") == 0)    // LABEL OVERLAY
         {
            sprintf(keystr, "overlays[%d].color", noverlay);  // Check for color

            if(json_val(layout, keystr, valstr))
            {
               if(mViewer_colorLookup(valstr, &(label[nlabel].red), &(label[nlabel].green), &(label[nlabel].blue)))
               {
                  strcpy(returnStruct->msg, montage_msgstr);
                  return returnStruct;
               }
            }
            else
            {
               label[nlabel].red   = ovlyred;
               label[nlabel].green = ovlygreen;
               label[nlabel].blue  = ovlyblue;
            }


            sprintf(keystr, "overlays[%d].font_scale", noverlay);  // Check for font scale

            if(json_val(layout, keystr, valstr))
            {
               label[nlabel].fontscale = strtod(valstr, &end);

               if(label[nlabel].fontscale <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Font scale (overlay %d) parameter must a number greater than zero.", noverlay);
                  return returnStruct;
               }
            }
            else
            {
               label[nlabel].fontscale = fontScale;
            }


            sprintf(keystr, "overlays[%d].alpha", noverlay);  // Check for alpha

            if(json_val(layout, keystr, valstr))
            {
                label[nlabel].alpha = strtod(valstr, &end);

               if(label[nlabel].alpha <= 0.  || end < valstr+strlen(valstr))
               {
                  sprintf(returnStruct->msg, "Alpha (overlay %d) parameter must a number between zero and one.", noverlay);
                  return returnStruct;
               }
            }
            else
               label[nlabel].alpha = alpha;


            sprintf(keystr, "overlays[%d].lon", noverlay);  // Require longitude

            if(json_val(layout, keystr, valstr) == (char *)NULL)
            {
               sprintf(returnStruct->msg, "Overlay %d has no 'lon' attribute.", noverlay);
               return returnStruct;
            }

            if(strstr(valstr, "p") != (char *)NULL)
               label[nlabel].inpix = 1;

            label[nlabel].x = atof(valstr);


            sprintf(keystr, "overlays[%d].lat", noverlay);  // Require latitude

            if(json_val(layout, keystr, valstr) == (char *)NULL)
            {
               sprintf(returnStruct->msg, "Overlay %d has no 'lat' attribute.", noverlay);
               return returnStruct;
            }

            if(strstr(valstr, "p") != (char *)NULL)
               label[nlabel].inpix = 1;

            label[nlabel].y = atof(valstr);


            sprintf(keystr, "overlays[%d].text", noverlay);  // Require text

            if(json_val(layout, keystr, valstr) == (char *)NULL)
            {
               sprintf(returnStruct->msg, "Overlay %d has no 'text' attribute.", noverlay);
               return returnStruct;
            }
            strcpy(label[nlabel].text, valstr);

            ++nlabel;
         }

         //-----------

         ++noverlay;
      }

      json_free(sv);
   }

   else   // CMDMODE mode: parse the original mViewer command-line arguments
   {
      if(mViewer_debug)
      {
         printf("DEBUG> CMDMODE mode\n");
         fflush(stdout);
      }

      argc = parsecmd(params, argv);

      if(argc < 2)
      {
         strcpy(returnStruct->msg, "Parameters: [-d] [-nowcs] [-noflip] [-t(rue-color) power] [-ct color-table] [-grid csys [epoch]] -gray in.fits minrange maxrange [logpower/gaussian] -red red.fits rminrange rmaxrange [rlogpower/gaussian] -green green.fits gminrange gmaxrange [glogpower/gaussian] -blue blue.fits bminrange bmaxrange [blogpower/gaussian] -out out.png");
         return returnStruct;
      }

      for(i=0; i<argc; ++i)
      {
         /* Debug (overrides the mViewer_debugin variable on the call) */

         if(strcmp(argv[i], "-d") == 0)
            mViewer_debug = 1;
         

         /* Ignore WCS */

         else if(strcmp(argv[i], "-nowcs") == 0)
            nowcs = 1;
         

         /* FONT SCALE */

         else if(strcmp(argv[i], "-fontscale") == 0)
         {
            fontScale = strtod(argv[i+1], &end);

            if(fontScale <= 0.  || end < argv[i+1]+strlen(argv[i+1]))
            {
               strcpy(returnStruct->msg, "Font scale parameter must a number greater than zero.");
               return returnStruct;
            }

            ++i;
         }
         

         /* LINE WIDTH */

         else if(strcmp(argv[i], "-linewidth") == 0)
         {
            lineWidth = strtod(argv[i+1], &end);

            if(lineWidth <= 0.  || end < argv[i+1]+strlen(argv[i+1]))
            {
               strcpy(returnStruct->msg, "Line width parameter must a number greater than zero.");
               return returnStruct;
            }

            ++i;
         }
         

         /* ALPHA  */

         else if(strcmp(argv[i], "-alpha") == 0)
         {
            alpha = strtod(argv[i+1], &end);

            if(alpha < 0. || alpha > 1. || end < argv[i+1]+strlen(argv[i+1]))
            {
               strcpy(returnStruct->msg, "Opacity (alpha) must a number between zero and one.");
               return returnStruct;
            }

            ++i;
         }
         

         /* IMGALPHA  */

         else if(strcmp(argv[i], "-imgalpha") == 0)
         {
            imgalpha = strtod(argv[i+1], &end);

            if(imgalpha < 0. || imgalpha > 1. || end < argv[i+1]+strlen(argv[i+1]))
            {
               strcpy(returnStruct->msg, "Image opacity (alpha) must a number between zero and one.");
               return returnStruct;
            }

            ++i;
         }
      
         
         /* TZERO */
         
         else if(strcmp(argv[i], "-tzero") == 0)
         {
            tzero = strtol(argv[i+1], &end, 0);

            if(end < valstr+strlen(valstr))
            {
               strcpy(returnStruct->msg, "Tzero parameter must be an integer (nominally zero or one for false/true).");
               return returnStruct;
            }
         }
         

         /* Don't flip the image (to get North up) */

         else if(strcmp(argv[i], "-noflip") == 0)
            noflip = 1;


         /* SATURATION VALUE */

         else if(strcmp(argv[i], "-saturate") == 0)
         {
            saturationValue = strtod(argv[i+1], &end);

            if(saturationValue < 0.)
               saturationValue = 0.;

            if(saturationValue > 255.)
               saturationValue = 255.;

            ++i;
         }
         

         /* TRUE COLOR */

         else if(strcmp(argv[i], "-t") == 0)
         {
            truecolor = strtod(argv[i+1], &end);

            if(truecolor < 1.  || truecolor > 4. || end < argv[i+1]+strlen(argv[i+1]))
            {
               strcpy(returnStruct->msg, "Color enhancement parameter must be a number between 1. and 4.");
               return returnStruct;
            }

            ++i;
         }
         

         /* OVERLAY COLOR */

         else if(strcmp(argv[i], "-color") == 0)
         {
            strcpy(colorColumn, "");

            strcpy(colorstr, argv[i+1]);

            ++i;

            if(mViewer_colorLookup(colorstr, &ovlyred, &ovlygreen, &ovlyblue))
            {
               strcpy(returnStruct->msg, montage_msgstr);
               return returnStruct;
            }
         }
         

         /* DEFAULT COORDINATE SYSTEM */

         else if(strcmp(argv[i], "-csys") == 0)
         {
            ref = JULIAN;

            csys  = EQUJ;
            epoch = -999.;

            if(i+2 < argc)
            {
               if(argv[i+2][0] == 'j' || argv[i+2][0] == 'J')
               {
                  ref = JULIAN;
                  epoch = atof(argv[i+2]+1);
               }

               else if(argv[i+2][0] == 'b' || argv[i+2][0] == 'B')
               {
                  ref = BESSELIAN;
                  epoch = atof(argv[i+2]+1);
               }
            }

            if(strncasecmp(argv[i+1], "eq", 2) == 0)
            {
               if(ref == BESSELIAN)
                  csys = EQUB;
               else
                  csys = EQUJ;
            }

            else if(strncasecmp(argv[i+1], "ec", 2) == 0)
            {
               if(ref == BESSELIAN)
                  csys = ECLB;
               else
                  csys = ECLJ;
            }

            else if(strncasecmp(argv[i+1], "ga", 2) == 0)
               csys = GAL;

            ++i;

            if(epoch == -999.)
               epoch = 2000.;
            else
               ++i;
         }


         /* OVERLAY GRID */

         else if(strcmp(argv[i], "-grid") == 0)
         {
            ref = JULIAN;

            grid[ngrid].fontscale = fontScale;

            grid[ngrid].linewidth = lineWidth;

            grid[ngrid].alpha = alpha;

            grid[ngrid].csys   = EQUJ;
            grid[ngrid].epoch  = -999.;

            grid[ngrid].red   = ovlyred;
            grid[ngrid].green = ovlygreen;
            grid[ngrid].blue  = ovlyblue;

            if(i+2 < argc)
            {
               if(argv[i+2][0] == 'j' || argv[i+2][0] == 'J')
               {
                  ref = JULIAN;
                  grid[ngrid].epoch = atof(argv[i+2]+1);
               }

               else if(argv[i+2][0] == 'b' || argv[i+2][0] == 'B')
               {
                  ref = BESSELIAN;
                  grid[ngrid].epoch = atof(argv[i+2]+1);
               }
            }

            if(strncasecmp(argv[i+1], "eq", 2) == 0)
            {
               if(ref == BESSELIAN)
                  grid[ngrid].csys = EQUB;
               else
                  grid[ngrid].csys = EQUJ;
            }

            else if(strncasecmp(argv[i+1], "ec", 2) == 0)
            {
               if(ref == BESSELIAN)
                  grid[ngrid].csys = ECLB;
               else
                  grid[ngrid].csys = ECLJ;
            }

            else if(strncasecmp(argv[i+1], "ga", 2) == 0)
               grid[ngrid].csys = GAL;

            ++i;

            if(grid[ngrid].epoch == -999.)
               grid[ngrid].epoch = 2000.;
            else
               ++i;

            ++ngrid;
         }


         /* LABEL */

         else if(strcmp(argv[i], "-label") == 0)
         {
            label[nlabel].red   = ovlyred;
            label[nlabel].green = ovlygreen;
            label[nlabel].blue  = ovlyblue;

            label[nlabel].alpha = alpha;

            if(i+3 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -label flag");
               return returnStruct;
            }

            label[nlabel].inpix = 0;
            if(strstr(argv[i+1], "p") != (char *)NULL
            || strstr(argv[i+2], "p") != (char *)NULL)
               label[nlabel].inpix = 1;

            label[nlabel].x = atof(argv[i+1]);
            label[nlabel].y = atof(argv[i+2]);

            strcpy(label[nlabel].text, argv[i+3]);

            label[nlabel].fontscale = fontScale;

            i += 3;

            ++nlabel;
         }


         /* NEXT SYMBOL ATTRIBUTES */

         else if(strcmp(argv[i], "-symbol") == 0)
         {
            if(i+1 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -symbol flag");
               return returnStruct;
            }

            symSize      = 0.;
            symRotAngle  = 0.;
            symType      = 0;
            symNMax      = 0;


            if(strcmp(argv[i+1], "none") == 0)
               ++i;

            else
            {
               ptr = argv[i+1] + strlen(argv[i+1]) - 1;


               symUnits = FRACTIONAL;

               if(*ptr == 's')
                  symUnits = SECONDS;

               else if(*ptr == 'm')
                  symUnits = MINUTES;

               else if(*ptr == 'd')
                  symUnits = DEGREES;

               else if(*ptr == 'p')
                  symUnits = PIXELS;

               if(symUnits != FRACTIONAL)
                  *ptr = '\0';

               symSize = strtod(argv[i+1], &end);

               if(end < (argv[i+1] + (int)strlen(argv[i+1])))
               {
                  strcpy(returnStruct->msg, "Invalid symbol size");
                  return returnStruct;
               }

               ++i;

               if(i+1 < argc && argv[i+1][0] != '-')
               {
                  if(strncasecmp(argv[i+1], "triangle", 3) == 0)
                  {
                     symNPnt     = 3;
                     symRotAngle = 120.;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "box", 3) == 0)
                  {
                     symNPnt     = 4;
                     symRotAngle = 45.;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "square", 3) == 0)
                  {
                     symNPnt     = 4;
                     symRotAngle = 45.;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "diamond", 3) == 0)
                  {
                     symNPnt     = 4;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "pentagon", 3) == 0)
                  {
                     symNPnt     = 5;
                     symRotAngle = 72.;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "hexagon", 3) == 0)
                  {
                     symNPnt     = 6;
                     symRotAngle = 60.;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "septagon", 3) == 0)
                  {
                     symNPnt     = 7;
                     symRotAngle = 360./7.;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "octagon", 3) == 0)
                  {
                     symNPnt     = 8;
                     symRotAngle = 45.;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "plus", 3) == 0)
                  {
                     symType     = 2;
                     symNPnt     = 4;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "el", 2) == 0)
                  {
                     symNPnt     = 4;
                     symRotAngle = 135.;
                     symNMax     = 2;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "circle", 3) == 0)
                  {
                     symNPnt     = 128;
                     symRotAngle = 0.;
                     ++i;
                  }

                  else if(strncasecmp(argv[i+1], "compass", 3) == 0)
                  {
                     symType      = 3;
                     symNPnt      = 4;
                     symRotAngle  = 0.;
                     ++i;
                  }

                  else
                  {
                     symType = strtol(argv[i+1], &end, 0);

                     if(end < (argv[i+1] + (int)strlen(argv[i+1])))
                     {
                        if(strncasecmp(argv[i+1], "polygon", 1) == 0)
                           symType = 0;

                        else if(strncasecmp(argv[i+1], "starred", 2) == 0)
                           symType = 1;

                        else if(strncasecmp(argv[i+1], "skeletal", 2) == 0)
                           symType = 2;

                        else
                        {
                           strcpy(returnStruct->msg, "Invalid symbol type");
                           return returnStruct;
                        }
                     }

                     ++i;
                    
                     if(i+1 < argc && argv[i+1][0] != '-')
                     {
                        symNPnt = strtol(argv[i+1], &end, 0);

                        if(end < (argv[i+1] + (int)strlen(argv[i+1])) || symNPnt < 3)
                        {
                           strcpy(returnStruct->msg, "Invalid vertex count for symbol (must be an integer >= 3)");
                           return returnStruct;
                        }

                        ++i;
                       
                        if(i+1 < argc && argv[i+1][0] != '-')
                        {
                           symRotAngle = strtod(argv[i+1], &end);

                           if(end < (argv[i+1] + (int)strlen(argv[i+1])))
                           {
                              strcpy(returnStruct->msg, "Invalid rotation angle for symbol (must be number)");
                              return returnStruct;
                           }

                           ++i;
                        }
                     }
                  }
               }
            }
         }


         /* CATALOG SYMBOL SCALE COLUMN */

         else if(strcmp(argv[i], "-scalecol") == 0)
         {
            if(strcmp(argv[i+1], "none") == 0)
            {
               strcpy(scaleColumn, "");
               ++i;
            }
            else
            {
               if(i+1 >= argc)
               {
                  strcpy(returnStruct->msg, "Too few arguments following -scalecol flag");
                  return returnStruct;
               }

               scaleVal  = 1.;
               scaleType = FLUX;

               if(i+1 < argc && argv[i+1][0] != '-')
               {
                  strcpy(scaleColumn, argv[i+1]);
                  ++i;

                  if(i+1 < argc && argv[i+1][0] != '-')
                  {
                     scaleVal = atof(argv[i+1]);
                     ++i;

                     if(i+1 < argc && argv[i+1][0] != '-')
                     {
                        scaleType = FLUX;

                        if(strncasecmp(argv[i+1], "mag", 3) == 0) scaleType = MAG;
                        if(strncasecmp(argv[i+1], "log", 3) == 0) scaleType = LOGFLUX;
                     }

                     ++i;
                  }
               }
            }
         }



         /* CATALOG COLOR COLUMN */

         else if(strcmp(argv[i], "-colorcol") == 0)
         {
            if(strcmp(argv[i+1], "none") == 0)
            {
               strcpy(colorColumn, "");
               ++i;
            }
            else
            {
               if(i+1 >= argc)
               {
                  strcpy(returnStruct->msg, "No color column given.");
                  return returnStruct;
               }

               strcpy(colorColumn, argv[i+1]);
               ++i;
            }
         }



         /* CATALOG SYMBOL SIZE COLUMN */

         else if(strcmp(argv[i], "-sizecol") == 0)
         {
            if(strcmp(argv[i+1], "none") == 0)
            {
               strcpy(symSizeColumn, "");
               ++i;
            }
            else
            {
               if(i+1 >= argc)
               {
                  strcpy(returnStruct->msg, "No symbol size column given.");
                  return returnStruct;
               }

               strcpy(symSizeColumn, argv[i+1]);
               ++i;
            }
         }



         /* CATALOG SYMBOL SHAPE COLUMN */

         else if(strcmp(argv[i], "-shapecol") == 0)
         {
            if(strcmp(argv[i+1], "none") == 0)
            {
               strcpy(symShapeColumn, "");
               ++i;
            }
            else
            {
               if(i+1 >= argc)
               {
                  strcpy(returnStruct->msg, "No symbol shape column given.");
                  return returnStruct;
               }

               strcpy(symShapeColumn, argv[i+1]);
               ++i;
            }
         }



         /* CATALOG LABEL COLUMN */

         else if(strcmp(argv[i], "-labelcol") == 0)
         {
            if(strcmp(argv[i+1], "none") == 0)
            {
               strcpy(labelColumn, "");
               ++i;
            }
            else
            {
               if(i+1 >= argc)
               {
                  strcpy(returnStruct->msg, "No label column given.");
                  return returnStruct;
               }

               strcpy(labelColumn, argv[i+1]);
               ++i;
            }
         }



         /* CATALOG OVERLAY */

         else if(strcmp(argv[i], "-catalog") == 0)
         {
            if(i+1 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -catalog flag");
               return returnStruct;
            }

            cat[ncat].isImgInfo = 0;
            cat[ncat].scaleVal  = scaleVal;
            cat[ncat].scaleType = scaleType;

            strcpy(cat[ncat].scaleColumn, scaleColumn);

            strcpy(cat[ncat].file, argv[i+1]);
            ++i;

            if(i+1 < argc && argv[i+1][0] != '-')
            {
               strcpy(cat[ncat].scaleColumn, argv[i+1]);
               ++i;

               if(i+1 < argc && argv[i+1][0] != '-')
               {
                  cat[ncat].scaleVal = atof(argv[i+1]);
                  ++i;

                  if(i+1 < argc && argv[i+1][0] != '-')
                  {
                     cat[ncat].scaleType = FLUX;

                     if(strncasecmp(argv[i+1], "mag", 3) == 0) cat[ncat].scaleType = MAG;
                     if(strncasecmp(argv[i+1], "log", 3) == 0) cat[ncat].scaleType = LOGFLUX;
                  }

                  ++i;
               }
            }

            cat[ncat].csys  = csys;
            cat[ncat].epoch = epoch;

            cat[ncat].symUnits    = symUnits;
            cat[ncat].symNPnt     = symNPnt;
            cat[ncat].symNMax     = symNMax;
            cat[ncat].symType     = symType;
            cat[ncat].symSize     = symSize;
            cat[ncat].symRotAngle = symRotAngle;

            cat[ncat].red   = ovlyred;
            cat[ncat].green = ovlygreen;
            cat[ncat].blue  = ovlyblue;

            strcpy(cat[ncat].colorColumn,    colorColumn);
            strcpy(cat[ncat].labelColumn,    labelColumn);
            strcpy(cat[ncat].symSizeColumn,  symSizeColumn);
            strcpy(cat[ncat].symShapeColumn, symShapeColumn);

            cat[ncat].fontscale = fontScale;
            cat[ncat].linewidth = lineWidth;

            cat[ncat].alpha = alpha;

            ++ncat;
         }


         /* SINGLE MARKER */

         else if(strcmp(argv[i], "-mark") == 0)
         {
            if(i+2 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -mark flag");
               return returnStruct;
            }

            mark[nmark].ra  = atof(argv[i+1]);
            mark[nmark].dec = atof(argv[i+2]);

            mark[nmark].inpix = 0;

            if(strstr(argv[i+1], "p") != (char *)NULL
            || strstr(argv[i+2], "p") != (char *)NULL)
               mark[nmark].inpix = 1;

            mark[nmark].csys  = csys;
            mark[nmark].epoch = epoch;

            mark[nmark].symUnits    = symUnits;
            mark[nmark].symNPnt     = symNPnt;
            mark[nmark].symNMax     = symNMax;
            mark[nmark].symType     = symType;
            mark[nmark].symSize     = symSize;
            mark[nmark].symRotAngle = symRotAngle;

            mark[nmark].red   = ovlyred;
            mark[nmark].green = ovlygreen;
            mark[nmark].blue  = ovlyblue;

            mark[nmark].linewidth = lineWidth;

            mark[nmark].alpha = alpha;

            ++nmark;

            i += 2;
         }


         /* IMAGE METADATA OVERLAY */

         else if(strcmp(argv[i], "-imginfo") == 0)
         {
            if(i+1 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -imginfo flag");
               return returnStruct;
            }

            cat[ncat].isImgInfo = 1;

            strcpy(cat[ncat].file, argv[i+1]);
            strcpy(cat[ncat].scaleColumn , "");

            cat[ncat].scaleVal  = 0.;
            cat[ncat].scaleType = FLUX;

            cat[ncat].csys  = csys;
            cat[ncat].epoch = epoch;

            cat[ncat].symSize     = 0.;
            cat[ncat].symUnits    = 0;
            cat[ncat].symNPnt     = 0;
            cat[ncat].symType     = 0;
            cat[ncat].symRotAngle = 0.;

            cat[ncat].red   = ovlyred;
            cat[ncat].green = ovlygreen;
            cat[ncat].blue  = ovlyblue;

            cat[ncat].linewidth = lineWidth;

            cat[ncat].alpha = alpha;

            strcpy(cat[ncat].colorColumn,    colorColumn);
            strcpy(cat[ncat].labelColumn,    "");
            strcpy(cat[ncat].symSizeColumn,  "");
            strcpy(cat[ncat].symShapeColumn, "");

            cat[ncat].fontscale = fontScale;

            ++ncat;

            ++i;
         }


         /* IMAGE METADATA OVERLAY */

         else if(strcmp(argv[i], "-draw") == 0)
         {
            if(i+1 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -drawing flag");
               return returnStruct;
            }

            strcpy(drawing[ndrawing].file, argv[i+1]);

            drawing[ndrawing].alpha = alpha;

            ++ndrawing;

            ++i;
         }


         /* COLOR TABLE */

         else if(strcmp(argv[i], "-ct") == 0)
         {
            if(i+1 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -ct flag");
               return returnStruct;
            }

            colortable = strtol(argv[i+1], &end, 10);

            if(colortable < 0  || colortable > 12 || end < argv[i+1]+strlen(argv[i+1]))
            {
               strcpy(returnStruct->msg, "Color table index must be a number between 0 and 12");
               return returnStruct;
            }

            ++i;
         }
         

         /* GRAY */

         else if(strcmp(argv[i], "-gray") == 0
              || strcmp(argv[i], "-grey") == 0)
         {
            if(i+1 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -gray flag");
               return returnStruct;
            }

            strcpy(grayfile, argv[i+1]);

            grayPlaneCount = mViewer_getPlanes(grayfile, grayPlanes);

            if(grayPlaneCount > 0)
               hdu = grayPlanes[0];
            else
               hdu = 0;

            if(!nowcs)
            {
               checkHdr = montage_checkHdr(grayfile, 0, hdu);
 
               if(checkHdr)
               {
                  strcpy(returnStruct->msg, checkHdr);
                  return returnStruct;
               }
            }


            // Two modes:  histogram from a file or histogram to be computed by this program 

            if(i+3 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -gray flag");
               return returnStruct;
            }

            if(strcmp(argv[i+2], "-histfile") == 0)
            {
               strcpy(grayhistfile, argv[i+3]);
               i += 2;
            }
            else
            {
               strcpy(grayminstr, argv[i+2]);
               strcpy(graymaxstr, argv[i+3]);

               grayType = POWER;

               if(i+4 < argc)
               {
                  if(argv[i+4][0] == 'g')
                  {
                     grayType = GAUSSIAN;

                     if(strlen(argv[i+4]) > 1 
                     && (   argv[i+4][strlen(argv[i+4])-1] == 'g'
                         || argv[i+4][strlen(argv[i+4])-1] == 'l'))
                        grayType = GAUSSIANLOG;

                     i += 1;
                  }
                  
                  else if(argv[i+4][0] == 'a')
                  {
                     grayType = ASINH;

                     strcpy(graybetastr, "2s");

                     if(i+5 < argc)
                        strcpy(graybetastr, argv[i+5]);
                     
                     i += 2;
                  }
                  
                  else if(strncmp(argv[i+4], "lin", 3) == 0)
                  {
                     graylogpower = 0;
                     ++i;
                  }
                  
                  else if(strcmp(argv[i+4], "log") == 0)
                  {
                     graylogpower = 1;
                     ++i;
                  }

                  else if(strcmp(argv[i+4], "loglog") == 0)
                  {
                     graylogpower = 2;
                     ++i;
                  }

                  else
                  {
                     graylogpower = strtol(argv[i+4], &end, 10);
       
                     if(graylogpower < 0  || end < argv[i+4] + strlen(argv[i+4]))
                        graylogpower = 0;
                     else
                        i += 1;
                  }
               }
               
               i += 2;
            }


            if(fits_open_file(&grayfptr, grayfile, READONLY, &status))
            {
               sprintf(returnStruct->msg, "Image file %s invalid FITS", grayfile);
               return returnStruct;
            }

            if(hdu > 0)
            {
               if(fits_movabs_hdu(grayfptr, hdu+1, NULL, &status))
               {
                  sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
                  return returnStruct;
               }
            }

            ++i;
         }


         /* RED */

         else if(strcmp(argv[i], "-red") == 0)
         {
            if(i+3 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -red flag");
               return returnStruct;
            }

            strcpy(redfile, argv[i+1]);

            redPlaneCount = mViewer_getPlanes(redfile, redPlanes);

            if(redPlaneCount > 0)
               hdu = redPlanes[0];
            else
               hdu = 0;

            if(!nowcs)
            {
               checkHdr = montage_checkHdr(redfile, 0, hdu);
 
               if(checkHdr)
               {
                  strcpy(returnStruct->msg, checkHdr);
                  return returnStruct;
               }
            }


            // Two modes:  histogram from a file or histogram to be computed by this program

            if(i+3 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -red flag");
               return returnStruct;
            }

            if(strcmp(argv[i+2], "-histfile") == 0)
            {
               strcpy(redhistfile, argv[i+3]);
               i += 2;
            }

            else
            {
               strcpy(redminstr, argv[i+2]);
               strcpy(redmaxstr, argv[i+3]);

               redType = POWER;

               if(i+4 < argc)
               {
                  if(argv[i+4][0] == 'g')
                  {
                     redType = GAUSSIAN;

                     if(strlen(argv[i+4]) > 1 
                     && (   argv[i+4][strlen(argv[i+4])-1] == 'g'
                         || argv[i+4][strlen(argv[i+4])-1] == 'l'))
                        redType = GAUSSIANLOG;
                     
                     i += 1;
                  }
                  
                  else if(argv[i+4][0] == 'a')
                  {
                     redType = ASINH;

                     strcpy(redbetastr, "2s");

                     if(i+5 < argc)
                        strcpy(redbetastr, argv[i+5]);

                     i += 2;
                  }

                  else if(strncmp(argv[i+4], "lin", 3) == 0)
                  {
                     redlogpower = 0;
                     ++i;
                  }
                  
                  else if(strcmp(argv[i+4], "log") == 0)
                  {
                     redlogpower = 1;
                     ++i;
                  }

                  else if(strcmp(argv[i+4], "loglog") == 0)
                  {
                     redlogpower = 2;
                     ++i;
                  }

                  else
                  {
                     redlogpower = strtol(argv[i+4], &end, 10);

                     if(redlogpower < 0  || end < argv[i+4] + strlen(argv[i+4]))
                        redlogpower = 0.;
                     else
                        i += 1;
                  }
               }
               
               i += 2;
            }

            if(fits_open_file(&redfptr, redfile, READONLY, &status))
            {
               sprintf(returnStruct->msg, "Image file %s invalid FITS", redfile);
               return returnStruct;
            }

            if(hdu > 0)
            {
               redhdu = hdu;

               if(fits_movabs_hdu(redfptr, hdu+1, NULL, &status))
               {
                  sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
                  return returnStruct;
               }
            }

            ++i;
         }


         else if(strcmp(argv[i], "-green") == 0)
         {
            if(i+3 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -green flag");
               return returnStruct;
            }

            strcpy(greenfile, argv[i+1]);

            greenPlaneCount = mViewer_getPlanes(greenfile, greenPlanes);

            if(greenPlaneCount > 0)
               hdu = greenPlanes[0];
            else
               hdu = 0;

            if(!nowcs)
            {
               checkHdr = montage_checkHdr(greenfile, 0, hdu);
 
               if(checkHdr)
               {
                  strcpy(returnStruct->msg, checkHdr);
                  return returnStruct;
               }
            }


            // Two modes:  histogram from a file or histogram to be computed by this program

            if(i+3 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -green flag");
               return returnStruct;
            }

            if(strcmp(argv[i+2], "-histfile") == 0)
            {
               strcpy(greenhistfile, argv[i+3]);
               i += 2;
            }

            else
            {
               strcpy(greenminstr, argv[i+2]);
               strcpy(greenmaxstr, argv[i+3]);

               greenType = POWER;

               if(i+4 < argc)
               {
                  if(argv[i+4][0] == 'g')
                  {
                     greenType = GAUSSIAN;

                     if(strlen(argv[i+4]) > 1 
                     && (   argv[i+4][strlen(argv[i+4])-1] == 'g'
                         || argv[i+4][strlen(argv[i+4])-1] == 'l'))
                        greenType = GAUSSIANLOG;

                     i += 1;
                  }

                  else if(argv[i+4][0] == 'a')
                  {
                     greenType = ASINH;

                     strcpy(greenbetastr, "2s");

                     if(i+5 < argc)
                        strcpy(greenbetastr, argv[i+5]);

                     i += 2;
                  }

                  else if(strncmp(argv[i+4], "lin", 3) == 0)
                  {
                     greenlogpower = 0;
                     ++i;
                  }
                  
                  else if(strcmp(argv[i+4], "log") == 0)
                  {
                     greenlogpower = 1;
                     ++i;
                  }

                  else if(strcmp(argv[i+4], "loglog") == 0)
                  {
                     greenlogpower = 2;
                     ++i;
                  }

                  else
                  {
                     greenlogpower = strtol(argv[i+4], &end, 10);

                     if(greenlogpower < 0  || end < argv[i+4] + strlen(argv[i+4]))
                        greenlogpower = 0;
                     else
                        i += 1;
                  }
               }
               
               i += 2;
            }

            if(fits_open_file(&greenfptr, greenfile, READONLY, &status))
            {
               sprintf(returnStruct->msg, "Image file %s invalid FITS", greenfile);
               return returnStruct;
            }

            if(hdu > 0)
            {
               if(fits_movabs_hdu(greenfptr, hdu+1, NULL, &status))
               {
                  sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
                  return returnStruct;
               }
            }

            ++i;
         }


         /* BLUE */

         else if(strcmp(argv[i], "-blue") == 0)
         {
            if(i+3 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -blue flag");
               return returnStruct;
            }

            strcpy(bluefile, argv[i+1]);

            bluePlaneCount = mViewer_getPlanes(bluefile, bluePlanes);

            if(bluePlaneCount > 0)
               hdu = bluePlanes[0];
            else
               hdu = 0;

            if(!nowcs)
            {
               checkHdr = montage_checkHdr(bluefile, 0, hdu);
 
               if(checkHdr)
               {
                  strcpy(returnStruct->msg, checkHdr);
                  return returnStruct;
               }
            }

            // Two modes:  histogram from a file or histogram to be computed by this program

            if(i+3 >= argc)
            {
               strcpy(returnStruct->msg, "Too few arguments following -blue flag");
               return returnStruct;
            }

            if(strcmp(argv[i+2], "-histfile") == 0)
            {
               strcpy(bluehistfile, argv[i+3]);
               i += 2;
            }

            else
            {
               strcpy(blueminstr, argv[i+2]);
               strcpy(bluemaxstr, argv[i+3]);

               blueType = POWER;

               if(i+4 < argc)
               {
                  if(argv[i+4][0] == 'g')
                  {
                     blueType = GAUSSIAN;

                     if(strlen(argv[i+4]) > 1 
                     && (   argv[i+4][strlen(argv[i+4])-1] == 'g'
                         || argv[i+4][strlen(argv[i+4])-1] == 'l'))
                        blueType = GAUSSIANLOG;

                     i += 1;
                  }

                  else if(argv[i+4][0] == 'a')
                  {
                     blueType = ASINH;

                     strcpy(bluebetastr, "2s");

                     if(i+5 < argc)
                        strcpy(bluebetastr, argv[i+5]);

                     i += 2;
                  }
                  
                  else if(strncmp(argv[i+4], "lin", 3) == 0)
                  {
                     bluelogpower = 0;
                     ++i;
                  }
                  
                  else if(strcmp(argv[i+4], "log") == 0)
                  {
                     bluelogpower = 1;
                     ++i;
                  }

                  else if(strcmp(argv[i+4], "loglog") == 0)
                  {
                     bluelogpower = 2;
                     ++i;
                  }

                  else
                  {
                     bluelogpower = strtol(argv[i+4], &end, 10);

                     if(bluelogpower < 0. || end < argv[i+4] + strlen(argv[i+4]))
                        bluelogpower = 0.;
                     else
                        i += 1;
                  }
               }
               
               i += 2;
            }

            if(fits_open_file(&bluefptr, bluefile, READONLY, &status))
            {
               sprintf(returnStruct->msg, "Image file %s invalid FITS", bluefile);
               return returnStruct;
            }

            if(hdu > 0)
            {
               if(fits_movabs_hdu(bluefptr, hdu+1, NULL, &status))
               {
                  sprintf(returnStruct->msg, "Can't find HDU %d", hdu);
                  return returnStruct;
               }
            }

            ++i;
         }


         /* OUTPUT FILE */

         else if(strcmp(argv[i], "-out") == 0
              || strcmp(argv[i], "-png") == 0)
         {
            if(i+1 >= argc)
            {
               strcpy(returnStruct->msg, "No output file given following -out/-png flag");
               return returnStruct;
            }

            strcpy(pngfile, argv[i+1]);

            outType = PNG;

            ++i;
         }

         else if(strcmp(argv[i], "-jpeg") == 0)
         {
            if(i+1 >= argc)
            {
               strcpy(returnStruct->msg, "No output file given following -jpeg flag");
               return returnStruct;
            }

            strcpy(jpegfile, argv[i+1]);


            jpegfp = fopen(jpegfile, "w+");

            if(jpegfp == (FILE *)NULL)
            {
               sprintf(returnStruct->msg, "Error opening output file '%s'", jpegfile);
               return returnStruct;
            }

            outType = JPEG;

            ++i;
         }

         else
         {
            if(argv[i][0] != '-')
               sprintf(returnStruct->msg, "Invalid 'directive': %s (probably a misplaced argument)", argv[i]);
            else
               sprintf(returnStruct->msg, "Invalid directive: %s", argv[i]);
            return returnStruct;
         }
      }
   }



   /***************************************************/
   /* In order to make "red/blue" images, we need to  */
   /* fake out the non-existent green image.  We'll   */
   /* make it the same as red but with a stretch that */
   /* sets all pixels black                           */
   /***************************************************/

   greenOff = 0;

   if(strlen(grayfile) == 0 && strlen(greenfile) == 0)
   {
      greenOff = 1;

      strcpy(greenfile, redfile);

      greenPlaneCount = redPlaneCount;

      for(i=0; i<redPlaneCount; ++i)
         greenPlanes[i] = redPlanes[i];

      strcpy(greenminstr, redminstr);
      strcpy(greenmaxstr, redmaxstr);

      greenType = redType;

      strcpy(greenbetastr, redbetastr);

      greenlogpower = redlogpower;

      fits_open_file(&greenfptr, greenfile, READONLY, &status);

      if(redhdu > 0)
         fits_movabs_hdu(greenfptr, redhdu+1, NULL, &status);
   }

   if(mViewer_debug)
   {
      printf("DEBUG> statusfile      = [%s]\n", statusfile);
      printf("DEBUG> colortable      = [%d]\n", colortable);
      printf("\n");
      printf("DEBUG> grayfile        = [%s]\n", grayfile);
      printf("DEBUG> grayminstr      = [%s]\n", grayminstr);
      printf("DEBUG> graymaxstr      = [%s]\n", graymaxstr);
      printf("DEBUG> graylogpower    = [%d]\n", graylogpower);
      printf("DEBUG> grayType        = [%d]\n", grayType);
      printf("DEBUG> graybetastr     = [%s]\n", graybetastr);
      printf("\n");
      printf("DEBUG> redfile         = [%s]\n", redfile);
      printf("DEBUG> redminstr       = [%s]\n", redminstr);
      printf("DEBUG> redmaxstr       = [%s]\n", redmaxstr);
      printf("DEBUG> redlogpower     = [%d]\n", redlogpower);
      printf("DEBUG> redType         = [%d]\n", redType);
      printf("DEBUG> redbetastr      = [%s]\n", redbetastr);
      printf("\n");
      printf("DEBUG> greenfile       = [%s]\n", greenfile);
      printf("DEBUG> greenminstr     = [%s]\n", greenminstr);
      printf("DEBUG> greenmaxstr     = [%s]\n", greenmaxstr);
      printf("DEBUG> greenlogpower   = [%d]\n", greenlogpower);
      printf("DEBUG> greenType       = [%d]\n", greenType);
      printf("DEBUG> greenbetastr    = [%s]\n", greenbetastr);
      printf("\n");
      printf("DEBUG> bluefile        = [%s]\n", bluefile);
      printf("DEBUG> blueminstr      = [%s]\n", blueminstr);
      printf("DEBUG> bluemaxstr      = [%s]\n", bluemaxstr);
      printf("DEBUG> bluelogpower    = [%d]\n", bluelogpower);
      printf("DEBUG> blueType        = [%d]\n", blueType);
      printf("DEBUG> bluebetastr     = [%s]\n", bluebetastr);
      printf("\n");

      for(i=0; i<ngrid; ++i)
      {
         printf("DEBUG> grid[%d].fontscale   = [%-g]\n", i, grid[i].fontscale );
         printf("DEBUG> grid[%d].linewidth   = [%-g]\n", i, grid[i].linewidth);
         printf("DEBUG> grid[%d].csys        = [%d]\n",  i, grid[i].csys );
         printf("DEBUG> grid[%d].epoch       = [%-g]\n", i, grid[i].epoch);
         printf("DEBUG> grid[%d].red         = [%-g]\n", i, grid[i].red  );
         printf("DEBUG> grid[%d].green       = [%-g]\n", i, grid[i].green);
         printf("DEBUG> grid[%d].blue        = [%-g]\n", i, grid[i].blue );
         printf("DEBUG> grid[%d].alpha       = [%-g]\n", i, grid[i].alpha);
      }

      for(i=0; i<ncat; ++i)
      {
         printf("DEBUG> cat[%d].symNPnt      = [%d]\n",  i, cat[i].symNPnt  );
         printf("DEBUG> cat[%d].file         = [%s]\n",  i, cat[i].file );
         printf("DEBUG> cat[%d].scaleColumn  = [%s]\n",  i, cat[i].scaleColumn  );
         printf("DEBUG> cat[%d].symUnits     = [%d]\n",  i, cat[i].symUnits);
         printf("DEBUG> cat[%d].scaleVal     = [%-g]\n", i, cat[i].scaleVal  );
         printf("DEBUG> cat[%d].scaleType    = [%d]\n",  i, cat[i].scaleType );
         printf("DEBUG> cat[%d].csys         = [%d]\n",  i, cat[i].csys );
         printf("DEBUG> cat[%d].epoch        = [%-g]\n", i, cat[i].epoch);
         printf("DEBUG> cat[%d].symNMax      = [%d]\n",  i, cat[i].symNMax  );
         printf("DEBUG> cat[%d].symType      = [%d]\n",  i, cat[i].symType );
         printf("DEBUG> cat[%d].symSize      = [%-g]\n", i, cat[i].symSize );
         printf("DEBUG> cat[%d].symRotAngle  = [%-g]\n", i, cat[i].symRotAngle  );
         printf("DEBUG> cat[%d].red          = [%-g]\n", i, cat[i].red  );
         printf("DEBUG> cat[%d].green        = [%-g]\n", i, cat[i].green);
         printf("DEBUG> cat[%d].blue         = [%-g]\n", i, cat[i].blue );
         printf("DEBUG> cat[%d].alpha        = [%-g]\n", i, cat[i].alpha);
         printf("DEBUG> cat[%d].linewidth    = [%-g]\n", i, cat[i].linewidth );
      }

      for(i=0; i<ndrawing; ++i)
      {
         printf("DEBUG> drawing[%d].file     = [%s]\n",  i, drawing[i].file);
         printf("DEBUG> drawing[%d].alpha    = [%-g]\n", i, drawing[i].alpha);
      }

      for(i=0; i<nmark; ++i)
      {
         printf("DEBUG> mark[%d].ra          = [%-g]\n", i, mark[i].ra   );
         printf("DEBUG> mark[%d].dec         = [%-g]\n", i, mark[i].dec  );
         printf("DEBUG> mark[%d].symUnits    = [%d]\n",  i, mark[i].symUnits);
         printf("DEBUG> mark[%d].csys        = [%d]\n",  i, mark[i].csys );
         printf("DEBUG> mark[%d].epoch       = [%-g]\n", i, mark[i].epoch);
         printf("DEBUG> mark[%d].symNPnt     = [%d]\n",  i, mark[i].symNPnt  );
         printf("DEBUG> mark[%d].symNMax     = [%d]\n",  i, mark[i].symNMax  );
         printf("DEBUG> mark[%d].symType     = [%d]\n",  i, mark[i].symType );
         printf("DEBUG> mark[%d].symSize     = [%-g]\n", i, mark[i].symSize );
         printf("DEBUG> mark[%d].symRotAngle = [%-g]\n", i, mark[i].symRotAngle  );
         printf("DEBUG> mark[%d].red         = [%-g]\n", i, mark[i].red  );
         printf("DEBUG> mark[%d].green       = [%-g]\n", i, mark[i].green);
         printf("DEBUG> mark[%d].blue        = [%-g]\n", i, mark[i].blue );
         printf("DEBUG> mark[%d].linewidth   = [%-g]\n", i, mark[i].linewidth );
      }

      for(i=0; i<nlabel; ++i)
      {
         printf("DEBUG> label[%d].x          = [%-g]\n", i, label[i].x   );
         printf("DEBUG> label[%d].y          = [%-g]\n", i, label[i].y   );
         printf("DEBUG> label[%d].text       = [%s]\n",  i, label[i].text);
         printf("DEBUG> label[%d].fontscale  = [%-g]\n", i, label[i].fontscale );
         printf("DEBUG> label[%d].alpha      = [%-g]\n", i, label[i].alpha);
      }

      printf("\n");
      printf("DEBUG> outType                 = [%d]\n", outType);
      printf("DEBUG> pngfile                 = [%s]\n", pngfile);
      printf("DEBUG> jpegfile                = [%s]\n", jpegfile);
   }


   isRGB = 0;

   if(strlen(redfile)   > 0
   || strlen(greenfile) > 0
   || strlen(bluefile)  > 0)
      isRGB = 1;
   

   if(isRGB)
   {
      if(strlen(redfile) == 0)
      {
         strcpy(returnStruct->msg, "No input 'red' FITS file name given");
         return returnStruct;
      }

      if(strlen(greenfile) == 0)
      {
         strcpy(returnStruct->msg, "No input 'green' FITS file name given");
         return returnStruct;
      }

      if(strlen(bluefile) == 0)
      {
         strcpy(returnStruct->msg, "No input 'blue' FITS file name given");
         return returnStruct;
      }
   }
   else
   {
      if(strlen(grayfile) == 0)
      {
         strcpy(returnStruct->msg, "No input FITS file name given");
         return returnStruct;
      }
   }

      
   if(strlen(pngfile)  == 0
   && strlen(jpegfile) == 0)
   {
      strcpy(returnStruct->msg, "No output PNG or JPEG file name given");
      return returnStruct;
   }


   /***************************/
   /* Set up pseudocolor info */
   /***************************/

   mViewer_createColorTable(colortable);


   /*****************************************************/
   /* We are either making a grayscale/pseudocolor JPEG */
   /* from a single FITS file or a true color JPEG from */
   /* three FITS files.                                 */
   /*****************************************************/

   /* Full color mode */

   if(isRGB)
   {
      if(mViewer_debug)
      {
         printf("DEBUG> Processing RGB mode\n");
      }

      /* First make sure that the WCS info is OK */
      /* and is the same for all three images    */

      if(strlen(redfile) == 0)
      {
         strcpy(returnStruct->msg, "Color mode but no red image given");
         return returnStruct;
      }

      if(strlen(greenfile) == 0)
      {
         strcpy(returnStruct->msg, "Color mode but no green image given");
         return returnStruct;
      }

      if(strlen(bluefile) == 0)
      {
         strcpy(returnStruct->msg, "Color mode but no blue image given");
         return returnStruct;
      }


      /* Get the red file's header */

      if(fits_get_hdrpos(redfptr, &keysexist, &keynum, &status))
         mViewer_printFitsError(status);

      if(fits_get_image_wcs_keys(redfptr, &header, &status))
         mViewer_printFitsError(status);

      wcs = wcsinit(header);

      free(header);


      if(wcs == (struct WorldCoor *)NULL)
      {
         if(nowcs)
         {
            status = 0;
            if(fits_read_key(redfptr, TLONG, "NAXIS1", &naxisl, (char *)NULL, &status))
               mViewer_printFitsError(status);

            naxis1 = naxisl;

            status = 0;
            if(fits_read_key(redfptr, TLONG, "NAXIS2", &naxisl, (char *)NULL, &status))
               mViewer_printFitsError(status);

            naxis2 = naxisl;

            wcs = mViewer_wcsfake(naxis1, naxis2);
         }
         else
         {
            sprintf(returnStruct->msg, "WCS init failed for [%s].", redfile);
            return returnStruct;
         }
      }

      if(strcmp(wcs->ptype, "HPX") == 0)
      {
         hpx = 1;

         hpxPix = 90.0 / fabs(wcs->xinc) / sqrt(2.0) + 0.5;

         hpxLevel = log10((double)hpxPix)/log10(2.) + 0.5;

         hpxPix = pow(2., (double)hpxLevel) + 0.5;

         hpxPix = 4 * hpxPix;
      }

      if(!nowcs)
         montage_checkWCS(wcs);

      naxis1 = wcs->nxpix;
      naxis2 = wcs->nypix;

      crval1 = wcs->xpos;
      crval2 = wcs->ypos;

      crpix1 = wcs->xrefpix;
      crpix2 = wcs->yrefpix;

      xinc   = wcs->xinc;
      yinc   = wcs->yinc;
      cdelt1 = wcs->cdelt[0];
      cdelt2 = wcs->cdelt[1];
      cd11   = wcs->cd[0];
      cd12   = wcs->cd[1];
      cd21   = wcs->cd[2];
      cd22   = wcs->cd[3];
      crota2 = wcs->rot;

      if(wcs->syswcs == WCS_J2000)
      {
         csysimg  = EQUJ;
         epochimg = 2000.;

         if(wcs->equinox == 1950.)
            epochimg = 1950;
      }
      else if(wcs->syswcs == WCS_B1950)
      {
         csysimg  = EQUB;
         epochimg = 1950.;

         if(wcs->equinox == 2000.)
            epochimg = 2000.;
      }
      else if(wcs->syswcs == WCS_GALACTIC)
      {
         csysimg  = GAL;
         epochimg = 2000.;
      }
      else if(wcs->syswcs == WCS_ECLIPTIC)
      {
         csysimg  = ECLJ;
         epochimg = 2000.;

         if(wcs->equinox == 1950.)
         {
            csysimg  = ECLB;
            epochimg = 1950.;
         }
      }
      else
      {
         csysimg  = EQUJ;
         epochimg = 2000.;
      }

      if(mViewer_debug)
      {
         printf("\nRed image:\n");
         printf("naxis1   = %d\n", (int)wcs->nxpix);
         printf("naxis2   = %d\n", (int)wcs->nypix);
         printf("crval1   = %.6f\n", wcs->xpos);
         printf("crval2   = %.6f\n", wcs->ypos);
         printf("crpix1   = %.6f\n", wcs->xrefpix);
         printf("crpix2   = %.6f\n", wcs->yrefpix);
         printf("xinc     = %.6f\n", wcs->xinc);
         printf("yinc     = %.6f\n", wcs->yinc);
         printf("crota2   = %.6f\n", wcs->rot);
         printf("csysimg  = %d\n",   csysimg);
         printf("epochimg = %-g\n",  epochimg);
         printf("\n");

         fflush(stdout);
      }

      redxmin = -wcs->xrefpix;
      redxmax = redxmin + wcs->nxpix;

      redymin = -wcs->yrefpix;
      redymax = redymin + wcs->nypix;

      if(mViewer_debug)
      {
         printf("redxmin   = %-g\n", redxmin);
         printf("redxmax   = %-g\n", redxmax);
         printf("redymin   = %-g\n", redymin);
         printf("redymax   = %-g\n", redymax);
         fflush(stdout);
      }



      /* Kludge to get around bug in WCS library:   */
      /* 360 degrees sometimes added to pixel coord */

      ix = 0.5;
      iy = 0.5;

      pix2wcs(wcs, ix, iy, &xpos, &ypos);

      offscl = wcs->offscl;

      x = 0.;
      y = 0.;

      if(!offscl)
         wcs2pix(wcs, xpos, ypos, &x, &y, &offscl);

      if(hpx) mViewer_hpxCheck(&offscl, &x, &y);

      xcorrection = x-ix;
      ycorrection = y-iy;

      if(offscl || mNaN(x) || mNaN(y))
      {
         xcorrection = 0.;
         ycorrection = 0.;
      }



      /* Check to see if the green image */
      /* has the same size / scale       */

      if(fits_get_hdrpos(greenfptr, &keysexist, &keynum, &status))
         mViewer_printFitsError(status);

      if(fits_get_image_wcs_keys(greenfptr, &header, &status))
         mViewer_printFitsError(status);

      wcsfree(wcs);

      wcs = wcsinit(header);

      free(header);

      if(wcs == (struct WorldCoor *)NULL)
      {
         if(nowcs)
         {
            status = 0;
            if(fits_read_key(greenfptr, TLONG, "NAXIS1", &naxisl, (char *)NULL, &status))
               mViewer_printFitsError(status);

            naxis1 = naxisl;

            status = 0;
            if(fits_read_key(greenfptr, TLONG, "NAXIS2", &naxisl, (char *)NULL, &status))
               mViewer_printFitsError(status);

            naxis2 = naxisl;

            wcs = mViewer_wcsfake(naxis1, naxis2);
         }
         else
         {
            sprintf(returnStruct->msg, "WCS init failed for [%s].", greenfile);
            return returnStruct;
         }
      }

      if(!nowcs)
         montage_checkWCS(wcs);

      if(mViewer_debug)
      {
         printf("\nGreen image:\n");
         printf("naxis1   = %d\n", (int)wcs->nxpix);
         printf("naxis2   = %d\n", (int)wcs->nypix);
         printf("crval1   = %.6f\n", wcs->xpos);
         printf("crval2   = %.6f\n", wcs->ypos);
         printf("crpix1   = %.6f\n", wcs->xrefpix);
         printf("crpix2   = %.6f\n", wcs->yrefpix);
         printf("xinc     = %.6f\n", wcs->xinc);
         printf("yinc     = %.6f\n", wcs->yinc);
         printf("crota2   = %.6f\n", wcs->rot);
         fflush(stdout);
      }

      if(!nowcs)
      {
         if(crval1 != wcs->xpos
         || crval2 != wcs->ypos
         || xinc   != wcs->xinc
         || yinc   != wcs->yinc
         || fabs(crota2 - wcs->rot) > 1.e-9)
         {
            strcpy(returnStruct->msg, "Red and green FITS images don't have matching projections (use -nowcs flag if you still want to proceed).");
            return returnStruct;
         }
      }

      greenxmin = -wcs->xrefpix;
      greenxmax = greenxmin + wcs->nxpix;

      greenymin = -wcs->yrefpix;
      greenymax = greenymin + wcs->nypix;

      if(mViewer_debug)
      {
         printf("greenxmin = %-g\n", greenxmin);
         printf("greenxmax = %-g\n", greenxmax);
         printf("greenymin = %-g\n", greenymin);
         printf("greenymax = %-g\n", greenymax);
         fflush(stdout);
      }



      /* Check to see if the blue image */
      /* has the same size / scale      */

      if(fits_get_hdrpos(bluefptr, &keysexist, &keynum, &status))
         mViewer_printFitsError(status);

      status = 0;
      fits_read_key(bluefptr, TSTRING, "BUNIT", bunit, (char *)NULL, &status);

      if(status == KEY_NO_EXIST)
         strcpy(bunit, "");

      status = 0;
      if(fits_get_image_wcs_keys(bluefptr, &header, &status))
         mViewer_printFitsError(status);

      wcsfree(wcs);

      wcs = wcsinit(header);

      free(header);

      if(wcs == (struct WorldCoor *)NULL)
      {
         if(nowcs)
         {
            status = 0;
            if(fits_read_key(bluefptr, TLONG, "NAXIS1", &naxisl, (char *)NULL, &status))
               mViewer_printFitsError(status);

            naxis1 = naxisl;

            status = 0;
            if(fits_read_key(bluefptr, TLONG, "NAXIS2", &naxisl, (char *)NULL, &status))
               mViewer_printFitsError(status);

            naxis2 = naxisl;

            wcs = mViewer_wcsfake(naxis1, naxis2);
         }
         else
         {
            sprintf(returnStruct->msg, "WCS init failed for [%s].", bluefile);
            return returnStruct;
         }
      }

      if(!nowcs)
         montage_checkWCS(wcs);

      if(mViewer_debug)
      {
         printf("\nBlue image:\n");
         printf("naxis1   = %d\n", (int)wcs->nxpix);
         printf("naxis2   = %d\n", (int)wcs->nypix);
         printf("crval1   = %.6f\n", wcs->xpos);
         printf("crval2   = %.6f\n", wcs->ypos);
         printf("crpix1   = %.6f\n", wcs->xrefpix);
         printf("crpix2   = %.6f\n", wcs->yrefpix);
         printf("xinc     = %.6f\n", wcs->xinc);
         printf("yinc     = %.6f\n", wcs->yinc);
         printf("crota2   = %.6f\n", wcs->rot);
         fflush(stdout);
      }

      if(!nowcs)
      {
         if(crval1 != wcs->xpos
         || crval2 != wcs->ypos
         || xinc   != wcs->xinc
         || yinc   != wcs->yinc
         || fabs(crota2 - wcs->rot) > 1.e-9)
         {
            strcpy(returnStruct->msg, "Red and blue FITS images don't have matching projections (use -nowcs flag if you still want to proceed).");
            return returnStruct;
         }
      }

      bluexmin = -wcs->xrefpix;
      bluexmax = bluexmin + wcs->nxpix;

      blueymin = -wcs->yrefpix;
      blueymax = blueymin + wcs->nypix;

      if(mViewer_debug)
      {
         printf("bluexmin  = %-g\n", bluexmin);
         printf("bluexmax  = %-g\n", bluexmax);
         printf("blueymin  = %-g\n", blueymin);
         printf("blueymax  = %-g\n", blueymax);
         fflush(stdout);
      }


      /* Find the X offsets and total size */

      xmin = redxmin;

      if(greenxmin < xmin) xmin = greenxmin;
      if(bluexmin  < xmin) xmin = bluexmin;

      xmax = redxmax;

      if(greenxmax > xmax) xmax = greenxmax;
      if(bluexmax  > xmax) xmax = bluexmax;

      redxoff   = redxmin   - xmin;
      greenxoff = greenxmin - xmin;
      bluexoff  = bluexmin  - xmin;

      greennaxis1 = greenxmax - greenxmin;
      rednaxis1   = redxmax   - redxmin;
      bluenaxis1  = bluexmax  - bluexmin;

      naxis1 = xmax - xmin;

      crpix1 = -xmin;


      /* Find the Y offsets and total size */

      ymin = redymin;

      if(greenymin < ymin) ymin = greenymin;
      if(blueymin  < ymin) ymin = blueymin;

      ymax = redymax;

      if(greenymax > ymax) ymax = greenymax;
      if(blueymax  > ymax) ymax = blueymax;

      redyoff   = redymin   - ymin;
      greenyoff = greenymin - ymin;
      blueyoff  = blueymin  - ymin;

      greennaxis2 = greenymax - greenymin;
      rednaxis2   = redymax   - redymin;
      bluenaxis2  = blueymax  - blueymin;

      naxis2 = ymax - ymin;

      crpix2 = -ymin;

      if(mViewer_debug)
      {
         printf("\n");
         printf("DEBUG> COLOR: crval1    = %.6f\n", crval1);
         printf("DEBUG> COLOR: crval2    = %.6f\n", crval2);
         printf("DEBUG> COLOR: cdelt1    = %.6f\n", cdelt1);
         printf("DEBUG> COLOR: cdelt2    = %.6f\n", cdelt2);
         printf("DEBUG> COLOR: crota2    = %.6f\n", crota2);
         printf("DEBUG> COLOR: naxis1    = %d\n",  naxis1);
         printf("DEBUG> COLOR: naxis2    = %d\n",  naxis2);
         printf("DEBUG> COLOR: crpix1    = %-g\n", crpix1);
         printf("DEBUG> COLOR: crpix2    = %-g\n", crpix2);
         printf("DEBUG> COLOR: xmin      = %-g\n", xmin);
         printf("DEBUG> COLOR: xmax      = %-g\n", xmax);
         printf("DEBUG> COLOR: ymin      = %-g\n", ymin);
         printf("DEBUG> COLOR: ymax      = %-g\n", ymax);
         printf("DEBUG> COLOR: redxoff   = %-g\n", redxoff);
         printf("DEBUG> COLOR: greenxoff = %-g\n", greenxoff);
         printf("DEBUG> COLOR: bluexoff  = %-g\n", bluexoff);
         printf("DEBUG> COLOR: redyoff   = %-g\n", redyoff);
         printf("DEBUG> COLOR: greenyoff = %-g\n", greenyoff);
         printf("DEBUG> COLOR: blueyoff  = %-g\n", blueyoff);
         fflush(stdout);
      }

      if(!noflip)
      {
         if(xinc > 0)
           flipX = 1;

         if(yinc > 0)
           flipY = 1;

         if(fabs(crota2) >  90.
         && fabs(crota2) < 270.)
         {
            flipX = 1 - flipX;
            flipY = 1 - flipY;

            crota2 = crota2 + 180;

            while(crota2 > 360) crota2 -= 360.;
         }

         if(wcs->coorflip)
            flipX = 1 - flipX;

         if(flipX)
         {
           cdelt1 = -cdelt1;
           cd11   = -cd11;
           cd21   = -cd21;
         }

         if(flipY)
         {
           cdelt2 = -cdelt2;
           cd12   = -cd12;
           cd22   = -cd22;
         }
      }


      /* Now adjust the data range if the limits */
      /* were percentiles.  We had to wait until */
      /* we had naxis1, naxis2 values which is   */
      /* why this is here                        */

      if(mViewer_debug)
         printf("\n RED RANGE:\n");

      if(strlen(redhistfile) > 0)
      {
         if(mViewer_debug)
         {
            printf("DEBUG> redhistfile = [%s]\n", redhistfile);
            fflush(stdout);
         }

         if(mViewer_readHist( redhistfile, &redminval,  &redmaxval, reddataval, 
                             &rdatamin, &rdatamax, &median, &sigma, &redType))
         {
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }
      else
      {
         if(mViewer_getRange( redfptr,        redminstr,    redmaxstr,   
                             &redminval,     &redmaxval,    redType,   
                              redbetastr,    &redbetaval,   reddataval,
                              rednaxis1,      rednaxis2, 
                             &rdatamin,      &rdatamax,
                             &median,        &sigma,
                              redPlaneCount,  redPlanes))
         {
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }

      redminpercent = mViewer_valuePercentile(redminval);
      redmaxpercent = mViewer_valuePercentile(redmaxval);

      redminsigma = (redminval - median) / sigma;
      redmaxsigma = (redmaxval - median) / sigma;

      reddiff   = redmaxval   - redminval;

      if(mViewer_debug)
      {
         printf("DEBUG> redminval   = %-g (%-g%%/%-gs)\n", redminval, redminpercent, redminsigma);
         printf("DEBUG> redmaxval   = %-g (%-g%%/%-gs)\n", redmaxval, redmaxpercent, redmaxsigma);
         printf("DEBUG> reddiff     = %-g\n\n", reddiff);
         fflush(stdout);
      }

      if(mViewer_debug)
         printf("\n GREEN RANGE:\n");

      if(strlen(greenhistfile) > 0)
      {
         if(mViewer_debug)
         {
            printf("DEBUG> greenhistfile = [%s]\n", greenhistfile);
            fflush(stdout);
         }

         if(mViewer_readHist( greenhistfile, &greenminval,  &greenmaxval, greendataval, 
                             &gdatamin, &gdatamax, &median, &sigma, &greenType))
         {
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }
      else
      {
         if(mViewer_getRange( greenfptr,        greenminstr,  greenmaxstr, 
                             &greenminval,     &greenmaxval,  greenType, 
                              greenbetastr,    &greenbetaval, greendataval,
                              greennaxis1,      greennaxis2,
                             &gdatamin,        &gdatamax,
                             &median,          &sigma,
                              greenPlaneCount,  greenPlanes))
         {
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }

      greenminpercent = mViewer_valuePercentile(greenminval);
      greenmaxpercent = mViewer_valuePercentile(greenmaxval);

      greenminsigma = (greenminval - median) / sigma;
      greenmaxsigma = (greenmaxval - median) / sigma;

      greendiff = greenmaxval - greenminval;
      
      if(mViewer_debug)
      {
         printf("DEBUG> greenminval = %-g (%-g%%/%-gs)\n", greenminval, greenminpercent, greenminsigma);
         printf("DEBUG> greenmaxval = %-g (%-g%%/%-gs)\n", greenmaxval, greenmaxpercent, greenmaxsigma);
         printf("DEBUG> greendiff   = %-g\n\n", greendiff);
         fflush(stdout);
      }

      if(mViewer_debug)
         printf("\n BLUE RANGE:\n");

      if(strlen(bluehistfile) > 0)
      {
         if(mViewer_debug)
         {
            printf("DEBUG> bluehistfile = [%s]\n", bluehistfile);
            fflush(stdout);
         }

         if(mViewer_readHist(bluehistfile, &blueminval,  &bluemaxval, bluedataval, 
                             &bdatamin, &bdatamax, &median, &sigma, &blueType))
         {
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }
      else
      {
         if(mViewer_getRange( bluefptr,        blueminstr,   bluemaxstr,  
                             &blueminval,     &bluemaxval,   blueType,  
                              bluebetastr,    &bluebetaval,  bluedataval,
                              bluenaxis1,      bluenaxis2,  
                             &bdatamin,       &bdatamax,
                             &median,         &sigma,
                              bluePlaneCount,  bluePlanes))
         {
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }

      blueminpercent = mViewer_valuePercentile(blueminval);
      bluemaxpercent = mViewer_valuePercentile(bluemaxval);

      blueminsigma = (blueminval - median) / sigma;
      bluemaxsigma = (bluemaxval - median) / sigma;

      bluediff = bluemaxval - blueminval;
      
      if(mViewer_debug)
      {
         printf("DEBUG> blueminval = %-g (%-g%%/%-gs)\n", blueminval, blueminpercent, blueminsigma);
         printf("DEBUG> bluemaxval = %-g (%-g%%/%-gs)\n", bluemaxval, bluemaxpercent, bluemaxsigma);
         printf("DEBUG> bluediff   = %-g\n\n", bluediff);
         fflush(stdout);
      }


      /* Figure out which set of pixels to use */
      /* and what order to read them in        */

      istart = 0;
      iend   = naxis1 - 1;
      nx     = naxis1;

      if(naxis1 > MAXDIM)
      {
         istart = (naxis1 - MAXDIM)/2;
         iend   = istart + MAXDIM - 1;
         nx     = MAXDIM;
      }

      jstart = 0;
      jend   = naxis2 - 1;
      ny     = naxis2;
      jinc   = 1;

      if(naxis2 > MAXDIM)
      {
         jstart = (naxis2 - MAXDIM)/2;
         jend   = istart + MAXDIM - 2;
         ny     = MAXDIM;
      }

      if(flipY)
      {
         itemp  = jstart;
         jstart = jend;
         jend   = itemp;
         jinc   = -1;
      }

      if(mViewer_debug)
      {
         printf("\n");
         printf("DEBUG> nx               = %d\n", nx);
         printf("DEBUG> istart           = %d\n", istart);
         printf("DEBUG> iend             = %d\n", iend);
         printf("DEBUG> flipX            = %d\n", flipX);
         printf("\n");
         printf("DEBUG> ny               = %d\n", ny);
         printf("DEBUG> jstart           = %d\n", jstart);
         printf("DEBUG> jend             = %d\n", jend);
         printf("DEBUG> jinc             = %d\n", jinc);
         printf("DEBUG> flipY            = %d\n", flipY);
         fflush(stdout);
      }



      /* Set up the JPEG library info  */
      /* and start the JPEG compressor */

      if(outType == JPEG)
      {
         cinfo.err = jpeg_std_error(&jerr);

         jpeg_create_compress(&cinfo);

         jpeg_stdio_dest(&cinfo, jpegfp);

         cinfo.image_width      = nx;
         cinfo.image_height     = ny;
         cinfo.input_components = 3;
         cinfo.in_color_space   = JCS_RGB;

         quality = 85;

         jpeg_set_defaults(&cinfo);

         jpeg_set_quality (&cinfo, quality, TRUE);

         jpeg_start_compress(&cinfo, TRUE);
      }

      else if(outType == PNG)
      {
         membytes = 4 * nx * ny;

         pngData = (unsigned char *)malloc(membytes * sizeof(unsigned char));
         pngOvly = (unsigned char *)malloc(membytes * sizeof(unsigned char));

         for(ii=0; ii<membytes; ++ii)
         {
            pngData[ii] = 0;
            pngOvly[ii] = 0;
         }
      }

      if(mViewer_debug)
      {
         printf("Image (PNG/JPEG) space allocated: %u\n", membytes);
         fflush(stdout);
      }


      /* Now, loop over the parts of the images we want */

      fpixelRed[0] = 1;
      fpixelRed[2] = 1;
      fpixelRed[3] = 1;

      if(redPlaneCount > 1)
         fpixelRed[2] = redPlanes[1];
      if(redPlaneCount > 2)
         fpixelRed[3] = redPlanes[2];

      fpixelGreen[0] = 1;
      fpixelGreen[2] = 1;
      fpixelGreen[3] = 1;

      if(greenPlaneCount > 1)
         fpixelGreen[2] = greenPlanes[1];
      if(greenPlaneCount > 2)
         fpixelGreen[3] = greenPlanes[2];

      fpixelBlue[0] = 1;
      fpixelBlue[2] = 1;
      fpixelBlue[3] = 1;

      if(bluePlaneCount > 1)
         fpixelBlue[2] = bluePlanes[1];
      if(bluePlaneCount > 2)
         fpixelBlue[3] = bluePlanes[2];

      nelements = naxis1;

      rfitsbuf = (double *)malloc(nelements * sizeof(double));
      gfitsbuf = (double *)malloc(nelements * sizeof(double));
      bfitsbuf = (double *)malloc(nelements * sizeof(double));

      if(outType == JPEG)
      {
         jpegData = (JSAMPROW *)malloc(ny * sizeof (JSAMPROW));
         jpegOvly = (JSAMPROW *)malloc(ny * sizeof (JSAMPROW));
      }

      ovlyweight = (double  **)malloc(ny * sizeof (double *));
      ovlylock   = (int     **)malloc(ny * sizeof (int    *));

      for(jj=0; jj<ny; ++jj)
      {
         if(outType == JPEG)
         {
            jpegData[jj] = (JSAMPROW)malloc(3*nelements);
            jpegOvly[jj] = (JSAMPROW)malloc(3*nelements);
         }

         ovlyweight[jj] = (double *)malloc(nelements * sizeof(double));
         ovlylock  [jj] = (int    *)malloc(nelements * sizeof(int));
      }

      for(jj=0; jj<ny; ++jj)
      {
         for(i=0; i<nelements; ++i)
         {
            rfitsbuf[i] = mynan;
            gfitsbuf[i] = mynan;
            bfitsbuf[i] = mynan;

            if(outType == JPEG)
            {
               jpegData[jj][3*i  ] = 0;
               jpegData[jj][3*i+1] = 0;
               jpegData[jj][3*i+2] = 0;

               jpegOvly[jj][3*i  ] = 0;
               jpegOvly[jj][3*i+1] = 0;
               jpegOvly[jj][3*i+2] = 0;
            }

            ovlyweight[jj][i] = 0.;
            ovlylock  [jj][i] = 0;
         }

         j = jstart + jj*jinc;


         /* Read red */

         if(j - redyoff >= 0 && j - redyoff < rednaxis2)
         {
            fpixelRed[1] = j - redyoff + 1;

            if(fits_read_pix(redfptr, TDOUBLE, fpixelRed, rednaxis1, &mynan,
                             rfitsbuf, &nullcnt, &status))
               mViewer_printFitsError(status);
         }


         /* Read green */

         if(j - greenyoff >= 0 && j - greenyoff < greennaxis2)
         {
            fpixelGreen[1] = j - greenyoff + 1;

            if(fits_read_pix(greenfptr, TDOUBLE, fpixelGreen, greennaxis1, &mynan,
                             gfitsbuf, &nullcnt, &status))
               mViewer_printFitsError(status);
         }


         /* Read blue */

         if(j - blueyoff >= 0 && j - blueyoff < bluenaxis2)
         {
            fpixelBlue[1] = j - blueyoff + 1;

            if(fits_read_pix(bluefptr, TDOUBLE, fpixelBlue, bluenaxis1, &mynan,
                             bfitsbuf, &nullcnt, &status))
               mViewer_printFitsError(status);
         }


         /* Look up pixel color values */

         for(i=0; i<nx; ++i)
         {
            /* RED */

            ipix = i + istart - redxoff;

            if(ipix < 0 || ipix >= rednaxis1)
               redval = mynan;
            else
               redval = rfitsbuf[ipix];


            /* Special case: blank pixel */

            if(mNaN(redval))
               redImVal = saturationValue;


            /* Gaussian histogram equalization */

            else if(redType == GAUSSIAN
                 || redType == GAUSSIANLOG)
            {
               for(index=0; index<256; ++index)
               {
                  if(reddataval[index] >= redval)
                     break;
               }

               if(index <   0) index =   0;
               if(index > 255) index = 255;

               redImVal = index;
            }


            /* ASIHN stretch */

            else if(redType == ASINH)
            {
               redImVal  = (redval - redminval)/(redmaxval - redminval);

               if(redImVal < 0.0)
                  redImVal = 0.;
               else
                  redImVal = 255. * asinh(redbetaval * redImVal)/redbetaval;

               if(redImVal < 0.)
                  redImVal = 0.;

               if(redImVal > 255.)
                  redImVal = 255.;

               index = (int)(redImVal+0.5);
            }


            /* Finally, log power (including linear) */

            else
            {
               if(redval < redminval)
                  redval = redminval;

               if(redval > redmaxval)
                  redval = redmaxval;

               redImVal = (redval - redminval)/reddiff;

               for(k=0; k<redlogpower; ++k)
                  redImVal = log(9.*redImVal+1.);
               
               redImVal = 255. * redImVal;
            }

            if(redImVal < 0.)
               redImVal = 0.;



            /* GREEN */

            ipix = i + istart - greenxoff;

            if(ipix < 0 || ipix >= greennaxis1)
               greenval = mynan;
            else
               greenval = gfitsbuf[ipix];


            /* Special case: blank pixel */

            if(mNaN(greenval))
               greenImVal = saturationValue;


            /* Special case: blank pixel */

            else if(greenOff)
               greenImVal = 0.;


            /* Gaussian histogram equalization */

            else if(greenType == GAUSSIAN
                 || greenType == GAUSSIANLOG)
            {
               for(index=0; index<256; ++index)
               {
                  if(greendataval[index] >= greenval)
                     break;
               }

               if(index <   0) index =   0;
               if(index > 255) index = 255;

               greenImVal = index;
            }

            /* ASIHN stretch */

            else if(greenType == ASINH)
            {
               greenImVal  = (greenval - greenminval)/(greenmaxval - greenminval);

               if(greenImVal < 0.0)
                  greenImVal = 0.;
               else
                  greenImVal = 255. * asinh(greenbetaval * greenImVal)/greenbetaval;

               if(greenImVal < 0.)
                  greenImVal = 0.;

               if(greenImVal > 255.)
                  greenImVal = 255.;

               index = (int)(greenImVal+0.5);
            }


            /* Finally, log power (including linear) */

            else
            {
               if(greenval < greenminval)
                  greenval = greenminval;

               if(greenval > greenmaxval)
                  greenval = greenmaxval;

               greenImVal = (greenval - greenminval)/greendiff;

               for(k=0; k<greenlogpower; ++k)
                  greenImVal = log(9.*greenImVal+1.);
               
               greenImVal = 255. * greenImVal;
            }

            if(greenImVal < 0.)
               greenImVal = 0.;



            /* BLUE */

            ipix = i + istart - bluexoff;

            if(ipix < 0 || ipix >= bluenaxis1)
               blueval = mynan;
            else
               blueval = bfitsbuf[ipix];


            /* Special case: blank pixel */

            if(mNaN(blueval))
               blueImVal = saturationValue;


            /* Gaussian histogram equalization */

            else if(blueType == GAUSSIAN
                 || blueType == GAUSSIANLOG)
            {
               for(index=0; index<256; ++index)
               {
                  if(bluedataval[index] >= blueval)
                     break;
               }

               if(index <   0) index =   0;
               if(index > 255) index = 255;

               blueImVal = index;
            }


            /* ASIHN stretch */

            else if(blueType == ASINH)
            {
               blueImVal  = (blueval - blueminval)/(bluemaxval - blueminval);

               if(blueImVal < 0.0)
                  blueImVal = 0.;
               else
                  blueImVal = 255. * asinh(bluebetaval * blueImVal)/bluebetaval;

               if(blueImVal < 0.)
                  blueImVal = 0.;

               if(blueImVal > 255.)
                  blueImVal = 255.;

               index = (int)(blueImVal+0.5);
            }


            /* Finally, log power (including linear) */

            else
            {
               if(blueval < blueminval)
                  blueval = blueminval;

               if(blueval > bluemaxval)
                  blueval = bluemaxval;

               blueImVal = (blueval - blueminval)/bluediff;

               for(k=0; k<bluelogpower; ++k)
                  blueImVal = log(9.*blueImVal+1.);
               
               blueImVal = 255. * blueImVal;
            }

            if(blueImVal < 0.)
               blueImVal = 0.;


            /* If we wish to preserve "color" */

            if(truecolor > 0.)
            {
               if(blueImVal  >= 255.
               || greenImVal >= 255.
               || redImVal   >= 255.)
               {
                  maxImVal = redImVal;

                  if(greenImVal > maxImVal) maxImVal = greenImVal;
                  if(blueImVal  > maxImVal) maxImVal = blueImVal;

                  redImVal   = pow(redImVal   / maxImVal, truecolor) * 255.;
                  greenImVal = pow(greenImVal / maxImVal, truecolor) * 255.;
                  blueImVal  = pow(blueImVal  / maxImVal, truecolor) * 255.;
               }
               else
               {
                  maxImVal = redImVal;

                  if(greenImVal > maxImVal) maxImVal = greenImVal;
                  if(blueImVal  > maxImVal) maxImVal = blueImVal;

                  if(maxImVal > 0.)
                  {
                     redImVal   = pow(redImVal   / maxImVal, truecolor) * maxImVal;
                     greenImVal = pow(greenImVal / maxImVal, truecolor) * maxImVal;
                     blueImVal  = pow(blueImVal  / maxImVal, truecolor) * maxImVal;
                  }
               }
            }
            else
            {
               if(blueImVal  > 255.) blueImVal  = 255.;
               if(greenImVal > 255.) greenImVal = 255.;
               if(redImVal   > 255.) redImVal   = 255.;
            }

            
            /* Populate the output JPEG array */

            if(outType == JPEG)
            {
               if(flipX)
               {
                  jpegData[jj][3*(nx-1-i)  ] = (int)redImVal;
                  jpegData[jj][3*(nx-1-i)+1] = (int)greenImVal;
                  jpegData[jj][3*(nx-1-i)+2] = (int)blueImVal;
               }
               else
               {
                  jpegData[jj][3*i  ] = (int)redImVal;
                  jpegData[jj][3*i+1] = (int)greenImVal;
                  jpegData[jj][3*i+2] = (int)blueImVal;
               }
            }

            else if (outType == PNG)
            {
               if(flipX)
               {
                  ii = 4 * nx * jj + 4 * (nx-1-i);

                  pngData[ii + 0] = (int)redImVal;
                  pngData[ii + 1] = (int)greenImVal;
                  pngData[ii + 2] = (int)blueImVal;
                  pngData[ii + 3] = (int)(255*imgalpha);

                  if(tzero && redImVal == 0. && greenImVal == 0. && blueImVal == 0.)
                     pngData[ii + 3] = 0;
               }
               else
               {
                  ii = 4 * nx * jj + 4 * i;

                  pngData[ii + 0] = (int)redImVal;
                  pngData[ii + 1] = (int)greenImVal;
                  pngData[ii + 2] = (int)blueImVal;
                  pngData[ii + 3] = (int)(255*imgalpha);

                  if(tzero && redImVal == 0. && greenImVal == 0. && blueImVal == 0.)
                     pngData[ii + 3] = 0;
               }
            }
         }
      }
   }


   /* Grayscale/pseudocolor mode */

   else
   {
      if(mViewer_debug)
      {
         printf("DEBUG> Processing Gray mode\n");
         fflush(stdout);
      }

      /* First make sure that the WCS info is OK */

      if(strlen(grayfile) == 0)
      {
         mViewer_memCleanup();
         strcpy(returnStruct->msg, "Grayscale/pseudocolor mode but no gray image given");
         return returnStruct;
      }

      if(fits_get_hdrpos(grayfptr, &keysexist, &keynum, &status))
         mViewer_printFitsError(status);

      status = 0;
      fits_read_key(grayfptr, TSTRING, "BUNIT", bunit, (char *)NULL, &status);

      if(status == KEY_NO_EXIST)
         strcpy(bunit, "");

      status = 0;
      if(fits_get_image_wcs_keys(grayfptr, &header, &status))
         mViewer_printFitsError(status);

      wcs = wcsinit(header);

      free(header);

      if(wcs == (struct WorldCoor *)NULL)
      {
         if(nowcs)
         {
            status = 0;
            if(fits_read_key(grayfptr, TLONG, "NAXIS1", &naxisl, (char *)NULL, &status))
               mViewer_printFitsError(status);

            naxis1 = naxisl;

            status = 0;
            if(fits_read_key(grayfptr, TLONG, "NAXIS2", &naxisl, (char *)NULL, &status))
               mViewer_printFitsError(status);

            naxis2 = naxisl;

            wcs = mViewer_wcsfake(naxis1, naxis2);
         }
         else
         {
            mViewer_memCleanup();
            sprintf(returnStruct->msg, "WCS init failed for [%s].", grayfile);
            return returnStruct;
         }
      }

      if(strcmp(wcs->ptype, "HPX") == 0)
      {
         hpx = 1;

         hpxPix = 90.0 / fabs(wcs->xinc) / sqrt(2.0) + 0.5;

         hpxLevel = log10((double)hpxPix)/log10(2.) + 0.5;

         hpxPix = pow(2., (double)hpxLevel) + 0.5;

         hpxPix = 4 * hpxPix;
      }


      if(!nowcs)
         montage_checkWCS(wcs);

      naxis1 = wcs->nxpix;
      naxis2 = wcs->nypix;

      crval1 = wcs->xpos;
      crval2 = wcs->ypos;

      crpix1 = wcs->xrefpix;
      crpix2 = wcs->yrefpix;

      xinc   = wcs->xinc;
      yinc   = wcs->yinc;
      cdelt1 = wcs->cdelt[0];
      cdelt2 = wcs->cdelt[1];
      cd11   = wcs->cd[0];
      cd12   = wcs->cd[1];
      cd21   = wcs->cd[2];
      cd22   = wcs->cd[3];
      crota2 = wcs->rot;

      if(wcs->syswcs == WCS_J2000)
      {
         csysimg  = EQUJ;
         epochimg = 2000.;

         if(wcs->equinox == 1950.)
            epochimg = 1950;
      }
      else if(wcs->syswcs == WCS_B1950)
      {
         csysimg  = EQUB;
         epochimg = 1950.;

         if(wcs->equinox == 2000.)
            epochimg = 2000.;
      }
      else if(wcs->syswcs == WCS_GALACTIC)
      {
         csysimg  = GAL;
         epochimg = 2000.;
      }
      else if(wcs->syswcs == WCS_ECLIPTIC)
      {
         csysimg  = ECLJ;
         epochimg = 2000.;

         if(wcs->equinox == 1950.)
         {
            csysimg  = ECLB;
            epochimg = 1950.;
         }
      }
      else
      {
         csysimg  = EQUJ;
         epochimg = 2000.;
      }

      if(mViewer_debug)
      {
         printf("\nGray image:\n");
         printf("naxis1   = %d\n", (int)wcs->nxpix);
         printf("naxis2   = %d\n", (int)wcs->nypix);
         printf("crval1   = %.6f\n", wcs->xpos);
         printf("crval2   = %.6f\n", wcs->ypos);
         printf("crpix1   = %.6f\n", wcs->xrefpix);
         printf("crpix2   = %.6f\n", wcs->yrefpix);
         printf("xinc     = %.6f\n", wcs->xinc);
         printf("yinc     = %.6f\n", wcs->yinc);
         printf("crota2   = %.6f\n", wcs->rot);
         printf("csysimg  = %d\n",   csysimg);
         printf("epochimg = %-g\n",  epochimg);
         printf("\n");

         fflush(stdout);
      }


      /* Kludge to get around bug in WCS library:   */
      /* 360 degrees sometimes added to pixel coord */

      ix = 0.5;
      iy = 0.5;

      pix2wcs(wcs, ix, iy, &xpos, &ypos);

      offscl = wcs->offscl;

      x = 0.;
      y = 0.;
      
      if(!offscl)
         wcs2pix(wcs, xpos, ypos, &x, &y, &offscl);

      if(hpx) mViewer_hpxCheck(&offscl, &x, &y);

      xcorrection = x-ix;
      ycorrection = y-iy;

      if(offscl || mNaN(x) || mNaN(y))
      {
         xcorrection = 0.;
         ycorrection = 0.;
      }


      if(mViewer_debug)
      {
         printf("\n");
         printf("DEBUG> GRAY: naxis1 = %d\n" , naxis1);
         printf("DEBUG> GRAY: naxis2 = %d\n",  naxis2);
         printf("DEBUG> GRAY: xinc   = %-g\n", xinc);
         printf("DEBUG> GRAY: yinc   = %-g\n", yinc);
         printf("DEBUG> GRAY: crota2 = %-g\n", crota2);
         fflush(stdout);
      }

      if(!noflip)
      {
         if(xinc > 0)
           flipX = 1;

         if(yinc > 0)
           flipY = 1;

         if(fabs(crota2) >  90.
         && fabs(crota2) < 270.)
         {
            flipX = 1 - flipX;
            flipY = 1 - flipY;

            crota2 = crota2 + 180;

            while(crota2 > 360) crota2 -= 360.;
         }

         if(wcs->coorflip)
            flipX = 1 - flipX;

         if(flipX)
         {
           cdelt1 = -cdelt1;
           cd11   = -cd11;
           cd21   = -cd21;
         }

         if(flipY)
         {
           cdelt2 = -cdelt2;
           cd12   = -cd12;
           cd22   = -cd22;
         }
      }


      /* Now adjust the data range if the limits */
      /* were percentiles.  We had to wait until */
      /* we had naxis1, naxis2 which is why this */
      /* is here                                 */

      if(mViewer_debug)
         printf("\n GRAY RANGE:\n");


      if(strlen(grayhistfile) > 0)
      {
         if(mViewer_readHist(grayhistfile, &grayminval,  &graymaxval, graydataval, 
                             &graydatamin, &graydatamax, &median, &sigma, &grayType))
         {
            mViewer_memCleanup();
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }
      else
      {
         if(mViewer_getRange( grayfptr,        grayminstr,  graymaxstr, 
                             &grayminval,     &graymaxval,  grayType, 
                              graybetastr,    &graybetaval, graydataval,
                              naxis1,          naxis2,
                             &graydatamin,    &graydatamax,
                             &median,         &sigma,
                              grayPlaneCount,  grayPlanes))
         {
            mViewer_memCleanup();
            strcpy(returnStruct->msg, montage_msgstr);
            return returnStruct;
         }
      }

      grayminpercent = mViewer_valuePercentile(grayminval);
      graymaxpercent = mViewer_valuePercentile(graymaxval);

      grayminsigma = (grayminval - median) / sigma;
      graymaxsigma = (graymaxval - median) / sigma;

      graydiff = graymaxval - grayminval;

      if(mViewer_debug)
      {
         printf("DEBUG> grayminval = %-g (%-g%%/%-gs)\n", grayminval, grayminpercent, grayminsigma);
         printf("DEBUG> graymaxval = %-g (%-g%%/%-gs)\n", graymaxval, graymaxpercent, graymaxsigma);
         printf("DEBUG> graydiff   = %-g\n", graydiff);
         fflush(stdout);
      }


      /* Figure out which set of pixels to use */
      /* and what order to read them in        */

      istart = 0;
      iend   = naxis1 - 1;
      nx     = naxis1;

      if(outType == JPEG && naxis1 > MAXDIM)
      {
         istart = (naxis1 - MAXDIM)/2;
         iend   = istart + MAXDIM - 1;
         nx     = MAXDIM;
      }

      jstart = 0;
      jend   = naxis2 - 1;
      ny     = naxis2;
      jinc   = 1;

      if(outType == JPEG && naxis2 > MAXDIM)
      {
         jstart = (naxis2 - MAXDIM)/2;
         jend   = istart + MAXDIM - 2;
         ny     = MAXDIM;
      }

      if(flipY)
      {
         itemp  = jstart;
         jstart = jend;
         jend   = itemp;
         jinc   = -1;
      }

      if(mViewer_debug)
      {
         printf("\n");
         printf("DEBUG> nx               = %d\n", nx);
         printf("DEBUG> istart           = %d\n", istart);
         printf("DEBUG> iend             = %d\n", iend);
         printf("DEBUG> flipX            = %d\n", flipX);
         printf("\n");
         printf("DEBUG> ny               = %d\n", ny);
         printf("DEBUG> jstart           = %d\n", jstart);
         printf("DEBUG> jend             = %d\n", jend);
         printf("DEBUG> jinc             = %d\n", jinc);
         printf("DEBUG> flipY            = %d\n", flipY);
         fflush(stdout);
      }


      /* Set up the JPEG library info  */
      /* and start the JPEG compressor */

      if(outType == JPEG)
      {
         cinfo.err = jpeg_std_error(&jerr);

         jpeg_create_compress(&cinfo);

         jpeg_stdio_dest(&cinfo, jpegfp);

         cinfo.image_width      = nx;
         cinfo.image_height     = ny;
         cinfo.input_components = 3;
         cinfo.in_color_space = JCS_RGB;

         jpeg_set_defaults(&cinfo);

         quality = 85;

         jpeg_set_quality (&cinfo, quality, TRUE);

         jpeg_start_compress(&cinfo, TRUE);
      }


      /* Now, loop over the part of the image we want */

      fpixelGray[0] = istart+1;
      fpixelGray[2] = 1;
      fpixelGray[3] = 1;

      if(grayPlaneCount > 1)
         fpixelGray[2] = grayPlanes[1];
      if(grayPlaneCount > 2)
         fpixelGray[3] = grayPlanes[2];

      nelements = nx;

      fitsbuf = (double *)malloc(nelements * sizeof(double));

      if(outType == JPEG)
      {
         jpegData = (JSAMPROW *)malloc(ny * sizeof (JSAMPROW));
         jpegOvly = (JSAMPROW *)malloc(ny * sizeof (JSAMPROW));
      }

      else if(outType == PNG)
      {
         membytes = 4 * nx * ny;

         pngData = (unsigned char *)malloc(membytes * sizeof(unsigned char));
         pngOvly = (unsigned char *)malloc(membytes * sizeof(unsigned char));

         for(ii=0; ii<membytes; ++ii)
         {
            pngData[ii] = 0;
            pngOvly[ii] = 0;
         }
      }
      
      if(mViewer_debug)
      {
         printf("Image (PNG/JPEG) space allocated\n");
         fflush(stdout);
      }

      ovlyweight = (double  **)malloc(ny * sizeof (double *));
      ovlylock   = (int     **)malloc(ny * sizeof (int    *));

      for(jj=0; jj<ny; ++jj)
      {
         if(outType == JPEG)
         {
            jpegData[jj] = (JSAMPROW)malloc(3*nelements);
            jpegOvly[jj] = (JSAMPROW)malloc(3*nelements);
         }

         ovlyweight[jj] = (double *)malloc(nelements * sizeof(double));
         ovlylock  [jj] = (int    *)malloc(nelements * sizeof(int));
      }

      for(jj=0; jj<ny; ++jj)
      {
         for(i=0; i<nelements; ++i)
         {
            fitsbuf[i] = mynan;

            if(outType == JPEG)
            {
               jpegData[jj][3*i  ] = 0;
               jpegData[jj][3*i+1] = 0;
               jpegData[jj][3*i+2] = 0;

               jpegOvly[jj][3*i  ] = 0;
               jpegOvly[jj][3*i+1] = 0;
               jpegOvly[jj][3*i+2] = 0;
            }

            ovlyweight[jj][i] = 0.;
            ovlylock  [jj][i] = 0;
         }

         j = jstart + jj*jinc;

         fpixelGray[1] = j+1;

         if(fits_read_pix(grayfptr, TDOUBLE, fpixelGray, nelements, &mynan,
                          fitsbuf, &nullcnt, &status))
            mViewer_printFitsError(status);

         for(i=0; i<nx; ++i)
         {
            /* Special case: blank pixel */

            grayval = fitsbuf[i-istart];

            if(mNaN(grayval))
               index = saturationValue;


            /* Gaussian histogram equalization */

            else if(grayType == GAUSSIAN
                 || grayType == GAUSSIANLOG)
            {
               for(index=0; index<256; ++index)
               {
                  if(graydataval[index] >= grayval)
                     break;
               }

               if(index <   0) index =   0;
               if(index > 255) index = 255;
            }


            /* ASIHN stretch */

            else if(grayType == ASINH)
            {
               grayImVal  = (grayval - grayminval)/(graymaxval - grayminval);

               if(grayImVal < 0.0)
                  grayImVal = 0.;
               else
                  grayImVal = 255. * asinh(graybetaval * grayImVal)/graybetaval;

               if(grayImVal < 0.)
                  grayImVal = 0.;

               if(grayImVal > 255.)
                  grayImVal = 255.;

               index = (int)(grayImVal+0.5);
            }


            /* Finally, log power (including linear) */

            else
            {
               if(grayval < grayminval)
                  grayval = grayminval;

               if(grayval > graymaxval)
                  grayval = graymaxval;

               grayImVal = (grayval - grayminval)/graydiff;

               for(k=0; k<graylogpower; ++k)
                  grayImVal = log(9.*grayImVal+1.);

               grayImVal = 255. * grayImVal;

               if(grayImVal <   0.)
                  grayImVal =   0.;

               if(grayImVal > 255.)
                  grayImVal = 255.;

               index = (int)(grayImVal+0.5);
            }

            if(outType == JPEG)
            {
               if(flipX)
               {
                  jpegData[jj][3*(nx-1-i)  ] = color_table[index][0];
                  jpegData[jj][3*(nx-1-i)+1] = color_table[index][1];
                  jpegData[jj][3*(nx-1-i)+2] = color_table[index][2];
               }
               else
               {
                  jpegData[jj][3*i  ] = color_table[index][0];
                  jpegData[jj][3*i+1] = color_table[index][1];
                  jpegData[jj][3*i+2] = color_table[index][2];
               }
            }

            else if(outType == PNG)
            {
               if(flipX)
               {
                  ii = 4 * nx * jj + 4 * (nx-1-i);

                  pngData[ii + 0] = color_table[index][0];
                  pngData[ii + 1] = color_table[index][1];
                  pngData[ii + 2] = color_table[index][2];
                  pngData[ii + 3] = (int)(255 * imgalpha);

                  if(tzero && grayImVal == 0.)
                     pngData[ii + 3] = 0;
               }
               else
               {
                  ii = 4 * nx * jj + 4 * i;

                  pngData[ii + 0] = color_table[index][0];
                  pngData[ii + 1] = color_table[index][1];
                  pngData[ii + 2] = color_table[index][2];
                  pngData[ii + 3] = (int)(255 * imgalpha);

                  if(tzero && grayImVal == 0.)
                     pngData[ii + 3] = 0;
               }
            }
         }
      }
   }


   /* Add graphics */

   charHeight = 0.0025 * ny * fabs(cdelt2);
   pixScale   = fabs(cdelt2);

   if(nx * fabs(cdelt1) > ny * fabs(cdelt2))
   {
      charHeight = 0.0025 * nx * fabs(cdelt1);
      pixScale   = fabs(cdelt1);
   }


   // GRIDS
   
   for(i=0; i<ngrid; ++i)
   {
      if(mViewer_debug)
      {
         printf("Grid %d of %d\n", i+1, ngrid);
         fflush(stdout);
      }

      mViewer_makeGrid(wcs, csysimg, epochimg, 
                       grid[i].csys, grid[i].epoch, grid[i].red, grid[i].green, grid[i].blue,  
                       fontfile, grid[i].fontscale, grid[i].linewidth);

      mViewer_addOverlay(grid[i].alpha);
   }


   // CATALOGS / IMAGE INFO
   
   for(i=0; i<ncat; ++i)
   {
      if(mViewer_debug)
      {
         printf("Catalog %d of %d\n", i+1, ncat);
         fflush(stdout);
      }

      if(cat[i].isImgInfo == 0) /* CATALOG */
      {
         if(mViewer_debug)
         {
            printf("Source catalog %s\n", cat[i].file);
            fflush(stdout);
         }

         ncol = topen(cat[i].file);

         if(ncol <= 0)
         {
            mViewer_memCleanup();
            sprintf(returnStruct->msg, "Invalid table file [%s].", cat[i].file);
            return returnStruct;
         }

         ira  = tcol("ra");
         idec = tcol("dec");

         if(ira  < 0) ira  = tcol("lon");
         if(idec < 0) idec = tcol("lat");

         if(ira < 0 || idec < 0)
         {
            tclose();
            mViewer_memCleanup();
            sprintf(returnStruct->msg, "Cannot find 'ra' and 'dec (or 'lon','lat') in table [%s]", cat[i].file);
            return returnStruct;
         }


         // Scaling 

         iscale = -1;

         if(strlen(cat[i].scaleColumn) > 0)
         {
            iscale = tcol(cat[i].scaleColumn);

            if(iscale < 0)
            {
               tclose();
               mViewer_memCleanup();
               sprintf(returnStruct->msg, "Cannot find flux/mag column [%s] in table [%s]", cat[i].scaleColumn, cat[i].file);
               return returnStruct;
            }
         }

         
         // Color

         icolor = -1;

         if(strlen(cat[i].colorColumn) > 0)
         {
            icolor = tcol(cat[i].colorColumn);

            if(icolor < 0)
            {
               tclose();
               mViewer_memCleanup();
               sprintf(returnStruct->msg, "Cannot find color column [%s] in table [%s]", cat[i].colorColumn, cat[i].file);
               return returnStruct;
            }
         }


         // Symbol size

         isymsize = -1;

         if(strlen(cat[i].symSizeColumn) > 0)
         {
            isymsize = tcol(cat[i].symSizeColumn);

            if(isymsize < 0)
            {
               tclose();
               mViewer_memCleanup();
               sprintf(returnStruct->msg, "Cannot find symbol size column [%s] in table [%s]", cat[i].symSizeColumn, cat[i].file);
               return returnStruct;
            }
         }


         // Symbol shape

         isymshape = -1;

         if(strlen(cat[i].symShapeColumn) > 0)
         {
            isymshape = tcol(cat[i].symShapeColumn);

            if(isymshape < 0)
            {
               tclose();
               mViewer_memCleanup();
               sprintf(returnStruct->msg, "Cannot find symbol shape column [%s] in table [%s]", cat[i].symShapeColumn, cat[i].file);
               return returnStruct;
            }
         }


         // Label

         ilabel = -1;

         if(strlen(cat[i].labelColumn) > 0)
         {
            ilabel = tcol(cat[i].labelColumn);

            if(ilabel < 0)
            {
               tclose();
               mViewer_memCleanup();
               sprintf(returnStruct->msg, "Cannot find label column [%s] in table [%s]", cat[i].labelColumn, cat[i].file);
               return returnStruct;
            }
         }


         // Read through the file

         while(1)
         {
            stat = tread();

            if(stat < 0)
               break;

            if(tnull(ira) || tnull(idec))
               continue;


            // Read location 

            ra  = atof(tval(ira));
            dec = atof(tval(idec));

            if(iscale < 0)
               flux = cat[i].symSize;
            else
            {
               flux = atof(tval(iscale));

               if(tnull(iscale))
                  continue;
            }


            // Read color

            ovlyred   = cat[i].red;
            ovlygreen = cat[i].green;
            ovlyblue  = cat[i].blue;

            if(icolor >= 0)
            {
               if(!tnull(icolor))
               {
                  strcpy(colorstr, tval(icolor));

                  if(mViewer_colorLookup(colorstr, &ovlyred, &ovlygreen, &ovlyblue))
                  {
                     mViewer_memCleanup();
                     strcpy(returnStruct->msg, montage_msgstr);
                     return returnStruct;
                  }
               }
            }


            // Read symbol size

            symSize  = cat[i].symSize;
            symUnits = cat[i].symUnits;

            if(isymsize >= 0)
            {
               if(!tnull(isymsize))
               {
                  strcpy(symbolstr, tval(isymsize));

                  ptr = symbolstr + strlen(symbolstr) - 1;

                  symUnits = FRACTIONAL;

                  if(*ptr == 's')
                     symUnits = SECONDS;

                  else if(*ptr == 'm')
                     symUnits = MINUTES;

                  else if(*ptr == 'd')
                     symUnits = DEGREES;

                  else if(*ptr == 'p')
                     symUnits = PIXELS;

                  if(symUnits != FRACTIONAL)
                     *ptr = '\0';

                  symSize = strtod(symbolstr, &end);


                  // If this fails, revert to default values

                  if(end < (symbolstr + (int)strlen(symbolstr)) || symSize <= 0.)
                  {
                     symSize  = cat[i].symSize;
                     symUnits = cat[i].symUnits;
                  }
               }
            }


            // Read symbol shape

            symNPnt     = cat[i].symNPnt;
            symType     = cat[i].symType;
            symRotAngle = cat[i].symRotAngle;

            if(isymshape >= 0)
            {
               if(!tnull(isymshape))
               {
                  strcpy(symbolstr, tval(isymshape));

                  istatus = mViewer_parseSymbol(symbolstr, &symNPnt, &symNMax, &symType, &symRotAngle);

                  if(istatus)
                  {
                     symNPnt     = cat[i].symNPnt;
                     symNMax     = cat[i].symNMax;
                     symType     = cat[i].symType;
                     symRotAngle = cat[i].symRotAngle;
                  }
               }
            }



            if(mViewer_debug)
               printf("Symbol: color=(%4.2f,%4.2f,%4.2f) shape=(%2d,%d,%6.2f) at (%6.2f,%6.2f) flux=%10.6f->", 
                  ovlyred, ovlygreen, ovlyblue, symNPnt, symType, symRotAngle, ra, dec, flux);


            // Process scaling information

            if(iscale >= 0)
            {
               if(cat[i].scaleType == FLUX)
                  flux = flux / cat[i].scaleVal * cat[i].symSize;
               else if(cat[i].scaleType == MAG)
                  flux = (cat[i].scaleVal - flux + 1) * cat[i].symSize;
               else if(cat[i].scaleType == LOGFLUX)
                  flux = log10(10. * flux / cat[i].scaleVal) * cat[i].symSize;
            }


            if(cat[i].symUnits == FRACTIONAL)
                flux = flux * charHeight;

             else if(cat[i].symUnits == SECONDS)
                flux = flux / 3600.;
        
             else if(cat[i].symUnits == MINUTES)
                flux = flux / 60.;
 
             else if(cat[i].symUnits == DEGREES)
                flux = flux * 1.;

            else if(cat[i].symUnits == PIXELS)
               flux = flux * pixScale;


            if(flux < 0.1*charHeight)
               flux = 0.1*charHeight;

            if(mViewer_debug)
            {
               printf("%10.6f\n", flux);
               fflush(stdout);
            }


            // Draw symbol

            if(symSize > 0)
            {
               mViewer_symbol(wcs, flipY, csysimg, epochimg, cat[i].csys, cat[i].epoch,
                              ra, dec, 0, flux, symNPnt, symNMax, symType, symRotAngle, 
                              ovlyred, ovlygreen, ovlyblue, cat[i].linewidth);

               if(mViewer_debug)
               {
                  printf("Symbol drawn.\n");
                  fflush(stdout);
               }
            }
            else
            {
               if(mViewer_debug)
               {
                  printf("Symbol not drawn.\n");
                  fflush(stdout);
               }
            }


            // Write label

            if(ilabel >= 0)
            {
               strcpy(labelstr, tval(ilabel));

               if(strlen(labelstr) > 0)
               {
                  convertCoordinates(EQUJ, 2000.,  ra, dec,
                                      csysimg,  epochimg, &xpos, &ypos, 0.0);

                  wcs2pix(wcs, xpos, ypos, &xpix, &ypix, &offscl);

                  if(hpx) mViewer_hpxCheck(&offscl, &xpix, &ypix);

                  fontSize = (int)(14. * cat[i].fontscale);

                  if(fontSize < 1)
                     fontSize = 1;

                  mViewer_draw_label(fontfile, fontSize, xpix, ypix, labelstr, ovlyred, ovlygreen, ovlyblue);
               }

               if(mViewer_debug)
               {
                  printf("Label [%s] at (%-g,%-g)\n", labelstr, xpix, ypix);
                  fflush(stdout);
               }
            }
         }

         tclose();
      }

      else /* IMAGE INFO */
      {
         if(mViewer_debug)
         {
            printf("Image info %s\n", cat[i].file);
            fflush(stdout);
         }

         ncol = topen(cat[i].file);

         if(ncol <= 0)
         {
            tclose();
            mViewer_memCleanup();
            sprintf(returnStruct->msg, "Invalid table file [%s].\" ]\n", cat[i].file);
            return returnStruct;
         }
         
         // Color

         icolor = -1;

         len = strlen(cat[i].colorColumn);

         if(len > 0)
         {
            icolor = tcol(cat[i].colorColumn);

            if(icolor < 0)
            {
               tclose();
               mViewer_memCleanup();
               sprintf(returnStruct->msg, "Cannot find color column [%s] in table [%s]", cat[i].colorColumn, cat[i].file);
               return returnStruct;
            }
         }


         // Find corner-related information

         ira1  = tcol("ra1");
         idec1 = tcol("dec1");
         ira2  = tcol("ra2");
         idec2 = tcol("dec2");
         ira3  = tcol("ra3");
         idec3 = tcol("dec3");
         ira4  = tcol("ra4");
         idec4 = tcol("dec4");

         datatype = FOURCORNERS;

         if(ira1 < 0 || idec1 < 0
         || ira2 < 0 || idec2 < 0
         || ira3 < 0 || idec3 < 0
         || ira4 < 0 || idec4 < 0)
         {
            ictype1  = tcol("ctype1");
            ictype2  = tcol("ctype2");
            iequinox = tcol("equinox");
            inl      = tcol("nl");
            ins      = tcol("ns");
            icrval1  = tcol("crval1");
            icrval2  = tcol("crval2");
            icrpix1  = tcol("crpix1");
            icrpix2  = tcol("crpix2");
            icdelt1  = tcol("cdelt1");
            icdelt2  = tcol("cdelt2");
            icrota2  = tcol("crota2");

            if(ins < 0)
               ins = tcol("naxis1");

            if(inl < 0)
               inl = tcol("naxis2");

            if(ictype1 >= 0
            && ictype2 >= 0
            && inl     >= 0
            && ins     >= 0
            && icrval1 >= 0
            && icrval2 >= 0
            && icrpix1 >= 0
            && icrpix2 >= 0
            && icdelt1 >= 0
            && icdelt2 >= 0
            && icrota2 >= 0)

               datatype = WCS;

            else
            {
               tclose();
               mViewer_memCleanup();
               sprintf(returnStruct->msg, "Cannot find 'ra1', 'dec1', etc. corners or WCS columns in table [%s]\n", cat[i].file);
               return returnStruct;
            }
         }

         nimages = 0;

         while(1)
         {
            stat = tread();

            ++nimages;

            if(stat < 0)
               break;


            ovlyred   = cat[i].red;
            ovlygreen = cat[i].green;
            ovlyblue  = cat[i].blue;

            if(icolor >= 0)
            {
               if(!tnull(icolor))
               {
                  strcpy(colorstr, tval(icolor));

                  if(mViewer_colorLookup(colorstr, &ovlyred, &ovlygreen, &ovlyblue))
                  {
                     mViewer_memCleanup();
                     strcpy(returnStruct->msg, montage_msgstr);
                     return returnStruct;
                  }
               }
            }


            if(datatype == FOURCORNERS)
            {
               if(tnull(ira1) || tnull(idec1)
               || tnull(ira2) || tnull(idec2)
               || tnull(ira3) || tnull(idec3)
               || tnull(ira4) || tnull(idec4))
                  continue;

               ra1  = atof(tval(ira1));
               dec1 = atof(tval(idec1));
               ra2  = atof(tval(ira2));
               dec2 = atof(tval(idec2));
               ra3  = atof(tval(ira3));
               dec3 = atof(tval(idec3));
               ra4  = atof(tval(ira4));
               dec4 = atof(tval(idec4));
            }
            else
            {
               strcpy(im_ctype1, tval(ictype1));
               strcpy(im_ctype2, tval(ictype2));

               im_naxis1    = atoi(tval(ins));
               im_naxis2    = atoi(tval(inl));
               im_crpix1    = atof(tval(icrpix1));
               im_crpix2    = atof(tval(icrpix2));
               im_crval1    = atof(tval(icrval1));
               im_crval2    = atof(tval(icrval2));
               im_cdelt1    = atof(tval(icdelt1));
               im_cdelt2    = atof(tval(icdelt2));
               im_crota2    = atof(tval(icrota2));
               im_equinox   = 2000;

               if(iequinox >= 0)
                  im_equinox = atoi(tval(iequinox));

               strcpy(im_header, "");
               sprintf(temp, "SIMPLE  = T"                 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "BITPIX  = -64"               ); mViewer_stradd(im_header, temp);
               sprintf(temp, "NAXIS   = 2"                 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "NAXIS1  = %d",     im_naxis1 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "NAXIS2  = %d",     im_naxis2 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "CTYPE1  = '%s'",   im_ctype1 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "CTYPE2  = '%s'",   im_ctype2 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "CRVAL1  = %11.6f", im_crval1 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "CRVAL2  = %11.6f", im_crval2 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "CRPIX1  = %11.6f", im_crpix1 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "CRPIX2  = %11.6f", im_crpix2 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "CDELT1  = %14.9f", im_cdelt1 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "CDELT2  = %14.9f", im_cdelt2 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "CROTA2  = %11.6f", im_crota2 ); mViewer_stradd(im_header, temp);
               sprintf(temp, "EQUINOX = %d",     im_equinox); mViewer_stradd(im_header, temp);
               sprintf(temp, "END"                         ); mViewer_stradd(im_header, temp);

               im_wcs = wcsinit(im_header);

               if(im_wcs == (struct WorldCoor *)NULL)
               {
                  tclose();
                  mViewer_memCleanup();
                  sprintf(returnStruct->msg, "Bad WCS for image %d", nimages);
                  return returnStruct;
               }

               montage_checkWCS(im_wcs);


               /* Get the coordinate system and epoch in a     */
               /* form compatible with the conversion library  */

               if(im_wcs->syswcs == WCS_J2000)
               {
                  im_sys   = EQUJ;
                  im_epoch = 2000.;

                  if(im_wcs->equinox == 1950)
                     im_epoch = 1950.;
               }
               else if(im_wcs->syswcs == WCS_B1950)
               {
                  im_sys   = EQUB;
                  im_epoch = 1950.;

                  if(im_wcs->equinox == 2000)
                     im_epoch = 2000;
               }
               else if(im_wcs->syswcs == WCS_GALACTIC)
               {
                  im_sys   = GAL;
                  im_epoch = 2000.;
               }
               else if(im_wcs->syswcs == WCS_ECLIPTIC)
               {
                  im_sys   = ECLJ;
                  im_epoch = 2000.;

                  if(im_wcs->equinox == 1950)
                  {
                     im_sys   = ECLB;
                     im_epoch = 1950.;
                  }
               }
               else
               {
                  im_sys   = EQUJ;
                  im_epoch = 2000.;
               }


               /* Compute the locations of the corners of the images */

               pix2wcs(im_wcs, 0.5, 0.5, &xpos, &ypos);

               convertCoordinates(im_sys, im_epoch, xpos, ypos,
                                  EQUJ, 2000., &ra1, &dec1, 0.0);

               pix2wcs(im_wcs, im_naxis1+0.5, 0.5, &xpos, &ypos);

               convertCoordinates(im_sys, im_epoch, xpos, ypos,
                                  EQUJ, 2000., &ra2, &dec2, 0.0);

               pix2wcs(im_wcs, im_naxis1+0.5, im_naxis2+0.5, &xpos, &ypos);

               convertCoordinates(im_sys, im_epoch, xpos, ypos,
                                  EQUJ, 2000., &ra3, &dec3, 0.0);

               pix2wcs(im_wcs, 0.5, im_naxis2+0.5, &xpos, &ypos);

               convertCoordinates(im_sys, im_epoch, xpos, ypos,
                                  EQUJ, 2000., &ra4, &dec4, 0.0);
               
               wcsfree(im_wcs);
            }

            mViewer_great_circle(wcs, flipY, csysimg, epochimg, cat[i].csys, cat[i].epoch, ra1, dec1, ra2, dec2, ovlyred, ovlygreen, ovlyblue, cat[i].linewidth);
            mViewer_great_circle(wcs, flipY, csysimg, epochimg, cat[i].csys, cat[i].epoch, ra2, dec2, ra3, dec3, ovlyred, ovlygreen, ovlyblue, cat[i].linewidth);
            mViewer_great_circle(wcs, flipY, csysimg, epochimg, cat[i].csys, cat[i].epoch, ra3, dec3, ra4, dec4, ovlyred, ovlygreen, ovlyblue, cat[i].linewidth);
            mViewer_great_circle(wcs, flipY, csysimg, epochimg, cat[i].csys, cat[i].epoch, ra4, dec4, ra1, dec1, ovlyred, ovlygreen, ovlyblue, cat[i].linewidth);
         }

         tclose();
      }

      mViewer_addOverlay(cat[i].alpha);
   }


   // MARKS
   
   for(i=0; i<nmark; ++i)
   {
      if(mViewer_debug)
      {
         printf("Marker %d of %d\n", i+1, nmark);
         fflush(stdout);
      }

      flux = mark[i].symSize;

      if(mark[i].symUnits == FRACTIONAL)
         flux = flux * charHeight;

      else if(mark[i].symUnits == SECONDS)
         flux = flux / 3600.;
 
      else if(mark[i].symUnits == MINUTES)
         flux = flux / 60.;
 
      else if(mark[i].symUnits == DEGREES)
         flux = flux * 1.;
 
      else if(mark[i].symUnits == PIXELS)
         flux = flux * pixScale;
 
      mViewer_symbol(wcs, flipY, csysimg, epochimg, mark[i].csys, mark[i].epoch,
                     mark[i].ra, mark[i].dec, mark[i].inpix, flux, 
                     mark[i].symNPnt, mark[i].symNMax, mark[i].symType, mark[i].symRotAngle, 
                     mark[i].red, mark[i].green, mark[i].blue, mark[i].linewidth);

      mViewer_addOverlay(mark[i].alpha);
   }


   // LABELS
   
   for(i=0; i<nlabel; ++i)
   {
      if(mViewer_debug)
      {
         printf("Label %d of %d\n", i+1, nlabel);
         fflush(stdout);
      }

      if(label[i].inpix == 0)
      {
         ra  = label[i].x;
         dec = label[i].y;

         convertCoordinates(EQUJ, 2000.,  ra, dec,
                             csysimg,  epochimg, &xpos, &ypos, 0.0);

         wcs2pix(wcs, xpos, ypos, &xpix, &ypix, &offscl);

         if(hpx) mViewer_hpxCheck(&offscl, &xpix, &ypix);

         label[i].x = xpix;
         label[i].y = ypix;

         if(mViewer_debug)
         {
            printf("DEBUG> label [%s]: (%-g,%-g) -> (%-g,%-g)\n", label[i].text, ra, dec, label[i].x, label[i].y);
            fflush(stdout);
         }
      }

      ix = label[i].x;
      iy = label[i].y;

      fontSize = (int)(14. * label[i].fontscale);

      if(fontSize < 1)
         fontSize = 1;

      mViewer_draw_label(fontfile, fontSize, ix, iy, label[i].text, label[i].red, label[i].green, label[i].blue);
      mViewer_addOverlay(label[i].alpha);
   }


   // DRAWINGS
   
   for(i=0; i<ndrawing; ++i)
      mViewer_drawing(drawing[i].file, flipY, wcs, csysimg, epochimg, fontfile);


   if(mViewer_debug)
   {
      printf("DEBUG> Image completed. Writing output.\n");
      fflush(stdout);
   }


   /* Write data to JPEG file */

   fflush(stdout);
      
   if(outType == JPEG)
   {
      for(jj=0; jj<ny; ++jj)
      {
         jpegptr = (JSAMPARRAY) &jpegData[jj];

         jpeg_write_scanlines(&cinfo, jpegptr, 1);
      }

      mViewer_memCleanup();
   }


   /* Close up the image file and get out */

   if(outType == JPEG)
   {
      jpeg_finish_compress(&cinfo);

      fclose(jpegfp);

      jpeg_destroy_compress(&cinfo);
   }

   else if(outType == PNG)
   {
      // pngError = lodepng_encode32_file(pngfile, pngData, nx, ny);

      pngError =  mViewer_writePNG(pngfile, pngData, nx, ny, "");

      mViewer_memCleanup();

      if(pngError)
      {
         strcpy(returnStruct->msg, lodepng_error_text(pngError));
         return(returnStruct);
      }
   }

   if(grayfptr ) fits_close_file( grayfptr, &status);
   if(bluefptr ) fits_close_file( bluefptr, &status);
   if(greenfptr) fits_close_file(greenfptr, &status);
   if(redfptr  ) fits_close_file(  redfptr, &status);


   returnStruct->status = 0;

   if(isRGB)
   {
      sprintf(returnStruct->msg, "type=\"color\", width=%d, height=%d, bmin=%-g, bminpercent=%.2f, bminsigma=%2f, bmax=%-g, bmaxpercent=%.2f, bmaxsigma=%.2f, gmin=%-g, gminpercent=%.2f, gminsigma=%.2f, gmax=%-g, gmaxpercent=%.2f, gmaxsigma=%.2f, rmin=%-g, rminpercent=%.2f, rminsigma=%.2f, rmax=%-g, rmaxpercent=%.2f, rmaxsigma=%.2f, rdatamin=%-g, rdatamax=%-g, gdatamin=%-g, gdatamax=%-g, bdatamin=%-g, bdatamax=%-g, xflip=%d, yflip=%d, bunit=\"%s\"",
         nx, ny,
         blueminval,  blueminpercent,  blueminsigma,
         bluemaxval,  bluemaxpercent,  bluemaxsigma,
         greenminval, greenminpercent, greenminsigma,
         greenmaxval, greenmaxpercent, greenmaxsigma,
         redminval,   redminpercent,   redminsigma,
         redmaxval,   redmaxpercent,   redmaxsigma,
         rdatamin,    rdatamax,
         gdatamin,    gdatamax,
         bdatamin,    bdatamax,
         flipX,       flipY,
         bunit);

      sprintf(returnStruct->json, "\"type\":\"color\", \"width\":%d, \"height\":%d, \"bmin\":%-g, \"bminpercent\":%.2f, \"bminsigma\":%2f, \"bmax\":%-g, \"bmaxpercent\":%.2f, \"bmaxsigma\":%.2f, \"gmin\":%-g, \"gminpercent\":%.2f, \"gminsigma\":%.2f, \"gmax\":%-g, \"gmaxpercent\":%.2f, \"gmaxsigma\":%.2f, \"rmin\":%-g, \"rminpercent\":%.2f, \"rminsigma\":%.2f, \"rmax\":%-g, \"rmaxpercent\":%.2f, \"rmaxsigma\":%.2f, \"rdatamin\":%-g, \"rdatamax\":%-g, \"gdatamin\":%-g, \"gdatamax\":%-g, \"bdatamin\":%-g, \"bdatamax\":%-g, \"xflip\":%d, \"yflip\":%d, \"bunit\":\"%s\"}",
         nx, ny,
         blueminval,  blueminpercent,  blueminsigma,
         bluemaxval,  bluemaxpercent,  bluemaxsigma,
         greenminval, greenminpercent, greenminsigma,
         greenmaxval, greenmaxpercent, greenmaxsigma,
         redminval,   redminpercent,   redminsigma,
         redmaxval,   redmaxpercent,   redmaxsigma,
         rdatamin,    rdatamax,
         gdatamin,    gdatamax,
         bdatamin,    bdatamax,
         flipX,       flipY,
         bunit);

      strcpy(returnStruct->type, "color");
      
      returnStruct->nx              = nx;
      returnStruct->ny              = ny;
      returnStruct->grayminval      = 0.;
      returnStruct->grayminpercent  = 0.;
      returnStruct->grayminsigma    = 0.;
      returnStruct->graymaxval      = 0.;
      returnStruct->graymaxpercent  = 0.;
      returnStruct->graymaxsigma    = 0.;
      returnStruct->blueminval      = blueminval;
      returnStruct->blueminpercent  = blueminpercent;
      returnStruct->blueminsigma    = blueminsigma;
      returnStruct->bluemaxval      = bluemaxval;
      returnStruct->bluemaxpercent  = bluemaxpercent;
      returnStruct->bluemaxsigma    = bluemaxsigma;
      returnStruct->greenminval     = greenminval;
      returnStruct->greenminpercent = greenminpercent;
      returnStruct->greenminsigma   = greenminsigma;
      returnStruct->greenmaxval     = greenmaxval;
      returnStruct->greenmaxpercent = greenmaxpercent;
      returnStruct->greenmaxsigma   = greenmaxsigma;
      returnStruct->redminval       = redminval;
      returnStruct->redminpercent   = redminpercent;
      returnStruct->redminsigma     = redminsigma;
      returnStruct->redmaxval       = redmaxval;
      returnStruct->redmaxpercent   = redmaxpercent;
      returnStruct->redmaxsigma     = redmaxsigma;
      returnStruct->graydatamin     = 0.;
      returnStruct->graydatamax     = 0.;
      returnStruct->rdatamin        = rdatamin;
      returnStruct->rdatamax        = rdatamax;
      returnStruct->gdatamin        = gdatamin;
      returnStruct->gdatamax        = gdatamax;
      returnStruct->bdatamin        = bdatamin;
      returnStruct->bdatamax        = bdatamax;
      returnStruct->flipX           = flipX;
      returnStruct->flipY           = flipY;
      returnStruct->colortable      = 0;

      strcpy(returnStruct->bunit, bunit);
   }
   else
   {
     sprintf(returnStruct->msg, "type=\"grayscale\", width=%d, height=%d, min=%-g, minpercent=%.2f, minsigma=%.2f, max=%-g, maxpercent=%.2f, maxsigma=%.2f, datamin=%-g, datamax=%-g, xflip=%d, yflip=%d, bunit=\"%s\", colortable=%d",
         nx, ny,
         grayminval,  grayminpercent, grayminsigma,
         graymaxval,  graymaxpercent, graymaxsigma,
         graydatamin, graydatamax,
         flipX,       flipY,
         bunit,       colortable);

     sprintf(returnStruct->json, "\"type\":\"grayscale\", \"width\":%d, \"height\":%d, \"min\":%-g, \"minpercent\":%.2f, \"minsigma\":%.2f, \"max\":%-g, \"maxpercent\":%.2f, \"maxsigma\":%.2f, \"datamin\":%-g, \"datamax\":%-g, \"xflip\":%d, \"yflip\":%d, \"bunit\":\"%s\", \"colortable\":%d",
         nx, ny,
         grayminval,  grayminpercent, grayminsigma,
         graymaxval,  graymaxpercent, graymaxsigma,
         graydatamin, graydatamax,
         flipX,       flipY,
         bunit,       colortable);

      strcpy(returnStruct->type, "grayscale");
     
      returnStruct->nx              = nx;
      returnStruct->ny              = ny;
      returnStruct->grayminval      = grayminval;
      returnStruct->grayminpercent  = grayminpercent;
      returnStruct->grayminsigma    = grayminsigma;
      returnStruct->graymaxval      = graymaxval;
      returnStruct->graymaxpercent  = graymaxpercent;
      returnStruct->graymaxsigma    = graymaxsigma;
      returnStruct->blueminval      = 0.;
      returnStruct->blueminpercent  = 0.;
      returnStruct->blueminsigma    = 0.;
      returnStruct->bluemaxval      = 0.;
      returnStruct->bluemaxpercent  = 0.;
      returnStruct->bluemaxsigma    = 0.;
      returnStruct->greenminval     = 0.;
      returnStruct->greenminpercent = 0.;
      returnStruct->greenminsigma   = 0.;
      returnStruct->greenmaxval     = 0.;
      returnStruct->greenmaxpercent = 0.;
      returnStruct->greenmaxsigma   = 0.;
      returnStruct->redminval       = 0.;
      returnStruct->redminpercent   = 0.;
      returnStruct->redminsigma     = 0.;
      returnStruct->redmaxval       = 0.;
      returnStruct->redmaxpercent   = 0.;
      returnStruct->redmaxsigma     = 0.;
      returnStruct->graydatamin     = graydatamin;
      returnStruct->graydatamax     = graydatamax;
      returnStruct->rdatamin        = 0.;
      returnStruct->rdatamax        = 0.;
      returnStruct->gdatamin        = 0.;
      returnStruct->gdatamax        = 0.;
      returnStruct->bdatamin        = 0.;
      returnStruct->bdatamax        = 0.;
      returnStruct->flipX           = flipX;
      returnStruct->flipY           = flipY;
      returnStruct->colortable      = colortable;

      strcpy(returnStruct->bunit, bunit);
   }

   return returnStruct;
}


void mViewer_memCleanup()
{
   int i;

   if(mViewer_debug)
   {
      printf("DEBUG> memory cleanup\n");
      fflush(stdout);
   }

   if(isRGB)
   {
      free(rfitsbuf);
      free(gfitsbuf);
      free(bfitsbuf);
   }
   else
      free(fitsbuf);

   if(outType == JPEG)
   {
      for(i=0; i<ny; ++i)
      {
         free(jpegData[i]);
         free(jpegOvly[i]);
      }

      free(jpegData);
      free(jpegOvly);
   }

   else if(outType == PNG)
   {
      free(pngData);
      free(pngOvly);
   }

   for(i=0; i<ny; ++i)
   {
      free(ovlyweight[i]);
      free(ovlylock  [i]);
   }

   free(ovlyweight);
   free(ovlylock);

   wcsfree(wcs);
}



/*************************************************************************/
/*                                                                       */
/*  Turn a text string into the appropriate RGV values.  For example     */
/*  "2.0 circle" or "15p 5 star 36."  The latter has size 15. (pixels),  */
/*  pentagon "starred", rotated 36 degrees).                             */
/*                                                                       */
/*************************************************************************/

int mViewer_parseSymbol(char *symbolstr, int *symNPnt, int *symNMax, int *symType, double *symRotAngle)
{
   int   i, cmdc;
   char *cmdv[256];
   char *end;

   cmdc = parsecmd(symbolstr, cmdv);

   if(cmdc <= 0)
      return 1;


   i = 0;

   *symRotAngle = 0.;
   *symType     = 0;
   *symNMax     = 0;

   if(strncasecmp(cmdv[i], "triangle", 3) == 0)
   {
      *symNPnt = 3;
      *symRotAngle = 120.;
   }

   else if(strncasecmp(cmdv[i], "box", 3) == 0)
   {
      *symNPnt = 4;
      *symRotAngle = 45.;
   }

   else if(strncasecmp(cmdv[i], "square", 3) == 0)
   {
      *symNPnt = 4;
      *symRotAngle = 45.;
   }

   else if(strncasecmp(cmdv[i], "diamond", 3) == 0)
      *symNPnt = 4;

   else if(strncasecmp(cmdv[i], "pentagon", 3) == 0)
   {
      *symNPnt = 5;
      *symRotAngle = 72.;
   }

   else if(strncasecmp(cmdv[i], "hexagon", 3) == 0)
   {
      *symNPnt = 6;
      *symRotAngle = 60.;
   }

   else if(strncasecmp(cmdv[i], "septagon", 3) == 0)
   {
      *symNPnt = 7;
      *symRotAngle = 360./7.;
   }

   else if(strncasecmp(cmdv[i], "octagon", 3) == 0)
   {
      *symNPnt = 8;
      *symRotAngle = 45.;
   }

   else if(strncasecmp(cmdv[i], "plus", 3) == 0)
   {
      *symType = 2;
      *symNPnt = 4;
   }

   else if(strncasecmp(cmdv[i], "el", 2) == 0)
   {
      *symNPnt = 4;
      *symRotAngle = 135.;
      *symNMax = 2;
   }

   else if(strncasecmp(cmdv[i], "circle", 3) == 0)
   {
      *symNPnt = 128;
      *symRotAngle = 0.;
   }

   else if(strncasecmp(cmdv[i], "compass", 3) == 0)
   {
      *symType = 3;
      *symNPnt  = 4;
      *symRotAngle  = 0.;
   }

   else
   {
      *symType = strtol(cmdv[i], &end, 0);

      if(end < (cmdv[i] + (int)strlen(cmdv[i])))
      {
         if(strncasecmp(cmdv[i], "polygon", 1) == 0)
            *symType = 0;

         else if(strncasecmp(cmdv[i], "starred", 2) == 0)
            *symType = 1;

         else if(strncasecmp(cmdv[i], "skeletal", 2) == 0)
            *symType = 2;

         else
            return 1;
      }

      ++i;
     
      if(i < cmdc)
      {
         *symNPnt = strtol(cmdv[i], &end, 0);

         if(end < (cmdv[i] + (int)strlen(cmdv[i])) || *symNPnt < 3)
            return 1;

         ++i;
        
         if(i < cmdc)
         {
            *symRotAngle = strtod(cmdv[i], &end);

            if(end < (cmdv[i] + (int)strlen(cmdv[i])))
               return 1;

            ++i;
         }
      }
   }

   return 0;
}




/*******************************************************/
/*                                                     */
/*  Turn a text string into the appropriate RGV values */
/*                                                     */
/*******************************************************/

int mViewer_colorLookup(char *colorin, double *ovlyred, double *ovlygreen, double *ovlyblue)
{
   int  j;
   char colorstr[MAXSTR];

   strcpy(colorstr, colorin);

   if(colorstr[0] == '#')
      strcpy(colorstr, colorin+1);

   if(strlen(colorstr) == 6 && mViewer_hexVal(colorstr[0]) >= 0)
   {
      for(j=0; j<strlen(colorstr); ++j)
      {
         if(mViewer_hexVal(colorstr[j]) < 0)
         {
            strcpy(montage_msgstr, "Invalid color specification");
            return 1;
         }

         *ovlyred   = mViewer_hexVal(colorstr[0]) * 16 + mViewer_hexVal(colorstr[1]);
         *ovlygreen = mViewer_hexVal(colorstr[2]) * 16 + mViewer_hexVal(colorstr[3]);
         *ovlyblue  = mViewer_hexVal(colorstr[4]) * 16 + mViewer_hexVal(colorstr[5]);
      }
   }

   else if(strcasecmp(colorstr, "black") == 0)
   {
      *ovlyred   =   0;
      *ovlygreen =   0;
      *ovlyblue  =   0;
   }

   else if(strcasecmp(colorstr, "white") == 0)
   {
      *ovlyred   = 255;
      *ovlygreen = 255;
      *ovlyblue  = 255;
   }

   else if(strcasecmp(colorstr, "red") == 0)
   {
      *ovlyred   = 255;
      *ovlygreen =   0;
      *ovlyblue  =   0;
   }

   else if(strcasecmp(colorstr, "green") == 0)
   {
      *ovlyred   =   0;
      *ovlygreen = 255;
      *ovlyblue  =   0;
   }

   else if(strcasecmp(colorstr, "blue") == 0)
   {
      *ovlyred   =   0;
      *ovlygreen =   0;
      *ovlyblue  = 255;
   }

   else if(strcasecmp(colorstr, "magenta") == 0)
   {
      *ovlyred   = 255;
      *ovlygreen =   0;
      *ovlyblue  = 255;
   }

   else if(strcasecmp(colorstr, "cyan") == 0)
   {
      *ovlyred   =   0;
      *ovlygreen = 255;
      *ovlyblue  = 255;
   }

   else if(strcasecmp(colorstr, "yellow") == 0)
   {
      *ovlyred   = 255;
      *ovlygreen = 255;
      *ovlyblue  =   0;
   }

   else if(strcasecmp(colorstr, "gray") == 0
        || strcasecmp(colorstr, "grey") == 0)
   {
      *ovlyred   = 128;
      *ovlygreen = 128;
      *ovlyblue  = 128;
   }

   else
   {
      *ovlyred   = 128;
      *ovlygreen = 128;
      *ovlyblue  = 128;
   }

   *ovlyred   = *ovlyred   / 255;
   *ovlygreen = *ovlygreen / 255;
   *ovlyblue  = *ovlyblue  / 255;

   return 0;
}


int mViewer_hexVal(char c)
{
   if(isdigit(c)) return (c - '0');
   if(c == 'a')   return 10;
   if(c == 'b')   return 11;
   if(c == 'c')   return 12;
   if(c == 'd')   return 13;
   if(c == 'e')   return 14;
   if(c == 'f')   return 15;
   if(c == 'A')   return 10;
   if(c == 'B')   return 11;
   if(c == 'C')   return 12;
   if(c == 'D')   return 13;
   if(c == 'E')   return 14;
   if(c == 'F')   return 15;

   return -1;
}


/******************************************/
/*                                        */
/*  Build a fake WCS for the pixel array  */
/*                                        */
/******************************************/

struct WorldCoor *mViewer_wcsfake(int naxis1, int naxis2)
{
   static struct WorldCoor *wcs;

   char header[4096];
   char hline  [256];

   header[0] = '\0';

   sprintf(hline, "SIMPLE = T");                       mViewer_stradd(header, hline);
   sprintf(hline, "NAXIS  = 2");                       mViewer_stradd(header, hline);
   sprintf(hline, "NAXIS1 = %d", naxis1);              mViewer_stradd(header, hline);
   sprintf(hline, "NAXIS2 = %d", naxis2);              mViewer_stradd(header, hline);
   sprintf(hline, "CTYPE1 = 'RA---TAN'");              mViewer_stradd(header, hline);
   sprintf(hline, "CTYPE2 = 'DEC--TAN'");              mViewer_stradd(header, hline);
   sprintf(hline, "CDELT1 = 0.000001");                mViewer_stradd(header, hline);
   sprintf(hline, "CDELT2 = 0.000001");                mViewer_stradd(header, hline);
   sprintf(hline, "CRVAL1 = 0.");                      mViewer_stradd(header, hline);
   sprintf(hline, "CRVAL2 = 0.");                      mViewer_stradd(header, hline);
   sprintf(hline, "CRPIX1 = %.2f", (naxis1 + 1.)/2.);  mViewer_stradd(header, hline);
   sprintf(hline, "CRPIX2 = %.2f", (naxis2 + 1.)/2.);  mViewer_stradd(header, hline);
   sprintf(hline, "CROTA2 = 0.");                      mViewer_stradd(header, hline);
   sprintf(hline, "END");                              mViewer_stradd(header, hline);

   wcs = wcsinit(header);

   if(wcs == (struct WorldCoor *)NULL)
   {
      strcpy(montage_msgstr, "wcsinit() failed for fake header.");
      return wcs;
   }

   return wcs;
}

int mViewer_stradd(char *header, char *card)
{
   int i, hlen, clen;

   hlen = strlen(header);
   clen = strlen(card);

   for(i=0; i<clen; ++i)
      header[hlen+i] = card[i];

   if(clen < 80)
      for(i=clen; i<80; ++i)
         header[hlen+i] = ' ';

   header[hlen+80] = '\0';

   return(strlen(header));
}



/**************************************************/
/*  Projections like CAR sometimes add an extra   */
/*  360 degrees worth of pixels to the return     */
/*  and call it off-scale.                        */
/**************************************************/

void mViewer_fixxy(double *x, double *y, int *offscl)
{
   *x = *x - xcorrection;
   *y = *y - ycorrection;

   if(*x < 0.
   || *x > wcs->nxpix+1.
   || *y < 0.
   || *y > wcs->nypix+1.)
      *offscl = 1;

   return;
}


/**************************************************/
/*                                                */
/*  Parse the HDU / plane info from the file name */
/*                                                */
/**************************************************/

int mViewer_getPlanes(char *file, int *planes)
{
   int   count, len;
   char *ptr, *ptr1;

   count = 0;

   ptr = file;

   len = strlen(file);

   while(ptr < file+len && *ptr != '[')
      ++ptr;

   while(1)
   {
      if(ptr >= file+len)
         return count;

      if(*ptr != '[')
         return count;

      *ptr = '\0';
      ++ptr;

      ptr1 = ptr;

      while(ptr1 < file+len && *ptr1 != ']')
         ++ptr1;

      if(ptr1 >= file+len)
         return count;

      if(*ptr1 != ']')
         return count;

      *ptr1 = '\0';

      planes[count] = atoi(ptr);
      ++count;

      ptr = ptr1;
      ++ptr;
   }
}
      


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mViewer_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);
}



/***********************************/
/*                                 */
/*  Color tables                   */
/*                                 */
/***********************************/


void mViewer_createColorTable(int itable)
{
    int    i, j, nseg;
    int   *dn, *red_tbl, *grn_tbl, *blue_tbl;
    double rscale, gscale, bscale, tmp; 

    int dn0[] = {  0, 255};
    
    int dn2[] = {  0,  17,  34,  51,  68,  85, 102, 119, 
                 136, 153, 170, 187, 204, 221, 238, 255};
    
    int dn3[] = {  0,  25,  40,  55,  70,  90, 110, 130,
                 150, 175, 200, 225, 255};
     
    int dn4[] = {  0,  23,  46,  70,  93, 116, 
                 139, 162, 185, 209, 232, 255};
    
    int dn5[] = {  0,  20,  39,  59,  78,  98,
                 118, 137, 156, 177, 196, 255};
    
    int red_tbl0[]   = {  0, 255};
    int grn_tbl0[]   = {  0, 255};
    int blue_tbl0[]  = {  0, 255};
        
    int red_tbl1[]   = {255,   0};
    int grn_tbl1[]   = {255,   0};
    int blue_tbl1[]  = {255,   0};
    
    int red_tbl2[]   = {  0,  98, 180, 255, 246, 220, 181,  88,
                          0,   0,   0,   0,   0,  85, 170, 255};

    int grn_tbl2[]   = {  0,   0,   0,   0,  88, 181, 220, 245, 
                        255, 235, 180,  98,   0, 128, 192, 255};

    int blue_tbl2[]  = {  0, 180,  98,   0,   0,   0,   0,   0,
                          0,  98, 180, 235, 255, 255, 255, 255};
        

    int red_tbl3[]   = {  0,   0,   0,   0,   0,   0,   0, 150,
                        225, 255, 255, 196, 255}; 

    int grn_tbl3[]   = {  0,   0,   0, 125, 196, 226, 255, 255,
                        225, 150,   0,   0, 255}; 

    int blue_tbl3[]  = {  0, 150, 255, 225, 196, 125,   0,   0,
                          0,   0,   0, 196, 255}; 


    int red_tbl4[]   = {  0,   0,   0,   0,   0,   0,   0,  20,
                        135, 210, 240, 255, 240, 255, 255, 255}; 

    int grn_tbl4[]   = {  0,   0,  20,  63, 112, 162, 210, 255,
                        240, 205, 135,  20,   0,  85, 170, 255}; 

    int blue_tbl4[]  = {  0,  95, 175, 250, 255, 175,  90,  30, 
                          0,   0,   0,   0,   0,  85, 170, 255}; 


    int red_tbl5[]   = {255, 255, 255, 240, 255, 240, 210, 135,
                         20,   0,   0,   0,   0,   0,   0,   0};

    int grn_tbl5[]   = {255, 170,  85,   0,  20, 135, 205, 240,
                        255, 210, 162, 112,  63,  20,   0,   0};

    int blue_tbl5[]  = {255, 170,  85,   0,   0,   0,   0,   0,
                         30,  90, 175, 255, 250, 175,  95,   0};


    int red_tbl6[]   = {  0,   0,   0,   0,   0,   0,   0,   0,
                          0,   0,   0,  20, 210, 240, 240, 255};

    int grn_tbl6[]   = {  0,   0,   0,  10,  20,  41,  63,  83,
                        112, 137, 162, 255, 205, 135,   0, 255};

    int blue_tbl6[]  = {  0,  47,  95, 140, 175, 212, 250, 252,
                        255, 212, 175,  30,   0,   0,   0, 255};


    int red_tbl7[]   = {255, 255, 255, 255, 255, 255, 255, 255,
                        224, 192, 160, 128,  96,  64,  32,   0};

    int grn_tbl7[]   = {  0,  32,  64,  96, 128, 160, 192, 224,
                        224, 192, 160, 128,  96,  64,  32,   0};

    int blue_tbl7[]  = {  0,  32,  64,  96, 128, 160, 192, 224,
                        255, 255, 255, 255, 255, 255, 255, 255};


    int red_tbl8[]   = {  0, 255};
    int grn_tbl8[]   = {  0,   0};
    int blue_tbl8[]  = {  0,   0};
        
    int red_tbl9[]   = {  0,   0};
    int grn_tbl9[]   = {  0, 255};
    int blue_tbl9[]  = {  0,   0};
        
    int red_tbl10[]  = {  0,   0};
    int grn_tbl10[]  = {  0,   0};
    int blue_tbl10[] = {  0, 255};
        

    int red_tbl11[]  = {    0,   0,   0,  20, 135, 210,
                          240, 255, 240, 255, 255, 255}; 

    int grn_tbl11[]  = {    0,  85, 170, 255, 240, 205, 
                          135,  20,   0,  85, 170, 255}; 

    int blue_tbl11[] = {  255, 175,  90,  30,   0,   0,   
                            0,   0,   0,  85, 170, 255}; 


    int red_tbl12[]  = {  255, 255, 191, 160, 128, 122, 
                          255, 255, 255, 255, 200, 128}; 

    int grn_tbl12[]  = {  255, 255, 204, 230, 255, 255, 
                          255, 200, 145,   0,   0,   0}; 

    int blue_tbl12[] = {  255, 255, 255, 255, 255, 147,   
                            0,   0,   0,   0,   0,   0}; 


   switch(itable)
   {
      case 0:
         dn       = dn0;
         nseg     = 2;
         red_tbl  = red_tbl0;
         grn_tbl  = grn_tbl0;
         blue_tbl = blue_tbl0;
         break;
          
      case 1:
         dn       = dn0;
         nseg     = 2;
         red_tbl  = red_tbl1;
         grn_tbl  = grn_tbl1;
         blue_tbl = blue_tbl1;
         break;
     
      case 2:
         dn       = dn2;
         nseg     = 16;
         red_tbl  = red_tbl2;
         grn_tbl  = grn_tbl2;
         blue_tbl = blue_tbl2;
         break;
      
      case 3:
         dn       = dn3;
         nseg     = 13;
         red_tbl  = red_tbl3;
         grn_tbl  = grn_tbl3;
         blue_tbl = blue_tbl3;
         break;

      case 4:
         dn       = dn2;
         nseg     = 16;
         red_tbl  = red_tbl4;
         grn_tbl  = grn_tbl4;
         blue_tbl = blue_tbl4;
         break;
      
      case 5:
         dn       = dn2;
         nseg     = 16;
         red_tbl  = red_tbl5;
         grn_tbl  = grn_tbl5;
         blue_tbl = blue_tbl5;
         break;
      
      case 6:
         dn       = dn2;
         nseg     = 16;
         red_tbl  = red_tbl6;
         grn_tbl  = grn_tbl6;
         blue_tbl = blue_tbl6;
         break;
      
      case 7:
         dn       = dn2;
         nseg     = 16;
         red_tbl  = red_tbl7;
         grn_tbl  = grn_tbl7;
         blue_tbl = blue_tbl7;
         break;
      
      case 8:
         dn       = dn0;
         nseg     = 2;
         red_tbl  = red_tbl8;
         grn_tbl  = grn_tbl8;
         blue_tbl = blue_tbl8;
         break;

      case 9:
         dn       = dn0;
         nseg     = 2;
         red_tbl  = red_tbl9;
         grn_tbl  = grn_tbl9;
         blue_tbl = blue_tbl9;
         break;

      case 10:
         dn       = dn0;
         nseg     = 2;
         red_tbl  = red_tbl10;
         grn_tbl  = grn_tbl10;
         blue_tbl = blue_tbl10;
         break;

      case 11:
         dn       = dn4;
         nseg     = 12;
         red_tbl  = red_tbl11;
         grn_tbl  = grn_tbl11;
         blue_tbl = blue_tbl11;
         break;
      
      case 12:
         dn       = dn5;
         nseg     = 12;
         red_tbl  = red_tbl12;
         grn_tbl  = grn_tbl12;
         blue_tbl = blue_tbl12;
         break;
      
      default:
         dn       = dn0;
         nseg     = 2;
         red_tbl  = red_tbl0;
         grn_tbl  = grn_tbl0;
         blue_tbl = blue_tbl0;
      break;
   }

   for (j=1; j<nseg; ++j)
   {
      rscale = (double)(( red_tbl[j] -  red_tbl[j-1])/ (dn[j]-dn[j-1]));
      gscale = (double)(( grn_tbl[j] -  grn_tbl[j-1])/ (dn[j]-dn[j-1]));
      bscale = (double)((blue_tbl[j] - blue_tbl[j-1])/ (dn[j]-dn[j-1]));

      for (i=dn[j-1]; i<=dn[j]; i++) 
      {
         tmp = rscale * (i-dn[j-1]) + red_tbl[j-1];

         if (tmp > 255.) tmp = 255.;
         if (tmp <   0.) tmp =   0.;

         color_table[i][0] = (int)(tmp);

         tmp = gscale*(i-dn[j-1]) + grn_tbl[j-1];

         if (tmp > 255.) tmp = 255.;
         if (tmp <   0.) tmp =   0.;

         color_table[i][1] = (int)(tmp);

         tmp = bscale*(i-dn[j-1]) + blue_tbl[j-1];

         if (tmp > 255.) tmp = 255.;
         if (tmp <   0.) tmp =   0.;

         color_table[i][2] = (int)(tmp);
      }
   }

   if(itable == 11)
   {
      color_table[0][0] = 0;
      color_table[0][1] = 0;
      color_table[0][2] = 0;
   }
}


/***********************************/
/*                                 */
/*  Parse a range string           */
/*                                 */
/***********************************/

int mViewer_parseRange(char const *str, char const *kind, double *val, double *extra, int *type) {
   char const *ptr;
   char *end;
   double sign = 1.0;

   ptr = str;
   while (isspace(*ptr)) ++ptr;
   if (*ptr == '-' || *ptr == '+') {
      sign = (*ptr == '-') ? -1.0 : 1.0;
      ++ptr;
   }
   while (isspace(*ptr)) ++ptr;
   errno = 0;
   *val = strtod(ptr, &end) * sign;
   if (errno != 0) {
      sprintf(montage_msgstr, "leading numeric term in %s '%s' cannot be converted to a finite floating point number",
             kind, str);
      return 1;
   }
   if (ptr == end) {
      /* range string didn't start with a number, check for keywords */
      if (strncmp(ptr, "min", 3) == 0) {
         *val = 0.0;
      } else if (strncmp(ptr, "max", 3) == 0) {
         *val = 100.0;
      } else if (strncmp(ptr, "med", 3) == 0) {
         *val = 50.0;
      } else {
         sprintf(montage_msgstr, "'%s' is not a valid %s", str, kind);
         return 1;
      }
      *type = PERCENTILE;
      ptr += 3;
   } else {
      /* range string began with a number, look for a type spec */
      ptr = end;
      while (isspace(*ptr)) ++ptr;
      switch (*ptr) {
         case '%':
            if (*val < 0.0) {
               sprintf(montage_msgstr, "'%s': negative percentile %s", str, kind);
               return 1;
            }
            if (*val > 100.0) {
               sprintf(montage_msgstr, "'%s': percentile %s larger than 100", str, kind);
               return 1;
            }
            *type = PERCENTILE;
            ++ptr;
            break;
         case 's':
         case 'S':
            *type = SIGMA;
            ++ptr;
            break;
         case '+':
         case '-':
         case '\0':
            *type = VALUE;
            break;
         default:
            sprintf(montage_msgstr, "'%s' is not a valid %s", str, kind);
            return 1;
      }
   }

   /* look for a trailing numeric term */
   *extra = 0.0;
   while (isspace(*ptr)) ++ptr;
   if (*ptr == '-' || *ptr == '+') {
      sign = (*ptr == '-') ? -1.0 : 1.0;
      ++ptr;
      while (isspace(*ptr)) ++ptr;
      *extra = strtod(ptr, &end) * sign;
      if (errno != 0) {
         sprintf(montage_msgstr, "extra numeric term in %s '%s' cannot be converted to a finite floating point number",
                kind, str);
         return 1;
      }
      if (ptr == end) {
         sprintf(montage_msgstr, "%s '%s' contains trailing junk", kind, str);
         return 1;
      }
      ptr = end;
   }
   while (isspace(*ptr)) ++ptr;
   if (*ptr != '\0') {
      sprintf(montage_msgstr, "%s '%s' contains trailing junk", kind, str);
      return 1;
   }

   return 0;
}


/***********************************/
/*                                 */
/*  Histogram percentile ranges    */
/*                                 */
/***********************************/

#define NBIN 200000

int     nbin;

int     hist    [NBIN];
double  chist   [NBIN];
double  datalev [NBIN];
double  gausslev[NBIN];

unsigned long npix;

double  delta, rmin, rmax;
 
int mViewer_getRange(fitsfile *fptr, char *minstr, char *maxstr,
                     double *rangemin, double *rangemax, 
                     int type, char *betastr, double *rangebeta, double *dataval,
                     int imnaxis1, int imnaxis2,  double *datamin, double *datamax,
                     double *median, double *sig,
                     int count, int *planes)
{
   int     i, j, k, mintype, maxtype, betatype, nullcnt, status;
   long    fpixel[4], nelements;
   double  d, diff, minval, maxval, betaval;
   double  lev16, lev50, lev84, sigma;
   double  minextra, maxextra, betaextra;
   double *data;

   double  glow, ghigh, gaussval, gaussstep;
   double  dlow, dhigh;
   double  gaussmin, gaussmax;


   nbin = NBIN - 1;

   /* MIN/MAX: Determine what type of  */
   /* range string we are dealing with */

   if(mViewer_parseRange(minstr, "display min", &minval, &minextra, &mintype))
      return 1;

   if(mViewer_parseRange(maxstr, "display max", &maxval, &maxextra, &maxtype))
      return 1;

   betaval   = 0.;
   betaextra = 0.;

   if (type == ASINH) {
      if(mViewer_parseRange(betastr, "beta value", &betaval, &betaextra, &betatype))
         return 1;
   }

   /* If we don't have to generate the image */
   /* histogram, get out now                 */

   *rangemin  = minval + minextra;
   *rangemax  = maxval + maxextra;
   *rangebeta = betaval + betaextra;


   /* Find the min, max values in the image */

   npix = 0;

   rmin =  1.0e10;
   rmax = -1.0e10;

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   if(count > 1)
      fpixel[2] = planes[1];
   if(count > 2)
      fpixel[3] = planes[2];

   nelements = imnaxis1;

   data = (double *)malloc(nelements * sizeof(double));

   status = 0;

   for(j=0; j<imnaxis2; ++j) 
   {
      if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, &mynan,
                       data, &nullcnt, &status))
         mViewer_printFitsError(status);
      
      for(i=0; i<imnaxis1; ++i) 
      {
         if(!mNaN(data[i])) 
         {
            if (data[i] > rmax) rmax = data[i];
            if (data[i] < rmin) rmin = data[i];

            ++npix;
         }
      }

      ++fpixel[1];
   }

   if(npix == 0)
   {
      strcpy(montage_msgstr, "Image contains no non-blank pixels.");
      return 1;
   }

   *datamin = rmin;
   *datamax = rmax;

   diff = rmax - rmin;

   if(mViewer_debug)
   {
      printf("DEBUG> mViewer_getRange(): rmin = %-g, rmax = %-g (diff = %-g)\n",
         rmin, rmax, diff);
      fflush(stdout);
   }


   /* Populate the histogram */

   for(i=0; i<nbin+1; i++) 
      hist[i] = 0;
   
   fpixel[1] = 1;

   for(j=0; j<imnaxis2; ++j) 
   {
      if(fits_read_pix(fptr, TDOUBLE, fpixel, nelements, &mynan,
                       data, &nullcnt, &status))
         mViewer_printFitsError(status);

      for(i=0; i<imnaxis1; ++i) 
      {
         if(!mNaN(data[i])) 
         {
            d = floor(nbin*(data[i]-rmin)/diff);
            k = (int)d;

            if(k > nbin-1)
               k = nbin-1;

            if(k < 0)
               k = 0;

            ++hist[k];
         }
      }

      ++fpixel[1];
   }

   free(data);


   /* Compute the cumulative histogram      */
   /* and the histogram bin edge boundaries */

   delta = diff/nbin;

   chist[0] = 0;

   for(i=1; i<=nbin; ++i)
      chist[i] = chist[i-1] + hist[i-1];


   /* Find the data value associated    */
   /* with the minimum percentile/sigma */

   lev16 = mViewer_percentileLevel(16.);
   lev50 = mViewer_percentileLevel(50.);
   lev84 = mViewer_percentileLevel(84.);

   sigma = (lev84 - lev16) / 2.;

   *median = lev50;
   *sig    = sigma;

   if(mintype == PERCENTILE)
      *rangemin = mViewer_percentileLevel(minval) + minextra;

   else if(mintype == SIGMA)
      *rangemin = lev50 + minval * sigma + minextra;


   /* Find the data value associated    */
   /* with the max percentile/sigma     */

   if(maxtype == PERCENTILE)
      *rangemax = mViewer_percentileLevel(maxval) + maxextra;
   
   else if(maxtype == SIGMA)
      *rangemax = lev50 + maxval * sigma + maxextra;


   /* Find the data value associated    */
   /* with the beta percentile/sigma    */

   if(type == ASINH)
   {
      if(betatype == PERCENTILE)
         *rangebeta = mViewer_percentileLevel(betaval) + betaextra;

      else if(betatype == SIGMA)
         *rangebeta = lev50 + betaval * sigma + betaextra;
   }

   if(*rangemin == *rangemax)
      *rangemax = *rangemin + 1.;
   
   if(mViewer_debug)
   {
      if(type == ASINH)
         printf("DEBUG> mViewer_getRange(): range = %-g to %-g (beta = %-g)\n", 
            *rangemin, *rangemax, *rangebeta);
      else
         printf("DEBUG> mViewer_getRange(): range = %-g to %-g\n", 
            *rangemin, *rangemax);
   }


   /* Find the data levels associated with */
   /* a Guassian histogram stretch         */

   if(type == GAUSSIAN
   || type == GAUSSIANLOG)
   {
      for(i=0; i<nbin; ++i)
      {
         datalev [i] = rmin+delta*i;
         gausslev[i] = mViewer_snpinv(chist[i+1]/npix);
      }


      /* Find the guassian levels associated */
      /* with the range min, max             */

      for(i=0; i<nbin-1; ++i)
      {
         if(datalev[i] > *rangemin)
         {
            gaussmin = gausslev[i];
            break;
         }
      }

      gaussmax = gausslev[nbin-2];

      for(i=0; i<nbin-1; ++i)
      {
         if(datalev[i] > *rangemax)
         {
            gaussmax = gausslev[i];
            break;
         }
      }

      if(type == GAUSSIAN)
      {
         gaussstep = (gaussmax - gaussmin)/255.;

         for(j=0; j<256; ++j)
         {
            gaussval = gaussmin + gaussstep * j;

            for(i=1; i<nbin-1; ++i)
               if(gausslev[i] >= gaussval)
                  break;

            glow  = gausslev[i-1];
            ghigh = gausslev[i];

            dlow  = datalev [i-1];
            dhigh = datalev [i];

            if(glow == ghigh)
               dataval[j] = dlow;
            else
               dataval[j] = dlow
                          + (gaussval - glow) * (dhigh - dlow) / (ghigh - glow);
         }
      }
      else
      {
         gaussstep = log10(gaussmax - gaussmin)/255.;

         for(j=0; j<256; ++j)
         {
            gaussval = gaussmax - pow(10., gaussstep * (256. - j));

            for(i=1; i<nbin-1; ++i)
               if(gausslev[i] >= gaussval)
                  break;

            glow  = gausslev[i-1];
            ghigh = gausslev[i];

            dlow  = datalev [i-1];
            dhigh = datalev [i];

            if(glow == ghigh)
               dataval[j] = dlow;
            else
               dataval[j] = dlow
                          + (gaussval - glow) * (dhigh - dlow) / (ghigh - glow);
         }
      }
   }

   return 0;
}


int mViewer_readHist(char *histfile,  double *minval,  double *maxval, double *dataval, 
                     double *datamin, double *datamax, double *median, double *sigma, int *type)
{
   int   i;

   FILE *fhist;

   char  line [1024];
   char  label[1024];

   fhist = fopen(histfile, "r");

   if(fhist == (FILE *)NULL)
   {
      strcpy(montage_msgstr, "Cannot open histogram file.");
      return 1;
   }

   while(1)
   {
      fgets(line, 1024, fhist);

      if(line[0] != '#')
         break;
   }

   sscanf(line, "%s %d", label, type);

   fgets(line, 1024, fhist);
   fgets(line, 1024, fhist);
   fgets(line, 1024, fhist);
   sscanf(line, "%s %lf %lf", label, minval, maxval);

   fgets(line, 1024, fhist);
   fgets(line, 1024, fhist);
   fgets(line, 1024, fhist);
   sscanf(line, "%s %lf %lf", label, datamin, datamax);

   fgets(line, 1024, fhist);
   sscanf(line, "%s %lf %lf", label, median, sigma);

   fgets(line, 1024, fhist);
   fgets(line, 1024, fhist);
   sscanf(line, "%s %lf", label, &rmin);

   fgets(line, 1024, fhist);
   sscanf(line, "%s %lf", label, &rmax);

   fgets(line, 1024, fhist);
   sscanf(line, "%s %lf", label, &delta);

   fgets(line, 1024, fhist);
   sscanf(line, "%s %lu", label, &npix);

   fgets(line, 1024, fhist);

   for(i=0; i<256; ++i)
   {
      fgets(line, 1024, fhist);
      sscanf(line, "%s %lf", label, dataval+i);
   }

   fgets(line, 1024, fhist);
   fgets(line, 1024, fhist);

   for(i=0; i<NBIN; ++i)
   {
      fgets(line, 1024, fhist);
      sscanf(line, "%s %lf %d %lf %lf", label, datalev+i, hist+i, chist+i, gausslev+i);
   }

   fclose(fhist);

   return 0;
}



/***********************************/
/* Find the data values associated */
/* with the desired percentile     */
/***********************************/

double mViewer_percentileLevel(double percentile)
{
   int    i, count;
   double percent, maxpercent, minpercent;
   double fraction, value;

   if(percentile <=   0) return rmin;
   if(percentile >= 100) return rmax;

   percent = 0.01 * percentile;

   count = (int)(npix*percent);

   i = 1;
   while(i < nbin+1 && chist[i] < count)
      ++i;
   
   minpercent = (double)chist[i-1] / npix;
   maxpercent = (double)chist[i]   / npix;

   fraction = (percent - minpercent) / (maxpercent - minpercent);

   value = rmin + (i-1+fraction) * delta;

   if(mViewer_debug)
   {
      printf("DEBUG> mViewer_percentileLevel(%-g):\n", percentile);
      printf("DEBUG> percent    = %-g -> count = %d -> bin %d\n",
         percent, count, i);
      printf("DEBUG> minpercent = %-g\n", minpercent);
      printf("DEBUG> maxpercent = %-g\n", maxpercent);
      printf("DEBUG> fraction   = %-g\n", fraction);
      printf("DEBUG> rmin       = %-g\n", rmin);
      printf("DEBUG> delta      = %-g\n", delta);
      printf("DEBUG> value      = %-g\n\n", value);
      fflush(stdout);
   }

   return(value);
}



/***********************************/
/* Find the percentile level       */
/* associated with a data value    */
/***********************************/

double mViewer_valuePercentile(double value)
{
   int    i;
   double maxpercent, minpercent;
   double ival, fraction, percentile;

   if(value <= rmin) return   0.0;
   if(value >= rmax) return 100.0;

   ival = (value - rmin) / delta;

   i = ival;

   fraction = ival - i;

   minpercent = (double)chist[i]   / npix;
   maxpercent = (double)chist[i+1] / npix;

   percentile = 100. *(minpercent * (1. - fraction) + maxpercent * fraction);

   if(mViewer_debug)
   {
      printf("DEBUG> mViewer_valuePercentile(%-g):\n", value);
      printf("DEBUG> rmin       = %-g\n", rmin);
      printf("DEBUG> delta      = %-g\n", delta);
      printf("DEBUG> value      = %-g -> bin %d (fraction %-g)\n",
         value, i, fraction);
      printf("DEBUG> minpercent = %-g\n", minpercent);
      printf("DEBUG> maxpercent = %-g\n", maxpercent);
      printf("DEBUG> percentile = %-g\n\n", percentile);
      fflush(stdout);
   }

   return(percentile);
}




/*************************************/
/* Turn a FITS header block into a   */
/* comment block (newline delimited) */
/*************************************/

int mViewer_fits_comment(char *header, char *comment)
{
   int   i, j, count;
   char *ptr, *end;
   char  line[81];

   ptr = header;
   end = ptr + strlen(header);

   strcpy(comment, "");

   count = 0;

   while(ptr < end)
   {
      for(i=0; i<80; ++i)
      {
         line[i] = *(ptr+i);

         if(ptr+i >= end)
            break;
      }

      line[80] = '\0';

      if(strncmp(line, "NAXIS1", 6) == 0)
         sprintf(line, "NAXIS1  = %d", naxis1);

      if(strncmp(line, "NAXIS2", 6) == 0)
         sprintf(line, "NAXIS2  = %d", naxis2);

      if(strncmp(line, "CRPIX1", 6) == 0)
         sprintf(line, "CRPIX1  = %15.10f", crpix1);

      if(strncmp(line, "CRPIX2", 6) == 0)
         sprintf(line, "CRPIX2  = %15.10f", crpix2);

      /*
      if(strncmp(line, "CDELT1", 6) == 0)
         sprintf(line, "CDELT1  = %15.10f", cdelt1);

      if(strncmp(line, "CDELT2", 6) == 0)
         sprintf(line, "CDELT2  = %15.10f", cdelt2);

      if(strncmp(line, "CROTA2", 6) == 0)
         sprintf(line, "CROTA2  = %15.10f", crota2);

      if(strncmp(line, "CD1_1",  5) == 0)
         sprintf(line, "CD1_1   = %15.10f", cd11);

      if(strncmp(line, "CD1_2",  5) == 0)
         sprintf(line, "CD1_2   = %15.10f", cd12);

      if(strncmp(line, "CD2_1",  5) == 0)
         sprintf(line, "CD2_1   = %15.10f", cd21);

      if(strncmp(line, "CD2_2",  5) == 0)
         sprintf(line, "CD2_2   = %15.10f", cd22);
      */

      for(j=i; j>=0; --j)
      {
         if(line[j] == ' '
         || line[j] == '\0')
            line[j] = '\0';
         else
            break;
      }
      
      strcat(comment, line);
      strcat(comment, "\n");

      count += strlen(line) + 1;

      if(count >= 65000)
      {
         strcat(comment, "END\n");
         break;
      }

      ptr += 80;
   }

   return 0;
}



/*****************************************/
/* Turn FITS header info into a SAMP/AVM */
/* comment block (newline delimited)     */
/*****************************************/

int mViewer_vamp_comment(char *comment)
{
   char  line[1024];

   int    naxis1, naxis2;
   double crval1, crval2;
   double crpix1, crpix2;
   double xinc, yinc;
   double crota2;

   char   proj[64];
   char   csys[64];
   double equinox;

        if(wcs->prjcode == WCS_PIX)  strcpy(proj, "PIX");
   else if(wcs->prjcode == WCS_LIN)  strcpy(proj, "LIN");
   else if(wcs->prjcode == WCS_AZP)  strcpy(proj, "AZP");
   else if(wcs->prjcode == WCS_SZP)  strcpy(proj, "SZP");
   else if(wcs->prjcode == WCS_TAN)  strcpy(proj, "TAN");
   else if(wcs->prjcode == WCS_SIN)  strcpy(proj, "SIN");
   else if(wcs->prjcode == WCS_STG)  strcpy(proj, "STG");
   else if(wcs->prjcode == WCS_ARC)  strcpy(proj, "ARC");
   else if(wcs->prjcode == WCS_ZPN)  strcpy(proj, "ZPN");
   else if(wcs->prjcode == WCS_ZEA)  strcpy(proj, "ZEA");
   else if(wcs->prjcode == WCS_AIR)  strcpy(proj, "AIR");
   else if(wcs->prjcode == WCS_CYP)  strcpy(proj, "CYP");
   else if(wcs->prjcode == WCS_CAR)  strcpy(proj, "CAR");
   else if(wcs->prjcode == WCS_MER)  strcpy(proj, "MER");
   else if(wcs->prjcode == WCS_CEA)  strcpy(proj, "CEA");
   else if(wcs->prjcode == WCS_COP)  strcpy(proj, "COP");
   else if(wcs->prjcode == WCS_COD)  strcpy(proj, "COD");
   else if(wcs->prjcode == WCS_COE)  strcpy(proj, "COE");
   else if(wcs->prjcode == WCS_COO)  strcpy(proj, "COO");
   else if(wcs->prjcode == WCS_BON)  strcpy(proj, "BON");
   else if(wcs->prjcode == WCS_PCO)  strcpy(proj, "PCO");
   else if(wcs->prjcode == WCS_SFL)  strcpy(proj, "SFL");
   else if(wcs->prjcode == WCS_PAR)  strcpy(proj, "PAR");
   else if(wcs->prjcode == WCS_AIT)  strcpy(proj, "AIT");
   else if(wcs->prjcode == WCS_MOL)  strcpy(proj, "MOL");
   else if(wcs->prjcode == WCS_CSC)  strcpy(proj, "CSC");
   else if(wcs->prjcode == WCS_QSC)  strcpy(proj, "QSC");
   else if(wcs->prjcode == WCS_TSC)  strcpy(proj, "TSC");
   else if(wcs->prjcode == WCS_NCP)  strcpy(proj, "NCP");
   else if(wcs->prjcode == WCS_GLS)  strcpy(proj, "GLS");
   else if(wcs->prjcode == WCS_DSS)  strcpy(proj, "DSS");
   else if(wcs->prjcode == WCS_PLT)  strcpy(proj, "PLT");
   else if(wcs->prjcode == WCS_TNX)  strcpy(proj, "TNX");
   else if(wcs->prjcode == WCS_ZPX)  strcpy(proj, "ZPX");
   else if(wcs->prjcode == WCS_TPV)  strcpy(proj, "TPV");
   else if(wcs->prjcode == NWCSTYPE) strcpy(proj, "NWCSTYPE");
   
   naxis1  = wcs->nxpix;
   naxis2  = wcs->nypix;

   crval1  = wcs->crval[0];
   crval2  = wcs->crval[1];

   crpix1  = wcs->xrefpix;
   crpix2  = wcs->yrefpix;

   xinc    = wcs->xinc;
   yinc    = wcs->yinc;
   crota2  = wcs->rot;

   equinox = wcs->equinox;

        if(wcs->syswcs == WCS_J2000)    strcpy(csys, "ICRS");
   else if(wcs->syswcs == WCS_B1950)    strcpy(csys, "FK4");
   else if(wcs->syswcs == WCS_GALACTIC) strcpy(csys, "GAL");
   else if(wcs->syswcs == WCS_ECLIPTIC) strcpy(csys, "ECL");
   else                                 strcpy(csys, "ICRS");

   strcpy(comment, "");

   sprintf(line, "<?xpacket begin=\" \" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\n");
   strcat(comment, line);

   sprintf(line, "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"Adobe XMP Core 4.2-c020 1.124078, Tue Sep 11 2007 23:21:40        \">\n");
   strcat(comment, line);

   sprintf(line, " <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n");                 strcat(comment, line);
   sprintf(line, "  <rdf:Description rdf:about=\"\"\n");                                                    strcat(comment, line);
   sprintf(line, "    xmlns:avm=\"http://www.communicatingastronomy.org/avm/1.0/\">\n");                    strcat(comment, line);
   sprintf(line, "   <avm:MetadataVersion>1.1</avm:MetadataVersion>\n");                                    strcat(comment, line);
   sprintf(line, "   <avm:Type>Observation</avm:Type>\n");                                                  strcat(comment, line);
   sprintf(line, "   <avm:Spatial.Quality>Full</avm:Spatial.Quality>\n");                                   strcat(comment, line);
   sprintf(line, "   <avm:Spatial.CoordinateFrame>%s</avm:Spatial.CoordinateFrame>\n", csys);               strcat(comment, line);
   sprintf(line, "   <avm:Spatial.Equinox>%.1f</avm:Spatial.Equinox>\n", equinox);                          strcat(comment, line);
   sprintf(line, "   <avm:Spatial.CoordsystemProjection>%s</avm:Spatial.CoordsystemProjection>\n", proj);   strcat(comment, line);
   sprintf(line, "   <avm:Spatial.Rotation>%.10e</avm:Spatial.Rotation>\n", crota2);                        strcat(comment, line);
   sprintf(line, "   <avm:Spatial.ReferenceDimension>\n");                                                  strcat(comment, line);
   sprintf(line, "    <rdf:Seq>\n");                                                                        strcat(comment, line);
   sprintf(line, "     <rdf:li>%d</rdf:li>\n", naxis1);                                                     strcat(comment, line);
   sprintf(line, "     <rdf:li>%d</rdf:li>\n", naxis2);                                                     strcat(comment, line);
   sprintf(line, "    </rdf:Seq>\n");                                                                       strcat(comment, line);
   sprintf(line, "   </avm:Spatial.ReferenceDimension>\n");                                                 strcat(comment, line);
   sprintf(line, "   <avm:Spatial.ReferenceValue>\n");                                                      strcat(comment, line);
   sprintf(line, "    <rdf:Seq>\n");                                                                        strcat(comment, line);
   sprintf(line, "     <rdf:li>%.10e</rdf:li>\n", crval1);                                                  strcat(comment, line);
   sprintf(line, "     <rdf:li>%.10e</rdf:li>\n", crval2);                                                  strcat(comment, line);
   sprintf(line, "    </rdf:Seq>\n");                                                                       strcat(comment, line);
   sprintf(line, "   </avm:Spatial.ReferenceValue>\n");                                                     strcat(comment, line);
   sprintf(line, "   <avm:Spatial.ReferencePixel>\n");                                                      strcat(comment, line);
   sprintf(line, "    <rdf:Seq>\n");                                                                        strcat(comment, line);
   sprintf(line, "     <rdf:li>%.10e</rdf:li>\n", crpix1);                                                  strcat(comment, line);
   sprintf(line, "     <rdf:li>%.10e</rdf:li>\n", crpix2);                                                  strcat(comment, line);
   sprintf(line, "    </rdf:Seq>\n");                                                                       strcat(comment, line);
   sprintf(line, "   </avm:Spatial.ReferencePixel>\n");                                                     strcat(comment, line);
   sprintf(line, "   <avm:Spatial.Scale>\n");                                                               strcat(comment, line);
   sprintf(line, "    <rdf:Seq>\n");                                                                        strcat(comment, line);
   sprintf(line, "     <rdf:li>%.10e</rdf:li>\n", xinc);                                                    strcat(comment, line);
   sprintf(line, "     <rdf:li>%.10e</rdf:li>\n", yinc);                                                    strcat(comment, line);
   sprintf(line, "    </rdf:Seq>\n");                                                                       strcat(comment, line);
   sprintf(line, "   </avm:Spatial.Scale>\n");                                                              strcat(comment, line);
   sprintf(line, "  </rdf:Description>\n");                                                                 strcat(comment, line);
   sprintf(line, " </rdf:RDF>\n");                                                                          strcat(comment, line);
   sprintf(line, "</x:xmpmeta>\n");                                                                         strcat(comment, line);
      
   // Padding? 

   sprintf(line, "<?xpacket end=\"r\"?>");
   strcat(comment, line);
   return 0;
}



/*********************************************************************/
/*                                                                   */
/* Wrapper around ERFINV to turn it into the inverse of the          */
/* standard normal probability function.                             */
/*                                                                   */
/*********************************************************************/

double mViewer_snpinv(double p)
{
   if(p > 0.5)
      return( sqrt(2) * mViewer_erfinv(2.*p-1.0));
   else
      return(-sqrt(2) * mViewer_erfinv(1.0-2.*p));
}



/*****************************************************************************/
/*                                                                           */
/* ERFINV: Evaluation of the inverse error function                          */
/*                                                                           */
/*        For 0 <= p <= 1,  w = mViewer_erfinv(p) where erf(w) = p.          */
/*        If either inequality on p is violated then mViewer_erfinv(p)       */
/*        is set to a negative value.                                        */
/*                                                                           */
/*    reference. mathematics of computation,oct.1976,pp.827-830.             */
/*                 j.m.blair,c.a.edwards,j.h.johnson                         */
/*                                                                           */
/*****************************************************************************/

double mViewer_erfinv (double p)
{
   double q, t, v, v1, s, retval;

   /*  c2 = ln(1.e-100)  */

   double c  =  0.5625;
   double c1 =  0.87890625;
   double c2 = -0.2302585092994046e+03;

   double a[6]  = { 0.1400216916161353e+03, -0.7204275515686407e+03,
                    0.1296708621660511e+04, -0.9697932901514031e+03,
                    0.2762427049269425e+03, -0.2012940180552054e+02 };

   double b[6]  = { 0.1291046303114685e+03, -0.7312308064260973e+03,
                    0.1494970492915789e+04, -0.1337793793683419e+04,
                    0.5033747142783567e+03, -0.6220205554529216e+02 };

   double a1[7] = {-0.1690478046781745e+00,  0.3524374318100228e+01,
                   -0.2698143370550352e+02,  0.9340783041018743e+02,
                   -0.1455364428646732e+03,  0.8805852004723659e+02,
                   -0.1349018591231947e+02 };
     
   double b1[7] = {-0.1203221171313429e+00,  0.2684812231556632e+01,
                   -0.2242485268704865e+02,  0.8723495028643494e+02,
                   -0.1604352408444319e+03,  0.1259117982101525e+03,
                   -0.3184861786248824e+02 };

   double a2[9] = { 0.3100808562552958e-04,  0.4097487603011940e-02,
                    0.1214902662897276e+00,  0.1109167694639028e+01,
                    0.3228379855663924e+01,  0.2881691815651599e+01,
                    0.2047972087262996e+01,  0.8545922081972148e+00,
                    0.3551095884622383e-02 };

   double b2[8] = { 0.3100809298564522e-04,  0.4097528678663915e-02,
                    0.1215907800748757e+00,  0.1118627167631696e+01,
                    0.3432363984305290e+01,  0.4140284677116202e+01,
                    0.4119797271272204e+01,  0.2162961962641435e+01 };

   double a3[9] = { 0.3205405422062050e-08,  0.1899479322632128e-05,
                    0.2814223189858532e-03,  0.1370504879067817e-01,
                    0.2268143542005976e+00,  0.1098421959892340e+01,
                    0.6791143397056208e+00, -0.8343341891677210e+00,
                    0.3421951267240343e+00 };

   double b3[6] = { 0.3205405053282398e-08,  0.1899480592260143e-05,
                    0.2814349691098940e-03,  0.1371092249602266e-01,
                    0.2275172815174473e+00,  0.1125348514036959e+01 };


   /* Error conditions */

   q = 1.0 - p;

   if(p < 0.0 || q < 0.0)
     return(-1.e99);


   /* Extremely large value */

   if (q == 0.0)
      return(1.e99);


   /* p between 0 and 0.75 */

   if(p <= 0.75)
   {
      v = p * p - c;

      t = p *  (((((a[5] * v + a[4]) * v + a[3]) * v + a[2]) * v
                             + a[1]) * v + a[0]);

      s = (((((v + b[5]) * v + b[4]) * v + b[3]) * v + b[2]) * v
                             + b[1]) * v + b[0];
    
      retval = t/s;
    
      return(retval);
   }


   /* p between 0.75 and 0.9375 */

   if (p <= 0.9375)
   {
      v = p*p - c1;

      t = p *  ((((((a1[6] * v + a1[5]) * v + a1[4]) * v + a1[3]) * v
                               + a1[2]) * v + a1[1]) * v + a1[0]);

      s = ((((((v + b1[6]) * v + b1[5]) * v + b1[4]) * v + b1[3]) * v
                  + b1[2]) * v + b1[1]) * v + b1[0];
    
      retval = t/s;
    
      return(retval);
   }


   /* q between 1.e-100 and 0.0625 */

   v1 = log(q);

   v  = 1.0/sqrt(-v1);

   if (v1 < c2)
   {
      t = (((((((a2[8] * v + a2[7]) * v + a2[6]) * v + a2[5]) * v + a2[4]) * v
                           + a2[3]) * v + a2[2]) * v + a2[1]) * v + a2[0];

      s = v *  ((((((((  v + b2[7]) * v + b2[6]) * v + b2[5]) * v + b2[4]) * v
                           + b2[3]) * v + b2[2]) * v + b2[1]) * v + b2[0]);
  
      retval = t/s;
    
      return(retval);
   }


   /* q between 1.e-10000 and 1.e-100 */

   t = (((((((a3[8] * v + a3[7]) * v + a3[6]) * v + a3[5]) * v + a3[4]) * v
                        + a3[3]) * v + a3[2]) * v + a3[1]) * v + a3[0];

   s = v *    ((((((  v + b3[5]) * v + b3[4]) * v + b3[3]) * v + b3[2]) * v
                        + b3[1]) * v + b3[0]);
   retval = t/s;

   return retval;
}


/****************************/
/*                          */
/* Pixel-level manipulation */
/*                          */
/****************************/

int mViewer_setPixel(int i, int j, double brightness, double red, double green, double blue, int replace)
{
   int offset;
   int rval, gval, bval;

   int rref = 0;
   int gref = 0;
   int bref = 0;

   int valmag, refmag;

   printf("XXX> setPixel(%d, %d, alpha=%.2f, [%.2f, %.2f, %.2f], %d)\n", 
      i, j, brightness, red, green, blue, replace);
   fflush(stdout);

   if(i < 0 || i >= nx)
      return 0;

   if(j < 0 || j >= ny)
      return 0;

   if(!replace && ovlylock[ny - 1 - j][i] == 1)
      return 0;

   rval = red   * 255;
   gval = green * 255;
   bval = blue  * 255;

   if(outType == JPEG)
   {
      rref = jpegOvly[ny - 1 - j][3*i]; 
      gref = jpegOvly[ny - 1 - j][3*i+1];
      bref = jpegOvly[ny - 1 - j][3*i+2];
   }

   else if(outType == PNG)
   {
      offset = 4 * nx * (ny-1-j) + 4 * i;

      if(brightness > 0)
      {
         rref = pngOvly[offset + 0]; 
         gref = pngOvly[offset + 1];
         bref = pngOvly[offset + 2];
      }
   }

   refmag = (rref + gref + bref) * ovlyweight[ny - 1 - j][i];
   valmag = (rval + gval + bval) * brightness;

   if(!replace && refmag > valmag)
      return 1;

   if(outType == JPEG)
   {
      jpegOvly[ny - 1 - j][3*i]   = rval; 
      jpegOvly[ny - 1 - j][3*i+1] = gval;
      jpegOvly[ny - 1 - j][3*i+2] = bval;
   }

   else if(outType == PNG)
   {
      offset = 4 * nx * (ny-1-j) + 4 * i;

      if(brightness > 0)
      {
         pngOvly[offset + 0] = rval; 
         pngOvly[offset + 1] = gval;
         pngOvly[offset + 2] = bval;
      }
   }

   ovlyweight[ny - 1 - j][i] = brightness;

   return 1;
}



void mViewer_lockPixel(int i, int j)
{
   if(i < 0 || i >= nx)
      return;

   if(j < 0 || j >= ny)
      return;

   ovlylock[ny - 1 - j][i] = 1;
   return;
}



int mViewer_getPixel(int i, int j, int color)
{
   int val;

   if(i < 0 || i >= nx)
      return 0;

   if(j < 0 || j >= ny)
      return 0;

   if(color > 2)
      return 0;

   val = 0;

   if(outType == JPEG)
      val = jpegData[ny - 1 - j][3*i+color];

   else if(outType == PNG)
      val = pngData[4 * nx * (ny - 1 - j) + 4 * i + color];

   return val;
}



/*****************************/
/*                           */
/* Combine overlay and image */
/*                           */
/*****************************/

void mViewer_addOverlay(double alpha)
{
   int    i, j, offset;
   double brightness;

   if(alpha < 0.) alpha = 0.;
   if(alpha > 1.) alpha = 1.;

   for(j=0; j<ny; ++j)
   {
      for(i=0; i<nx; ++i)
      {
         brightness = ovlyweight[j][i] * alpha;

         printf("XXX> addOverlay: (%d %d) [%d %d %d] + [%d %d %d]  weight=%-g alpha=%-g brightness=%-g\n", 
            i, j,
            pngData[offset + 0], pngData[offset + 1], pngData[offset + 2],
            pngOvly[offset + 0], pngOvly[offset + 1], pngOvly[offset + 2],
            ovlyweight[j][i], alpha, brightness);
         fflush(stdout);

         if(outType == JPEG)
         {
            jpegData[j][3*i  ] = brightness * jpegOvly[j][3*i+0] + (1. - brightness) * jpegData[j][3*i  ];
            jpegData[j][3*i+1] = brightness * jpegOvly[j][3*i+1] + (1. - brightness) * jpegData[j][3*i+1];
            jpegData[j][3*i+2] = brightness * jpegOvly[j][3*i+2] + (1. - brightness) * jpegData[j][3*i+2];
         }

         else if(outType == PNG)
         {
            offset = 4 * nx * j + 4 * i;

            if(brightness > 0.)
            {
               pngData[offset + 0] = brightness * pngOvly[offset + 0] + (1. - brightness) * pngData[offset + 0];
               pngData[offset + 1] = brightness * pngOvly[offset + 1] + (1. - brightness) * pngData[offset + 1];
               pngData[offset + 2] = brightness * pngOvly[offset + 2] + (1. - brightness) * pngData[offset + 2];

               pngData[offset + 3] = (brightness + (1. - brightness) * pngData[offset + 3]/255.) * 255.;
            }
         }

         ovlyweight[j][i] = 0.;
         ovlylock  [j][i] = 0;
      }
   }
}


/******************************************/
/*                                        */
/* Draw a label along a latitude line     */
/* and centered at a specified coordinate */
/*                                        */
/******************************************/

void mViewer_coord_label(char *face_path, int fontsize, 
                         double lonlab, double latlab, char *label,
                         int csysimg, double epochimg, int csysgrid, double epochgrid,
                         double red, double green, double blue)
{
   int     ii, offscl, convert;

   double  dlon, dpix;
   double  lon, lat;
   double  xval, yval;
   double  xprev, yprev;
   double  reflat, reflon;
   double  lablen, laboffset;

   double *xlab;
   double *ylab;
   int     nlab;

   if(mViewer_debug)
   {
      printf("DEBUG> mViewer_coord_label(\"%s\", %d, %-g, %-g, \"%s\", %d, %-g, %d, %-g, %-g, %-g, %-g)\n",
              face_path, fontsize, 
              lonlab, latlab, label,
              csysimg, epochimg, csysgrid, epochgrid,
              red, green, blue);

      fflush(stdout);
   }

   convert = 0;

   if(csysgrid != csysimg || epochgrid != epochimg)
      convert = 1;

   dlon = fabs(cdelt2);

   lablen = mViewer_label_length(face_path, fontsize, label);

   lon = lonlab;
   lat = latlab;


   // Find the XY location of the label center 

   reflon = lon;
   reflat = lat;

   if(convert)
      convertCoordinates(csysgrid, epochgrid,  lon, lat,
                          csysimg,  epochimg, &reflon, &reflat, 0.0);

   offscl = 0;
   wcs2pix(wcs, reflon, reflat, &xprev, &yprev, &offscl);

   if(hpx) mViewer_hpxCheck(&offscl, &xprev, &yprev);

   if(offscl || mNaN(xprev) || mNaN(yprev))
      return;

   if(!flipY || wcs->imflip)
      yprev = wcs->nypix - yprev;


   // Find the XY of a small offset from this in longitude

   lon -= dlon;

   reflon = lon;
   reflat = lat;

   if(convert)
      convertCoordinates(csysgrid, epochgrid,  lon, lat,
                          csysimg,  epochimg, &reflon, &reflat, 0.0);

   offscl = 0;
   wcs2pix(wcs, reflon, reflat, &xval, &yval, &offscl);

   if(hpx) mViewer_hpxCheck(&offscl, &xval, &yval);

   if(offscl || mNaN(xval) || mNaN(yval))
      return;

   if(!flipY || wcs->imflip)
      yval = wcs->nypix - yval;


   // Compare the two to determine which direction gives
   // correctly oriented characters

   if(xprev < xval)
      dlon = -dlon;


   // Starting again at the label center, move in longitude in small steps 
   // until we have gone half the length of the string (this is where 
   // we want to start drawing the string).

   lon = lonlab;

   laboffset = 0.;

   xprev = xval;
   yprev = yval;

   while(1)
   {
      lon -= dlon;

      reflon = lon;
      reflat = lat;

      if(convert)
         convertCoordinates(csysgrid, epochgrid,  lon, lat,
                             csysimg,  epochimg, &reflon, &reflat, 0.0);

      offscl = 0;
      wcs2pix(wcs, reflon, reflat, &xval, &yval, &offscl);

      if(hpx) mViewer_hpxCheck(&offscl, &xval, &yval);

      if(!flipY || wcs->imflip)
         yval = wcs->nypix - yval;

      dpix = sqrt((xval-xprev)*(xval-xprev) + (yval-yprev)*(yval-yprev));

      laboffset += dpix;

      if(offscl || mNaN(xval) || mNaN(yval))
         break;

      if(laboffset > lablen/2.)
      {
         xprev = xval;
         yprev = yval;

         break;
      }

      xprev = xval;
      yprev = yval;
   }


   // Create an array of XY points, starting at the start point
   // we just found and going far enough to define the full arc
   // of the string we want to draw.

   xlab = (double *)malloc(NLAB * sizeof(double));
   ylab = (double *)malloc(NLAB * sizeof(double));
   nlab = NLAB;

   ii = 0;

   xlab[ii] = xval;
   ylab[ii] = yval;

   laboffset = 0.;

   while(1)
   {
      lon += dlon;

      reflon = lon;
      reflat = lat;

      if(convert)
         convertCoordinates(csysgrid, epochgrid,  lon, lat,
                             csysimg,  epochimg, &reflon, &reflat, 0.0);

      offscl = 0;
      wcs2pix(wcs, reflon, reflat, &xval, &yval, &offscl);

      if(hpx) mViewer_hpxCheck(&offscl, &xval, &yval);

      if(!flipY || wcs->imflip)
         yval = wcs->nypix - yval;

      laboffset += sqrt((xval-xprev)*(xval-xprev) + (yval-yprev)*(yval-yprev));

      if(offscl || mNaN(xval) || mNaN(yval))
         break;

      if(laboffset > lablen && ii > 1)
         break;

      xlab[ii] = xval;
      ylab[ii] = yval;
      ++ii;

      if(ii >= nlab)
      {
         nlab += NLAB;
         xlab = (double *) realloc(xlab, nlab*sizeof(double));
         ylab = (double *) realloc(ylab, nlab*sizeof(double));
      }

      xprev = xval;
      yprev = yval;
   }


   // Use this arc of points to control placement of the string characters

   mViewer_labeledCurve(face_path, fontsize, 0, xlab, ylab, ii, label, 0., red, green, blue, 1);

   free(xlab);
   free(ylab);
}



/*************************/
/*                       */
/* Draw a longitude line */
/*                       */
/*************************/

void mViewer_longitude_line(double lon, double latmin, double latmax, 
                            int csysimg, double epochimg, int csysgrid, double epochgrid,
                            double red, double green, double blue, double linewidth)
{
   int     ii, offscl, convert;

   double  dlat;
   double  lat;
   double  xval, yval;
   double  xprev, yprev;
   double  reflat, reflon;

   double *xlin;
   double *ylin;
   int     nlin;

   if(mViewer_debug)
   {
      printf("mViewer_longitude_line(%-g, %-g, %-g, %d, %-g, %d, %-g, %-g, %-g, %-g, %-g)\n",
              lon, latmin, latmax, 
              csysimg, epochimg, csysgrid, epochgrid,
              red, green, blue, linewidth);
      fflush(stdout);
   }

   convert = 0;

   if((csysgrid != csysimg) || (epochgrid != epochimg))
      convert = 1;

   xlin = (double *)malloc(NLIN * sizeof(double));
   ylin = (double *)malloc(NLIN * sizeof(double));
   nlin = NLIN;

   xprev = -1;
   yprev = -1;

   dlat = fabs(cdelt2) * 0.5;

   lat = latmin;

   reflon = lon;
   reflat = lat;

   if(convert)
      convertCoordinates(csysgrid, epochgrid,  lon, lat,
                          csysimg,  epochimg, &reflon, &reflat, 0.0);

   ii = 0;
   offscl = 0;
   wcs2pix(wcs, reflon, reflat, &xval, &yval, &offscl);

   if(hpx) mViewer_hpxCheck(&offscl, &xval, &yval);

   if(!flipY || wcs->imflip)
      yval = wcs->nypix - yval;

   if((!offscl) && (!mNaN(xval)) && (!mNaN(yval)))
   {
      xlin[ii] = xval;
      ylin[ii] = yval;

      ++ii;

      xprev = xval;
      yprev = yval;
   }

   while(1)
   {
      lat += dlat;

      if(lat > latmax)
         break;

      reflon = lon;
      reflat = lat;

      if(convert)
         convertCoordinates(csysgrid, epochgrid,  lon, lat,
                             csysimg,  epochimg, &reflon, &reflat, 0.0);

      offscl = 0;
      wcs2pix(wcs, reflon, reflat, &xval, &yval, &offscl);

      if(hpx) mViewer_hpxCheck(&offscl, &xval, &yval);

      if(!flipY || wcs->imflip)
         yval = wcs->nypix - yval;

      if((offscl > 0 || mNaN(xval) || mNaN(yval)) && ii > 1)
      {
         mViewer_curve(xlin, ylin, ii, red, green, blue, linewidth);
         
         ii = 0;

         xprev = -1;
         yprev = -1;
      }
      else if(offscl == 0)
      {
         if(xval != xprev || yval != yprev)
         {
            xlin[ii] = xval;
            ylin[ii] = yval;

            ++ii;

            if(ii >= nlin)
            {
               nlin += NLIN;
               xlin = (double *) realloc(xlin, nlin*sizeof(double));
               ylin = (double *) realloc(ylin, nlin*sizeof(double));
            }

            xprev = xval;
            yprev = yval;
         }
      }
   }

   if(ii > 0)
      mViewer_curve(xlin, ylin, ii, red, green, blue, linewidth);

   free(xlin);
   free(ylin);
}



/************************/
/*                      */
/* Draw a latitude line */
/*                      */
/************************/

void mViewer_latitude_line(double lat, double lonmin, double lonmax, 
                           int csysimg, double epochimg, int csysgrid, double epochgrid,
                           double red, double green, double blue, double linewidth)
{
   int     ii, offscl, convert;

   double  dlon;
   double  lon;
   double  xval, yval;
   double  xprev, yprev;
   double  reflat, reflon;

   double *xlin;
   double *ylin;
   int     nlin;

   if(mViewer_debug)
   {
      printf("mViewer_latitude_line(%-g, %-g, %-g, %d, %-g, %d, %-g, %-g, %-g, %-g, %-g)\n",
              lat, lonmin, lonmax, 
              csysimg, epochimg, csysgrid, epochgrid,
              red, green, blue, linewidth);
      fflush(stdout);
   }

   printf("\n\n\nXXX> LATITUDE_LINE\n");
   fflush(stdout);


   if(lat >= 90. || lat <= -90.)
      return;

   convert = 0;

   if((csysgrid != csysimg) || (epochgrid != epochimg))
      convert = 1;

   xlin = (double *)malloc(NLIN * sizeof(double));
   ylin = (double *)malloc(NLIN * sizeof(double));
   nlin = NLIN;

   xprev = -1;
   yprev = -1;

   dlon = fabs(cdelt2) * 0.5;

   lon = lonmin;

   reflon = lon;
   reflat = lat;

   if(convert)
      convertCoordinates(csysgrid, epochgrid,  lon, lat,
                          csysimg,  epochimg, &reflon, &reflat, 0.0);

   ii = 0;
   offscl = 0;
   wcs2pix(wcs, reflon, reflat, &xval, &yval, &offscl);

   if(hpx) mViewer_hpxCheck(&offscl, &xval, &yval);

   if(!flipY || wcs->imflip)
      yval = wcs->nypix - yval;

   if((!offscl) && (!mNaN(xval)) && (!mNaN(yval)))
   {
      xlin[ii] = xval;
      ylin[ii] = yval;

      ++ii;

      xprev = xval;
      yprev = yval;
   }

   while(1)
   {
      lon += dlon;

      reflon = lon;
      reflat = lat;

      if(convert)
         convertCoordinates(csysgrid, epochgrid,  lon, lat,
                             csysimg,  epochimg, &reflon, &reflat, 0.0);

      offscl = 0;
      wcs2pix(wcs, reflon, reflat, &xval, &yval, &offscl);

      if(hpx) mViewer_hpxCheck(&offscl, &xval, &yval);

      if(!flipY || wcs->imflip)
         yval = wcs->nypix - yval;


      if((offscl > 0 || mNaN(xval) || mNaN(xval)) && ii > 1)
      {
         mViewer_curve(xlin, ylin, ii, red, green, blue, linewidth);
         
         ii = 0;

         xprev = -1;
         yprev = -1;
      }
      else if(offscl == 0)
      {
         if(xval != xprev || yval != yprev)
         {
            xlin[ii] = xval;
            ylin[ii] = yval;

            ++ii;

            if(ii >= nlin)
            {
               nlin += NLIN;
               xlin = (double *) realloc(xlin, nlin*sizeof(double));
               ylin = (double *) realloc(ylin, nlin*sizeof(double));
            }

            xprev = xval;
            yprev = yval;
         }
      }

      if(lon > lonmax)
         break;
   }

   if(ii > 0)
      mViewer_curve(xlin, ylin, ii, red, green, blue, linewidth);

   free(xlin);
   free(ylin);
}


/***********************************************/
/*                                             */
/* Draw a boundary around an Aitoff projection */
/* (actually any 2:1 ellipse projection but    */
/*  Aitoff is the only one we check for).      */
/*                                             */
/***********************************************/

void mViewer_draw_boundary(double red, double green, double blue, double linewidth)
{
   int     ii, offscl, side;

   double  dlat;
   double  lat, lon;
   double  latmin, latmax;
   double  xval, yval;
   double  xprev, yprev;
   double  reflat, reflon;

   double *xlin;
   double *ylin;
   int     nlin;

   if(mViewer_debug)
   {
      printf("mViewer_draw_boundary(%-g, %-g, %-g)\n", red, green, blue);
      fflush(stdout);
   }

   xlin = (double *)malloc(NLIN * sizeof(double));
   ylin = (double *)malloc(NLIN * sizeof(double));
   nlin = NLIN;

   latmin = -90.;
   latmax =  90.;

   lon = wcs->crval[0] + 180.;

   while(lon > 360.) lon -= 360.;
   while(lon <   0.) lon += 360.;

   for(side=0; side<2; ++side)
   {
      xprev = -1;
      yprev = -1;

      dlat = fabs(cdelt2) * 0.5;

      lat = latmin;

      reflon = lon;
      reflat = lat;

      ii = 0;
      offscl = 0;
      wcs2pix(wcs, reflon, reflat, &xval, &yval, &offscl);

      if(hpx) mViewer_hpxCheck(&offscl, &xval, &yval);

      if(!flipY || wcs->imflip)
         yval = wcs->nypix - yval;

      if((!offscl) && (!mNaN(xval)) && (!mNaN(yval)))
      {
         xlin[ii] = xval;
         ylin[ii] = yval;

         ++ii;

         xprev = xval;
         yprev = yval;
      }

      while(1)
      {
         lat += dlat;

         if(lat > latmax)
            break;

         reflon = lon;
         reflat = lat;

         offscl = 0;
         wcs2pix(wcs, reflon, reflat, &xval, &yval, &offscl);

         if(hpx) mViewer_hpxCheck(&offscl, &xval, &yval);

         if(!flipY || wcs->imflip)
            yval = wcs->nypix - yval;

         if(side == 1)
            xval = wcs->nxpix - xval;

         if((offscl > 0 || mNaN(xval) || mNaN(yval)) && ii > 1)
         {
            mViewer_curve(xlin, ylin, ii, red, green, blue, linewidth);

            ii = 0;

            xprev = -1;
            yprev = -1;
         }
         else if(offscl == 0)
         {
            if(xval != xprev || yval != yprev)
            {
               xlin[ii] = xval;
               ylin[ii] = yval;

               ++ii;

               if(ii >= nlin)
               {
                  nlin += NLIN;
                  xlin = (double *) realloc(xlin, nlin*sizeof(double));
                  ylin = (double *) realloc(ylin, nlin*sizeof(double));
               }

               xprev = xval;
               yprev = yval;
            }
         }
      }

      if(ii > 0)
         mViewer_curve(xlin, ylin, ii, red, green, blue, linewidth);
   }

   free(xlin);
   free(ylin);
}


/****************/
/*              */
/* Draw a label */
/*              */
/****************/

void mViewer_draw_label(char *fontfile, int fontsize, 
                        int xlab, int ylab, char *text, 
                        double red, double green, double blue)
{
   int     i;
   double  lablen;
   double *xcurve, *ycurve;

   xcurve = (double *)malloc(nx * sizeof(double));
   ycurve = (double *)malloc(nx * sizeof(double));

   for(i=0; i<nx; ++i)
   {
      xcurve[i] = (double)i;
      ycurve[i] = (double)ylab;
   }

   lablen = mViewer_label_length(fontfile, 8, text);

   mViewer_labeledCurve(fontfile, fontsize, 0, xcurve, ycurve, nx, text, (double)xlab - lablen/2., red, green, blue, 1);

   free(xcurve);
   free(ycurve);
}


int mViewer_writePNG(const char* filename, const unsigned char* image, unsigned w, unsigned h, char *comment)
{
   LodePNGColorType colortype;
   unsigned         bitdepth;
   unsigned char*   buffer;
   size_t           buffersize;
   unsigned         error;
   LodePNGState     state;

   colortype = LCT_RGBA;
   bitdepth  = 8;

   lodepng_state_init(&state);

   state.info_raw.colortype       = colortype;
   state.info_raw.bitdepth        = bitdepth;
   state.info_png.color.colortype = colortype;
   state.info_png.color.bitdepth  = bitdepth;

   // We have a couple of routines for generating comment text but are not using them right now.
   // lodepng_add_itext(&state.info_png, "XML:com.adobe.xmp", "", "", comment);

   lodepng_encode(&buffer, &buffersize, image, w, h, &state);

   error = state.error;

   lodepng_state_cleanup(&state);

   if(!error)
      error = lodepng_save_file(buffer, buffersize, filename);

   free(buffer);

   return error;
}


void mViewer_parseCoordStr(char *coordStr, int *csys, double *epoch)
{
   int   cmdc, ref;
   char *cmdv[256];

   cmdc = parsecmd(coordStr, cmdv);

   *csys  = EQUJ;
   *epoch = -999.;
   ref    = JULIAN;

   if(cmdc > 1)
   {
      if(cmdv[1][0] == 'j' || cmdv[1][0] == 'J')
      {
         ref = JULIAN;
         *epoch = atof(cmdv[1]+1);
      }

      else if(cmdv[1][0] == 'b' || cmdv[1][0] == 'B')
      {
         ref = BESSELIAN;
         *epoch = atof(cmdv[1]+1);
      }
   }

   if(strncasecmp(cmdv[0], "eq", 2) == 0)
   {
      if(ref == BESSELIAN)
         *csys = EQUB;
      else
         *csys = EQUJ;
   }

   else if(strncasecmp(cmdv[0], "ec", 2) == 0)
   {
      if(ref == BESSELIAN)
         *csys = ECLB;
      else
         *csys = ECLJ;
   }

   else if(strncasecmp(cmdv[0], "ga", 2) == 0)
      *csys = GAL;

   if(*epoch == -999.)
      *epoch = 2000.;

   return;
}


void mViewer_hpxCheck(int *offscl, double *x, double *y)
{
   *offscl = 0;

   if(*x < -(double)hpxPix/2.) *x += (double)hpxPix;
   if(*x >  (double)hpxPix/2.) *x -= (double)hpxPix;

   if(*y < -(double)hpxPix/2.) *y += (double)hpxPix;
   if(*y >  (double)hpxPix/2.) *y -= (double)hpxPix;
}
