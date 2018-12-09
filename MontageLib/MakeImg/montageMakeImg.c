/* Module: mMakeImg.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
2.0      John Good        17Nov14  Cleanup to avoid compiler warnings, in proparation
                                   for new development cycle.
1.4      John Good        20Jul07  Add checks for 'short' image sides
1.3      John Good        24Jun07  Need fix for CAR offset problem 
1.2      John Good        13Oct06  Add 'region' and 'replace' modes
1.1      John Good        13Oct04  Changed format for printing time
1.0      John Good        11Sep03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <time.h>

#include <fitsio.h>
#include <coord.h>
#include <wcs.h>
#include <mtbl.h>
#include <cmd.h>
#include <json.h>

#include <mMakeImg.h>
#include <montage.h>

int       isJSON;

double    noise;
double    bg1;
double    bg2;
double    bg3;
double    bg4;
int       ncat;
char    **cat_file;
char    **colname;
double   *width;
int      *flat;
double   *ref;
int      *ismag;
int      *sys;
double   *epoch;
int       nimage;
char   **image_file;
double   *refval;
char    *arrayfile;
int       replace;

static struct
{
   fitsfile         *fptr;
   long              naxes[2];
   struct WorldCoor *wcs;
   int               sys;
   double            epoch;
}
   output;

static double pixscale;

static double xcorrection;
static double ycorrection;

typedef struct vec
{
   double x;
   double y;
   double z;
}
Vec;

static int     Cross    (Vec *a, Vec *b, Vec *c);
static double  Dot      (Vec *a, Vec *b);
static double  Normalize(Vec *a);

static char montage_msgstr[1024];
static char montage_json  [1024];

static int mMakeImg_debug;


/*-****************************************************************************************/
/*                                                                                        */
/*  mMakeImg -- A point source image generation program                                   */
/*                                                                                        */
/*  A general output FITS image is defined and its pixels are then populated from a table */
/*  of point sources.  The source fluxes from the table are distributed based on a        */
/*  source-specific point-spread function.                                                */
/*                                                                                        */
/*   char     *template_file Header template describing the image (and to be part of it). */
/*   char     *output_file   Output FITS file.                                            */
/*                                                                                        */
/*   char     *layout        Command string or JSON string.  If given, overrides all the  */
/*                           parameters below except 'debug' since the JSON covers all    */
/*                           the same parameters. If null or an empty string, we expect   */
/*                           at least some of the below. JSON and parameter mode can't    */
/*                           be mixed.                                                    */
/*                                                                                        */
/*   int       debug         Debugging flag.                                              */
/*                                                                                        */
/*  The parsed command string / JSON contains some subset of the following parameters:    */
/*                                                                                        */
/*   double    noise         Additive noise level.                                        */
/*   double    bg1           Background value for pixel (1,1).                            */
/*   double    bg2           Background value for pixel (NAXIS1, 1).                      */
/*   double    bg3           Background value for pixel (NAXIS1, NAXIS2).                 */
/*   double    bg4           Background value for pixel (1, NAXIS2).                      */
/*                                                                                        */
/*   int       ncat          Table file(s) with coordinates and source magnitudes.        */
/*   char    **cat_file      Table file(s) with coordinates and source magnitudes.        */
/*   char    **colname       Magnitude column in cat_file.                                */
/*   double   *width         'PSF' (gaussian) width for catalog sources.                  */
/*   int      *flat          Use uniform brightness rather than gaussian drop-off.        */
/*   double   *ref           Reference magnitude for scaling catalog sources.             */
/*   int      *ismag         Boolean: Are the reference value and values magnitudes.      */
/*   int      *sys           Coordinate system ID (EQUJ:0, EQUB:1, ECLJ:2, ECLB:3, GAL:4, */
/*                           SGAL:5)                                                      */
/*   double   *epoch         Epoch for coordinates in catalog table.                      */
/*                                                                                        */
/*   int       nimage        Table file(s) with coordinates and source magnitudes.        */
/*   char    **image_file    Image metadata (four corners) tables (region fill).          */
/*   double   *refval        Fill values to use for image areas.                          */
/*                                                                                        */
/*   char     *arrayfile     ASCII file with pixel value array.                           */
/*                                                                                        */
/*   int       replace       Boolean: if true replace pixel values instead of adding      */
/*                           (applies to both catalogs and image lists).                  */
/*                                                                                        */
/******************************************************************************************/

struct mMakeImgReturn  *mMakeImg(char *template_file, char *output_file, char *layout, int mode, int debugin)
{
   int       i, j, jnext, k, l, m, count, ncol;
   int       dl, dm, inext, ndataset;
   int       loncol, latcol, fluxcol;
   int       imgcnt, srccnt, index;
   long      fpixel[4], nelements;
   double    oxpix, oypix;
   int       ifile, offscl;
   double    dx, dy, dist2, width2, weight;
   double    noise_level, randnum;
   double    x, y, background;

   double    tolerance  = 0.0000277778;
   int       nShortSide;
   int       shortSide[4];

   double    ilon, ilat;
   double    olon, olat;
   double    ref_lon[5], ref_lat[5];
   double    xref[5], yref[5], zref[5];
   double    xarc, yarc, zarc;

   double    pixel_value;

   double    theta0, theta, alpha, a, A;

   double  **data;

   int       status = 0;

   int       endline, stat;

   char      valstr  [STRLEN];
   char      keystr  [STRLEN];
   char      dataType[STRLEN];
   char      csys    [STRLEN];
   char      usage   [STRLEN];
   char      coordStr[STRLEN];

   int       bitpix = DOUBLE_IMG; 
   long      naxis  = 2;  

   double    imin, imax;
   double    jmin, jmax;
   double    xpos, ypos;
   double    ix, iy;
   double    ixtest, iytest;

   int       ira [5];
   int       idec[5];
   double    ra  [5];
   double    dec [5];
   double    ipix;
   double    jpix;

   int       npix;

   Vec       image_corner[4];
   Vec       image_normal[4];

   Vec       normal1, normal2, direction, pixel_loc;

   int       clockwise, interior;

   double    dtr;

   char     *end;
   FILE     *farray;

   int       argc;
   char     *argv[4096];

   JSON     *sv;

   struct mMakeImgReturn *returnStruct;


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

