/* Module: mImgtbl.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.11     R. Moseley       09Mar26  Fixed /tmp fd leak for gzipped FITS;
                                   close mkstemp fd before unlink in
                                   mImgtbl_get_list and mImgtbl_get_files.
                                   Improved inline documentation throughout.
1.10     John Good        29Sep04  Added file size in MByte to table
1.9      John Good        12Aug04  Made tmp file for unzip unique
1.8      John Good        18Mar04  Added mode to read the candidate
                                   image list from a table file
1.7      John Good        14Jan03  Added "bad image" output option.
1.6      John Good        25Nov03  Added extern optarg references
1.5      John Good        23Aug03  Added 'status file' output mode.
                                   Added check for trailing slash on
                                   file path.  Added processing for
                                   "-f" (additional keyword) flag
1.4      John Good        27Jun03  Added a few comments for clarity
1.3      John Good        04May03  Added check for ordering of corners
1.2      John Good        18Mar03  Added a count for bad FITS files
                                   to the output.
1.1      John Good        14Mar03  Modified to use only full-path
                                   or no-path file names and to 
                                   use getopt() for command-line 
                                   parsing. Check to see if directory
                                   exists.
1.0      John Good        29Jan03  Baseline code


From original get_files.c:

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.7      John Good        17Jul04  Use unique temporary file for unzipping
1.6      John Good        01Jul04  Add path to name if using "./"
                                                       construct.
1.5      John Good        25Aug03  Added status file processing
1.4      John Good        27Jun03  Added a few comments for clarity
1.3      John Good        18Mar03  Added processing for "bad FITS" count
1.2      John Good        14Mar03  Modified to use only full-path
                                                       or no-path file names
1.1      John Good        12Feb03  Changed error msg for non-existant dir
1.0      John Good        29Jan03  Baseline code


From original get_hdr.c:

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.9      John Good        29Oct13  Invalid WCS keywords can cause
                                   library to seg fault. Check.
1.8      John Good        09Oct12  Fixed "clockwise" processing.
1.7      John Good        14Jan04  Added "bad image" output option.
1.6.1    Anastasia Laity  03Sep03  Fixed read_fits_keyword problem
                                   with status value by resetting
                                   status to 0 after each attempt
                                   to read keywords
1.6      John Good        21Aug03  Flipped and rotated cdelts under
                                   certain conditions (see code).
1.5.1    A. C. Laity      30Jun03  Added explanatory comments at top
1.5      John Good        22Mar03  Fixed processing of "bad headers"
1.4      John Good        22Mar03  Renamed wcsCheck to checkWCS for
                                   consistency.
1.3      John Good        18Mar03  Added processing for "bad FITS"
                                   count.
1.2      John Good        14Mar03  Minor modification associated
                                   with removing leading "./" 
                                   from file names
1.1      John Good        13Mar03  Added WCS header check
1.0      John Good        29Jan03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <fitshead.h>
#include <fitsio.h>
#include <coord.h>
#include <wcs.h>
#include <coord.h>
#include <mtbl.h>
#include <montage.h>
#include <mImgtbl.h>

#define MAXLEN 100000
#define MAXSTR 1024

FILE *fdopen(int fildes, const char *mode);

int mkstemp(char *template);

static int   mImgtbl_debug;
static int   recursiveMode;
static int   noGZIP;
static int   showCorners;
static int   showbad;
static int   processAreaFiles;

static int   cntr;
static int   failed;
static int   hdrlen;
static FILE *tblf;
static FILE *ffields;

typedef struct
{
 char name  [128];
 char type  [128];
 char value [128];
 char defval[128];
 int  width;
}
FIELDS;

static FIELDS *fields = (FIELDS *)NULL;
static int     nfields;

static int  ncube = 9;

static char cname [9][32] = {"NAXIS", "NAXIS3", "CRVAL3", "CDELT3", "CRPIX3", "NAXIS4", "CRVAL4", "CDELT4", "CRPIX4"};
static char ctype [9][32] = {"int",   "int",    "double", "double", "double", "int",    "double", "double", "double"};
static int  cwidth[9]     = { 6,       6,        22,       22,       22,       6,        22,       22,       22};

static int     info    = 0;
static int     nfile   = 0;
static int     badfile = 0;
static int     nhdu    = 0;
static int     nbadwcs = 0;
static int     nwrite  = 0;
static int     badwcs  = 0;

static struct Hdr_rec hdr_rec;


static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/*  mImgtbl                                                              */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mImgtbl, makes a list (with WCS information) of all     */
/*  FITS image files in the named directory (and optionally recursively) */
/*                                                                       */
/*  mImgtbl supports a lot of variation in input (and a little in        */
/*  output) so there are lot of parameters on the call, most of which    */
/*  can be "defaulted" (i.e. set to zero / NULL).  The full parameter    */
/*  list is:                                                             */
/*                                                                       */
/*   char *pathname        Directory (or top of tree) for image file     */
/*                         search (default to current directory).        */
/*   char *tblname         Output table file name (no default).          */
/*                                                                       */
/*   int recursiveMode     Search for images recursively (default just   */
/*                         top directory).                               */
/*   int processAreaFiles  Include "area" files in the list (not         */
/*                         normally advisable).                          */
/*   int haveCubes         False if we know we don't have datacubes      */
/*                         (fewer table columns)                         */
/*   int noGZIP            mImgtbl normally includes analysis of         */
/*                         .fits.gz files.  This turns that off.         */
/*   int showCorners       Include ra1,dec1, ... dec4 image corner       */
/*                         columns in output.                            */
/*   int showinfo          For HDUs that are are rejected, emit an       */
/*                         INFO message.  Best used in application       */
/*                         mode or when debugging.                       */
/*   int showbad           Show running "INFO" messages for FITS files   */
/*                         that cannot be opened by CFITSIO.             */
/*                                                                       */
/*   char *imgListFile     Rather than searching through directories,    */
/*                         get the list of images from a table file.     */
/*   char *fieldListFile   List of FITS keywords to include in output    */
/*                         table (in addition to the standard WCS info). */
/*                                                                       */
/*   int debug             Turn on debugging output (not for general     */
/*                         use).                                         */
/*                                                                       */
/*************************************************************************/

