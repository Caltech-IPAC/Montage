#ifndef MHPXGAPDIFF_H
#define MHPXGAPDIFF_H

#include <wcs.h>

#define mNaN(x) (isnan(x) || !isfinite(x))


/******************************************/
/* Define mHPXGapDiff function prototypes */
/******************************************/

struct mHPXGapDiffReturn
{
   int    status;              // Return status (0: OK, 1:ERROR)
   char   msg [1024];          // Return message (for error return)
   char   json[4096];          // Return parameters as JSON string
   char   file[2][4096];       // File fit.
   double a[2];                // Plane fit coefficient for X axis.
   double b[2];                // Plane fit coefficient for Y axis.
   double c[2];                // Plane fit constant offset.
   int    naxis1[2];           // Original image size (X).
   int    naxis2[2];           // Original image size (Y).
   double crpix1[2];           // X-axis pixel coordinate for center of region.
   double crpix2[2];           // Y-axis pixel coordinate for center of region.
   int    xmin[2];             // Minimum X-axis value.
   int    xmax[2];             // Maximum X-axis value.
   int    ymin[2];             // Minimum Y-axis value.
   int    ymax[2];             // Maximum Y-axis value.
   double xcenter[2];          // Center X location.
   double ycenter[2];          // Center Y location.
   int    npixel[2];           // Total number of pixels fit.
   double rms[2];              // RMS of fit (pixels with large offset values were excluded in fit).
   double boxx[2];             // Rectanguar bounding box X center.
   double boxy[2];             // Rectanguar bounding box Y center.
   double boxwidth[2];         // Rectanguar bounding box width.
   double boxheight[2];        // Rectanguar bounding box height.
   double boxang[2];           // Rectanguar bounding box rotation angle.
   double transform[2][3][3];  // Tranform matrix from ABC to plane for 'matched' image.
};

struct mHPXGapDiffReturn *mHPXGapDiff(char *plus_file, int plus_edge, char *minus_file, int minus_edge,
                                      char *gap_dir, int levelOnly, int width, int debug);

void   mHPXGapDiff_fitImage(char *input_file, int levelOnly, int edge, int width, 
                            struct mHPXGapDiffReturn *returnStruct, int index, int debug);

int    mHPXGapDiff_gaussj        (double **, int, double **, int);
void   mHPXGapDiff_nrerror       (char *);
int   *mHPXGapDiff_ivector       (int);
void   mHPXGapDiff_free_ivector  (int *);
void   mHPXGapDiff_printFitsError(int);

int   montage_checkFile   (char *filename);
char *montage_checkHdr    (char *infile, int hdrflag, int hdu);
char *montage_parseHdr    (char *infile, int hdrflag, int hdu);
char *montage_getHdr      (void);
char *montage_checkWCS    (struct WorldCoor *wcs);
int   montage_debugCheck  (char *debugStr);
char *montage_filePath    (char *path, char *fname);
char *montage_fileName    (char *fname);

#define TOP    0
#define LEFT   1
#define RIGHT  2
#define BOTTOM 3

#endif