   dtr = atan(1.)/45.;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mMakeImgReturn *)malloc(sizeof(struct mMakeImgReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   mMakeImg_debug = debugin;


   /****************************************************/ 
   /* If the JSON parameter structure is given, ignore */ 
   /* any other paramters sent and repopulate the data */ 
   /* arrays from the JSON.                            */ 
   /****************************************************/ 

   cat_file   = (char **)malloc(MAXFILE*sizeof(char *));
   image_file = (char **)malloc(MAXFILE*sizeof(char *));
   colname    = (char **)malloc(MAXFILE*sizeof(char *));

   for(i=0; i<MAXFILE; ++i)
   {
      cat_file[i]   = (char *)malloc(STRLEN*sizeof(char));
      image_file[i] = (char *)malloc(STRLEN*sizeof(char));
      colname[i]    = (char *)malloc(STRLEN*sizeof(char));
   }

   width          = (double *)malloc(MAXFILE * sizeof(double));
   flat           = (int    *)malloc(MAXFILE * sizeof(int));
   ref            = (double *)malloc(MAXFILE * sizeof(double));
   ismag          = (int    *)malloc(MAXFILE * sizeof(int));
   sys            = (int    *)malloc(MAXFILE * sizeof(int));
   epoch          = (double *)malloc(MAXFILE * sizeof(double));
   refval         = (double *)malloc(MAXFILE * sizeof(double));

   arrayfile      = (char   *)malloc(STRLEN * sizeof(char));

   strcpy(usage, "Usage: mMakeImg [-d level] [-r(eplace)] [-n noise_level] [-b bg1 bg2 bg3 bg4] [-t tblfile col width csys epoch refval mag/flux flat/gaussian] [-i imagetbl refval] [-a array.txt] template.hdr out.fits (-t and -i args can be repeated)");


   if(mode == CMDMODE)
   {
      ncat     = 0;
      nimage   = 0;

      srccnt   = 0;
      imgcnt   = 0;

      argc = parsecmd(layout, argv);

      
      index = 0;

      while(1)
      {
         if(index >= argc)
            break;

         if(argv[index][0] == '-')
         {
            if(strlen(argv[index]) < 2)
            {
               strcpy(returnStruct->msg, usage);
               return returnStruct;
            }

            switch(argv[index][1])
            {
               case 'd':
                  index += 2;
                  break;


               case 'r':
                  replace = 1;

                  ++index;

                  break;


               case 'a':
                  if(argc < index+2)
                  {
                     strcpy(returnStruct->msg, usage);
                     return returnStruct;
                  }

                  strcpy(arrayfile, argv[index+1]);

                  index += 2;

                  break;


               case 'n':
                  if(argc < index+2)
                  {
                     strcpy(returnStruct->msg, usage);
                     return returnStruct;
                  }

                  noise = atof(argv[index+1]);

                  index += 2;

                  break;


               case 'b':
                  if(argc < index+5)
                  {
                     strcpy(returnStruct->msg, usage);
                     return returnStruct;
                  }

                  bg1 = atof(argv[index+1]);
                  bg2 = atof(argv[index+2]);
                  bg3 = atof(argv[index+3]);
                  bg4 = atof(argv[index+4]);

                  index += 5;

                  break;


               case 't':
                  if(argc < index+9)
                  {
                     strcpy(returnStruct->msg, usage);
                     return returnStruct;
                  }

                  strcpy(cat_file[ncat], argv[index+1]);
                  strcpy(colname [ncat], argv[index+2]);

                  width[ncat] = atof(argv[index+3]);
                  
                  strcpy(coordStr, argv[index+4]);
                  strcat(coordStr, " ");
                  strcat(coordStr, argv[index+5]);

                  mMakeImg_parseCoordStr(coordStr, &sys[ncat], &epoch[ncat]);

                  ref[ncat] = atof(argv[index+6]);

                  ismag[ncat] = 0;
                  if(strcasecmp(argv[index+7], "mag") == 0)
                     ismag[ncat] = 1;

                  flat[ncat] = 0;
                  if(strcasecmp(argv[index+8], "flat") == 0)
                     flat[ncat] = 1;

                  index += 9;

                  ++ncat;

                  break;
                  

               case 'i':
                  if(argc < index+3)
                  {
                     strcpy(returnStruct->msg, usage);
                     return returnStruct;
                  }

                  strcpy(image_file[nimage], argv[index+1]);

                  refval[nimage] = atof(argv[index+2]);

                  index += 3;

                  ++nimage;

                  break;

                  
               default:
                  strcpy(returnStruct->msg, usage);
                  return returnStruct;
                  break;
            }
         }

         else
         {
            strcpy(returnStruct->msg, usage);
            return returnStruct;
         }
      }
   }

   if(mode == JSONMODE)
   {
      if((sv = json_struct(layout)) == (JSON *)NULL)
      {
         mMakeImg_cleanup();
         strcpy(returnStruct->msg, "Invalid JSON structure.");
         return returnStruct;
      }


      // Noise level and background values at four corners
     
      noise = 0.;

      if(json_val(layout, "background.noise", valstr))
      {
         noise = strtod(valstr, &end);

         if(end < valstr+strlen(valstr) || noise < 0.)
         {
            mMakeImg_cleanup();
            strcpy(returnStruct->msg, "Noise level parameter must a number greater than zero.");
            return returnStruct;
         }
      }


      bg1 = 0.;

      if(json_val(layout, "background.bg11", valstr))
      {
         bg1 = strtod(valstr, &end);

         if(end < valstr+strlen(valstr))
         {
            mMakeImg_cleanup();
            strcpy(returnStruct->msg, "Background levels must numbers.");
            return returnStruct;
         }
      }


      bg2 = 0.;

      if(json_val(layout, "background.bg1N", valstr))
      {
         bg2 = strtod(valstr, &end);

         if(end < valstr+strlen(valstr))
         {
            mMakeImg_cleanup();
            strcpy(returnStruct->msg, "Background levels must numbers.");
            return returnStruct;
         }
      }


      bg3 = 0.;

      if(json_val(layout, "background.bgNN", valstr))
      {
         bg3 = strtod(valstr, &end);

         if(end < valstr+strlen(valstr))
         {
            mMakeImg_cleanup();
            strcpy(returnStruct->msg, "Background levels must numbers.");
            return returnStruct;
         }
      }


      bg4 = 0.;

      if(json_val(layout, "background.bgN1", valstr))
      {
         bg4 = strtod(valstr, &end);

         if(end < valstr+strlen(valstr))
         {
            mMakeImg_cleanup();
            strcpy(returnStruct->msg, "Background levels must numbers.");
            return returnStruct;
         }
      }


      // Datasets

      ndataset = 0;
      ncat     = 0;
      nimage   = 0;

      srccnt   = 0;
      imgcnt   = 0;

      while(1)
      {
         sprintf(keystr, "datasets[%d]", ndataset);

         if(json_val(layout, keystr, valstr) == (char *)NULL)
            break;

         sprintf(keystr, "datasets[%d].type", ndataset);

         if(json_val(layout, keystr, dataType) == (char *)NULL)
         {
            sprintf(returnStruct->msg, "Dataset %d has no 'type' attribute.", ndataset);
            return returnStruct;
         }

         if(strcmp(dataType, "catalog") == 0)
         {
            sprintf(keystr, "datasets[%d].file", ndataset);  // Catalog file

            if(json_val(layout, keystr, valstr))
               strcpy(cat_file[ncat], valstr);
            else
            {
               mMakeImg_cleanup();
               strcpy(returnStruct->msg, "No file name given for catalog.");
               return returnStruct;
            }


            sprintf(keystr, "datasets[%d].column", ndataset);  // Flux/mag column name

            if(json_val(layout, keystr, valstr))
               strcpy(colname[ncat], valstr);
            else
            {
               mMakeImg_cleanup();
               strcpy(returnStruct->msg, "No column name given for catalog.");
               return returnStruct;
            }


            sprintf(keystr, "datasets[%d].width", ndataset);  // Source width

            width[ncat] = 1.;

            if(json_val(layout, keystr, valstr))
               width[ncat] = atof(valstr);

            if(width[ncat] <= 0.)
               width[ncat] = 1.0;


            sprintf(keystr, "datasets[%d].shape", ndataset);  // Source shape

            flat[ncat] = 0;

            if(json_val(layout, keystr, valstr))
            {
               if(strcasecmp(valstr, "flat") == 0)
                  flat[ncat] = 1;

               else if(strcasecmp(valstr, "gaussian") == 0)
                  flat[ncat] = 0;

               else
               {
                  mMakeImg_cleanup();
                  strcpy(returnStruct->msg, "Shape parameter must be 'flat' or 'gaussian'.");
                  return returnStruct;
               }
            }


            sprintf(keystr, "datasets[%d].refval", ndataset);  // Reference data value

            ref[ncat] = 1.;

            if(json_val(layout, keystr, valstr))
               ref[ncat] = atof(valstr);

            if(ref[ncat] <= 0.)
               ref[ncat] = 1.0;


            sprintf(keystr, "datasets[%d].mode", ndataset);  // Flux mode   

            ismag[ncat] = 1;

            if(json_val(layout, keystr, valstr))
            {
               if(strncasecmp(valstr, "mag", 3) == 0)
                  ismag[ncat] = 1;

               else if(strcasecmp(valstr, "flux") == 0)
                  ismag[ncat] = 0;
            }


            sprintf(keystr, "datasets[%d].csys", ndataset);  // Coordinate system   

            strcpy(csys, "EQU J2000");

            sys  [ncat] = EQUJ;
            epoch[ncat] = 2000.;

            if(json_val(layout, keystr, valstr))
               strcpy(csys, valstr);

            mMakeImg_parseCoordStr(csys, &sys[ncat], &epoch[ncat]);

            ++ncat;
         }

         if(strcmp(dataType, "imginfo") == 0)
         {
            sprintf(keystr, "datasets[%d].file", ndataset);  // Catalog file

            if(json_val(layout, keystr, valstr))
               strcpy(image_file[nimage], valstr);
            else
            {
               mMakeImg_cleanup();
               strcpy(returnStruct->msg, "No file name given for catalog.");
               return returnStruct;
            }


            sprintf(keystr, "datasets[%d].refval", ndataset);  // Reference data value

            refval[nimage] = 1.;

            if(json_val(layout, keystr, valstr))
               refval[nimage] = atof(valstr);

            if(refval[nimage] <= 0.)
               refval[nimage] = 1.0;

            ++nimage;
         }

         ++ndataset;
      }


      // Data values array from ASCII file
     
      strcpy(valstr, "");

      if(json_val(layout, "arrayfile", valstr))
         strcpy(arrayfile, valstr);


      // Replace with new pixel value rather than add
      
      replace = 0;

      if(json_val(layout, "replace", valstr))
      {
         if(strcasecmp(valstr, "true") == 0
         || strcasecmp(valstr, "1"   ) == 0)
            replace = 1;

         else if(strcasecmp(valstr, "true") == 0
              || strcasecmp(valstr, "1"   ) == 0)
            replace = 0;
      }
   }

   if(mMakeImg_debug >= 1)
   {
      printf("from JSON:\n");
      printf("noise           =  %-g\n", noise);
      printf("bg1             =  %-g\n", bg1);
      printf("bg2             =  %-g\n", bg2);
      printf("bg3             =  %-g\n", bg3);
      printf("bg4             =  %-g\n", bg4);

      printf("\nncat            =  %d\n",  ncat);

      for(i=0; i<ncat; ++i)
      {
         printf("cat_file[%d]     = [%s]\n",  i, cat_file[i]);
         printf("colname [%d]     = [%s]\n",  i, colname [i]);
         printf("width   [%d]     =  %-g\n",  i, width   [i]);
         printf("flat    [%d]     =  %d \n",  i, flat    [i]);
         printf("ref     [%d]     =  %-g\n",  i, ref     [i]);
         printf("ismag   [%d]     =  %d \n",  i, ismag   [i]);
         printf("sys     [%d]     =  %d \n",  i, sys     [i]);
         printf("epoch   [%d]     =  %-g\n",  i, epoch   [i]);
      }

      printf("\nnimage          =  %d\n",  nimage);

      for(i=0; i<nimage; ++i)
      {
         printf("image_file[%d]   = [%s]\n",  i, image_file[i]);
         printf("refval    [%d]   =  %-g\n",  i, refval    [i]);
      }
   
      printf("arrayfile       = [%s]\n", arrayfile);
   }


   /************************************/ 
   /* Open the array file if it exists */ 
   /************************************/ 

   farray = (FILE *)NULL;

   if(strlen(arrayfile) > 0)
   {
      farray = fopen(arrayfile, "r");

      if(farray == (FILE *)NULL)
      {
         sprintf(returnStruct->msg, "Image array file [%s] not found.\n", arrayfile);
         return returnStruct;
      }
   }


   /*************************************************/ 
   /* Process the output header template to get the */ 
   /* image size, coordinate system and projection  */ 
   /*************************************************/ 

   if(mMakeImg_readTemplate(template_file) > 0)
   {
      mMakeImg_cleanup();
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mMakeImg_debug >= 1)
   {
      printf("output.naxes[0] =  %ld\n", output.naxes[0]);
      printf("output.naxes[1] =  %ld\n", output.naxes[1]);
      printf("output.sys      =  %d\n",  output.sys);
      printf("output.epoch    =  %-g\n", output.epoch);
      printf("output proj     =  %s\n",  output.wcs->ptype);
      printf("output crval[0] =  %-g\n", output.wcs->crval[0]);
      printf("output crval[1] =  %-g\n", output.wcs->crval[1]);
      printf("output crpix[0] =  %-g\n", output.wcs->crpix[0]);
      printf("output crpix[1] =  %-g\n", output.wcs->crpix[1]);
      printf("output cdelt[0] =  %-g\n", output.wcs->cdelt[0]);
      printf("output cdelt[1] =  %-g\n", output.wcs->cdelt[1]);

      fflush(stdout);
   }


   /***********************************************/ 
   /* Allocate memory for the output image pixels */ 
   /***********************************************/ 

   data = (double **)malloc(output.naxes[1] * sizeof(double *));

   data[0] = (double *)malloc(output.naxes[0] * output.naxes[1] * sizeof(double));

   if(mMakeImg_debug >= 1)
   {
      printf("%ld bytes allocated for image pixels\n", 
         output.naxes[0] * output.naxes[1] * sizeof(double));
      fflush(stdout);
   }


   /**********************************************************/
   /* Initialize pointers to the start of each row of pixels */
   /**********************************************************/

   for(i=1; i<output.naxes[1]; i++)
      data[i] = data[i-1] + output.naxes[0];

   if(mMakeImg_debug >= 1)
   {
      printf("pixel line pointers populated\n"); 
      fflush(stdout);
   }


   /**************************************************/
   /* Initialize the data with background plus noise */
   /**************************************************/

   for (j=0; j<output.naxes[1]; ++j)
   {
      for (i=0; i<output.naxes[0]; ++i)
      {
         x = i / (output.naxes[0] - 1.);
         y = j / (output.naxes[1] - 1.);

         background = bg1 * (1 - x) * (1 - y)
                    + bg2 * x * (1 - y)
                    + bg3 * (x * y)
                    + bg4 * y * (1 - x);

         if(noise > 0.)
         {
            randnum = ((double)rand())/RAND_MAX; 

            noise_level = mMakeImg_ltqnorm(randnum) * noise;
         }
         else
            noise_level = 0.;

         data[j][i] = background + noise_level;

      }
   }


   /******************************************************************/
   /* If there is an image array file, load that into the data array */
   /******************************************************************/

   if(farray)
   {
      i = 0;
      j = 0;
 
      endline = 0;

      while(1)
      {
         stat = mMakeImg_nextStr(farray, valstr);

         if(valstr[strlen(valstr)-1] == '\n')
         {
            valstr[strlen(valstr)-1] = '\0';
            endline = 1;
         }

         if(strlen(valstr) > 0)
         {
            if(valstr[0] < 0)
               break;

            data[j][i] = strtod(valstr, &end);

            if(end < valstr + (int)strlen(valstr))
               data[j][i] = nan;

            ++i;
         }
    
         if(endline)
         {
            ++j;

            i = 0;
            
            endline = 0;
         }

         if(stat == EOF)
            break;
      }

      fclose(farray);

      if(mMakeImg_debug >= 1)
      {
         printf("Array loaded into data\n"); 
         fflush(stdout);
      }
   }

   /************************/
   /* Create the FITS file */
   /************************/

   remove(output_file);               

   if(fits_create_file(&output.fptr, output_file, &status)) 
   {
      mMakeImg_cleanup();
      mMakeImg_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }


   /*********************************************************/
   /* Create the FITS image.  All the required keywords are */
   /* handled automatically.                                */
   /*********************************************************/

   if (fits_create_img(output.fptr, bitpix, naxis, output.naxes, &status))
   {
      mMakeImg_cleanup();
      mMakeImg_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mMakeImg_debug >= 1)
   {
      printf("FITS image created (not yet populated)\n"); 
      fflush(stdout);
   }


   /*****************************/
   /* Loop over the table files */
   /*****************************/

   count = 0;

   for(ifile=0; ifile<ncat; ++ifile)
   {
      /************************************************/ 
      /* Open the table file and find the data column */ 
      /************************************************/ 

      width2 = width[ifile] * width[ifile] / 4.;

      ncol = topen(cat_file[ifile]);

      if(ncol <= 0)
      {
         mMakeImg_cleanup();
         sprintf(returnStruct->msg, "Can't open table file %s.", cat_file[ifile]);
         return returnStruct;
      }

      loncol  = tcol("ra");
      latcol  = tcol("dec");

      if(loncol <  0 || latcol <  0)
      {
         loncol  = tcol("glon");
         latcol  = tcol("glat");

         if(loncol <  0 || latcol <  0)
         {
            loncol  = tcol("elon");
            latcol  = tcol("elat");

            if(loncol <  0 || latcol <  0)
            {
               mMakeImg_cleanup();
               strcpy(returnStruct->msg, "Can't find lon, lat columns.");
               return returnStruct;
            }
         }
      }

      fluxcol = tcol(colname[ifile]);


      /*******************/
      /* For each source */
      /*******************/

      while(tread() >= 0)
      {
         ++count;

         ilon = atof(tval(loncol));
         ilat = atof(tval(latcol));

         convertCoordinates(sys[ifile], epoch[ifile],  ilon,  ilat,
                            output.sys, output.epoch, &olon, &olat, 0.);

         if(fluxcol < 0)
            pixel_value = 1.;
         else
         {
            pixel_value = atof(tval(fluxcol));

            if(ismag[ifile])
               pixel_value = pow(10., 0.4 * (ref[ifile] - pixel_value));
            else
               pixel_value = ref[ifile] * pixel_value;

            if(pixel_value < 3.)
               pixel_value = 3.;
         }

         offscl = 0;

         wcs2pix(output.wcs, olon, olat, &oxpix, &oypix, &offscl);

         mMakeImg_fixxy(&oxpix, &oypix, &offscl);

         if(mMakeImg_debug >= 2)
         {
            printf(" value = %11.3e at coord = (%12.8f,%12.8f) -> (%12.8f,%12.8f)",
                  pixel_value, ilon, ilat, olon, olat);

            if(offscl)
            {
               printf(" -> opix = (%7.1f,%7.1f) OFF SCALE",
                     oxpix, oypix);
               fflush(stdout);
            }
            else
            {
               printf(" -> opix = (%7.1f,%7.1f)",
                     oxpix, oypix);
            }

            if(mMakeImg_debug == 2)
               printf("\r");
            else
               printf("\n");

            fflush(stdout);
         }

         if(!offscl)
         {
            l = (int)(oxpix + 0.5) - 1;
            m = (int)(oypix + 0.5) - 1;

            if(l < 0 || m < 0 || l >= output.naxes[0] || m >= output.naxes[1])
            {
               if(mMakeImg_debug >= 2)
               {
                  printf("Bad Values: l=%d, m=%d\n", l, m);
                  fflush(stdout);
               }
            }

            else
            {
               if(width[ifile] == 0)
               {
                  if(replace)
                     data[m][l] = pixel_value;
                  else
                     data[m][l] += pixel_value;
               }
               else
               {
                  for(dl=-3*width[ifile]; dl<=3*width[ifile]; ++dl)
                  {
                     if(l+dl < 0 || l+dl>= output.naxes[0])
                        continue;

                     dx = oxpix - (l+dl);

                     for(dm=-3*width[ifile]; dm<=3*width[ifile]; ++dm)
                     {
                        if(m+dm < 0 || m+dm>= output.naxes[1])
                           continue;

                        dy = oypix - (m+dm);

                        dist2 = dx*dx + dy*dy;

                        if(flat[ifile])
                           weight = 1;
                        else
                           weight = exp(-dist2/width2);

                        if(mMakeImg_debug >= 5)
                        {
                           printf("Pixel update: data[%d][%d] with value %-g*%-g\n", m+dm, l+dl, weight, pixel_value);
                           fflush(stdout);
                        }

                        if(replace)
                           data[m+dm][l+dl] = weight * pixel_value;
                        else
                           data[m+dm][l+dl] += weight * pixel_value;
                     }
                  }
               }
            }

            ++srccnt;
         }
      }

      tclose();
   }

   /*****************************/
   /* Loop over the image files */
   /*****************************/

   count = 0;

   for(ifile=0; ifile<nimage; ++ifile)
   {
      /******************************************************/ 
      /* Open the image table file and find the data column */ 
      /******************************************************/ 

      if(mMakeImg_debug > 2)
      {
         printf("Image file[%d] =\"%s\"\n", ifile, image_file[ifile]);
         fflush(stdout);
      }

      ncol = topen(image_file[ifile]);

      if(ncol <= 0)
      {
         mMakeImg_cleanup();
         sprintf(returnStruct->msg, "Can't open table table %s.", cat_file[ifile]);
         return returnStruct;
      }

      ira [0] = tcol("ra");
      idec[0] = tcol("dec");

      ira [1] = tcol("ra1");
      idec[1] = tcol("dec1");

      ira [2] = tcol("ra2");
      idec[2] = tcol("dec2");

      ira [3] = tcol("ra3");
      idec[3] = tcol("dec3");

      ira [4] = tcol("ra4");
      idec[4] = tcol("dec4");

      if(ira[0] <  0 || idec[0] <  0
      || ira[1] <  0 || idec[1] <  0
      || ira[2] <  0 || idec[2] <  0
      || ira[3] <  0 || idec[3] <  0
      || ira[4] <  0 || idec[4] <  0)
      {
         mMakeImg_cleanup();
         strcpy(returnStruct->msg, "Can't find image center or four corners.");
         return returnStruct;
      }


      /******************/
      /* For each image */
      /******************/

      while(tread() >= 0)
      {
         ++count;

         if(mMakeImg_debug >= 3)
         {
            printf("\nImage %d:\n", count);
            fflush(stdout);
         }

         if(tnull(ira [0])) continue;
         if(tnull(idec[0])) continue;
         if(tnull(ira [1])) continue;
         if(tnull(idec[1])) continue;
         if(tnull(ira [2])) continue;
         if(tnull(idec[2])) continue;
         if(tnull(ira [3])) continue;
         if(tnull(idec[3])) continue;
         if(tnull(ira [4])) continue;
         if(tnull(idec[4])) continue;

         /****************************************/
         /* Process the center and  four corners */
         /* to find the min/max pixel coverage   */                     
         /****************************************/

         imin =  100000000;
         imax = -100000000;
         jmin =  100000000;
         jmax = -100000000;

         for(i=0; i<5; ++i)
         {
            ra [i] = atof(tval(ira [i]));
            dec[i] = atof(tval(idec[i]));

            convertCoordinates(EQUJ, 2000., ra[i], dec[i],
                               output.sys, output.epoch, &ref_lon[i], &ref_lat[i], 0.);

            xref[i] = cos(ref_lon[i]*dtr) * cos(ref_lat[i]*dtr);
            yref[i] = sin(ref_lon[i]*dtr) * cos(ref_lat[i]*dtr);
            zref[i] = sin(ref_lat[i]*dtr);
         }

         nShortSide = 0;

         for(i=1; i<5; ++i)
         {
            inext = i+1;

            if(inext == 5)
               inext = 1;

            theta0 = acos(xref[i]*xref[inext] + yref[i]*yref[inext] + zref[i]*zref[inext])/dtr;

            shortSide[i-1] = 0;

            if(theta0 < tolerance)
            {
               if(mMakeImg_debug >= 3)
               {
                  printf("   Side %d: (%10.6f,%10.6f) -> (%10.6f,%10.6f) [theta0 = %10.6f, pixscale = %12.9f SHORT SIDE]\n", 
                     i, ra[i], dec[i], ra[inext], dec[inext], theta0, pixscale);
                  fflush(stdout);
               }

               ++nShortSide;
               shortSide[i-1] = 1;
               continue;
            }

            if(mMakeImg_debug >= 3)
            {
               printf("   Side %d: (%10.6f,%10.6f) -> (%10.6f,%10.6f) [theta0 = %10.6f, pixscale = %12.9f]\n", 
                  i, ra[i], dec[i], ra[inext], dec[inext], theta0, pixscale);
               fflush(stdout);
            }

            for(alpha=0; alpha<=theta0; alpha+=pixscale/2.)
            {
               theta = theta0/2. - alpha;

               A = tan(theta*dtr) * cos(theta0/2.*dtr);

               a = (sin(theta0/2.*dtr) - A)/(2.*sin(theta0/2.*dtr));

               xarc = (1.-a) * xref[i] + a * xref[inext];
               yarc = (1.-a) * yref[i] + a * yref[inext];
               zarc = (1.-a) * zref[i] + a * zref[inext];

               olon = atan2(yarc, xarc)/dtr;
               olat = asin(zarc)/dtr;

               offscl = 0;

               wcs2pix(output.wcs, olon, olat, &oxpix, &oypix, &offscl);

               mMakeImg_fixxy(&oxpix, &oypix, &offscl);

               if(mMakeImg_debug >= 4)
               {
                  printf("theta = %.6f -> A = %.6f -> a = %.6f -> (%.6f,%.6f,%.6f) -> (%12.8f,%12.8f)",
                     theta, A, a, xarc, yarc, zarc, olon, olat);

                  if(offscl)
                  {
                     printf(" -> opix = (%7.1f,%7.1f) OFF SCALE\n",
                        oxpix, oypix);
                     fflush(stdout);
                  }
                  else
                  {
                     printf(" -> opix = (%7.1f,%7.1f)\n",
                        oxpix, oypix);
                     fflush(stdout);
                  }
               }
               
               if(!offscl)
               {
                  ipix = (int)(oxpix + 0.5);
                  jpix = (int)(oypix + 0.5);

                  if(ipix < 0 || jpix < 0 || ipix >= output.naxes[0] || jpix >= output.naxes[1])
                     offscl = 1;
                  else
                  {
                     if(ipix < imin) imin = ipix;
                     if(ipix > imax) imax = ipix;
                     if(jpix < jmin) jmin = jpix;
                     if(jpix > jmax) jmax = jpix;
                  }
               }
            }
         }

         if(mMakeImg_debug >= 3)
         {
            printf("\n   Range:  i = %.2f -> %.2f   j= %.2f -> %.2f\n", 
               imin, imax, jmin, jmax);
            fflush(stdout);
         }


         /*****************************************/
         /* Compute the image corners and normals */
         /*****************************************/

         for(i=0; i<4; ++i)
         {
            image_corner[i].x = cos(ra [i+1]*dtr) * cos(dec[i+1]*dtr);
            image_corner[i].y = sin(ra [i+1]*dtr) * cos(dec[i+1]*dtr);
            image_corner[i].z = sin(dec[i+1]*dtr);
         }


         /* Reverse if counterclockwise on the sky */

         Cross(&image_corner[0], &image_corner[1], &normal1);
         Cross(&image_corner[1], &image_corner[2], &normal2);
         Cross(&normal1, &normal2, &direction);

         Normalize(&direction);

         clockwise = 0;
         if(Dot(&direction, &image_corner[1]) > 0.)
            clockwise = 1;

         if(!clockwise)
         {
            mMakeImg_swap(&image_corner[0].x, &image_corner[3].x);
            mMakeImg_swap(&image_corner[0].y, &image_corner[3].y);
            mMakeImg_swap(&image_corner[0].z, &image_corner[3].z);

            mMakeImg_swap(&image_corner[1].x, &image_corner[2].x);
            mMakeImg_swap(&image_corner[1].y, &image_corner[2].y);
            mMakeImg_swap(&image_corner[1].z, &image_corner[2].z);
         }

         for(j=0; j<4; ++j)
         {
            jnext = (j+1)%4;

            Cross(&image_corner[j], &image_corner[jnext], &image_normal[j]);

            Normalize(&image_normal[j]);
         }



         /***************************************************************************/
         /*                                                                         */
         /* Normally, we can check the pixels between imin->imax and jmin->jmax     */
         /* to see if they are inside the image.  There are two situations where    */
         /* this fails, however.                                                    */
         /*                                                                         */
         /* First, in an all-sky projection, the image may include the pole, in     */
         /* which case the latitude range should be extended to include it, even    */
         /* though the pixel range from checking the region edge doesn't include    */
         /* it.                                                                     */
         /*                                                                         */
         /* Second, the image may be so small compared to the pixel that checking   */
         /* pixel centers may find none that are inside the image.  If this happens */
         /* (no pixels are turned on for this image), we can force the pixel        */
         /* associated with the image center on.                                    */
         /*                                                                         */
         /***************************************************************************/

         /*********************************************/
         /* Special check for inclusion of North pole */
         /*********************************************/

         pixel_loc.x = 0.;
         pixel_loc.y = 0.;
         pixel_loc.z = 1.;

         interior = 1;

         if(nShortSide == 4)
            interior = 0;

         for(k=0; k<4; ++k)
         {
            if(shortSide[k])
               continue;

            if(Dot(&image_normal[k], &pixel_loc) < 0)
            {
               interior = 0;
               break;
            }
         }

         if(interior)
         {
            offscl = 0;

            wcs2pix(output.wcs, 0., 90., &oxpix, &oypix, &offscl);

            mMakeImg_fixxy(&oxpix, &oypix, &offscl);

            if(offscl)
            {
               if(output.wcs->cdelt[1] > 0.)
               {
                  jmax = output.naxes[0];

                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   North pole in image:  jmax -> %.2f\n", jmax); 
                     fflush(stdout);
                  }
               }
               else
               {
                  jmin = output.naxes[0];

                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   North pole in image:  jmin -> %.2f\n", jmin); 
                     fflush(stdout);
                  }
               }
            }
            else
            {
               if(oypix < jmin)
               {
                  jmin = oypix;

                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   North pole in image:  jmin -> %.2f\n", jmin); 
                     fflush(stdout);
                  }
               }
               else if(oypix > jmax)
               {
                  jmax = oypix;

                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   North pole in image:  jmax -> %.2f\n", jmax); 
                     fflush(stdout);
                  }
               }
               else
               {
                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   North pole in image:  no range change\n"); 
                     fflush(stdout);
                  }
               }
            }
         }


         /*********************************************/
         /* Special check for inclusion of South pole */
         /*********************************************/

         pixel_loc.x =  0.;
         pixel_loc.y =  0.;
         pixel_loc.z = -1.;

         interior = 1;

         if(nShortSide == 4)
            interior = 0;

         for(k=0; k<4; ++k)
         {
            if(shortSide[k])
               continue;

            if(Dot(&image_normal[k], &pixel_loc) < 0)
            {
               interior = 0;
               break;
            }
         }

         if(interior)
         {
            offscl = 0;

            wcs2pix(output.wcs, 0., -90., &oxpix, &oypix, &offscl);

            mMakeImg_fixxy(&oxpix, &oypix, &offscl);

            if(offscl)
            {
               if(output.wcs->cdelt[1] > 0.)
               {
                  jmax = 0.;

                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   South pole in image:  jmax -> %.2f\n", jmax); 
                     fflush(stdout);
                  }
               }
               else
               {
                  jmin = 0.;

                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   South pole in image:  jmin -> %.2f\n", jmin); 
                     fflush(stdout);
                  }
               }
            }
            else
            {
               if(oypix < jmin)
               {
                  jmin = oypix;

                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   South pole in image:  jmin -> %.2f\n", jmin); 
                     fflush(stdout);
                  }
               }
               else if(oypix > jmax)
               {
                  jmax = oypix;

                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   South pole in image:  jmax -> %.2f\n", jmax); 
                     fflush(stdout);
                  }
               }
               else
               {
                  if(mMakeImg_debug >= 3)
                  {
                     printf("\n   South pole in image:  no range change\n"); 
                     fflush(stdout);
                  }
               }
            }
         }


         /*******************************************/
         /* Loop over the possible pixel range,     */
         /* checking to see if that pixel is inside */                     
         /* the image (turn it on if so)            */                     
         /*******************************************/

         npix = 0;

         for(i=imin; i<=imax; ++i)
         {
            for(j=jmin; j<=jmax; ++j)
            {
               ix = i;
               iy = j;

               pix2wcs(output.wcs, ix, iy, &xpos, &ypos);
         
               convertCoordinates(output.sys, output.epoch,  xpos,  ypos,
                                  EQUJ,       2000.,        &olon, &olat, 0.);

               offscl = output.wcs->offscl;

               if(!offscl)
                  wcs2pix(output.wcs, xpos, ypos, &ixtest, &iytest, &offscl);

               if(offscl
               || fabs(ixtest - ix) > 0.01
               || fabs(iytest - iy) > 0.01)
                  continue;

               pixel_loc.x = cos(olon*dtr) * cos(olat*dtr);
               pixel_loc.y = sin(olon*dtr) * cos(olat*dtr);
               pixel_loc.z = sin(olat*dtr);

               interior = 1;

               if(nShortSide == 4)
                  interior = 0;

               for(k=0; k<4; ++k)
               {
                  if(shortSide[k])
                     continue;

                  if(Dot(&image_normal[k], &pixel_loc) < 0)
                  {
                     interior = 0;
                     break;
                  }
               }

               if(mMakeImg_debug >= 4)
               {
                  printf("%6d %6d -> %11.6f %11.6f -> %11.6f %11.6f (%d)\n",
                     i, j, xpos, ypos, olon, olat, interior);
                  fflush(stdout);
               }

               if(interior)
               {
                  ++npix;

                  if(replace)
                     data[j][i]  = refval[ifile];
                  else
                     data[j][i] += refval[ifile];
               }
            }
         }

        
         /* Make sure small images have at least one pixel on */

         if(npix == 0)
         {
            wcs2pix(output.wcs, ref_lon[0], ref_lat[0], &ipix, &jpix, &offscl);

            mMakeImg_fixxy(&ipix, &jpix, &offscl);

            if(!offscl)
            {
               i = (int)(ipix);
               j = (int)(jpix);

               if(mMakeImg_debug >= 4)
               {
                  printf("Single pixel turn-on: %6d %6d\n", i, j);
                  fflush(stdout);
               }

               if(replace)
                  data[j][i]  = 1.;
               else
                  data[j][i] += 1.;
            }
         }

         ++imgcnt;
      }

      tclose();
   }


   /************************/
   /* Write the image data */
   /************************/

   fpixel[0] = 1;
   fpixel[1] = 1;
   fpixel[2] = 1;
   fpixel[3] = 1;

   nelements = output.naxes[0];

   for(j=0; j<output.naxes[1]; ++j)
   {
      if (fits_write_pix(output.fptr, TDOUBLE, fpixel, nelements,
                         (void *)(data[j]), &status))
      {
         mMakeImg_cleanup();
         mMakeImg_printFitsError(status);
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }

      ++fpixel[1];
   }

   if(mMakeImg_debug >= 1)
   {
      printf("Data written to FITS data image\n");
      fflush(stdout);
   }


   /*************************************/
   /* Add keywords from a template file */
   /*************************************/

   if(fits_write_key_template(output.fptr, template_file, &status))
   {
      mMakeImg_cleanup();
      mMakeImg_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mMakeImg_debug >= 1)
   {
      printf("Template keywords written to FITS image\n"); 
      fflush(stdout);
   }


   /***********************/
   /* Close the FITS file */
   /***********************/

   if(fits_close_file(output.fptr, &status))
   {
      mMakeImg_cleanup();
      mMakeImg_printFitsError(status);
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   if(mMakeImg_debug >= 1)
   {
      printf("FITS image finalized\n"); 
      fflush(stdout);
   }

   sprintf(montage_msgstr, "sources=%d, images=%d", srccnt, imgcnt);
   sprintf(montage_json,   "{\"sources\":%d, \"images\":%d}", srccnt, imgcnt);

   returnStruct->status = 0;
   returnStruct->srccnt = srccnt;
   returnStruct->imgcnt = imgcnt;

   strcpy(returnStruct->msg,  montage_msgstr);
   strcpy(returnStruct->json, montage_json);

   mMakeImg_cleanup();
   return returnStruct;
}


