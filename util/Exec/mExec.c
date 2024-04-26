/* Module: mExec.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        31Jan06  Baseline code
2.0      John Good        29Aug15  Updated SDSS requires file name change
                                   due to use of bzip2.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <fitsio.h>
#include <mtbl.h>
#include <svc.h>
#include <wcs.h>
#include <boundaries.h>
#include <coord.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/uio.h>

#include <math.h>
#include <time.h>

#define MAXLEN   4096
#define BUFSIZE 32769
#define MAXHDR  80000
#define MAXSOCK  4096

#define INTRINSIC 0
#define COMPUTED  1
#define FAILED    2

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

char *mktemp       (char *template);
int   debugCheck   (char *debugStr);

char *url_encode   ();
char *svc_value    ();
char *keyword_value();
char *filePath     ();
int   printerr     (char *str);
int   stradd       (char *header, char *card);
int   FITSerror    (char *fname, int status);

static time_t currtime, start, lasttime;

int debug;

FILE *fstatus;
FILE *fdebug;
FILE *finfo;

char msg [MAXLEN];

static struct TBL_INFO *imgs;
static struct TBL_INFO *corrs;

static int  ifile;

static int  cntr;
static char astr[MAXLEN];
static char bstr[MAXLEN];
static char cstr[MAXLEN];


/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*  MEXEC  --  Mosaicking executive for 2MASS, SDSS, DSS.                */
/*             Includes remote data and metadata access.                 */
/*             Alternatively, can mosaic preexisting user                */
/*             data.                                                     */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*  Positional parameters:                                               */
/*                                                                       */
/*  ------------    ----------------------------------------------       */
/*   Parameter        Description                                        */
/*  ------------    ----------------------------------------------       */
/*                                                                       */
/*   survey          2MASS, SDSS, or DSS                                 */
/*   band            Depends on survey. e.g: J, H, K for 2MASS           */
/*                   (If working with existing data, these two           */
/*                    parameters must be omitted)                        */
/*                                                                       */
/*   template.hdr    FITS header template for mosaic                     */
/*                                                                       */
/*  [workspace]      Directory where we can create working stuff.        */
/*                   Best if it is empty.                                */
/*                                                                       */
/*                                                                       */
/*  If no workspace is given, a unique local subdirectory will be        */
/*  created (e.g.; ./MOSAIC_AAAaa17v).  If you are going to use this     */
/*  it is best to run the program where there is space for all the       */
/*  intermediate files and you should also consider using the            */
/*  -o option.                                                           */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*  Additional controls:                                                 */
/*                                                                       */
/*  ------------    ------------  ------------------------               */
/*   Flag             Default      Description                           */
/*  ------------    ------------  ------------------------               */
/*                                                                       */
/*  -r rawdir        none         Location of user-supplied images       */
/*                                 to mosaic                             */
/*  -f region.hdr    none         FITS header file                       */
/*  -h headertext    none         FITS header as a text string           */
/*                                 (either -f or -h must exist)          */
/*  -n tilecount     no tiling    Make an NxM set of tiles instead of    */
/*  -m tilecount                   one big mosaic (and no PNGs)          */
/*  -s factor        1 (none)     "Shrink" the input image pixels prior  */
/*                                 to reprojection.  Saves time if the   */
/*                                 output is much lower resolution than  */
/*                                 the original data.                    */
/*  -e pixelerror    0.1          Set maximum mTANHdr pixel error to be  */
/*                                 allowed (abort if error is larger)    */
/*  -a               0 (false)    Don't run mCoverageCheck (get all      */
/*                                 images in expanded region)            */
/*  -i               0 (false)    Emit ROME-friendly info messages       */
/*  -l               0 (false)    Background matching adjust levels only */
/*  -g               0 (false)    Global flattening (flatten each input  */
/*                                 image individually.                   */
/*  -b               0 (false)    Skip background matching and just      */
/*                                 coadd the reprojected images.         */
/*  -k               0 (false)    Keep all working files                 */
/*  -c               0 (false)    Delete everything. Pointless unless    */
/*                                 used with 'savefile'.  Ignored if     */
/*                                 tiling.  If the raw data is user-     */
/*                                 supplied, it is never deleted.        */
/*  -o savefile      none         Location to save mosaic.  This can't   */
/*                                 be used when tiling (do your own      */
/*                                 moving/cleanup). If multiband, the    */
/*                                 band workspace name will be prepended */
/*                                 as there are multiple save files.     */
/*  -d level         0 (none)     Debugging output                       */
/*  -D filename      none         File for debug output                  */
/*                                 (otherwise it goes to stdout)         */
/*  -L labeltext     none         Label text used in HTML                */
/*  -O loctext       none         Location string text                   */
/*  -M contact       none         "Contact" string text                  */
/*  -x               0 (false)    Add a location marker to the PNG       */
/*  -W "xoff yoff"  "0 0"         Special 'wrap-around' offsets for      */
/*                                mCoverageCheck when the region         */
/*                                projection has a +180/-180 (or         */
/*                                whatever) wrap-around from one side    */
/*                                of the image to the other.  Added for  */
/*                                HiPS HPX projection.                   */
/*                                                                       */
/*                                                                       */
/*  So minimal calls would look like:                                    */
/*                                                                       */
/*                                                                       */
/*           mExec -f region.hdr 2MASS J region.hdr                      */
/*  or                                                                   */
/*           mExec -f region.hdr -r mydata region.hdr                    */
/*                                                                       */
/*                                                                       */
/*     The first would produce a mosaic (called mosaic.fits) of 2MASS    */
/*     J band data in a unique subdirectory.  The region.hdr file can be */
/*     generated with mHdr.  All other files will be cleaned up when the */
/*     processing is done.                                               */
/*                                                                       */
/*     The second will produce a mosaic of whatever is in subdirectory   */
/*     "mydata".                                                         */
/*                                                                       */
/*  To produce specifically named output mosaic                          */
/*  and clean up all the intermediate files:                             */
/*                                                                       */
/*                                                                       */
/*           mExec -f region.hdr -o region.fits -c 2MASS J workspace     */
/*                                                                       */
/*                                                                       */
/*     This will produce a half-degree square mosaic of the              */
/*     2MASS J band data in the subdirectory "workspace"                 */
/*     called mosaic.fits .  All other files will be                     */
/*     cleaned up when the processing is done.                           */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv, char **envp)
{
   int    i, j, k, iband, ch, index, baseCount, count, sys, ival;
   int    ncols, ifname, failed, nocorrection, nooverlap, istat;
   int    flag, noverlap, nimages, ntile, mtile;
   int    naxis1, naxis2, naxismax, nxtile, nytile;
   int    ix, jy, nx, ny;
   int    intan, outtan, iscale, ncell;
   int    keepAll, deleteAll, noSubset, infoMsg, levelOnly, levSlope, noBackground;
   int    ftmp, userRaw, showMarker, quickMode, globalFlatten;
   int    iurl;

   double val, factor, shrink;

   double xoff, yoff;

   struct WorldCoor *wcs, *wcsin;
   double epoch;

   fitsfile *infptr;

   int    fitsstat = 0;

   char   fheader[28800];
   char  *inheader;

   char  *ptr;

   char   temp   [MAXLEN];
   char   buf    [BUFSIZE];
   char   cwd    [MAXLEN];

   int    icntr1;
   int    icntr2;
   int    ifname1;
   int    ifname2;
   int    idiffname;

   int    iid;
   int    icntr;
   int    id;
   int    ia;
   int    ib;
   int    ic;

   int    cntr, maxcntr;
   double *a;
   double *b;
   double *c;
   int    *have;

   int    nmatches;

   int    cntr1;
   int    cntr2;

   char   file       [MAXLEN];
   char   filebase   [MAXLEN];
   char   url        [MAXLEN];
   char   fname1     [MAXLEN];
   char   fname2     [MAXLEN];
   char   diffname   [MAXLEN];
   char   areafile   [MAXLEN];
   char   survey  [3][MAXLEN];
   char   hostName   [MAXLEN];

   char   hdrfile    [MAXLEN];
   char   hdrtext    [MAXLEN];
   char   outstr     [MAXLEN];
   char   savefile   [MAXLEN];
   char   savetmp    [MAXLEN];
   char   rawdir     [MAXLEN];
   char   datadir    [MAXLEN];
   char   scale_str  [MAXLEN];
   char   debugFile  [MAXLEN];
   char   pngFile    [MAXLEN];
   char   infoFile   [MAXLEN];
   char   labelText  [MAXLEN];
   char   locText    [MAXLEN];
   char   contactText[MAXLEN];
   char   color      [MAXLEN];

   double aval;
   double bval;
   double cval;
   double crpix1;
   double crpix2;
   int    xmin;
   int    xmax;
   int    ymin;
   int    ymax;
   double xcenter;
   double ycenter;
   double npixel;
   double rms;
   double boxx;
   double boxy;
   double boxwidth;
   double boxheight;
   double boxangle;

   double width, height;

   double error, maxerror;

   FILE  *fin;
   FILE  *fout;
   FILE  *fsave;
   FILE  *fhtml;
  
   char   band   [3][16];

   int    nband;

   char   cmd         [MAXLEN];
   char   status      [MAXLEN];
   char   infile      [MAXLEN];
   char   outfile     [MAXLEN];
   char   path        [MAXLEN];
   char   goodFile    [MAXLEN];

   char   locstr      [MAXLEN];
   char   sizestr     [MAXLEN];

   char   template    [MAXLEN];
   char   tmpfile     [MAXLEN];
   char   wrapStr     [MAXLEN];
   char   workspace[3][MAXLEN];

   FILE  *fhdr;
   FILE  *bhdr;

   double allowedError;

   double ra[4], dec[4];
   double rac, decc;
   double x1, y1, z1;
   double x2, y2, z2;
   double xpos, ypos;
   double dtr;

   int     npts;
   double *lons;
   double *lats;


   struct bndInfo *box = (struct bndInfo *)NULL;

   int    rflag, dflag;

   int    rh, rm, dd, dm;
   double rs, ds;

   allowedError = 0.1;

   dtr = atan(1.)/45.;

   inheader = malloc(MAXHDR);

   getcwd(cwd, MAXLEN);

   gethostname(hostName, MAXLEN);


   /************************/
   /* Initialization stuff */
   /************************/

   time(&currtime);

   start    = currtime;
   lasttime = currtime;

   svc_sigset();


   /*****************************/
   /* Read the input parameters */
   /*****************************/

   for(iband=0; iband<3; ++iband)
      strcpy(workspace[iband], "");

   strcpy(savefile,  "");
   strcpy(tmpfile,   "");
   strcpy(hdrfile,   "");
   strcpy(hdrtext,   "");
   strcpy(debugFile, "");
   strcpy(labelText, "");
   strcpy(locText,   "");
   strcpy(wrapStr,   "");
        
   noSubset      = 0;
   showMarker    = 0;
   infoMsg       = 0;
   keepAll       = 0;
   deleteAll     = 0;
   levelOnly     = 0;
   levSlope      = 0;
   globalFlatten = 0;
   noBackground  = 0;
   userRaw       = 0;
   ntile         = 0;
   mtile         = 0;
   quickMode     = 0;
 
   shrink       = 1.0;

   xoff = 0.;
   yoff = 0.;

   strcpy(rawdir, "raw");

   debug  = 0;
   opterr = 0;

   finfo = (FILE *)NULL;

   strcpy(pngFile, "");


   while ((ch = getopt(argc, argv, "iI:ltbgkcaxqh:f:o:d:D:e:r:s:n:m:L:O:M:P:W:")) != EOF)
   {
      switch (ch)
      {
         case 'a':
            noSubset = 1;
            break;

         case 'I':
            strcpy(infoFile, optarg);

            if(strcmp(infoFile, "-") == 0)
               finfo = stdout;
            else
               finfo = fopen(infoFile, "w+");

            if(finfo)
            {
               fprintf(finfo, "<html>\n");
               fprintf(finfo, "<body>\n");
               fflush(finfo);
            }
            break;

         case 'i':
            infoMsg = 1;
            break;

         case 'l':
            levelOnly = 1;
            break;

         case 't':
            levSlope = 1;
            break;

         case 'b':
            noBackground = 1;
            break;

         case 'g':
            globalFlatten = 1;
            break;

         case 'k':
            keepAll = 1;
            break;

         case 'c':
            deleteAll = 1;
            break;

         case 'q':
            quickMode = 1;
            break;

         case 'x':
            showMarker = 1;
            break;

         case 'h':
            strcpy(hdrtext, optarg);
            break;

         case 'n':
            ntile = atoi(optarg);
            break;

         case 'm':
            mtile = atoi(optarg);
            break;

         case 'e':
            allowedError = atof(optarg);
            break;

         case 'f':
            strcpy(hdrfile, optarg);
            break;

         case 'o':
            strcpy(tmpfile, optarg);
            break;

         case 'd':
            debug = debugCheck(optarg);
            break;

         case 'D':
            strcpy(debugFile, optarg);
            break;

         case 'P':
            strcpy(pngFile, optarg);
            break;

         case 'L':
            strcpy(labelText, optarg);
            break;

         case 'O':
            strcpy(locText, optarg);
            break;

         case 'M':
            strcpy(contactText, optarg);
            break;

         case 'W':
            strcpy(wrapStr, optarg);
            sscanf(wrapStr, "%lf %lf", &xoff, &yoff);
            break;

         case 'r':
            userRaw = 1;
            strcpy(rawdir, optarg);

            if(rawdir[0] != '/')
            {
               strcpy(temp, cwd);

               if(temp[strlen(temp)-1] != '/')
                  strcat(temp, "/");

               if(strlen(rawdir) == 0)
                  temp[strlen(temp)-1] = '\0';
               else
                  strcat(temp, rawdir);
               
               strcpy(rawdir, temp);
            }

            if(rawdir[strlen(rawdir) - 1] == '/')
               rawdir[strlen(rawdir) - 1]  = '\0';

            break;

         case 's':
            shrink = atof(optarg);

            if(shrink <= 0.0)
               shrink = 1.;

            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-q(uick-mode)][-r rawdir][-n ntilex][-m ntiley][-l(evel only)][-k(eep all)][-c(lean)][-s shrinkFactor][-o output.fits][-d(ebug) level][-f region.hdr | -h header] survey band [workspace-dir]\"]\n", argv[0]);
            exit(1);
            break;
      }
   }

   if(infoMsg)
   {
      printf("[struct stat=\"INFO\", msg=\"Compute node: %s\"]\n",  hostName);
      fflush(stdout);
   }

   if(ntile > 1 && mtile < 1) mtile = ntile;
   if(mtile > 1 && ntile < 1) ntile = mtile;

   if(ntile < 1 && mtile < 1) 
   {
      ntile = 1;
      mtile = 1;
   }

   if(strlen(tmpfile) > 0)
      strcpy(savefile, filePath(cwd, tmpfile));

   if(strlen(savefile) > 5 && strcasecmp(savefile+strlen(savefile)-5, ".fits") == 0) *(savefile+strlen(savefile)-5) = '\0';
   if(strlen(savefile) > 4 && strcasecmp(savefile+strlen(savefile)-4, ".fit" ) == 0) *(savefile+strlen(savefile)-4) = '\0';
   if(strlen(savefile) > 4 && strcasecmp(savefile+strlen(savefile)-4, ".fit" ) == 0) *(savefile+strlen(savefile)-4) = '\0';
   if(strlen(savefile) > 5 && strcasecmp(savefile+strlen(savefile)-5, ".fts" ) == 0) *(savefile+strlen(savefile)-5) = '\0';

   if(ntile*mtile > 1)
   {
      if(infoMsg)
      {
         printf("[struct stat=\"INFO\", msg=\"Tiling %d x %d\"]\n",  ntile, mtile);
         fflush(stdout);
      }

      strcpy(savefile, "");
      deleteAll = 0;
   }

   if (!userRaw && argc - optind < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-q(uick-mode)][-r rawdir][-n ntile][-m ntiley][-l(evel only)][-k(eep all)][-c(lean)][-s shrinkFactor][-o output.fits][-d(ebug) level][-f region.hdr | -h header] survey band [workspace-dir]\"]\n", argv[0]);
      exit(1);
   }

   if(userRaw)
   {
      if(argc > optind)
         strcpy(workspace[0], argv[optind]);

      nband = 1;
   }

   else
   {
      nband = argc - optind;

      if(nband <= 3)
      {
         strcpy(survey[0],  argv[optind]);
         strcpy(band[0],    argv[optind+1]);

         if(argc - optind > 2)
            strcpy(workspace[0], argv[optind+2]);

         nband = 1;
      }

      else if(nband / 3 * 3 != nband)
      {
         printf("[struct stat=\"ERROR\", msg=\"Usage: %s [-q(uick-mode)][-r rawdir][-n ntile][-m ntiley][-l(evel only)][-k(eep all)][-c(lean)][-s shrinkFactor][-o output.fits][-d(ebug) level][-f region.hdr | -h header] survey band [workspace-dir] (for color 'survey band workspace' must be repeated two or three times)\"]\n", argv[0]);
         exit(1);
      }

      else
      {
         nband = nband / 3;

         for(iband=0; iband<nband; ++iband)
         {
            strcpy(survey[iband],    argv[optind+3*iband]);
            strcpy(band[iband],      argv[optind+3*iband+1]);
            strcpy(workspace[iband], argv[optind+3*iband+2]);
         }
      }
   }

   if(strlen(workspace[0]) == 0)
   {
      strcpy(template, "MOSAIC_XXXXXX");

      mkdtemp(template);

      strcpy(workspace[0], template);

      chmod(workspace[0], 0755);
   }
   else
   {
      for(iband=0; iband<nband; ++iband)
         mkdir(workspace[iband], 0775);
   }

   for(iband=0; iband<nband; ++iband)
   {
      if(workspace[iband][0] != '/')
      {
         strcpy(temp, cwd);

         if(temp[strlen(temp)-1] != '/')
            strcat(temp, "/");

         if(strlen(workspace[iband]) == 0)
            temp[strlen(temp)-1] = '\0';
         else
            strcat(temp, workspace[iband]);
         
         strcpy(workspace[iband], temp);
      }
   }

   if(debug)
   {
      fdebug = stdout;

      if(strlen(debugFile) > 0)
      {
         fdebug = fopen(debugFile, "w+");

         if(fdebug == (FILE *)NULL)
         {
            printf("[struct stat=\"ERROR\", msg=\"Invalid debug file [%s]", debugFile);
            exit(1);
         }
      }

      if(debug >= 4)
         svc_debug(fdebug);
   }


   /***********************/
   /* Loop over the bands */
   /***********************/

   for(iband=0; iband<nband; ++iband)
   {
      if(infoMsg)
      {
         printf("[struct stat=\"INFO\", msg=\"Processing survey %s, band %s\"]\n", 
            survey[iband], band[iband]);
         fflush(stdout);
      }

      if(finfo)
      {
         strcpy(color, "gray"); 

         if(nband == 2)
         {
            if(iband == 0) strcpy(color, "Blue");
            if(iband == 1) strcpy(color, "Red");
         }

         if(nband == 3)
         {
            if(iband == 0) strcpy(color, "Blue");
            if(iband == 1) strcpy(color, "Green");
            if(iband == 2) strcpy(color, "Red");
         }

         fprintf(finfo, "<p><h2>Survey: %s / Band %s (%s image)</h2></p>\n", survey[iband], band[iband], color);
         fflush(finfo);
      }


      /******************************************************************/
      /* Copy the header template from a file, if that is the way it    */
      /* was given.                                                     */
      /******************************************************************/

      if(strlen(hdrfile) > 0)
      {
         fin = fopen(hdrfile, "r" );

         if(fin == (FILE *)NULL)
         {
            sprintf(msg, "Can't open original header template file: [%s]",
               hdrfile);

            printerr(msg);
         }

         sprintf(cmd, "%s/region.hdr", workspace[iband]);

         fout = fopen(cmd, "w+");

         if(fout == (FILE *)NULL)
         {
            sprintf(msg, "Can't open workspace header template file: [%s]", 
               cmd);

            printerr(msg);
         }

         while(1)
         {
            count = fread(buf, sizeof(char), BUFSIZE, fin);

            if(count == 0)
               break;

            fwrite(buf, sizeof(char), count, fout);
         }

         fflush(fout);
         fclose(fout);
         fclose(fin);
      }

      
      /******************************************************************/
      /* Or if the header was given on the command-line, copy that.     */
      /******************************************************************/

      else
      {
         if(strlen(hdrtext) == 0)
         {
            printf("[struct stat=\"ERROR\", msg=\"Must have either header file (-f) or header text (-h)\"]\n");
            exit(1);
         }

         sprintf(cmd, "%s/region.hdr", workspace[iband]);

         fout = fopen(cmd, "w+");

         if(fout == (FILE *)NULL)
         {
            sprintf(msg, "Can't open workspace header template file: [%s]", 
               cmd);

            printerr(msg);
         }
    
         while(hdrtext[strlen(hdrtext)-1] == '\n'
            || hdrtext[strlen(hdrtext)-1] == '\r')
               hdrtext[strlen(hdrtext)-1]  = '\0';

         k = 0;

         for(j=0; j<=strlen(hdrtext); ++j)
         {
            if(hdrtext[j] == '\0')
            {
               outstr[k] = '\0';

               if(strlen(outstr) > 0)
                  fprintf(fout, "%s\n", outstr);

               break;
            }

            else if(strncmp(hdrtext+j, "\\n", 2) == 0)
            {
               outstr[k] = '\0';

               fprintf(fout, "%s\n", outstr);

               ++j;

               k = 0;
            }

            else if(hdrtext[j] == '\r' || hdrtext[j] == '\n')
            {
               /* do nothing */
            }

            else
            {
               outstr[k] = hdrtext[j];
               ++k;
            }
         }

         fclose(fout);
      }



      /******************************/
      /* Print out input parameters */
      /******************************/

      if(debug >= 1)
      {
        fprintf(fdebug, "\n\nMEXEC INPUT PARAMETERS:\n\n");
        fprintf(fdebug, "survey      = [%s]\n",  survey[iband]);
        fprintf(fdebug, "band        = [%s]\n",  band[iband]);
        fprintf(fdebug, "hdrfile     = [%s]\n",  hdrfile);
        fprintf(fdebug, "hdrtext     =  %lu characters\n",  strlen(hdrtext));
        fprintf(fdebug, "workspace   = [%s]\n",  workspace[iband]);
        fprintf(fdebug, "levelOnly   =  %d\n",   levelOnly);
        fprintf(fdebug, "levSlope    =  %d\n",   levSlope);
        fprintf(fdebug, "keepAll     =  %d\n",   keepAll);
        fprintf(fdebug, "deleteAll   =  %d\n\n", deleteAll);
        fprintf(fdebug, "cwd         = [%s]\n",  cwd);
        fflush(fdebug);
      }


      /*************************/
      /* Create subdirectories */
      /*************************/

      if(debug >= 4)
      {
         fprintf(fdebug, "chdir to [%s]\n", workspace[iband]);
         fflush(fdebug);
      }

      chdir(workspace[iband]);

      flag = 0;

      if(!userRaw)
      {
         if(mkdir(rawdir, 0775) < 0)
            flag = 1;
      }

      if(globalFlatten)
      {
         if(mkdir("flattened", 0775) < 0)
            flag = 1;
      }

      if(mkdir("projected", 0775) < 0)
         flag = 1;

      if(mkdir("diffs", 0775) < 0)
         flag = 1;

      if(mkdir("corrected", 0775) < 0)
         flag = 1;

      if(shrink != 1)
      {
         if(mkdir("shrunken", 0775) < 0)
            flag = 1;
      }

      if(ntile*mtile > 1)
      {
         if(mkdir("tiles", 0775) < 0)
            flag = 1;

         if(mkdir("tmp", 0775) < 0)
            flag = 1;
      }

      if(flag)
         printerr("Can't create proper subdirectories in workspace (may already exist)");
      



      /***********************************************/
      /* Create the WCS using the header template    */
      /***********************************************/

      fhdr = fopen("region.hdr", "r");

      if(fhdr == (FILE *)NULL)
         printerr("Can't open header template file");


      bhdr = fopen("big_region.hdr", "w+");

      if(bhdr == (FILE *)NULL) 
         printerr("Can't open expanded header file: [big_region.hdr]");


      if(debug >= 1)
      {
         fprintf(fdebug, "\nHEADER:\n");
         fprintf(fdebug, "-----------------------------------------------------------\n");
         fflush(fdebug);
      }

      if(finfo)
      {
         fprintf(finfo, "<p><h3>FITS header for mosaic</h3></p>\n");
         fflush(finfo);
      }

      while(1)
      {
         if(fgets(temp, MAXLEN, fhdr) == (char *)NULL)
            break;

         if(temp[strlen(temp)-1] == '\n')
            temp[strlen(temp)-1] =  '\0';

         if(debug >= 1)
         {
            fprintf(fdebug, "%s\n", temp);
            fflush(fdebug);
         }

         if(finfo)
         {
            fprintf(finfo, "<tt>%s</tt><br/>\n", temp);
            fflush(finfo);
         }

         if(strncmp(temp, "NAXIS1", 6) == 0)
         {       
            ival = atoi(temp+9);
            fprintf(bhdr, "NAXIS1  = %d\n", ival+3000);
            naxis1 = ival; 
         }       
         else if(strncmp(temp, "NAXIS2", 6) == 0)
         {       
            ival = atoi(temp+9);
            fprintf(bhdr, "NAXIS2  = %d\n", ival+3000);
            naxis2 = ival; 
         }       
         else if(strncmp(temp, "CRPIX1", 6) == 0)
         {       
            val = atof(temp+9);
            fprintf(bhdr, "CRPIX1  = %15.10f\n", val+1500);
         }
         else if(strncmp(temp, "CRPIX2", 6) == 0)
         {
            val = atof(temp+9);
            fprintf(bhdr, "CRPIX2  = %15.10f\n", val+1500);
         }
         else
            fprintf(bhdr, "%s\n", temp);

         stradd(fheader, temp);
      }

      if(debug >= 1)
      {
         fprintf(fdebug, "-----------------------------------------------------------\n\n");
         fflush(fdebug);
      }

      naxismax = naxis1;

      if(naxis2 > naxismax)
         naxismax = naxis2;

      nxtile = (naxis1+ntile) / ntile;
      nytile = (naxis2+mtile) / mtile;

      if(debug >= 1 && ntile*mtile > 1)
      {
        fprintf(fdebug, "TILING: %dx%d tiles (%dx%d pixels each)\n\n", 
           ntile, mtile, nxtile, nytile);
        fflush(fdebug);
      }

      fclose(fhdr);
      fclose(bhdr);


      /****************************************************************/
      /* Collect the pixel corner coordinates around the outside of   */
      /* the header and determine a vertical bounding box (to be used */
      /* in image metadata searching.                                 */
      /****************************************************************/

      wcs = wcsinit(fheader);

      if(wcs == (struct WorldCoor *)NULL)
      {
         printf("[struct stat=\"ERROR\", msg=\"Output wcsinit() failed.\"]\n");
         exit(1);
      }


      /* Get the coordinate system and epoch in a form     */
      /* compatible with the coordinate conversion library */

      if(wcs->syswcs == WCS_J2000)
      {
         sys   = EQUJ; 
         epoch = 2000.;

         if(wcs->equinox == 1950)
            epoch = 1950.;
      }
      else if(wcs->syswcs == WCS_B1950)
      {
         sys   = EQUB; 
         epoch = 1950.;

         if(wcs->equinox == 2000)
            epoch = 2000; 
      }
      else if(wcs->syswcs == WCS_GALACTIC)
      {
         sys   = GAL;  
         epoch = 2000.;
      }
      else if(wcs->syswcs == WCS_ECLIPTIC)
      {
         sys   = ECLJ; 
         epoch = 2000.;

         if(wcs->equinox == 1950)
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

      npts = 2*(naxis1 + 1) + 2*(naxis2 + 1);

      lons = (double *)malloc(2 * npts * sizeof(double));
      lats = (double *)malloc(2 * npts * sizeof(double));

      npts = 0;

      convertCoordinates(sys, epoch, xpos, ypos,
                         EQUJ, 2000., &rac, &decc, 0.0);

      // BOTTOM
      
      for(i=0; i<=naxis1; ++i)
      {
         for(j=0; j<=naxis2; ++j)
         {
            pix2wcs(wcs, i-0.5, j-0.5, &xpos, &ypos);

            if(wcs->offscl)
               continue;

            convertCoordinates(sys, epoch, xpos, ypos,
                               EQUJ, 2000., &lons[npts], &lats[npts], 0.0);
            ++npts;

            break;
         }
      }

      pix2wcs(wcs, wcs->nxpix+0.5, wcs->nypix+0.5, &xpos, &ypos);

      // TOP   
      
      for(i=0; i<=naxis1; ++i)
      {
         for(j=naxis2; j>=0; --j)
         {
            pix2wcs(wcs, i-0.5, j-0.5, &xpos, &ypos);

            if(wcs->offscl)
               continue;

            convertCoordinates(sys, epoch, xpos, ypos,
                               EQUJ, 2000., &lons[npts], &lats[npts], 0.0);
            ++npts;

            break;
         }
      }

      
      // LEFT
      
      for(j=0; j<=naxis2; ++j)
      {
         for(i=0; i<=naxis1; ++i)
         {
            pix2wcs(wcs, i-0.5, j-0.5, &xpos, &ypos);

            if(wcs->offscl)
               continue;

            convertCoordinates(sys, epoch, xpos, ypos,
                               EQUJ, 2000., &lons[npts], &lats[npts], 0.0);
            ++npts;

            break;
         }
      }


      // RIGHT
      
      for(j=0; j<=naxis2; ++j)
      {
         for(i=naxis1; i>=0; --i)
         {
            pix2wcs(wcs, i-0.5, j-0.5, &xpos, &ypos);

            if(wcs->offscl)
               continue;

            convertCoordinates(sys, epoch, xpos, ypos,
                               EQUJ, 2000., &lons[npts], &lats[npts], 0.0);
            ++npts;

            break;
         }
      }

      fout = fopen("boundary.tbl", "w+");

      fprintf(fout, "|%12s|%12s|\n", "ra", "dec");

      for(i=0; i<npts; ++i)
         fprintf(fout, " %12.6f %12.6f \n", lons[i], lats[i]);

      box = bndVerticalBoundingBox(npts, lons, lats);

      rac    = box->centerLon;
      decc   = box->centerLat;
      width  = box->lonSize;
      height = box->latSize;

      free((char *)box);

      free(lons);
      free(lats);

      if(debug >= 2)
      {
         fprintf(fdebug, "Width  = %-g\n", width);
         fprintf(fdebug, "Height = %-g\n", height);
         fflush(fdebug);
      }

      if(finfo)
      {
         fprintf(finfo, "<p>Region width on the sky:   <b>%-g degrees</b>.</p>\n", width);
         fprintf(finfo, "<p>Region height on the sky:  <b>%-g degrees</b>.</p>\n", height);
         fprintf(finfo, "<hr style='color: #fefefe;' />\n");
         fflush(finfo);
      }


      /* Generate the location string */

      pix2wcs(wcs, wcs->nxpix/2., wcs->nypix/2., &xpos, &ypos);

      degreeToHMS(xpos, 2, &rflag, &rh, &rm, &rs);
      degreeToDMS(ypos, 1, &dflag, &dd, &dm, &ds);

      if(wcs->syswcs == WCS_J2000)
      {
         if(dflag)
            sprintf(locstr, "%dh%02dm%05.2fs&nbsp;-%dd%02dm%04.1fs&nbsp;J2000", rh, rm, rs, dd, dm, ds);
         else
            sprintf(locstr, "%dh%02dm%05.2fs&nbsp;+%dd%02dm%04.1fs&nbsp;J2000", rh, rm, rs, dd, dm, ds);

         if(wcs->equinox == 1950)
         {
            if(dflag)
               sprintf(locstr, "%dh%02dm%05.2fs&nbsp;-%dd%02dm%04.1fs&nbsp;J1950", rh, rm, rs, dd, dm, ds);
            else
               sprintf(locstr, "%dh%02dm%05.2fs&nbsp;+%dd%02dm%04.1fs&nbsp;J1950", rh, rm, rs, dd, dm, ds);
         }
      }
      else if(wcs->syswcs == WCS_B1950)
      {
         if(dflag)
            sprintf(locstr, "%dh%02dm%05.2fs&nbsp;-%dd%02dm%04.1fs&nbsp;B1950", rh, rm, rs, dd, dm, ds);
         else
            sprintf(locstr, "%dh%02dm%05.2fs&nbsp;+%dd%02dm%04.1fs&nbsp;B1950", rh, rm, rs, dd, dm, ds);


         if(wcs->equinox == 1950)
         {
            if(dflag)
               sprintf(locstr, "%dh%02dm%05.2fs&nbsp;-%dd%02dm%04.1fs&nbsp;B2000", rh, rm, rs, dd, dm, ds);
            else
               sprintf(locstr, "%dh%02dm%05.2fs&nbsp;+%dd%02dm%04.1fs&nbsp;B2000", rh, rm, rs, dd, dm, ds);
         }
      }
      else if(wcs->syswcs == WCS_GALACTIC)
      {
         sprintf(locstr, "%.4f %.4f Galactic", xpos, ypos);
      }
      else if(wcs->syswcs == WCS_ECLIPTIC)
      {
         sprintf(locstr, "%.4f %.4f Ecl J2000", xpos, ypos);

         if(wcs->equinox == 1950)
            sprintf(locstr, "%.4f %.4f Ecl J1950", xpos, ypos);
      }
      else  
      {
         if(dflag)
            sprintf(locstr, "%dh%02dm%05.2fs&nbsp;-%dd%02dm%04.1fs&nbsp;J2000", rh, rm, rs, dd, dm, ds);
         else
            sprintf(locstr, "%dh%02dm%05.2fs&nbsp;+%dd%02dm%04.1fs&nbsp;J2000", rh, rm, rs, dd, dm, ds);
      }


      /* Generate the size string */

      sprintf(sizestr, "%.2f x %.2f", width, height);



      /*************************************/
      /* Get the image list for the region */
      /* of interest                       */
      /*************************************/

      if(!userRaw)
      {
         if(infoMsg)
         {
            printf("[struct stat=\"INFO\", msg=\"Computing image coverage list\"]\n");
            fflush(stdout);
         }

         time(&currtime);

         lasttime = currtime;

         // scale = scale * 1.42;
         scale = scale * 2.00;
       
         if(noSubset)
            sprintf(cmd, "mArchiveList %s %s \"%.4f %.4f eq j2000\" %.2f %.2f remote.tbl", 
               survey[iband], band[iband], rac, decc, width, height);
         else
            sprintf(cmd, "mArchiveList %s %s \"%.4f %.4f eq j2000\" %.2f %.2f remote_big.tbl", 
               survey[iband], band[iband], rac, decc, width, height);

         if(debug >= 4)
         {
            fprintf(fdebug, "[%s]\n", cmd);
            fflush(fdebug);
         }

         if(finfo)
         {
            fprintf(finfo, "<p><h3>Retrieving image metadata list</h3></p>\n");
            fprintf(finfo, "<p><span style='color: blue;'><tt>%s</tt></span></p>\n", cmd);
            fflush(finfo);
         }

         svc_run(cmd);

         if(debug >= 4)
         {
            printf("%s\n", svc_value((char *)NULL));
            fflush(stdout);
         }

         strcpy( status, svc_value( "stat" ));

         if (strcmp( status, "ERROR") == 0)
         {
            strcpy( msg, svc_value( "msg" ));

            printerr(msg);
         }
            
         if (strcmp(status, "ABORT") == 0) 
         {
            strcpy( msg, svc_value( "msg" ));

            printerr(msg);
         }
            
         nimages = atof(svc_value("count"));

         if (nimages == 0)
         {
            sprintf( msg, "%s/%s has no data covering area", survey[iband], band[iband]);

            printerr(msg);
         }

         time(&currtime);

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: mArchiveList     %6d sec  (%d images)\n", (int)(currtime - lasttime), nimages);
            fflush(fdebug);
         }

         if(finfo)
         {
            fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> (%d images)</p>\n",
               (int)(currtime - lasttime), nimages);
            fprintf(finfo, "<hr style='color: #fefefe;' />\n");
            fflush(finfo);
         }

         lasttime = currtime;


         /**************************************/
         /* Shrink it down to the exact region */
         /**************************************/

         if(!noSubset)
         {
            if(xoff != 0. || yoff != 0.)
               sprintf(cmd, "mCoverageCheck  -x %-g -y %-g remote_big.tbl remote.tbl -header region.hdr", xoff, yoff); 
            else
               sprintf(cmd, "mCoverageCheck remote_big.tbl remote.tbl -header region.hdr"); 

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            strcpy( status, svc_value( "stat" ));

            if (strcmp( status, "ERROR") == 0)
            {
               strcpy( msg, svc_value( "msg" ));

               printerr(msg);
            }
               
            nimages = atof(svc_value("count"));

            if (nimages == 0)
            {
               sprintf( msg, "%s has no data overlapping this area", survey[iband]);

               printerr(msg);
            }

            time(&currtime);

            if(debug >= 1)
            {
               fprintf(fdebug, "TIME: mCoverageCheck   %6d sec  (%d images)\n",
                  (int)(currtime - lasttime), nimages);
               fflush(fdebug);
            }

            if(finfo)
            {
               fprintf(finfo, "<p><h3>Exact coverage check</h3></p>\n");
               fprintf(finfo, "<p><span style='color: blue;'><tt>%s</tt></span></p>\n", cmd);
               fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> (%d images)</p>\n",
                  (int)(currtime - lasttime), nimages);
               fprintf(finfo, "<hr style='color: #fefefe;' />\n");
               fflush(finfo);
            }

            lasttime = currtime;
         }


         /******************/
         /* Get the images */
         /******************/

         if(debug >= 4)
         {
            fprintf(fdebug, "chdir to [%s]\n", rawdir);
            fflush(fdebug);
         }

         chdir(rawdir);

         if(!userRaw)
         {
            if(finfo)
            {
               fprintf(finfo, "<p><h3>Retrieve images</h3></p>\n");
               fflush(finfo);
            }

            sprintf(cmd, "mArchiveExec ../remote.tbl");

            /***********************************/ 
            /* Open the region list table file */
            /***********************************/ 

            ncols = topen("../remote.tbl");

            iurl = tcol( "URL");
            if(iurl < 0)
               iurl = tcol( "url");

            ifile = tcol( "fname");
            if(ifile < 0)
               ifile = tcol("file");

            if(iurl < 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Remote images table needs column 'URL' or 'url' and can optionally have column 'fname'/'file'\"]\n");
               exit(1);
            }

            svc_run(cmd);

            /*****************************************/ 
            /* Read the records and call mArchiveGet */
            /*****************************************/ 

            count  = 0;
            failed = 0;

            while(1)
            {
               istat = tread();

               if(istat < 0)
                  break;

               strcpy(url, tval(iurl));

               if(ifile >= 0)
                  strcpy(file, tval(ifile));
               else
               {
                  if(debug > 1)
                  {
                     fprintf(fdebug, "DEBUG> url = [%s]\n", url);
                     fflush(fdebug);
                  }

                  ptr = url+strlen(url)-1;

                  while(1)
                  {
                     if(ptr == url || *ptr == '/')
                     {
                        strcpy(file, ptr+1);
                        break;
                     }

                     --ptr;
                  }
               }

               ++count;

               sprintf(cmd, "mArchiveGet %s %s", url, file);

               if(finfo)
               {
                  fprintf(finfo, "<span style='color: blue;'><tt>%s</tt></span><br/>\n", cmd);
                  fflush(finfo);
               }

               if(debug >= 4)
               {
                  fprintf(fdebug, "DEBUG> [%s]\n", cmd);
                  fflush(fdebug);
               }

               if(debug >= 3)
               {
                  fprintf(fdebug, "Retrieving image %s (%3d of %3d)\n", 
                     file, count, nimages);
                  fflush(fdebug);
               }

               svc_run(cmd);

               if(debug >= 4)
               {
                  printf("%s\n", svc_value((char *)NULL));
                  fflush(stdout);
               }

               strcpy( status, svc_value( "stat" ));

               if(strcmp( status, "ERROR") == 0)
               {
                  ++failed;
                  continue;
               }

               if(strlen(file) > 3 && strcmp(file+strlen(file)-3, ".gz") == 0)
               {
                  sprintf(cmd, "gunzip %s", file);
                  system(cmd);
               }
            }
         
            time(&currtime);

            if(debug >= 1)
            {
               fprintf(fdebug, "TIME: mArchiveGet      %6d sec  (%d images)\n",
                  (int)(currtime - lasttime), count);
               fflush(fdebug);
            }

            if(finfo)
            {
               fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> %d downloaded (%d failed)</p>\n",
                  (int)(currtime - lasttime), count, failed);
               fprintf(finfo, "<hr style='color: #fefefe;' />\n");
               fflush(finfo);
            }

            lasttime = currtime;

            if(debug >= 4)
            {
               fprintf(fdebug, "chdir to [%s]\n", workspace[iband]);
               fflush(fdebug);
            }

            chdir(workspace[iband]);
         }
      }


      

      /********************************************/ 
      /* Create and open the raw image table file */
      /********************************************/ 

      if(debug >= 4)
      {
         fprintf(fdebug, "chdir to [%s]\n", rawdir);
         fflush(fdebug);
      }

      chdir(rawdir);
       
      if(userRaw)
      {
         sprintf(cmd, "mImgtbl -c . %s/rimages_big.tbl", workspace[iband]);

         if(debug >= 4)
         {
            fprintf(fdebug, "[%s]\n", cmd);
            fflush(fdebug);
         }

         svc_run(cmd);

         if(debug >= 4)
         {
            printf("%s\n", svc_value((char *)NULL));
            fflush(stdout);
         }

         if(xoff != 0. || yoff != 0.)
            sprintf(cmd, "mCoverageCheck  -x %-g -y %-g %s/rimages_big.tbl %s/rimages.tbl -header %s/region.hdr", xoff, yoff, workspace[iband], workspace[iband], workspace[iband]); 
         else
            sprintf(cmd, "mCoverageCheck %s/rimages_big.tbl %s/rimages.tbl -header %s/region.hdr", workspace[iband], workspace[iband], workspace[iband]); 

         if(debug >= 4)
         {
            fprintf(fdebug, "[%s]\n", cmd);
            fflush(fdebug);
         }

         svc_run(cmd);

         if(debug >= 4)
         {
            printf("%s\n", svc_value((char *)NULL));
            fflush(stdout);
         }

         nimages = atof(svc_value("count"));

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: mCoverageCheck   %6d sec  (%d images)\n",
               (int)(currtime - lasttime), nimages);
            fflush(fdebug);
         }

         if(finfo)
         {
            fprintf(finfo, "<p><h3>Exact coverage check</h3></p>\n");
            fprintf(finfo, "<p><span style='color: blue;'><tt>%s</tt></span></p>\n", cmd);
            fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> (%d images)</p>\n",
               (int)(currtime - lasttime), nimages);
            fprintf(finfo, "<hr style='color: #fefefe;' />\n");
            fflush(finfo);
         }

         lasttime = currtime;
      }
      else
      {
         sprintf(cmd, "mImgtbl -c . %s/rimages.tbl", workspace[iband]);

         if(debug >= 4)
         {
            fprintf(fdebug, "[%s]\n", cmd);
            fflush(fdebug);
         }

         svc_run(cmd);

         if(debug >= 4)
         {
            printf("%s\n", svc_value((char *)NULL));
            fflush(stdout);
         }

         nimages = atof(svc_value("count"));
      }

      if (nimages == 0)
      {
         sprintf( msg, "%s/%s has no data covering area", survey[iband], band[iband]);

         printerr(msg);
      }

      if(infoMsg)
      {
         printf("[struct stat=\"INFO\", msg=\"Reprojecting %d images\"]\n", nimages);
         fflush(stdout);
      }

      if(finfo)
      {
         fprintf(finfo, "<p><h3>Reprojecting images</h3></p>\n");
         fprintf(finfo, "<span style='color: blue;'><tt>mImgtbl -c raw rimages.tbl</tt></span><br/>\n");
         fflush(finfo);
      }

      if(debug >= 4)
      {
         fprintf(fdebug, "chdir to [%s]\n", workspace[iband]);
         fflush(fdebug);
      }

      chdir(workspace[iband]);

      sprintf(cmd, "mv %s/rimages_full.tbl .", rawdir);

      if(debug >= 4)
      {
         fprintf(fdebug, "[%s]\n", cmd);
         fflush(fdebug);
      }

      system(cmd);

      sprintf(cmd, "mCoverageCheck rimages_full.tbl rimages.tbl -header region.hdr");

      if(debug >= 4)
      {
         fprintf(fdebug, "[%s]\n", cmd);
         fflush(fdebug);
      }

      svc_run(cmd);

      strcpy( status, svc_value( "stat" ));

      if(strcmp( status, "ERROR") == 0)
      {
         strcpy (msg, svc_value( "msg" ));
         printerr(msg);
      }

      nimages = atof(svc_value("count"));

      if (nimages == 0)
      {
         sprintf( msg, "%s/%s has no data covering area", survey[iband], band[iband]);

         printerr(msg);
      }

      ncols = topen("rimages.tbl");

      ifname = tcol( "fname");

      if(ifname < 0)
         printerr("Need column 'fname' in input");

      iscale = tcol("scale");

      time(&currtime);

      if(debug >= 1)
      {
         fprintf(fdebug, "TIME: mImgtbl(raw)     %6d sec \n", (int)(currtime - lasttime));
         fflush(fdebug);
      }

      lasttime = currtime;


      /*************************************************/ 
      /* Try to generate an alternate header so we can */
      /* use the fast projection                       */
      /*************************************************/ 

      outtan = INTRINSIC;

      if(!quickMode
      && wcs->prjcode != WCS_TAN
      && wcs->prjcode != WCS_SIN
      && wcs->prjcode != WCS_ZEA
      && wcs->prjcode != WCS_STG
      && wcs->prjcode != WCS_ARC)
      {
         sprintf(cmd, "mTANHdr big_region.hdr altout.hdr");

         if(debug >= 4)
         {
            fprintf(fdebug, "[%s]\n", cmd);
            fflush(fdebug);
         }

         svc_run(cmd);

         if(debug >= 4)
         {
            printf("%s\n", svc_value((char *)NULL));
            fflush(stdout);
         }

         strcpy( status, svc_value( "stat" ));

         if(strcmp( status, "ERROR") == 0)
         {
            strcpy (msg, svc_value( "msg" ));
            printerr(msg);
         }
         else
         {
            outtan = COMPUTED;

            maxerror = 0.;

            error = atof(svc_value("revxerr"));

            if(error > maxerror)
               maxerror = error;

            error = atof(svc_value("revyerr"));

            if(error > maxerror)
               maxerror = error;

            if(debug >= 4)
            {
               fprintf(fdebug, "   Distorted TAN for output: max error = %-g, allowed error = %-g\n", 
                  maxerror, allowedError);
               fflush(fdebug);
            }

            if(maxerror > allowedError)
               outtan = FAILED;
         }

         time(&currtime);

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: mTANHdr          %6d sec \n", (int)(currtime - lasttime));
            fflush(fdebug);
         }

         lasttime = currtime;
      }

      
      /*********************************************************/ 
      /* If we are flattening the images beforehand, do it now */
      /*********************************************************/ 

      strcpy(datadir, rawdir);

      if(globalFlatten)
      {
         count = 0;

         while(1)
         {
            istat = tread();

            if(istat < 0)
               break;

            strcpy ( infile, tval(ifname));

            ++count;

            if(debug >= 3)
            {
               fprintf(fdebug, "Flattening image %s (%3d of %3d)\n", 
                  infile, count, nimages);
               fflush(fdebug);
            }

            sprintf(cmd, "mFlatten %s/%s flattened/%s", 
               datadir, infile, infile);

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            strcpy( status, svc_value( "stat" ));

            if(strcmp( status, "ERROR") == 0)
            {
               strcpy( msg, svc_value( "msg" ));

               printerr(msg);
            }
         }

         tseek(0);

         strcpy(datadir, "flattened");

         time(&currtime);

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: mFlatten         %6d sec  (%d images)\n", (int)(currtime - lasttime), nimages);
            fflush(fdebug);
         }

         lasttime = currtime;
      }


      /**********************************************************************/ 
      /* If we are shrinking or flattening the images beforehand, do it now */
      /**********************************************************************/ 

      if(shrink != 1.)
      {
         count = 0;

         while(1)
         {
            istat = tread();

            if(istat < 0)
               break;

            ++count;

            strcpy ( infile, tval(ifname));

            if(debug >= 3)
            {
               fprintf(fdebug, "Shrinking image %s (%3d of %3d)\n", 
                  infile, count, nimages);
               fflush(fdebug);
            }

            sprintf(cmd, "mShrink %s/%s shrunken/%s %-g", 
               datadir, infile, infile, shrink);

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            strcpy( status, svc_value( "stat" ));

            if(strcmp( status, "ERROR") == 0)
            {
               strcpy( msg, svc_value( "msg" ));

               printerr(msg);
            }
         }

         tseek(0);

         strcpy(datadir, "shrunken");

         time(&currtime);

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: mShrink          %6d sec  (%d images)\n", (int)(currtime - lasttime), nimages);
            fflush(fdebug);
         }

         lasttime = currtime;
      }


      /************************************************************/ 
      /* Read the records and call mProject/mProjectPP/mProjectQL */
      /************************************************************/ 

      index     = 0;
      failed    = 0;
      nooverlap = 0;

      while(1)
      {
         istat = tread();

         if(istat < 0)
            break;

         strcpy ( infile, tval(ifname));

         if(strlen(infile) > 4 && strcmp(infile+strlen(infile)-4, ".bz2") == 0)
            *(infile+strlen(infile)-4) = '\0';

         strcpy (outfile, infile);

         if(strlen(outfile) > 3 && strcmp(outfile+strlen(outfile)-3, ".gz") == 0)
            *(outfile+strlen(outfile)-3) = '\0';

         if(strlen(outfile) > 4 && strcmp(outfile+strlen(outfile)-4, ".fit") == 0)
            strcat(outfile, "s");


         if(strlen(outfile) > 5 &&
            strncmp(outfile+strlen(outfile)-5, ".FITS", 5) == 0)
               outfile[strlen(outfile)-5] = '\0';

         else if(strlen(outfile) > 5 &&
            strncmp(outfile+strlen(outfile)-5, ".fits", 5) == 0)
               outfile[strlen(outfile)-5] = '\0';

         else if(strlen(outfile) > 4 &&
            strncmp(outfile+strlen(outfile)-4, ".FIT", 4) == 0)
               outfile[strlen(outfile)-4] = '\0';

         else if(strlen(outfile) > 4 &&
            strncmp(outfile+strlen(outfile)-4, ".fit", 4) == 0)
               outfile[strlen(outfile)-4] = '\0';

         strcat(outfile, ".fits");

         if(iscale < 0)
            sprintf(scale_str, "1.0");
         else
            strcpy(scale_str, tval(iscale));


         /* Try to generate an alternate input header so we can */
         /* use the fast projection                             */

         intan = INTRINSIC;

         strcpy(path, filePath(datadir, infile));

         fitsstat = 0;
         if(fits_open_file(&infptr, path, READONLY, &fitsstat))
            continue;

         fitsstat = 0;
         if(fits_get_image_wcs_keys(infptr, &inheader, &fitsstat))
            FITSerror(infile, fitsstat);

         fitsstat = 0;
         if(fits_close_file(infptr, &fitsstat))
            FITSerror(infile, fitsstat);

         wcsin = wcsinit(inheader);

         if(wcsin == (struct WorldCoor *)NULL)
         {
            strcpy(msg, "Bad WCS in input image");

            printerr(msg);
         }

         if(!quickMode
         && strcmp(wcsin->ptype, "TAN") != 0
         && strcmp(wcsin->ptype, "SIN") != 0
         && strcmp(wcsin->ptype, "ZEA") != 0
         && strcmp(wcsin->ptype, "STG") != 0
         && strcmp(wcsin->ptype, "ARC") != 0)
         {
            sprintf(cmd, "mGetHdr %s orig.hdr", path);

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            strcpy( status, svc_value( "stat" ));

            if(strcmp( status, "ERROR") == 0)
            {
               strcpy(msg, svc_value( "msg" ));

               printerr(msg);
            }

            sprintf(cmd, "mTANHdr orig.hdr altin.hdr");

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            strcpy( status, svc_value( "stat" ));

            if(strcmp( status, "ERROR") == 0)
            {
               strcpy(msg, svc_value( "msg" ));

               printerr(msg);
            }
            else
            {
               intan = COMPUTED;

               maxerror = 0.;

               error = atof(svc_value("fwdxerr"));

               if(error > maxerror)
                  maxerror = error;

               error = atof(svc_value("fwdyerr"));

               if(error > maxerror)
                  maxerror = error;

               error = atof(svc_value("revxerr"));

               if(error > maxerror)
                  maxerror = error;

               error = atof(svc_value("revyerr"));

               if(error > maxerror)
                  maxerror = error;

               if(debug >= 4)
               {
                  fprintf(fdebug, "   Distorted TAN on input: max error = %-g, allowed error = %-g\n",
                     maxerror, allowedError);
                  fflush(fdebug);
               }

               if(maxerror > allowedError)
                  intan = FAILED;
            }
         }
         else
            intan = INTRINSIC;

         if(wcs->syswcs != wcsin->syswcs)
         {
             intan = FAILED;
            outtan = FAILED;

            if(debug >= 4)
            {
               fprintf(fdebug, "   Can't use distorted TAN when projecting between coordinate systems.\n");
               fflush(fdebug);
            }
         }


         /* Now run mProject, mProjectPP or mProjectQL */
         /* (depending on what we have to work with)   */

         if(quickMode)
            sprintf(cmd, "mProjectQL -x %s -X %s/%s projected/%s big_region.hdr",
               scale_str, datadir, infile, outfile);

         else if(intan == COMPUTED  && outtan == COMPUTED )
            sprintf(cmd, "mProjectPP -b 1 -i altin.hdr -o altout.hdr -x %s -X %s/%s projected/%s big_region.hdr",
               scale_str, datadir, infile, outfile);

         else if(intan == COMPUTED  && outtan == INTRINSIC)
            sprintf(cmd, "mProjectPP -b 1 -i altin.hdr -x %s -X %s/%s projected/%s big_region.hdr",
               scale_str, datadir, infile, outfile);

         else if(intan == INTRINSIC && outtan == COMPUTED )
            sprintf(cmd, "mProjectPP -b 1 -o altout.hdr -x %s -X %s/%s projected/%s big_region.hdr",
               scale_str, datadir, infile, outfile);

         else if(intan == INTRINSIC && outtan == INTRINSIC)
            sprintf(cmd, "mProjectPP -b 1 -x %s -X %s/%s projected/%s big_region.hdr",
               scale_str, datadir, infile, outfile);

         else
            sprintf(cmd, "mProject -x %s -X %s/%s projected/%s big_region.hdr",
               scale_str, datadir, infile, outfile);

         if(debug >= 4)
         {
            fprintf(fdebug, "[%s]\n", cmd);
            fflush(fdebug);
         }

         svc_run(cmd);

         if(debug >= 4)
         {
            printf("%s\n", svc_value((char *)NULL));
            fflush(stdout);
         }

         strcpy( status, svc_value( "stat" ));

         ++index;

         if(strcmp( status, "ABORT") == 0)
         {
            strcpy( msg, svc_value( "msg" ));

            printerr(msg);
         }

         else if(strcmp( status, "ERROR") == 0)
         {
            strcpy( msg, svc_value( "msg" ));

            if(strlen(msg) > 30)
               msg[30] = '\0';

            if(strcmp( msg, "No overlap")           == 0
            || strcmp( msg, "All pixels are blank") == 0)
            {
               ++nooverlap;
               strcat(msg, ": ");
               strcat(msg, tval(ifname));
            }
            else
            {
               ++failed;
               strcat(msg, ": ");
               strcat(msg, tval(ifname));
            }
         }
         else
         {
            strcpy(goodFile, outfile);

            if(strlen(goodFile) > 3 && strcmp(goodFile+strlen(goodFile)-3, ".gz") == 0)
               *(goodFile+strlen(goodFile)-3) = '\0';

            if(debug >= 3)
            {
               fprintf(fdebug, "Reprojecting %s took %s seconds (%3d of %3d)\n", 
                  tval(ifname), svc_value("time"), index, nimages);
               fflush(fdebug);
            }

            if(finfo)
            {
               fprintf(finfo, "<span style='color: blue;'><tt>%s (%d of %d) %s sec</tt></span><br/>\n", 
                  cmd, index, nimages, svc_value("time"));
               fflush(finfo);
            }
         }

         if(!keepAll && !userRaw)
         {
            strcpy(cmd, filePath(rawdir, infile));
            unlink(cmd);
         }
      }

      baseCount = index - failed - nooverlap;

      time(&currtime);

      if(debug >= 1)
      {
         if(quickMode)
            fprintf(fdebug, "TIME: mProjectQL       %6d sec  (%d successful, %d failed, %d no overlap)\n",
               (int)(currtime - lasttime), baseCount, failed, nooverlap);

         else if(intan == FAILED && outtan == FAILED)
            fprintf(fdebug, "TIME: mProject         %6d sec  (%d successful, %d failed, %d no overlap)\n",
               (int)(currtime - lasttime), baseCount, failed, nooverlap);

         else
            fprintf(fdebug, "TIME: mProjectPP       %6d sec  (%d successful, %d failed, %d no overlap)\n", 
               (int)(currtime - lasttime), baseCount, failed, nooverlap);
         fflush(fdebug);
      }

      if(finfo)
      {
         fprintf(finfo, "<span style='color: blue;'><tt>mImgtbl -c projected pimages.tbl</tt></span>\n");
         fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> (%d successful, %d failed, %d no overlap)</p>\n",
            (int)(currtime - lasttime), baseCount, failed, nooverlap);
         fflush(finfo);
      }

      lasttime = currtime;

      tclose();

      unlink("altin.hdr");
      unlink("altout.hdr");

      if(baseCount == 0)
         printerr("None of the images had data in this area");

      if(baseCount == 1)
      {
         sprintf(cmd, "mSubimage projected/%s mosaic.fits %.6f %.6f %.6f %.6f", 
            goodFile, rac, decc, fabs(wcs->nxpix * wcs->xinc), fabs(wcs->nypix * wcs->yinc));

         if(debug >= 4)
         {
            fprintf(fdebug, "[%s]\n", cmd);
            fflush(fdebug);
         }

         svc_run(cmd);

         if(debug >= 4)
         {
            printf("%s\n", svc_value((char *)NULL));
            fflush(stdout);
         }

         sprintf(cmd, "mImgtbl -c projected pimages.tbl");

         if(debug >= 4)
         {
            fprintf(fdebug, "[%s]\n", cmd);
            fflush(fdebug);
         }

         svc_run(cmd);

         if(debug >= 4)
         {
            printf("%s\n", svc_value((char *)NULL));
            fflush(stdout);
         }

         nimages = atof(svc_value("count"));

         if(nimages <= 0)
            printerr("None of the projected images were good.");
      }


      /*********************************/
      /* Generate the differences list */
      /*********************************/

      if(baseCount > 1)
      {
         sprintf(cmd, "mImgtbl -c projected pimages.tbl");

         if(debug >= 4)
         {
            fprintf(fdebug, "[%s]\n", cmd);
            fflush(fdebug);
         }

         svc_run(cmd);

         if(debug >= 4)
         {
            printf("%s\n", svc_value((char *)NULL));
            fflush(stdout);
         }

         nimages = atof(svc_value("count"));

         if(nimages <= 0)
            printerr("None of the projected images were good.");

         time(&currtime);

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: mImgtbl(proj)    %6d sec \n", (int)(currtime - lasttime));
            fflush(fdebug);
         }

         lasttime = currtime;
      }

      if(!noBackground)
      {
         if(baseCount > 1)
         {
            sprintf(cmd, "mOverlaps pimages.tbl diffs.tbl");

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            if(finfo)
            {
               fprintf(finfo, "<p><h3>Determining image overlaps for background matching</h3></p>\n");
               fprintf(finfo, "<p><span style='color: blue;'><tt>mOverlaps pimages.tbl diffs.tbl</tt></span></p>\n");
               fflush(finfo);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            strcpy( status, svc_value( "stat" ));

            if(strcmp( status, "ERROR") == 0)
            {
               strcpy( msg, svc_value( "msg" ));

               printerr(msg);
            }

            noverlap = atof(svc_value("count"));

            if(infoMsg)
            {
                printf("[struct stat=\"INFO\", msg=\"Performing background correction analysis (%d overlaps)\"]\n", 
                     noverlap);
               fflush(stdout);
            }

            time(&currtime);

            if(debug >= 1)
            {
               fprintf(fdebug, "TIME: mOverlaps        %6d sec  (%d overlaps)\n", (int)(currtime - lasttime), noverlap);
               fflush(fdebug);
            }

            if(finfo)
            {
               fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> (%d overlaps)</p>\n",
                  (int)(currtime - lasttime), noverlap);
               fprintf(finfo, "<hr style='color: #fefefe;' />\n");
               fflush(finfo);
            }

            lasttime = currtime;
         }



         /***************************************/ 
         /* Open the difference list table file */
         /***************************************/ 

         if(baseCount > 1)
         {
            ncols = topen("diffs.tbl");

            if(ncols <= 0)
            {
               printf("[struct stat=\"ERROR\", msg=\"Invalid image metadata file: diffs.tbl\"]\n");
               exit(1);
            }

            icntr1    = tcol( "cntr1");
            icntr2    = tcol( "cntr2");
            ifname1   = tcol( "plus");
            ifname2   = tcol( "minus");
            idiffname = tcol( "diff");

            if(finfo)
            {
               fprintf(finfo, "<p><h3>Fitting image differences (mDiff / mFitplane)</h3></p>\n");
               fflush(finfo);
            }

      /***************************************/ 
      /* Open the difference list table file */
      /***************************************/ 

            /***************************************************/ 
            /* Read the records and call mDiff, then mFitplane */
            /***************************************************/ 

            count   = 0;
            failed  = 0;

            fout = fopen("fits.tbl", "w+");

            fprintf(fout, "|   plus  |  minus  |         a      |        b       |        c       |    crpix1    |    crpix2    |   xmin   |   xmax   |   ymin   |   ymax   |   xcenter   |   ycenter   |    npixel   |      rms       |      boxx      |      boxy      |    boxwidth    |   boxheight    |     boxang     |\n");
            fflush(fout);

            while(1)
            {
               istat = tread();

               if(istat < 0)
                  break;

               cntr1 = atoi(tval(icntr1));
               cntr2 = atoi(tval(icntr2));

               ++count;

               if(debug >= 3)
               {
                  fprintf(fdebug, "Processing image difference %3d - %3d (%3d of %3d)\n", 
                     cntr1+1, cntr2+1, count, noverlap);
                  fflush(fdebug);
               }

               strcpy(fname1,   tval(ifname1));
               strcpy(fname2,   tval(ifname2));
               strcpy(diffname, tval(idiffname));

               if(quickMode)
                  sprintf(cmd, "mDiff -n projected/%s projected/%s diffs/%s big_region.hdr", fname1, fname2, diffname);
               else
                  sprintf(cmd, "mDiff projected/%s projected/%s diffs/%s big_region.hdr", fname1, fname2, diffname);

               if(debug >= 4)
               {
                  fprintf(fdebug, "[%s]\n", cmd);
                  fflush(fdebug);
               }

               if(finfo)
               {
                  fprintf(finfo, "<span style='color: blue;'><tt>%s</tt></span><br/>\n", cmd);
                  fflush(finfo);
               }

               svc_run(cmd);

               if(debug >= 4)
               {
                  printf("%s\n", svc_value((char *)NULL));
                  fflush(stdout);
               }

               strcpy( status, svc_value( "stat" ));

               if(strcmp( status, "ABORT") == 0)
               {
                  strcpy( msg, svc_value( "msg" ));

                  printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
                  fflush(fdebug);

                  exit(1);
               }

               if(strcmp( status, "ERROR"  ) == 0
               || strcmp( status, "WARNING") == 0)
                  ++failed;


               if(levelOnly || levSlope)
                  sprintf(cmd, "mFitplane -l diffs/%s", diffname);
               else
                  sprintf(cmd, "mFitplane diffs/%s", diffname);

               if(debug >= 4)
               {
                  fprintf(fdebug, "[%s]\n", cmd);
                  fflush(fdebug);
               }

               if(finfo)
               {
                  fprintf(finfo, "<span style='color: blue;'><tt>%s</tt></span><p/>\n\n", cmd);
                  fflush(finfo);
               }

               svc_run(cmd);

               if(debug >= 4)
               {
                  printf("%s\n", svc_value((char *)NULL));
                  fflush(stdout);
               }

               strcpy( status, svc_value( "stat" ));

               if(strcmp( status, "ABORT") == 0)
               {
                  strcpy( msg, svc_value( "msg" ));

                  printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
                  fflush(stdout);

                  exit(1);
               }

               if(strcmp( status, "ERROR")   == 0
               || strcmp( status, "WARNING") == 0)
                  ++failed;
               else
               {
                  aval      = atof(svc_value("a"));
                  bval      = atof(svc_value("b"));
                  cval      = atof(svc_value("c"));
                  crpix1    = atof(svc_value("crpix1"));
                  crpix2    = atof(svc_value("crpix2"));
                  xmin      = atoi(svc_value("xmin"));
                  xmax      = atoi(svc_value("xmax"));
                  ymin      = atoi(svc_value("ymin"));
                  ymax      = atoi(svc_value("ymax"));
                  xcenter   = atof(svc_value("xcenter"));
                  ycenter   = atof(svc_value("ycenter"));
                  npixel    = atof(svc_value("npixel"));
                  rms       = atof(svc_value("rms"));
                  boxx      = atof(svc_value("boxx"));
                  boxy      = atof(svc_value("boxy"));
                  boxwidth  = atof(svc_value("boxwidth"));
                  boxheight = atof(svc_value("boxheight"));
                  boxangle  = atof(svc_value("boxang"));

                  fprintf(fout, " %9d %9d %16.5e %16.5e %16.5e %14.2f %14.2f %10d %10d %10d %10d %13.2f %13.2f %13.0f %16.5e %16.1f %16.1f %16.1f %16.1f %16.1f \n",
                     cntr1, cntr2, aval, bval, cval, crpix1, crpix2, xmin, xmax, ymin, ymax, 
                     xcenter, ycenter, npixel, rms, boxx, boxy, boxwidth, boxheight, boxangle);
                  fflush(fout);
               }

               if(!keepAll)
               {
                  sprintf(cmd, "diffs/%s", diffname);
                  unlink(cmd);

                  strcpy(areafile, cmd);
                  areafile[strlen(areafile) - 5] = '\0';
                  strcat(areafile, "_area.fits");

                  unlink(areafile);
               }
            }

            tclose();

            time(&currtime);

            if(debug >= 1)
            {
               fprintf(fdebug, "TIME: mDiff/mFitplane  %6d sec  (%d diffs,  %d successful, %d failed)\n", 
                  (int)(currtime - lasttime), count, count - failed,  failed);

               fflush(fdebug);
            }

            if(finfo)
            {
               fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> (%d diffs, %d successful, %d failed)</p>\n",
                  (int)(currtime - lasttime), count, count-failed, failed);
               fprintf(finfo, "<hr style='color: #fefefe;' />\n");
               fflush(finfo);
            }

            lasttime = currtime;
         }

            strcpy( status, svc_value( "stat" ));

         /*********************************/
         /* Generate the correction table */
         /*********************************/

         if(baseCount > 1)
         {
            if(levelOnly)
               sprintf(cmd, "mBgModel -l -a pimages.tbl fits.tbl corrections.tbl");
            else if(levSlope)
               sprintf(cmd, "mBgModel -f pimages.tbl fits.tbl corrections.tbl");
            else
               sprintf(cmd, "mBgModel -t pimages.tbl fits.tbl corrections.tbl");

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            if(finfo)
            {
               fprintf(finfo, "<p><h3>Modeling differences to determine background corrections</h3></p>\n");
               fprintf(finfo, "<p><span style='color: blue;'><tt>%s</tt></span></p>\n", cmd);
               fflush(finfo);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            strcpy( status, svc_value( "stat" ));

            if(strcmp( status, "ERROR") == 0)
            {
               strcpy( msg, svc_value( "msg" ));

               printerr(msg);
            }

            time(&currtime);

            if(debug >= 1)
            {
               fprintf(fdebug, "TIME: mBgModel         %6d sec \n", (int)(currtime - lasttime));
               fflush(fdebug);
            }

            if(finfo)
            {
               fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span></p>\n",
                  (int)(currtime - lasttime));
               fprintf(finfo, "<hr style='color: #fefefe;' />\n");
               fflush(finfo);
            }
         }

            lasttime = currtime;
         }

         if(finfo)
         {
            fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> (%d diffs, %d successful, %d failed)</p>\n",
               (int)(currtime - lasttime), count, count-failed, failed);
            fprintf(finfo, "<hr style='color: #fefefe;' />\n");
            fflush(finfo);
         }

         /**************************************/ 
         /* Background correct all the images  */
         /**************************************/ 

         if(baseCount > 1)
         {
            if(infoMsg)
            {
               printf("[struct stat=\"INFO\", msg=\"Background correcting %d images\"]\n", 
                  nimages);
               fflush(stdout);
            }

            if(finfo)
            {
               fprintf(finfo, "<p><h3>Applying background corrections</h3></p>\n");
               fflush(finfo);
            }

         svc_run(cmd);

            /**************************************/
            /* Allocate space for the corrections */
            /**************************************/
         
            maxcntr = nimages;

            a = (double *)malloc(maxcntr * sizeof(double));
            b = (double *)malloc(maxcntr * sizeof(double));
            c = (double *)malloc(maxcntr * sizeof(double));
         
            have = (int *)malloc(maxcntr * sizeof(int));
         
            for(i=0; i<maxcntr; ++i)
            {  
               a[i]    = 0.;
               b[i]    = 0.;
               c[i]    = 0.;
               have[i] = 0;
            }
         
         
            /******************************************/
            /* Open the corrections table file        */
            /******************************************/
         
            ncols = topen("corrections.tbl");
         
            iid = tcol( "id");
            ia  = tcol( "a");
            ib  = tcol( "b");
            ic  = tcol( "c");
         
            if(iid < 0
            || ia  < 0
            || ib  < 0
            || ic  < 0)
            {
               printerr("Need columns: id,a,b,c in corrections file");
               exit(1);
            }
         
         
            /********************************/
            /* And read in the corrections. */
            /********************************/
         
            while(1)
            {
               if(tread() < 0)
                  break;
         
               id = atoi(tval(iid));
         
               a[id] = atof(tval(ia));
               b[id] = atof(tval(ib));
               c[id] = atof(tval(ic));
         
               have[id] = 1;
            }
         
            tclose();

            printerr(msg);
         }

            /***************************************************/ 
            /* Read through the image list.                    */
            /*                                                 */
            /* If there is no correction for an image file,    */
            /* increment 'nocorrection' and copy it unchanged. */
            /* Then run mBackground to create the corrected    */
            /* image.  If there is an image in the list for    */
            /* which we don't actually have a projected file   */
            /* (can happen if the list was created from the    */
            /* 'raw' set), increment the 'failed' count.       */
            /***************************************************/ 

            ncols = topen("pimages.tbl");

            icntr  = tcol("cntr");
            ifname = tcol("fname");

            count        = 0;
            nocorrection = 0;
            failed       = 0;

            while(1)
            {
               if(tread() < 0)
                  break;

               cntr = atoi(tval(icntr));

               strcpy(file, tval(ifname));

               if(quickMode)
               {
                  sprintf(cmd, "mBackground -n projected/%s corrected/%s %-g %-g %-g", 
                        file, file, a[cntr], b[cntr], c[cntr]);
               }
               else
                  sprintf(cmd, "mBackground projected/%s corrected/%s %-g %-g %-g", 
                        file, file, a[cntr], b[cntr], c[cntr]);

               if(have[cntr] == 0)
                  ++nocorrection;

               if(debug >= 3)
               {
                  fprintf(fdebug, "Background %d of %d: %s\n", cntr, nimages, cmd); 
                  fflush(fdebug);
               }

               if(finfo)
               {
                  fprintf(finfo, "<span style='color: blue;'><tt>%s</tt></span><br/>\n", cmd);
                  fflush(finfo);
               }

               svc_run(cmd);

               if(debug >= 4)
               {
                  printf("%s\n", svc_value((char *)NULL));
                  fflush(stdout);
               }

               if(strcmp( status, "ABORT") == 0)
               {
                  strcpy( msg, svc_value( "msg" ));

                  printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
                  fflush(stdout);

                  exit(1);
               }

               strcpy( status, svc_value( "stat" ));

               ++count;
               if(strcmp( status, "ERROR") == 0)
                  ++failed;

               if(!keepAll)
               {
                  sprintf(cmd, "projected/%s", file);
                  unlink(cmd);

                  strcpy(areafile, cmd);
                  areafile[strlen(areafile) - 5] = '\0';
                  strcat(areafile, "_area.fits");

                  unlink(areafile);
               }

            if(finfo)
            {
               fprintf(finfo, "<span style='color: blue;'><tt>%s</tt></span><br/>\n", cmd);
               fflush(finfo);
            }

            svc_run(cmd);

            if(strcmp( status, "ABORT") == 0)
            {
               strcpy( msg, svc_value( "msg" ));

               printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
               fflush(stdout);

               exit(1);
            }

            strcpy( status, svc_value( "stat" ));

            ++count;
            if(strcmp( status, "ERROR") == 0)
               ++failed;

            if(!keepAll)
            {
               sprintf(cmd, "projected/%s", file);
               unlink(cmd);

               strcpy(areafile, cmd);
               areafile[strlen(areafile) - 5] = '\0';
               strcat(areafile, "_area.fits");

               unlink(areafile);
            }

            time(&currtime);

            if(debug >= 1)
            {
               fprintf(fdebug, "TIME: mBackground      %6d sec  (%d corrected)\n", (int)(currtime - lasttime), count);
               fflush(fdebug);
            }

            if(finfo)
            {
               fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> (%d images corrected)</p>\n",
                  (int)(currtime - lasttime), count);
               fprintf(finfo, "<hr style='color: #fefefe;' />\n");
               fflush(finfo);
            }

            lasttime = currtime;
         }

         time(&currtime);

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: mBackground      %6d (%d corrected)\n", (int)(currtime - lasttime), count);
            fflush(fdebug);
         }

         if(finfo)
         {
            fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span> (%d images corrected)</p>\n",
               (int)(currtime - lasttime), count);
            fprintf(finfo, "<hr style='color: #fefefe;' />\n");
            fflush(finfo);
         }

         lasttime = currtime;
      }


      /**************************/
      /* Coadd for final mosaic */
      /**************************/

      if(baseCount > 1)
      {
         if(infoMsg)
         {
            printf("[struct stat=\"INFO\", msg=\"Coadding %d images for final mosaic\"]\n", count);
            fflush(stdout);
         }

         if(!noBackground)
         {
            sprintf(cmd, "mImgtbl -c corrected cimages.tbl");

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            time(&currtime);

            if(debug >= 1)
            {
               fprintf(fdebug, "TIME: mImgtbl(corr)    %6d sec \n", (int)(currtime - lasttime));
               fflush(fdebug);
            }

            lasttime = currtime;
         }

         if(finfo)
         {
            fprintf(finfo, "<p><h3>Coadding image for final mosaic</h3></p>\n");

            if(!noBackground)
               fprintf(finfo, "<span style='color: blue;'>mImgtbl -c corrected cimages.tbl</span><br/>\n");

            fprintf(finfo, "<span style='color: blue;'>mAdd -n -p corrected cimages.tbl region.hdr mosaic.fits<br/></span>\n");
            fflush(finfo);
         }

         svc_run(cmd);

         time(&currtime);

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: mImgtbl(corr)    %6d\n", (int)(currtime - lasttime));
            fflush(fdebug);
         }

         lasttime = currtime;

         if(ntile*mtile == 1)
         {
            if(noBackground)
            {
               if(quickMode)
                  sprintf(cmd, "mAdd -n -p projected pimages.tbl region.hdr mosaic.fits");
               else
                  sprintf(cmd, "mAdd -p projected pimages.tbl region.hdr mosaic.fits");
            }
            else
            {
               if(quickMode)
                  sprintf(cmd, "mAdd -n -p corrected cimages.tbl region.hdr mosaic.fits");
               else
                  sprintf(cmd, "mAdd -p corrected cimages.tbl region.hdr mosaic.fits");
            }

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            strcpy( status, svc_value( "stat" ));

            if(strcmp( status, "ERROR") == 0)
            {
               strcpy( msg, svc_value( "msg" ));

               printerr(msg);
            }
         }
         else
         {
            for(i=0; i<ntile; ++i)
            {
               for(j=0; j<mtile; ++j)
               {
                  sprintf(cmd, "mTileHdr region.hdr tmp/region_%d_%d.hdr %d %d %d %d 100 100",
                                 i, j, ntile, mtile, i, j);

                  if(debug >= 4)
                  {
                     fprintf(fdebug, "[%s]\n", cmd);
                     fflush(fdebug);
                  }

                  svc_run(cmd);

                  if(debug >= 4)
                  {
                     printf("%s\n", svc_value((char *)NULL));
                     fflush(stdout);
                  }

                  strcpy( status, svc_value( "stat" ));

                  if(strcmp( status, "ERROR") == 0)
                  {
                     strcpy( msg, svc_value( "msg" ));

                     printerr(msg);
                  }

                  if(noBackground)
                  {
                     if(xoff != 0. || yoff != 0.)
                        sprintf(cmd, "mCoverageCheck -x %-g -y %-g pimages.tbl tmp/pimages_%d_%d.tbl -f tmp/region_%d_%d.hdr",
                           xoff, yoff, i, j, i, j);
                     else
                        sprintf(cmd, "mCoverageCheck pimages.tbl tmp/pimages_%d_%d.tbl -f tmp/region_%d_%d.hdr",
                           i, j, i, j);
                  }
                  else
                  {
                     if(xoff != 0. || yoff != 0.)
                        sprintf(cmd, "mCoverageCheck -x %-g -y %-g cimages.tbl tmp/cimages_%d_%d.tbl -f tmp/region_%d_%d.hdr",
                           xoff, yoff, i, j, i, j);
                     else
                        sprintf(cmd, "mCoverageCheck cimages.tbl tmp/cimages_%d_%d.tbl -f tmp/region_%d_%d.hdr",
                           i, j, i, j);
                  }

                  if(debug >= 4)
                  {
                     fprintf(fdebug, "[%s]\n", cmd);
                     fflush(fdebug);
                  }

                  svc_run(cmd);

                  if(debug >= 4)
                  {
                     printf("%s\n", svc_value((char *)NULL));
                     fflush(stdout);
                  }

                  strcpy( status, svc_value( "stat" ));

                  if(strcmp( status, "ERROR") == 0)
                  {
                     strcpy( msg, svc_value( "msg" ));

                     printerr(msg);
                  }

                  nmatches = atoi(svc_value("count"));

                  if(nmatches > 0)
                  {
                     if(noBackground)
                     {
                        if(quickMode)
                           sprintf(cmd, "mAdd -n -p projected tmp/pimages_%d_%d.tbl tmp/region_%d_%d.hdr tiles/tile_%d_%d.fits",
                              i, j, i, j, i, j);
                        else
                           sprintf(cmd, "mAdd -p projected tmp/pimages_%d_%d.tbl tmp/region_%d_%d.hdr tiles/tile_%d_%d.fits",
                              i, j, i, j, i, j);
                     }
                     else
                     {
                        if(quickMode)
                           sprintf(cmd, "mAdd -n -p corrected tmp/cimages_%d_%d.tbl tmp/region_%d_%d.hdr tiles/tile_%d_%d.fits",
                              i, j, i, j, i, j);
                        else
                           sprintf(cmd, "mAdd -p corrected tmp/cimages_%d_%d.tbl tmp/region_%d_%d.hdr tiles/tile_%d_%d.fits",
                              i, j, i, j, i, j);
                     }

                     if(debug >= 4)
                     {
                        fprintf(fdebug, "[%s]\n", cmd);
                        fflush(fdebug);
                     }

                     svc_run(cmd);

                     if(debug >= 4)
                     {
                        printf("%s\n", svc_value((char *)NULL));
                        fflush(stdout);
                     }

                     strcpy( status, svc_value( "stat" ));

                     if(strcmp( status, "ERROR") == 0)
                     {
                        strcpy( msg, svc_value( "msg" ));

                        printerr(msg);
                     }
                  }

                  if(!keepAll)
                  {
                     sprintf(cmd, "tmp/region_%d_%d.hdr", i, j);
                     unlink(cmd);

                     if(!noBackground)
                     {
                        sprintf(cmd, "tmp/cimages_%d_%d.tbl", i, j);
                        unlink(cmd);
                     }
                  }
               }
            }

            sprintf(cmd, "mImgtbl -c tiles timages.tbl");

            if(debug >= 4)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            time(&currtime);

            if(debug >= 1)
            {
               fprintf(fdebug, "TIME: mAdd(tiles)      %6d sec \n", (int)(currtime - lasttime));
               fflush(fdebug);
            }

            lasttime = currtime;
            
            if(quickMode)
               sprintf(cmd, "mAdd -n -p tiles timages.tbl region.hdr mosaic.fits");
            else
               sprintf(cmd, "mAdd -p tiles timages.tbl region.hdr mosaic.fits");

            if(debug >= 1)
            {
               fprintf(fdebug, "[%s]\n", cmd);
               fflush(fdebug);
            }

            svc_run(cmd);

            if(debug >= 4)
            {
               printf("%s\n", svc_value((char *)NULL));
               fflush(stdout);
            }

            strcpy( status, svc_value( "stat" ));

            if(strcmp( status, "ERROR") == 0)
            {
               strcpy( msg, svc_value( "msg" ));

               printerr(msg);
            }
      
            if(!keepAll)
            {
               unlink("timages.tbl");

               sprintf(cmd, "rm -rf tiles/*_area.fits");

               if(debug >= 3)
               {
                  fprintf(fdebug, "%s\n", cmd);
                  fflush(fdebug);
               }

               system(cmd);
            }
         }

         time(&currtime);

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: mAdd             %6d sec \n", (int)(currtime - lasttime));
            fflush(fdebug);
         }

         if(finfo)
         {
            fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span></p>\n",
               (int)(currtime - lasttime));
            fprintf(finfo, "<hr style='color: #fefefe;' />\n");
            fflush(finfo);
         }

         lasttime = currtime;
      }



      /******************************/ 
      /* Save file if so instructed */
      /******************************/ 

      if(strlen(savefile) > 0)
      {
         if(nband == 1)
            sprintf(savetmp, "%s.fits", savefile);
         else
            sprintf(savetmp, "%s_%s.fits", savefile, band[iband]);

         fin   = fopen("mosaic.fits", "r" );

         if(fin == (FILE *)NULL)
         {
            sprintf(msg, "Can't open mosaic file: [mosaic.fits]");

            printerr(msg);
         }

         fsave = fopen( savetmp, "w+");

         if(fsave == (FILE *)NULL)
         {
            sprintf(msg, "Can't open save file: [%s]", savetmp);

            printerr(msg);
         }

         while(1)
         {
            count = fread(buf, sizeof(char), BUFSIZE, fin);

            if(count == 0)
               break;

            fwrite(buf, sizeof(char), count, fsave);
         }

         fflush(fsave);
         fclose(fsave);
         fclose(fin);

         time(&currtime);

         if(debug >= 1)
         {
            fprintf(fdebug, "TIME: Copy output      %6d sec  (%s)\n",
               (int)(currtime - lasttime), savetmp);
            fflush(fdebug);
         }

         lasttime = currtime;
      }


      /*******************************/ 
      /* Delete the corrected images */
      /* or the projected image if   */
      /* there was only the one      */
      /*******************************/ 

      if(baseCount > 1)
      {
         if(!noBackground)
         {
            ncols = topen("cimages.tbl");

            ifname = tcol( "fname");

            if(ifname < 0)
            {
               strcpy(msg, "Need column 'fname' in input");

               printerr(msg);
            }

            count = 0;

            while(1)
            {
               istat = tread();

               if(istat < 0)
                  break;

               strcpy(infile, filePath("corrected", tval(ifname)));

               if(!keepAll)
               {
                  unlink(infile);

                  strcpy(areafile, infile);
                  areafile[strlen(areafile) - 5] = '\0';
                  strcat(areafile, "_area.fits");

                  unlink(areafile);

                  count += 2;
               }
            }

            tclose();
         }

         tclose();

         if(!keepAll)
         {
            unlink("big_region.hdr");
            unlink("remote_big.tbl");
            unlink("pimages.tbl");

            if(!noBackground)
               unlink("cimages.tbl");

            unlink("diffs.tbl");
            unlink("fits.tbl");
            unlink("corrections.tbl");

            count += 11;

            if(!userRaw)
               rmdir ("raw");

            rmdir ("projected");
            rmdir ("diffs");
            rmdir ("corrected");
         }
      }
      else
      {
         ncols = topen("pimages.tbl");

         ifname = tcol("fname");

         if(ifname < 0)
         {
            strcpy(msg, "Need column 'fname' in input");

            printerr(msg);
         }

         count = 0;

         while(1)
         {
            istat = tread();

            if(istat < 0)
               break;

            strcpy(infile, filePath("projected", tval(ifname)));

            if(!keepAll)
            {
               unlink(infile);

               strcpy(areafile, infile);
               areafile[strlen(areafile) - 5] = '\0';
               strcat(areafile, "_area.fits");

               unlink(areafile);

               count += 2;
            }
         }

         tclose();

         if(!keepAll)
         {
            unlink("big_region.hdr");
            unlink("remote_big.tbl");
            unlink("pimages.tbl");

            count += 7;

            if(!userRaw)
               rmdir ("raw");

            rmdir ("projected");
            rmdir ("corrected");
         }
      }

      if(!keepAll && shrink != 1.)
      {
         sprintf(cmd, "rm -rf shrunken/*");

         if(debug >= 3)
         {
            fprintf(fdebug, "%s\n", cmd);
            fflush(fdebug);
         }

         system(cmd);
      }

      rmdir ("shrunken");

      if(!keepAll)
      {
         if(!noBackground)
         {
            sprintf(cmd, "rm -rf corrected");

            if(debug >= 4)
            {
               fprintf(fdebug, "%s\n", cmd);
               fflush(fdebug);
            }

            system(cmd);
         }

         system(cmd);
      }

      if(!noBackground)
         rmdir ("corrected");

      time(&currtime);

      if(debug >= 1)
      {
         fprintf(fdebug, "TIME: Delete workfiles %6d sec  (%d files)\n", (int)(currtime - lasttime), count);

         fflush(fdebug);
      }

      lasttime = currtime;


      /*****************************/
      /* Create PNG of final image */
      /*****************************/

      if(infoMsg)
      {
         printf("[struct stat=\"INFO\", msg=\"Creating presentation\"]\n");
         fflush(stdout);
      }

      factor = 1.0;

      if(showMarker)
         sprintf(cmd, "mViewer -saturate 255 -ct 1 -mark %.6f %.6f eq J2000 7 red -gray mosaic.fits -2s max gaussian-log -out mosaic.png",
               rac, decc);
      else
         sprintf(cmd, "mViewer -saturate 255 -ct 1 -gray mosaic.fits -2s max gaussian-log -out mosaic.png");


      if(debug >= 4)
      {
         fprintf(fdebug, "[%s]\n", cmd);
         fflush(fdebug);
      }

      if(finfo)
      {
         fprintf(finfo, "<p><h3>Creating band %s grayscale PNG file</h3></p>\n", band[iband]);
         fprintf(finfo, "<p><span style='color: blue;'><tt>%s</tt></span></p>\n", cmd);
         fflush(finfo);
      }

      svc_run(cmd);

      if(debug >= 4)
      {
         printf("%s\n", svc_value((char *)NULL));
         fflush(stdout);
      }

      strcpy( status, svc_value( "stat" ));

      if(strcmp( status, "ERROR") == 0)
      {
         strcpy( msg, svc_value( "msg" ));

         printerr(msg);
      }

      time(&currtime);

      if(debug >= 1)
      {
         fprintf(fdebug, "TIME: mViewer          %6d sec \n", (int)(currtime - lasttime));
         fflush(fdebug);
      }

      if(finfo)
      {
         fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span></p>\n",
            (int)(currtime - lasttime));
         fprintf(finfo, "<hr style='color: #000000; height: 5px;' />\n");
         fflush(finfo);
      }

      lasttime = currtime;


      /**********************************************/ 
      /* Create the index.html file for the results */
      /**********************************************/ 

      ncell = 4;

      if(factor != 1.0)
         ++ncell;

      if(ntile*mtile > 1)
         ++ncell;

      if(strlen(labelText) == 0)
      {
         strcpy(labelText, locText);
         strcpy(locText, "");
      }

      fhtml = fopen("index.html", "w+");

      fprintf(fhtml, "<html>\n");
      fprintf(fhtml, "<head>\n");
      fprintf(fhtml, "<title>%s Mosaic</title>\n", labelText);
      fprintf(fhtml, "<body bgcolor='#ffffff'>\n");
      fprintf(fhtml, "<center>\n");
      fprintf(fhtml, "<table cellpadding='3' border='1'>\n");

      fprintf(fhtml, "  <tr>\n");

      fprintf(fhtml, "    <td colspan='%d' align='center' bgcolor='#669999'>\n", ncell);

      fprintf(fhtml, "<table width=100%% cellpadding=5 border=0><tr>\n");
      fprintf(fhtml, "<td align=left><a href='https://www.nsf.gov/awardsearch/showAward?AWD_ID=1440620&HistoricalAwards=false'><img border=0 src='http://montage.ipac.caltech.edu/images/NSF_logo.gif'></td>\n");
      fprintf(fhtml, "<td align=center><font color='#ffffff' size=+3><b>%s</b></font></td>\n", labelText);
      fprintf(fhtml, "<td align=right><a href='http://montage.ipac.caltech.edu'><img border=0 src='http://montage.ipac.caltech.edu/images/ipac_logo.png' height=50></a></td>\n");
      fprintf(fhtml, "</tr></table>\n");

      fprintf(fhtml, "<table width=100%% cellpadding=5 border=0><tr>\n");

      if(strlen(locText) > 0)
         fprintf(fhtml, "<td align=left><font color='#ffff00'>%s</font></td>\n", locText);

      fprintf(fhtml, "<td align=center><font color='#ffff00'>(%s)</font></td>\n", locstr);
      fprintf(fhtml, "<td align=center><font color='#ffff00'>Size: %s degrees</font></td>\n", sizestr);
      fprintf(fhtml, "<td align=right><font color='#ffff00'>%s / %s</font></td></tr>\n", survey[iband], band[iband]);
      fprintf(fhtml, "</tr></table>\n");

      fprintf(fhtml, "    </td>\n");
      fprintf(fhtml, "  </tr>\n");
      fprintf(fhtml, "  <tr>\n");

      fprintf(fhtml, "    <td colspan='%d' bgcolor='#669999'>\n", ncell);

      fprintf(fhtml, "      <center>\n");
      fprintf(fhtml, "        <img src='mosaic.png' width=600 alt='Region mosaic image'><br>\n");
      fprintf(fhtml, "      </center>\n");
      fprintf(fhtml, "    </td>\n");
      fprintf(fhtml, "   </tr>\n");
      fprintf(fhtml, "   <tr>\n");
      fprintf(fhtml, "     <td bgcolor='#669999'>\n");
      fprintf(fhtml, "      <center>\n");
      fprintf(fhtml, "      <a href='mosaic.fits'><font size=-1>Mosaic&nbsp;in<br>FITS&nbsp;format</a></font>\n");
      fprintf(fhtml, "      </center>\n");
      fprintf(fhtml, "     </td>\n");

      fprintf(fhtml, "     <td bgcolor='#669999'>\n");
      fprintf(fhtml, "      <center>\n");
      fprintf(fhtml, "      <a href='mosaic_area.fits'><font size=-1>Coverage&nbsp;map<br>in&nbsp;FITS&nbsp;format</a></font>\n");
      fprintf(fhtml, "      </center>\n");
      fprintf(fhtml, "     </td>\n");

      if(factor != 1.0)
      {
         fprintf(fhtml, "     <td bgcolor='#669999'>\n");
         fprintf(fhtml, "      <center>\n");
         fprintf(fhtml, "      <a href='mosaic_small.fits'><font size=-1>%-gx&nbsp;shrunken&nbsp;version&nbsp;of&nbsp;FITS&nbsp;image<br>(used&nbsp;to&nbsp;make&nbsp;above&nbsp;PNG)</font></a>\n", factor);
         fprintf(fhtml, "      </center>\n");
         fprintf(fhtml, "     </td>\n");
      }

      fprintf(fhtml, "     <td bgcolor='#669999'>\n");
      fprintf(fhtml, "      <center>\n");
      fprintf(fhtml, "      <a href='rimages.tbl' target='montageinfo'><font size=-1>List&nbsp;of&nbsp;input<br>images</font></a>\n");
      fprintf(fhtml, "      </center>\n");
      fprintf(fhtml, "     </td>\n");
      fprintf(fhtml, "     <td bgcolor='#669999'>\n");
      fprintf(fhtml, "      <center>\n");
      fprintf(fhtml, "      <a href='region.hdr' target='montageinfo'><font size=-1>FITS&nbsp;header&nbsp;from<br>mosaic&nbsp;file</font></a>\n");
      fprintf(fhtml, "      </center>\n");
      fprintf(fhtml, "     </td>\n");

      if(ntile*mtile > 1)
      {
         fprintf(fhtml, "     <td bgcolor='#669999'>\n");
         fprintf(fhtml, "      <center>\n");
         fprintf(fhtml, "      <a href='iles' target='montageinfo'><font size=-1>Tiled subimages of mosaic region</font></a>\n");
         fprintf(fhtml, "      </center>\n");
      }

      fprintf(fhtml, "     </td>\n");
      fprintf(fhtml, "  </tr>\n");
      fprintf(fhtml, "</table><br>\n");
      fprintf(fhtml, "<font color='#000000'><b>Powered by</b> <a href='http://montage.ipac.caltech.edu'>\n");
      fprintf(fhtml, "<font color='#880000'><b>Montage</b><font></a></font><p>\n");

      if(strlen(contactText) > 0)
         fprintf(fhtml, "%s\n", contactText);
      
      fprintf(fhtml, "</center>\n");
      fprintf(fhtml, "</body>\n");
      fprintf(fhtml, "</html>\n");

      fclose(fhtml);

      chdir(cwd);
   }


   /**********************************************/
   /* If an output PNG was requested, make it at */
   /* full resolution (grayscale or color)       */
   /**********************************************/

   if(strlen(pngFile) > 0)
   {
      if(debug >= 1)
      {
         fprintf(fdebug, "PNG file: [%s]\n", pngFile);
         fflush(fdebug);
      }

   if(strlen(pngFile) > 0)
   {
      if(nband == 1)
      {
         if(finfo)
         {
            fprintf(finfo, "<p><h3>Generating output grayscale PNG file</h3></p>\n");
            fflush(finfo);
         }

         if(showMarker)
            sprintf(cmd, "mViewer -saturate 255 -ct 1 -mark %.6f %.6f eq J2000 7 red -gray %s/mosaic.fits -2s max gaussian-log -out %s",
               rac, decc, workspace[0], pngFile);
         else
            sprintf(cmd, "mViewer -saturate 255 -ct 1 -gray %s/mosaic.fits -2s max gaussian-log -out %s", workspace[0], pngFile);
      }

      else if(nband == 2)
      {
         if(finfo)
         {
            fprintf(finfo, "<p><h3>Generating output blue/red color PNG file</h3></p>\n");
            fflush(finfo);
         }

         if(showMarker)
            sprintf(cmd, "mViewer -saturate 255 -ct 1 -mark %.6f %.6f eq J2000 7 red -blue %s/mosaic.fits -2s max gaussian-log -red %s/mosaic.fits -2s max gaussian-log -out %s",
               rac, decc, workspace[0], workspace[1], pngFile);
         else
            sprintf(cmd, "mViewer -saturate 255 -ct 1 -blue %s/mosaic.fits -2s max gaussian-log -red %s/mosaic.fits -2s max gaussian-log -out %s", 
               workspace[0], workspace[1], pngFile);
      }

      else if(nband == 3)
      {
         if(finfo)
         {
            fprintf(finfo, "<p><h3>Generating output full color PNG file</h3></p>\n");
            fflush(finfo);
         }

         if(showMarker)
            sprintf(cmd, "mViewer -saturate 255 -mark %.6f %.6f eq J2000 7 red -blue %s/mosaic.fits -0.50s max gaussian-log -green %s/mosaic.fits -0.50s max gaussian-log -red %s/mosaic.fits -0.50s max gaussian-log -out %s",
               rac, decc, workspace[0], workspace[1], workspace[2], pngFile);
         else
            sprintf(cmd, "mViewer -saturate 255 -blue %s/mosaic.fits -0.50s max gaussian-log -green %s/mosaic.fits -0.50s max gaussian-log -red %s/mosaic.fits -0.50s max gaussian-log -out %s", 
               workspace[0], workspace[1], workspace[2], pngFile);
      }

      if(debug >= 4)
      {
         fprintf(fdebug, "%s\n", cmd);
         fflush(fdebug);
      }

      if(finfo)
      {
         fprintf(finfo, "<p><span style='color: blue;'><tt>%s</tt></span></p>\n", cmd);
         fflush(finfo);
      }

      svc_run(cmd);

      if(debug >= 4)
      {
         printf("%s\n", svc_value((char *)NULL));
         fflush(stdout);
      }

      strcpy( status, svc_value( "stat" ));

      if(strcmp( status, "ERROR") == 0)
      {
         strcpy( msg, svc_value( "msg" ));

         printerr(msg);
      }

      time(&currtime);

      if(debug >= 1)
      {
         fprintf(fdebug, "TIME: mViewer (color)  %6d sec \n", (int)(currtime - lasttime));
         fflush(fdebug);
      }

      if(finfo)
      {
         fprintf(finfo, "<p>TIME: <span style='color:red; font-size: 16px;'>%d sec</span></p>\n",
            (int)(currtime - lasttime));
         fprintf(finfo, "<hr style='color: #fefefe;' />\n");
         fflush(finfo);
      }

      lasttime = currtime;
   }


   /**************************************/ 
   /* Delete everything if so instructed */
   /**************************************/ 

   if(!keepAll && ntile*mtile == 1)
   {
      for(iband=0; iband<nband; ++iband)
      {
         sprintf(cmd, "rm -rf %s", workspace[iband]);

         if(debug >= 2)
         {
            fprintf(fdebug, "%s\n", cmd);
            fflush(fdebug);
         }

         system(cmd);
      }
   }


   /*************/
   /* Finish up */
   /*************/

   time(&currtime);

   if(debug)
   {
      if(debug == 1)
         fprintf(fdebug, "\n");

      fprintf(fdebug, "Mosaic complete.       %6d sec  (total)\n\n", 
             (int)(currtime - start));
      fflush(fdebug);
   }

   if(infoMsg)
   {
      printf("[struct stat=\"INFO\", msg=\"Mosaic complete (%d sec)\"]\n", 
             (int)(currtime - start));
      fflush(stdout);
   }

   if(finfo)
   {
      fprintf(finfo, "<p>Total processing time: <span style='color:red; font-size: 16px;'>%d sec</span></p>\n",
         (int)(currtime - start));
      fprintf(finfo, "</body>\n");
      fprintf(finfo, "</html>\n");
      fflush(finfo);
      fclose(finfo);
   }

   printf("[struct stat=\"OK\"]\n");
   fflush(stdout);

   exit(0);
}


int printerr(char *str)
{
   if(finfo)
   {
      fprintf(finfo, "<p><h3>ERROR</h3></p>\n");
      fprintf(finfo, "<p><span style='color:red; font-size: 16px;'>%s</span></p>\n", str);
      fprintf(finfo, "</body>\n");
      fprintf(finfo, "</html>\n");
      fflush(finfo);
      fclose(finfo);
   }

   printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", str);
   fflush(stdout);
   exit(1);
}


/* stradd adds the string "card" to a header line, and */
/* pads the header out to 80 characters.               */

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



/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

int FITSerror(char *fname, int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   sprintf(msg, "Bad FITS file [%s]",
      fname);

   printerr(msg);

   return 0;
}