struct mImgtblReturn *mImgtbl(char *pathnamein, char *tblname,
                              int recursiveModein, int processAreaFilesin, int haveCubes, int noGZIPin, 
                              int showCornersin, int showinfo, int showbadin, char *imgListFile, char *fieldListFile,
                              int debugin)
{
   int   i, istat, ncols, ifname;

   char  pathname [1024];
   char  line     [1024];
   char *end;

   int   maxfields;

   char *ptr, *pname, *ptype, *pwidth;

   struct stat type;

   struct mImgtblReturn *returnStruct;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mImgtblReturn *)malloc(sizeof(struct mImgtblReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /* Set global control variables from input parameters */
   mImgtbl_debug    = debugin;
   info             = showinfo;
   recursiveMode    = recursiveModein;
   processAreaFiles = processAreaFilesin;
   noGZIP           = noGZIPin;
   showCorners      = showCornersin;
   showbad          = showbadin;

#ifdef WINDOWS
   /* Special processing for Windows: CFITSIO/gunzip integration may vary, 
      so we disable GZIP processing by default. */
   noGZIP = 1;
#endif

   cntr   = 0;
   failed = 0;


   /* Check for null pathname; default to current directory if not provided */
   if(pathnamein == (char *)NULL)
      strcpy(pathname, "./");
   else
      strcpy(pathname, pathnamein);


   /* Handle additional FITS header fields requested by the user. 
      These will be added as extra columns in the output table. */
   if(nfields > 0)
      free(fields);

   nfields   = 0;
   maxfields = 32;

   fields = (FIELDS *) malloc(maxfields * sizeof(FIELDS));

   if(fieldListFile != (char *)NULL && strlen(fieldListFile) > 0)
   {
      if((ffields = fopen(fieldListFile, "r")) == (FILE *)NULL)
      {
         sprintf(returnStruct->msg, "Cannot open field list file: %s", fieldListFile);
         return returnStruct;
      }

      /* Parse the field list file (comma or whitespace separated) */
      while(fgets(line, 1024, ffields) != (char *)NULL)
      {
         while(line[strlen(line)-1] == '\r'
            || line[strlen(line)-1] == '\n')
               line[strlen(line)-1]  = '\0';

         ptr = line;

         end = line + strlen(line);

         /* Skip leading whitespace/delimiters */
         while(ptr < end &&
              (*ptr == ' ' || *ptr == '\t' || *ptr == ','))
            ++ptr;
            
         if(ptr == end)
            break;

         pname = ptr;

         /* Extract field name */
         while(ptr < end &&
              *ptr != ' ' && *ptr != '\t' && *ptr != ',')
            ++ptr;
      
         *ptr = '\0';
         ++ptr;
         
         while(ptr < end &&
              (*ptr == ' ' || *ptr == '\t' || *ptr == ','))
            ++ptr;

         ptype = ptr;
   
         /* Extract field type */
         while(ptr < end && 
              *ptr != ' ' && *ptr != '\t' && *ptr != ',')
            ++ptr;

         *ptr = '\0';
         ++ptr;

         while(ptr < end &&
              (*ptr == ' ' || *ptr == '\t' || *ptr == ','))
            ++ptr;
 
         pwidth = ptr;

         /* Extract field width */
         while(ptr < end && 
              *ptr != ' ' && *ptr != '\t' && *ptr != ',')
            ++ptr;

         *ptr = '\0';

         strcpy(fields[nfields].name, pname);
         strcpy(fields[nfields].type, ptype);

         fields[nfields].width = atoi(pwidth);

         /* Ensure width is at least as long as the column name */
         if(strlen(fields[nfields].name) > fields[nfields].width)
            fields[nfields].width = strlen(fields[nfields].name);

         if(strlen(fields[nfields].name) < 1)
         {
            free(fields);
            sprintf(returnStruct->msg, "Illegal field name (line %d)", nfields);
            return returnStruct;
         }

         if(strlen(fields[nfields].type) < 1)
         {
            free(fields);
            sprintf(returnStruct->msg, "Illegal field type (line %d)", nfields);
            return returnStruct;
         }

         strcpy(fields[nfields].value,  "");
         strcpy(fields[nfields].defval, "");

         if(mImgtbl_debug)
         {
            printf("DEBUG> fields[%d]: [%s][%s][%s]\n",
               nfields, pname, ptype, pwidth);
            fflush(stdout);
         }

         ++nfields;

         /* Expand the fields array if needed */
         if(nfields >= maxfields)
         {
            maxfields += 32;

            fields = (FIELDS *)
                         realloc(fields, maxfields * sizeof(FIELDS));
         }
      }
   }


   /* If cube processing is enabled, add 3rd and 4th dimension keywords
      to the fields list so they are extracted and written as output columns */
   if(haveCubes)
   {
      for(i=0; i<ncube; ++i)
      {
         strcpy(fields[nfields].name, cname[i]);
         strcpy(fields[nfields].type, ctype[i]);

         fields[nfields].width = cwidth[i];

         if(mImgtbl_debug)
         {
            printf("DEBUG> fields[%d]: [%s][%s][%d] (cube info)\n",
               nfields, fields[nfields].name, fields[nfields].type, fields[nfields].width);
            fflush(stdout);
         }

         ++nfields;

         if(nfields >= maxfields)
         {
            maxfields += 32;
            
            fields = (FIELDS *)
                         realloc(fields, maxfields * sizeof(FIELDS));
         }
      }
   }


   /* Verify that the target directory exists and is actually a directory */
   istat = stat(pathname, &type);

   if(istat < 0)
   {
      sprintf(returnStruct->msg, "Cannot access %s", pathname);
      return returnStruct;
   }

   else if (S_ISDIR(type.st_mode) != 1)
   {
      sprintf(returnStruct->msg, "%s is not a directory", pathname);
      return returnStruct;
   }


   /* Determine how much of the file path to strip for the table output */
   hdrlen = 0;
   if(pathname[0] != '/')
      hdrlen = strlen(pathname);

   if(hdrlen && pathname[strlen(pathname) - 1] != '/')
      ++hdrlen;

   if(mImgtbl_debug)
   {
      printf("DEBUG: path = [%s](%d)\n", pathname, hdrlen);
      fflush(stdout);
   }

   /* Open the output table file */
   tblf = fopen(tblname, "w+");

   if(tblf == (FILE *)NULL)
   {
       sprintf(returnStruct->msg, "Can't open output table.");
       return returnStruct;
   }


   /* Two modes for finding files: 
      1) From an explicit image list table (imgListFile)
      2) By scanning the directory (default) */

   if(imgListFile != (char *)NULL && strlen(imgListFile) > 0)
   {
      ncols = topen(imgListFile);

      if(ncols < 1)
      {
         sprintf(returnStruct->msg, "Cannot open image list file: %s", imgListFile);
         return returnStruct;
      }

      /* The list table must have a column named 'fname' or 'file' */
      ifname = tcol( "fname");

      if(ifname < 0)
         ifname = tcol( "file");

      if(ifname < 0)
      {
         sprintf(returnStruct->msg, "Image table needs column fname/file");
         return returnStruct;
      }

      if(mImgtbl_get_list(pathname, ifname) > 0)
      {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }
   else
   {
      /* Scan the directory for FITS files */
      if(mImgtbl_get_files(pathname) > 0)
      {
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      }
   }

   fclose(tblf);


   /* Final pass to format the table (padding columns for fixed-width output) */
   if(mImgtbl_update_table(tblname) > 0)
   {
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }

   /* Populate return structure with summary statistics */
   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d, nfile=%d, nhdu=%d, badfits=%d, badwcs=%d",
      cntr, nfile, nhdu, badfile, badwcs);

   sprintf(returnStruct->json, "{\"count\":%d, \"nfile\":%d, \"nhdu\":%d, \"badfits\":%d, \"badwcs\":%d}", 
      cntr, nfile, nhdu, badfile, badwcs);

   returnStruct->count   = cntr;
   returnStruct->nfile   = nfile;
   returnStruct->nhdu    = nhdu;
   returnStruct->badfits = badfile;
   returnStruct->badwcs  = badwcs;

   return returnStruct;
}



/* mImgtbl_get_list reads the image filenames from a provided list table
   and processes each one. */

int mImgtbl_get_list (char *pathname, int ifname)
{
   char dirname [MAXLEN], msg  [MAXLEN];
   char tempfile[MAXLEN], cmd  [MAXLEN];
   char fname   [MAXLEN];

   int  fd, istatus, len;

   struct stat type;
  

   while (1)
   {
      /* Read next record from the image list table */
      istatus = tread();

      if(istatus < 0)
         break;

      strcpy(fname, tval(ifname));

      if(mImgtbl_debug)
      {
         printf("DEBUG:  entry [%s]\n", fname);
         fflush(stdout);
      }

      /* Construct full path to the image file */
      sprintf (dirname, "%s/%s", pathname, fname);

      strcpy (hdr_rec.fname, fname);

      if(mImgtbl_debug)
      {
         printf("DEBUG: [%s] -> [%s]\n", dirname, hdr_rec.fname);
         fflush(stdout);
      }

      /* Check if the file exists and is accessible */
      if (stat(dirname, &type) == 0) 
      {
         len = strlen(dirname);

         if(mImgtbl_debug)
         {
            printf("DEBUG: Found file      [%s]\n", dirname);
            fflush(stdout);
         }

         /* Skip compressed files if noGZIP is set */
         if(noGZIP && strncmp(dirname+len-3,  ".gz", 3) == 0)
            continue;

         /* Optionally skip Montage area files */
         if(!processAreaFiles)
         {
            if ((strncmp(dirname+len-9,  "_area.fit",     9 ) == 0) ||
                (strncmp(dirname+len-9,  "_area.FIT",     9 ) == 0) || 
                (strncmp(dirname+len-10, "_area.fits",    10) == 0) || 
                (strncmp(dirname+len-10, "_area.FITS",    10) == 0) ||
                (strncmp(dirname+len-12, "_area.fit.gz",  12) == 0) ||
                (strncmp(dirname+len-12, "_area.FIT.gz",  12) == 0) || 
                (strncmp(dirname+len-13, "_area.fits.gz", 13) == 0) || 
                (strncmp(dirname+len-13, "_area.FITS.gz", 13) == 0)) 
               continue;
         }

         /* Process FITS files (including compressed ones) */
         if ((strncmp(dirname+len-4, ".fit",     4) == 0) ||
             (strncmp(dirname+len-4, ".FIT",     4) == 0) || 
             (strncmp(dirname+len-5, ".fits",    5) == 0) || 
             (strncmp(dirname+len-5, ".FITS",    5) == 0) ||
             (strncmp(dirname+len-7, ".fit.gz",  7) == 0) ||
             (strncmp(dirname+len-7, ".FIT.gz",  7) == 0) || 
             (strncmp(dirname+len-8, ".fits.gz", 8) == 0) || 
             (strncmp(dirname+len-8, ".FITS.gz", 8) == 0)) 
         { 
            msg[0] = '\0';

            /* If it's a gzipped FITS file, gunzip it to a temporary file first */
            if((strncmp(dirname+len-7, ".fit.gz",  7) == 0) ||
               (strncmp(dirname+len-7, ".FIT.gz",  7) == 0) || 
               (strncmp(dirname+len-8, ".fits.gz", 8) == 0) || 
               (strncmp(dirname+len-8, ".FITS.gz", 8) == 0)) 
            {
               strcpy(tempfile, "/tmp/IMXXXXXX");

               fd = mkstemp(tempfile);

               if(fd < 0)
               {
                  sprintf(montage_msgstr, "Can't create temporary input file for gunzip output.");
                  return 1;
               }

               /* FIX: close the file descriptor before any further processing.
                  The original code called unlink() while this fd was still open.
                  On Unix, unlink() only removes the directory entry -- the kernel
                  keeps the data blocks on disk until the last open fd is closed.
                  Since the fd was never closed, every .gz file processed left a
                  nameless temp file consuming space in /tmp (visible in lsof as
                  "(deleted)"). On large datasets this filled /tmp entirely. */
               close(fd);

               sprintf(cmd, "gunzip -c %s > %s", dirname, tempfile);
               system(cmd);

               istatus = mImgtbl_get_hdr (tempfile, &hdr_rec, msg);

               if (istatus != 0)
                  failed += istatus;

               unlink(tempfile);
            }
            else
            {
               /* Process regular FITS file */
               istatus = mImgtbl_get_hdr (dirname, &hdr_rec, msg);

               if (istatus != 0) 
                  failed += istatus;
            }
         }
      }
   }

   return 0;
}



/* mImgtbl_get_files recursively scans a directory tree for FITS files 
   and calls mImgtbl_get_hdr for each one found. */

int mImgtbl_get_files (char *pathname)
{
   char            dirname[MAXSTR], msg[MAXSTR];
   char            tempfile[MAXSTR], cmd[MAXSTR];
   int             fd, istatus, len;
   DIR            *dp;
   struct dirent  *entry;
   struct stat     type;


   dp = opendir (pathname);

   if(mImgtbl_debug)
   {
      printf("DEBUG: Opening path    [%s]\n", pathname);
      fflush(stdout);
   }

   if (dp == NULL) 
      return 0;

   while ((entry=(struct dirent *)readdir(dp)) != (struct dirent *)0) 
   {
      if(mImgtbl_debug)
      {
         printf("DEBUG:  entry [%s]\n", entry->d_name);
         fflush(stdout);
      }

      /* Construct the full path and the relative filename for the table */
      sprintf (dirname, "%s/%s", pathname, entry->d_name);

      if(strncmp(dirname, "./", 2) == 0)
         strcpy (hdr_rec.fname, dirname+2);
      else
         strcpy (hdr_rec.fname, dirname+hdrlen);

      if(mImgtbl_debug)
      {
         printf("DEBUG: [%s] -> [%s]\n", dirname, hdr_rec.fname);
         fflush(stdout);
      }

      if (stat(dirname, &type) == 0) 
      {
         /* If it's a directory, descend recursively if recursiveMode is set */
         if (S_ISDIR(type.st_mode) == 1)
         {
            if (recursiveMode
            && (strcmp(entry->d_name, "." ) != 0)
            && (strcmp(entry->d_name, "..") != 0))
            {
               if(mImgtbl_debug)
               {
                  printf("DEBUG: Found directory [%s]\n", dirname);
                  fflush(stdout);
               }

               if(mImgtbl_get_files (dirname) > 1)
                  return 1;
            }
         }
         else
         {
            /* Check if the file is a FITS file based on its extension */
            len = strlen(dirname);

            if(mImgtbl_debug)
            {
               printf("DEBUG: Found file      [%s]\n", dirname);
               fflush(stdout);
            }

            if(noGZIP && strncmp(dirname+len-3,  ".gz", 3) == 0)
               continue;

            if(!processAreaFiles)
            {
               if ((strncmp(dirname+len-9,  "_area.fit",     9 ) == 0) ||
                   (strncmp(dirname+len-9,  "_area.FIT",     9 ) == 0) || 
                   (strncmp(dirname+len-10, "_area.fits",    10) == 0) || 
                   (strncmp(dirname+len-10, "_area.FITS",    10) == 0) ||
                   (strncmp(dirname+len-12, "_area.fit.gz",  12) == 0) ||
                   (strncmp(dirname+len-12, "_area.FIT.gz",  12) == 0) || 
                   (strncmp(dirname+len-13, "_area.fits.gz", 13) == 0) || 
                   (strncmp(dirname+len-13, "_area.FITS.gz", 13) == 0)) 
                  continue;
            }

            if ((strncmp(dirname+len-4, ".fit",     4) == 0) ||
                (strncmp(dirname+len-4, ".FIT",     4) == 0) || 
                (strncmp(dirname+len-5, ".fits",    5) == 0) || 
                (strncmp(dirname+len-5, ".FITS",    5) == 0) ||
                (strncmp(dirname+len-7, ".fit.gz",  7) == 0) ||
                (strncmp(dirname+len-7, ".FIT.gz",  7) == 0) || 
                (strncmp(dirname+len-8, ".fits.gz", 8) == 0) || 
                (strncmp(dirname+len-8, ".FITS.gz", 8) == 0)) 
            { 
               msg[0] = '\0';

               /* If gzipped, decompress to a temporary file before processing */
               if((strncmp(dirname+len-7, ".fit.gz",  7) == 0) ||
                  (strncmp(dirname+len-7, ".FIT.gz",  7) == 0) || 
                  (strncmp(dirname+len-8, ".fits.gz", 8) == 0) || 
                  (strncmp(dirname+len-8, ".FITS.gz", 8) == 0)) 
               {
                  strcpy(tempfile, "/tmp/IMTXXXXXX");

                  fd = mkstemp(tempfile);

                  if(fd < 0)
                  {
                     sprintf(montage_msgstr, "Can't create temporary input table.");
                     return 1;
                  }

                  /* FIX: close the file descriptor before any further processing.
                     The original code called unlink() while this fd was still open.
                     On Unix, unlink() only removes the directory entry -- the kernel
                     keeps the data blocks on disk until the last open fd is closed.
                     Since the fd was never closed, every .gz file processed left a
                     nameless temp file consuming space in /tmp (visible in lsof as
                     "(deleted)"). On large datasets this filled /tmp entirely. */
                  close(fd);

                  sprintf(cmd, "gunzip -c %s > %s", dirname, tempfile);
                  system(cmd);

                  istatus = mImgtbl_get_hdr (tempfile, &hdr_rec, msg);

                  unlink(tempfile);

                  if (istatus != 0)
                     failed += istatus;
               }
               else
               {
                  /* Process the FITS file */
                  istatus = mImgtbl_get_hdr (dirname, &hdr_rec, msg);

                  if (istatus != 0) 
                     failed += istatus;
               }
            }
         }
      }
   }

   closedir(dp);
   return 0;
}


/* mImgtbl_get_hdr reads the FITS headers from a file (potentially multiple HDUs)
   and parses the WCS values into a structure (Hdr_rec). It also handles
   validation to prevent library crashes and extracts user-specified keywords. */

int mImgtbl_get_hdr (char *fname, struct Hdr_rec *hdr_rec, char *msg)
{
   int       isDSS;
   char     *header;
   char      value[1024], comment[1024], *ptr;
   char     *checkWCS;
   fitsfile *fptr;
   int       nowcs, badhdr;
   int       i, status, csys, nfailed, first_failed, clockwise;
   double    lon, lat, equinox;
   double    ra2000, dec2000;
   double    ra, dec;
   double    x1, y1, z1;
   double    x2, y2, z2;

   double    dtr = 1.745329252e-2;

   struct WorldCoor *wcs;

   struct stat buf;

   nfailed      = 0;
   first_failed = 0;

   ++nfile;

   if(mImgtbl_debug)
   {
      printf("DEBUG> file = \"%s\"\n", fname);
      fflush(stdout);
   }

   /* Open the FITS file using CFITSIO */
   status = 0;
   if(fits_open_file(&fptr, fname, READONLY, &status)) 
   {
      sprintf (msg, "Cannot open FITS file %s", fname);

      ++badfile;

      if(info)
      {
         printf("Cannot open file \"%s\"\n", fname);
         fflush(stdout);
      }
      return (1);
   }

   /* Get file size */
   stat(fname, &buf);

   hdr_rec->size = buf.st_size;

   if(mImgtbl_debug)
   {
      printf("DEBUG> file size = %lld\n", (long long)hdr_rec->size);
      fflush(stdout);
   }

   hdr_rec->hdu = 0;

   /* Iterate through all HDUs in the file */
   while(1)
   {
      ++hdr_rec->hdu;

      nowcs  = 0;
      badhdr = 0;

      status = 0;
      if(fits_movabs_hdu(fptr, hdr_rec->hdu, NULL, &status))
         break;

      if(mImgtbl_debug)
      {
         printf("DEBUG> hdu  = %d\n", hdr_rec->hdu);
         fflush(stdout);
      }

      ++nhdu;


      /* Missing or invalid values for some keywords will cause the WCS library 
         initialization to segfault. We perform a series of manual checks here. */

      /* DSS plate projections are an exception and handle keywords differently. */
      isDSS = 1;
      status = 0;
      if(fits_read_keyword(fptr, "PLTRAH", value, comment, &status))
         isDSS = 0;

      if(!isDSS)
      {
         /* Check for CTYPE1 existence */
         status = 0;
         if(fits_read_keyword(fptr, "CTYPE1", value, comment, &status))
         {
            if(mImgtbl_debug)
            {
               printf("Missing CTYPE1 in file %s\n", fname);
               fflush(stdout);
            }

            wcs = (struct WorldCoor *)NULL;

            if(hdr_rec->hdu == 1)
               first_failed = 1;

            ++nfailed;
            ++badwcs;

            if(info)
            {
               printf("[struct stat=\"INFO\", msg=\"Missing CTYPE1\", file=\"%s\", hdu=%d]\n", fname, hdr_rec->hdu);
               fflush(stdout);
            }

            badhdr = 1;

            ++nbadwcs;

            if(!showbad)
               continue;
         }

         if(mImgtbl_debug)
         {
            printf("DEBUG> CTYPE1 check: [%s] badhdr -> %d\n", value, badhdr);
            fflush(stdout);
         }


         /* Validate CTYPE1 value format */
         if(!badhdr)
         {
            ptr = value;

            if(*ptr == '\'' && value[strlen(value)-1] == '\'')
            {
               value[strlen(value)-1] = '\0';
               ++ptr;
            }

            if(strlen(ptr) < 8)
               *ptr = '\0';

            while(*ptr != '-' && *ptr != '\0') ++ptr;
            while(*ptr == '-' && *ptr != '\0') ++ptr;

            if(strlen(ptr) == 0)
            {
               if(mImgtbl_debug)
               {
                  printf("Invalid CTYPE1 in file %s\n", fname);
                  fflush(stdout);
               }

               wcs = (struct WorldCoor *)NULL;

               if(hdr_rec->hdu == 1)
                  first_failed = 1;

               ++nfailed;
               ++badwcs;

               if(info)
               {
                  printf("[struct stat=\"INFO\", msg=\"Invalid CTYPE1\", file=\"%s\", hdu=%d]\n",
                     fname, hdr_rec->hdu);
                  
                  fflush(stdout);
               }

               badhdr = 1;

               ++nbadwcs;

               if(!showbad)
                  continue;
            }
         }

         if(mImgtbl_debug)
         {
            printf("DEBUG> CTYPE1 value check: badhdr -> %d\n", badhdr);
            fflush(stdout);
         }



         /* Check for CTYPE2 existence */
         if(!badhdr)
         {
            status = 0;
            if(fits_read_keyword(fptr, "CTYPE2", value, comment, &status))
            {
               if(mImgtbl_debug)
               {
                  printf("Missing CTYPE2 in file %s\n", fname);
                  fflush(stdout);
               }

               wcs = (struct WorldCoor *)NULL;

               if(hdr_rec->hdu == 1)
                  first_failed = 1;

               ++nfailed;
               ++badwcs;

               if(info)
               {
                  printf("[struct stat=\"INFO\", msg=\"Missing CTYPE2\", file=\"%s\", hdu=%d]\n", fname, hdr_rec->hdu);
                  
                  fflush(stdout);
               }

               badhdr = 1;

               ++nbadwcs;

               if(showbad)
                  continue;
            }
         }

         if(mImgtbl_debug)
         {
            printf("DEBUG> CTYPE2 check: [%s] badhdr -> %d\n", value, badhdr);
            fflush(stdout);
         }


         /* Validate CTYPE2 value format */
         if(!badhdr)
         {
            ptr = value;

            if(*ptr == '\'' && value[strlen(value)-1] == '\'')
            {
               value[strlen(value)-1] = '\0';
               ++ptr;
            }

            if(strlen(ptr) < 8)
               *ptr = '\0';

            while(*ptr != '-' && *ptr != '\0') ++ptr;
            while(*ptr == '-' && *ptr != '\0') ++ptr;

            if(strlen(ptr) == 0)
            {
               if(mImgtbl_debug)
               {
                  printf("Invalid CTYPE2 in file %s\n", fname);
                  fflush(stdout);
               }

               wcs = (struct WorldCoor *)NULL;

               if(hdr_rec->hdu == 1)
                  first_failed = 1;

               ++nfailed;
               ++badwcs;

               if(info)
               {
                  printf("[struct stat=\"INFO\", msg=\"Invalid CTYPE2\", file=\"%s\", hdu=%d]\n", fname, hdr_rec->hdu);
                  
                  fflush(stdout);
               }

               badhdr = 1;

               ++nbadwcs;

               if(!showbad)
                  continue;
            }
         }

         if(mImgtbl_debug)
         {
            printf("DEBUG> CTYPE2 value check: badhdr -> %d\n", badhdr);
            fflush(stdout);
         }
      }


      /* Extract additional keywords. If we are in HDU 1, we also save these as 
         defaults for subsequent HDUs in case they are global to the file. */
      if(hdr_rec->hdu == 1)
      {
         for(i=0; i<nfields; ++i)
         {
            status = 0;
            if(fits_read_keyword(fptr, fields[i].name, value, comment, &status))
               strcpy(fields[i].defval, "");

            else
            {
               ptr = value;

               if(*ptr == '\'' && value[strlen(value)-1] == '\'')
               {
                  value[strlen(value)-1] = '\0';
                  ++ptr;
               }

               strcpy(fields[i].defval, ptr);
            }
         }
      }

      if(hdr_rec->hdu == 2 && first_failed)
         --nfailed;

      nowcs = 1;

      /* Read the full WCS header from the HDU */
      if(!badhdr)
      {
         status = 0;
         if(fits_get_image_wcs_keys(fptr, &header, &status)) 
         {
            badhdr = 1;

            ++nbadwcs;

            if(!showbad)
               continue;
         }
         else
            nowcs = 0;
      }

      /* Initialize the WCS library structure */
      if(!nowcs)
      {
         wcs = wcsinit(header);

         if(mImgtbl_debug)
         {
            if(wcs == (struct WorldCoor *)NULL) 
            {
               printf("DEBUG> WCSINIT failed\n");
               fflush(stdout);
            }
            else
            {
               printf("DEBUG> WCSINIT OK\n");
               fflush(stdout);
            }
         }

         if(wcs == (struct WorldCoor *)NULL) 
         {
            if(hdr_rec->hdu == 1)
               first_failed = 1;

            ++nfailed;
            ++badwcs;
            ++nbadwcs;

            if(!badhdr)
            {
               if(info)
               {
                  printf("[struct stat=\"INFO\", msg=\"WCS lib init failure\", file=\"%s\", hdu=%d]\n", fname, hdr_rec->hdu);
                  
                  fflush(stdout);
               }

               badhdr = 1;

               ++nbadwcs;
            }

            if(!showbad)
               continue;
         } 

         /* Further validation of the WCS structure */
         checkWCS = montage_checkWCS(wcs); 

         if(checkWCS)
         {
            if(mImgtbl_debug)
            {
               printf("Bad WCS for file %s\n", fname);
               fflush(stdout);
            }

            wcs = (struct WorldCoor *)NULL;

            if(hdr_rec->hdu == 1)
               first_failed = 1;

            ++nfailed;
            ++badwcs;

            if(info)
            {
               printf("[struct stat=\"INFO\", msg=\"Bad WCS\", file=\"%s\", hdu=%d]\n", fname, hdr_rec->hdu);
               
               fflush(stdout);
            }

            badhdr = 1;

            ++nbadwcs;

            if(!showbad)
               continue;
         }
      }

      /* If the header is bad, try to at least get basic image dimensions */
      if(badhdr)
      {
         status=0;
         if(fits_read_keyword(fptr, "NAXIS1", value, comment, &status))
            hdr_rec->ns = 0;
         else
            hdr_rec->ns = atoi(value);

         status=0;
         if(fits_read_keyword(fptr, "NAXIS2", value, comment, &status))
            hdr_rec->nl = 0;
         else
            hdr_rec->nl = atoi(value);


         /* Check for compressed image dimensions if stored in a binary table (ZNAXIS) */
         status=0;
         if(!fits_read_keyword(fptr, "ZNAXIS1", value, comment, &status))
            hdr_rec->ns = atoi(value);

         status=0;
         if(!fits_read_keyword(fptr, "ZNAXIS2", value, comment, &status))
            hdr_rec->nl = atoi(value);
         
        
         strcpy(hdr_rec->ctype1, "");
         strcpy(hdr_rec->ctype2, "");

         hdr_rec->crpix1  = 0.;
         hdr_rec->crpix2  = 0.;
         hdr_rec->equinox = 0.;
         hdr_rec->crval1  = 0.;
         hdr_rec->crval2  = 0.;
         hdr_rec->cdelt1  = 0.;
         hdr_rec->cdelt2  = 0.;
         hdr_rec->crota2  = 0.;

         hdr_rec->ra1     = 0.;
         hdr_rec->dec1    = 0.;
         hdr_rec->ra2     = 0.;
         hdr_rec->dec2    = 0.;
         hdr_rec->ra3     = 0.;
         hdr_rec->dec3    = 0.;
         hdr_rec->ra4     = 0.;
         hdr_rec->dec4    = 0.;

         hdr_rec->ra2000  = 0.;
         hdr_rec->dec2000 = 0.;

         hdr_rec->radius  = 0.;
      }

      else
      {
         /* Successfully initialized WCS; extract all relevant parameters */
         hdr_rec->ns = (int) wcs->nxpix;
         hdr_rec->nl = (int) wcs->nypix;

         strcpy(hdr_rec->ctype1, wcs->ctype[0]);
         strcpy(hdr_rec->ctype2, wcs->ctype[1]);

         hdr_rec->crpix1  = wcs->xrefpix;
         hdr_rec->crpix2  = wcs->yrefpix;
         hdr_rec->equinox = wcs->equinox;
         hdr_rec->crval1  = wcs->xref;
         hdr_rec->crval2  = wcs->yref;
         hdr_rec->cdelt1  = wcs->xinc;
         hdr_rec->cdelt2  = wcs->yinc;
         hdr_rec->crota2  = wcs->rot;

         /* Standardize orientation: if image is flipped/rotated, adjust cdelt/crota 
            to a conventional representation. */
         if(hdr_rec->cdelt1 > 0.
         && hdr_rec->cdelt2 > 0.
         && (hdr_rec->crota2 < -90. || hdr_rec->crota2 > 90.))
         {
            hdr_rec->cdelt1 = -hdr_rec->cdelt1;
            hdr_rec->cdelt2 = -hdr_rec->cdelt2;

            hdr_rec->crota2 += 180.;

            while(hdr_rec->crota2 >= 360.)
               hdr_rec->crota2 -= 360.;

            while(hdr_rec->crota2 <= -360.)
               hdr_rec->crota2 += 360.;
         }


         /* Identify the native coordinate system from CTYPE1 (equatorial, galactic, or ecliptic) */
         csys = EQUJ;

         if(strncmp(hdr_rec->ctype1, "RA",   2) == 0)
            csys = EQUJ;
         if(strncmp(hdr_rec->ctype1, "GLON", 4) == 0)
            csys = GAL;
         if(strncmp(hdr_rec->ctype1, "ELON", 4) == 0)
            csys = ECLJ;

         equinox = hdr_rec->equinox;

         pix2wcs (wcs, hdr_rec->ns/2., hdr_rec->nl/2., &lon, &lat);


         /* Final conversion to Equatorial J2000 decimal degrees */
         convertCoordinates (csys, equinox, lon, lat,
                             EQUJ, 2000., &ra2000, &dec2000, 0.);

         hdr_rec->ra2000  = ra2000;
         hdr_rec->dec2000 = dec2000;

         /* Calculate corner coordinates (RA/Dec J2000) */
         clockwise = 0;

         if((hdr_rec->cdelt1 < 0 && hdr_rec->cdelt2 < 0)
         || (hdr_rec->cdelt1 > 0 && hdr_rec->cdelt2 > 0)) clockwise = 1;


         if(clockwise)
         {
            pix2wcs(wcs, 0.5, 0.5, &lon, &lat);
            convertCoordinates (csys, equinox, lon, lat,
                                EQUJ, 2000., &ra, &dec, 0.);

            hdr_rec->ra1 = ra;
            hdr_rec->dec1 = dec;


            pix2wcs(wcs, wcs->nxpix+0.5, 0.5, &lon, &lat);
            convertCoordinates (csys, equinox, lon, lat,
                                EQUJ, 2000., &ra, &dec, 0.);

            hdr_rec->ra2 = ra;
            hdr_rec->dec2 = dec;


            pix2wcs(wcs, wcs->nxpix+0.5, wcs->nypix+0.5, &lon, &lat);
            convertCoordinates (csys, equinox, lon, lat,
                                EQUJ, 2000., &ra, &dec, 0.);

            hdr_rec->ra3 = ra;
            hdr_rec->dec3 = dec;


            pix2wcs(wcs, 0.5, wcs->nypix+0.5, &lon, &lat);
            convertCoordinates (csys, equinox, lon, lat,
                                EQUJ, 2000., &ra, &dec, 0.);

            hdr_rec->ra4 = ra;
            hdr_rec->dec4 = dec;
         }
         else
         {
            pix2wcs(wcs, 0.5, 0.5, &lon, &lat);
            convertCoordinates (csys, equinox, lon, lat,
                                EQUJ, 2000., &ra, &dec, 0.);

            hdr_rec->ra1 = ra;
            hdr_rec->dec1 = dec;

            pix2wcs(wcs, wcs->nxpix+0.5, 0.5, &lon, &lat);
            convertCoordinates (csys, equinox, lon, lat,
                                EQUJ, 2000., &ra, &dec, 0.);

            hdr_rec->ra4 = ra;
            hdr_rec->dec4 = dec;


            pix2wcs(wcs, wcs->nxpix+0.5, wcs->nypix+0.5, &lon, &lat);
            convertCoordinates (csys, equinox, lon, lat,
                                EQUJ, 2000., &ra, &dec, 0.);

            hdr_rec->ra3 = ra;
            hdr_rec->dec3 = dec;


            pix2wcs(wcs, 0.5, wcs->nypix+0.5, &lon, &lat);
            convertCoordinates (csys, equinox, lon, lat,
                                EQUJ, 2000., &ra, &dec, 0.);

            hdr_rec->ra2 = ra;
            hdr_rec->dec2 = dec;
         }


         /* Calculate an approximate radius for the image footprint */
         x1 = cos(hdr_rec->ra2000*dtr) * cos(hdr_rec->dec2000*dtr);
         y1 = sin(hdr_rec->ra2000*dtr) * cos(hdr_rec->dec2000*dtr);
         z1 = sin(hdr_rec->dec2000*dtr);

         x2 = cos(ra*dtr) * cos(dec*dtr);
         y2 = sin(ra*dtr) * cos(dec*dtr);
         z2 = sin(dec*dtr);

         hdr_rec->radius = acos(x1*x2 + y1*y2 + z1*z2) / dtr;

         free (header);    
      }


      /* Extract any extra keywords requested via fieldListFile */
      for(i=0; i<nfields; ++i)
      {
         status=0;
         if(fits_read_keyword(fptr, fields[i].name, value, comment, &status))
            strcpy(fields[i].value,  fields[i].defval);

         else
         {
            ptr = value;

            if(*ptr == '\'' && value[strlen(value)-1] == '\'')
            {
               value[strlen(value)-1] = '\0';
               ++ptr;
            }

            strcpy(fields[i].value, ptr);

            if(strlen(fields[i].value) == 0)
               strcpy(fields[i].value, fields[i].defval);
         }
      }

      hdr_rec->cntr = cntr;

      /* Write this HDU's metadata record to the table */
      mImgtbl_print_rec (hdr_rec);

      ++nwrite;

      if(!nowcs)
         free(wcs);
   }

   status = 0;

   /* Close FITS file */
   fits_close_file(fptr, &status);

   return(nfailed);
}



/* Given WCS information (and optionally corners) */
/* for an image, incrementally write a record to  */
/* an output image metadata (ASCII) table         */

/* mImgtbl_print_rec writes a single image metadata record to the output table. 
   If this is the first record (cntr == 0), it also writes the table header 
   including all dynamic columns for extra FITS keywords. */

void mImgtbl_print_rec (struct Hdr_rec *hdr_rec) 
{
    int  i, j;
    char fmt[32];
    char tmpname[256];

    struct COORD in, out;

    /* Setup coordinate transformation for sexagesimal output */
    strcpy(in.sys,   "EQ");
    strcpy(in.fmt,   "DDR");
    strcpy(in.epoch, "J2000");

    strcpy(out.sys,   "EQ");
    strcpy(out.fmt,   "SEXC");
    strcpy(out.epoch, "J2000");

    if(cntr == 0)
    {
       /* Write the IPAC table header */
       if(showCorners)
       {
         fprintf(tblf, "\\datatype = fitshdr\n");

         /* Column names */
         fprintf(tblf, "| cntr |      ra     |     dec     |      cra     |     cdec     |naxis1|naxis2| ctype1 | ctype2 |     crpix1    |     crpix2    |");
         fprintf(tblf, "    crval1   |    crval2   |       cdelt1      |       cdelt2      |   crota2    |equinox |");

         for(i=0; i<nfields; ++i)
         {
            sprintf(fmt, "%%%ds|", fields[i].width);

            for(j=0; j<=strlen(fields[i].name); ++j)
               tmpname[j] = tolower(fields[i].name[j]);

            fprintf(tblf, fmt, tmpname);
         }

         fprintf(tblf, "      ra1    |     dec1    |      ra2    |     dec2    |      ra3    |     dec3    |      ra4    |     dec4    |");
         fprintf(tblf, "    size    | hdu  | fname\n");

         /* Column types */
         fprintf(tblf, "| int  |     double  |     double  |      char    |     char     | int  | int  |  char  |  char  |     double    |     double    |");
         fprintf(tblf, "    double   |    double   |      double       |      double       |   double    | double |");

         for(i=0; i<nfields; ++i)
         {
            sprintf(fmt, "%%%ds|", fields[i].width);
            fprintf(tblf, fmt, fields[i].type);
         }

         fprintf(tblf, "     double  |     double  |     double  |     double  |     double  |     double  |     double  |     double  |");
         fprintf(tblf, "    int     | int  | char\n");
      }
      else
      {
         fprintf(tblf, "\\datatype = fitshdr\n");

         fprintf(tblf, "| cntr |      ra     |     dec     |      cra     |     cdec     |naxis1|naxis2| ctype1 | ctype2 |     crpix1    |     crpix2    |");
         fprintf(tblf, "    crval1   |    crval2   |      cdelt1       |       cdelt2      |   crota2    |equinox |");

         for(i=0; i<nfields; ++i)
         {
            sprintf(fmt, "%%%ds|", fields[i].width);
            
            for(j=0; j<=strlen(fields[i].name); ++j)
               tmpname[j] = tolower(fields[i].name[j]);

            fprintf(tblf, fmt, tmpname);
         }

         fprintf(tblf, "    size    | hdu  | fname\n");

         fprintf(tblf, "| int  |    double   |    double   |      char    |    char      | int  | int  |  char  |  char  |     double    |     double    |");
         fprintf(tblf, "    double   |    double   |       double      |       double      |   double    | double |");

         for(i=0; i<nfields; ++i)
         {
            sprintf(fmt, "%%%ds|", fields[i].width);
            fprintf(tblf, fmt, fields[i].type);
         }

         fprintf(tblf, "     int    | int  | char\n");
      }
    }

    /* Convert decimal RA/Dec to sexagesimal strings */
    in.lon = hdr_rec->ra2000;
    in.lat = hdr_rec->dec2000;

    ccalc(&in, &out, "t", "t");

    /* Print the metadata values */
    fprintf(tblf, " %6d",     hdr_rec->cntr);
    fprintf(tblf, " %13.7f",  hdr_rec->ra2000);
    fprintf(tblf, " %13.7f",  hdr_rec->dec2000);
    fprintf(tblf, " %13s",    out.clon);
    fprintf(tblf, " %13s",    out.clat);
    fprintf(tblf, " %6d",     hdr_rec->ns);
    fprintf(tblf, " %6d",     hdr_rec->nl);
    fprintf(tblf, " %8s",     hdr_rec->ctype1);
    fprintf(tblf, " %8s",     hdr_rec->ctype2);
    fprintf(tblf, " %15.5f",  hdr_rec->crpix1);
    fprintf(tblf, " %15.5f",  hdr_rec->crpix2);
    fprintf(tblf, " %13.7f",  hdr_rec->crval1);
    fprintf(tblf, " %13.7f",  hdr_rec->crval2);
    fprintf(tblf, " %19.10e", hdr_rec->cdelt1);
    fprintf(tblf, " %19.10e", hdr_rec->cdelt2);
    fprintf(tblf, " %13.7f",  hdr_rec->crota2);
    fprintf(tblf, " %8.2f",   hdr_rec->equinox);

    /* Print extra keyword values */
    for(i=0; i<nfields; ++i)
    {
       sprintf(fmt, " %%%ds", fields[i].width);
       fprintf(tblf, fmt, fields[i].value);
    }

    if(showCorners)
    {
       fprintf(tblf, " %13.7f", hdr_rec->ra1);
       fprintf(tblf, " %13.7f", hdr_rec->dec1);
       fprintf(tblf, " %13.7f", hdr_rec->ra2);
       fprintf(tblf, " %13.7f", hdr_rec->dec2);
       fprintf(tblf, " %13.7f", hdr_rec->ra3);
       fprintf(tblf, " %13.7f", hdr_rec->dec3);
       fprintf(tblf, " %13.7f", hdr_rec->ra4);
       fprintf(tblf, " %13.7f", hdr_rec->dec4);
    }

    fprintf(tblf, " %12lld", (long long)hdr_rec->size);
    fprintf(tblf, " %6d",    hdr_rec->hdu-1);
    fprintf(tblf, " %s\n",   hdr_rec->fname);
    fflush(tblf);

    ++cntr;
}



/* mImgtbl_update_table performs a second pass on the generated table to ensure 
   it is correctly formatted as a fixed-width ASCII (IPAC format) table. 
   It pads all records to the same maximum length. */

int mImgtbl_update_table(char *tblname)
{
   char  str[MAXLEN], tempfile[1024];
   int   i, len, maxlen;
   FILE *fdata, *ftmp;

   
   fdata = fopen(tblname, "r");

   if(fdata == (FILE *)NULL)
   {
      sprintf(montage_msgstr, "Can't open copy table.");
      return 1;
   }


   sprintf(tempfile, "%s.tmp", tblname);
   
   ftmp = fopen(tempfile, "w+");

   if(ftmp == (FILE *)NULL)
   {
      sprintf(montage_msgstr, "Can't open temporary input table.");
      return 1;
   }


   /* Find the maximum line length in the table */
   maxlen = 0;

   while(1)
   {
      if(fgets(str, MAXLEN, fdata) == (char *)NULL)
         break;

      str[MAXLEN-1] = '\0';

      len = strlen(str) - 1;

      if(len > maxlen)
         maxlen = len;

      fputs(str, ftmp);
   }

   fclose(fdata);
   fclose(ftmp);


   ftmp  = fopen(tempfile, "r");

   if(ftmp == (FILE *)NULL)
   {
      sprintf(montage_msgstr, "Can't open tmp (out) table.");
      return 1;
   }

   fdata = fopen(tblname, "w+");

   if(fdata == (FILE *)NULL)
   {
      sprintf(montage_msgstr, "Can't open final table.");
      return 1;
   }


   /* Rewrite the table with padded lines for fixed-width alignment */
   while(1)
   {
      if(fgets(str, MAXLEN, ftmp) == (char *)NULL)
         break;

      if(str[strlen(str) - 1] == '\n')
         str[strlen(str) - 1]  = '\0';

      /* Comments/header metadata starting with '\' are not padded */
      if(str[0] == '\\')
      {
         strcat(str, "\n");
         fputs(str, fdata);
         continue;
      }

      len = strlen(str);

      /* Pad the line with spaces */
      for(i=len; i<MAXLEN; ++i)
         str[i] =  ' ';
      
      str[maxlen] = '\0';

      /* Add the closing bar or newline */
      if(str[0] == '|')
         strcat(str, "|\n");
      else
         strcat(str, " \n");

      fputs(str, fdata);
   }

   fclose(fdata);
   fclose(ftmp);

   unlink(tempfile);

   return 0;
}