/**************************************************/
/*  Analyze csys/epoch text to extract the two.   */
/**************************************************/

void mMakeImg_parseCoordStr(char *coordStr, int *csys, double *epoch)
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


/**************************************************/
/*  Projections like CAR sometimes add an extra   */
/*  360 degrees worth of pixels to the return     */
/*  and call it off-scale.                        */
/**************************************************/

void mMakeImg_fixxy(double *x, double *y, int *offscl)
{
   *x = *x - xcorrection;
   *y = *y - ycorrection;

   if(*x < 0.
   || *x > output.wcs->nxpix+1.
   || *y < 0.
   || *y > output.wcs->nypix+1.)
      *offscl = 1;

   return;
}


/**************************************************/
/*                                                */
/*  Read the output header template file.         */
/*  Specifically extract the image size info.     */
/*  Also, create a single-string version of the   */
/*  header data and use it to initialize the      */
/*  output WCS transform.                         */
/*                                                */
/**************************************************/

int mMakeImg_readTemplate(char *filename)
{
   int       i;

   FILE     *fp;

   char      line[STRLEN];

   char     *header[2];

   int       sys;
   double    epoch;

   double    x, y;
   double    ix, iy;
   double    xpos, ypos;
   int       offscl;

   header[0] = malloc(32768);
   header[1] = (char *)NULL;


   /********************************************************/
   /* Open the template file, read and parse all the lines */
   /********************************************************/

   fp = fopen(filename, "r");

   if(fp == (FILE *)NULL)
   {
      sprintf(montage_msgstr, "Template file [%s] not found.", filename);
      return 1;
   }

   while(1)
   {
      if(fgets(line, STRLEN, fp) == (char *)NULL)
         break;

      if(line[strlen(line)-1] == '\n')
         line[strlen(line)-1]  = '\0';

      if(mMakeImg_debug >= 2)
      {
         printf("Template line: [%s]\n", line);
         fflush(stdout);
      }

      for(i=strlen(line); i<80; ++i)
         line[i] = ' ';
      
      line[80] = '\0';

      strcat(header[0], line);

      mMakeImg_parseLine(line);
   }

   fclose(fp);

   if(mMakeImg_debug >= 2)
   {
      printf("\nheader ----------------------------------------\n");
      printf("%s\n", header[0]);
      printf("-----------------------------------------------\n\n");
   }


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   output.wcs = wcsinit(header[0]);

   if(output.wcs == (struct WorldCoor *)NULL)
   {
      strcpy(montage_msgstr, "Output wcsinit() failed.");
      return 1;
   }

   pixscale = fabs(output.wcs->xinc);
   if(fabs(output.wcs->yinc) < pixscale)
      pixscale = fabs(output.wcs->xinc);


   /* Kludge to get around bug in WCS library:   */
   /* 360 degrees sometimes added to pixel coord */

   ix = (output.naxes[0] + 1.)/2.;
   iy = (output.naxes[1] + 1.)/2.;

   offscl = 0;

   pix2wcs(output.wcs, ix, iy, &xpos, &ypos);
   wcs2pix(output.wcs, xpos, ypos, &x, &y, &offscl);

   xcorrection = x-ix;
   ycorrection = y-iy;

   if(mMakeImg_debug)
   {
      printf("DEBUG> xcorrection = %.2f\n", xcorrection);
      printf("DEBUG> ycorrection = %.2f\n\n", ycorrection);
      fflush(stdout);
   }


   /*************************************/
   /*  Set up the coordinate transform  */
   /*************************************/

   if(output.wcs->syswcs == WCS_J2000)
   {
      sys   = EQUJ;
      epoch = 2000.;

      if(output.wcs->equinox == 1950.)
         epoch = 1950;
   }
   else if(output.wcs->syswcs == WCS_B1950)
   {
      sys   = EQUB;
      epoch = 1950.;

      if(output.wcs->equinox == 2000.)
         epoch = 2000;
   }
   else if(output.wcs->syswcs == WCS_GALACTIC)
   {
      sys   = GAL;
      epoch = 2000.;
   }
   else if(output.wcs->syswcs == WCS_ECLIPTIC)
   {
      sys   = ECLJ;
      epoch = 2000.;

      if(output.wcs->equinox == 1950.)
      {
         sys   = ECLB;
         epoch = 1950.;
      }
   }
   else       
   {
      sys   = EQUJ;
      epoch = 2000.;
   }

   output.sys   = sys;
   output.epoch = epoch;

   free(header[0]);

   return 0;
}



/**************************************************/
/*                                                */
/*  Parse header lines from the template,         */
/*  looking for NAXIS1 and NAXIS2                 */
/*                                                */
/**************************************************/

int mMakeImg_parseLine(char *line)
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

   if(mMakeImg_debug >= 2)
   {
      printf("keyword [%s] = value [%s]\n", keyword, value);
      fflush(stdout);
   }

   if(strcmp(keyword, "NAXIS1") == 0)
      output.naxes[0] = atoi(value);

   if(strcmp(keyword, "NAXIS2") == 0)
      output.naxes[1] = atoi(value);

   return 0;
}


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void mMakeImg_printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   strcpy(montage_msgstr, status_str);

   return;
}


/****************************************************/
/* Sort of like fscanf() but with newline detection */
/****************************************************/

int mMakeImg_nextStr(FILE *fin, char *val)
{
   int ch, i;
   static char valstr[STRLEN];

   i = 0;

   valstr[0] = '\0';

   while(1)
   {
      ch = fgetc(fin);

      if(ch != ' ' && ch != '\t')
         break;
   }

   if(ch == '\n')
   {
      valstr[0] = '\n';
      valstr[1] = '\0';

      strcpy(val, valstr);

      return 1;
   }

   else
   {
      valstr[0] = ch;
      valstr[1] = '\0';

      i = 1;
   }

   while(1)
   {
      ch = fgetc(fin);

      if(ch == EOF)
      {
         valstr[i] = '\0';

         strcpy(val, valstr);

         return EOF;
      }

      if(ch == ' ' || ch == '\t')
         break;

      valstr[i] = ch;

      ++i;
   }

   if(i == 0)
      return(0);

   valstr[i] = '\0';

   strcpy(val, valstr);

   return 1;
}


/***************************************************/
/*                                                 */
/* swap()                                          */
/*                                                 */
/* Switches the values of two memory locations     */
/*                                                 */
/***************************************************/

int mMakeImg_swap(double *x, double *y)
{
   double tmp;

   tmp = *x;
   *x  = *y;
   *y  = tmp;

   return(0);
}



/***************************************************/
/*                                                 */
/* Cross()                                         */
/*                                                 */
/* Vector cross product.                           */
/*                                                 */
/***************************************************/

int Cross(Vec *v1, Vec *v2, Vec *v3)
{
   v3->x =  v1->y*v2->z - v2->y*v1->z;
   v3->y = -v1->x*v2->z + v2->x*v1->z;
   v3->z =  v1->x*v2->y - v2->x*v1->y;

   if(v3->x == 0.
   && v3->y == 0.
   && v3->z == 0.)
      return 0;

   return 1;
}


/***************************************************/
/*                                                 */
/* Dot()                                           */
/*                                                 */
/* Vector dot product.                             */
/*                                                 */
/***************************************************/

double Dot(Vec *a, Vec *b)
{
   double sum = 0.0;

   sum = a->x * b->x
       + a->y * b->y
       + a->z * b->z;

   return sum;
}


/***************************************************/
/*                                                 */
/* Normalize()                                     */
/*                                                 */
/* Normalize the vector                            */
/*                                                 */
/***************************************************/

double Normalize(Vec *v)
{
   double len;

   len = 0.;

   len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);

   v->x = v->x / len;
   v->y = v->y / len;
   v->z = v->z / len;

   return len;
}



/*
 * Lower tail quantile for standard normal distribution function.
 *
 * This function returns an approximation of the inverse cumulative
 * standard normal distribution function.  I.e., given P, it returns
 * an approximation to the X satisfying P = Pr{Z <= X} where Z is a
 * random variable from the standard normal distribution.
 *
 * The algorithm uses a minimax approximation by rational functions
 * and the result has a relative error whose absolute value is less
 * than 1.15e-9.
 *
 * Author:      Peter J. Acklam
 * Time-stamp:  2002-06-09 18:45:44 +0200
 * E-mail:      jacklam@math.uio.no
 * WWW URL:     http://www.math.uio.no/~jacklam
 *
 * C implementation adapted from Peter's Perl version
 */

#include <stdio.h>
#include <math.h>
#include <errno.h>

/* Coefficients in rational approximations. */
static const double a[] =
{
	-3.969683028665376e+01,
	 2.209460984245205e+02,
	-2.759285104469687e+02,
	 1.383577518672690e+02,
	-3.066479806614716e+01,
	 2.506628277459239e+00
};

static const double b[] =
{
	-5.447609879822406e+01,
	 1.615858368580409e+02,
	-1.556989798598866e+02,
	 6.680131188771972e+01,
	-1.328068155288572e+01
};

static const double c[] =
{
	-7.784894002430293e-03,
	-3.223964580411365e-01,
	-2.400758277161838e+00,
	-2.549732539343734e+00,
	 4.374664141464968e+00,
	 2.938163982698783e+00
};

static const double d[] =
{
	7.784695709041462e-03,
	3.224671290700398e-01,
	2.445134137142996e+00,
	3.754408661907416e+00
};

#define LOW 0.02425
#define HIGH 0.97575

double mMakeImg_ltqnorm(double p)
{
	double q, r;

	errno = 0;

	if (p < 0 || p > 1)
	{
		errno = EDOM;
		return 0.0;
	}
	else if (p == 0)
	{
		errno = ERANGE;
		return -HUGE_VAL /* minus "infinity" */;
	}
	else if (p == 1)
	{
		errno = ERANGE;
		return HUGE_VAL /* "infinity" */;
	}
	else if (p < LOW)
	{
		/* Rational approximation for lower region */
		q = sqrt(-2*log(p));
		return (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
			((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);
	}
	else if (p > HIGH)
	{
		/* Rational approximation for upper region */
		q  = sqrt(-2*log(1-p));
		return -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
			((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);
	}
	else
	{
		/* Rational approximation for central region */
    		q = p - 0.5;
    		r = q*q;
		return (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q /
			(((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1);
	}
}



void mMakeImg_cleanup()
{
   int i;

   if(isJSON)
   {
      for(i=0; i<MAXFILE; ++i)
      {
         free(cat_file[i]);
         free(image_file[i]);
         free(colname[i]);
      }

      free(cat_file);
      free(image_file);
      free(colname);

      free(width);
      free(flat);
      free(ref);
      free(ismag);
      free(sys);
      free(epoch);

      free(arrayfile);
   }
}
