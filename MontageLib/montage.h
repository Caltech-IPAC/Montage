#ifndef MONTAGE_H
#define MONTAGE_H

#include <wcs.h>


/*************************************/
/* Define Montage library prototypes */
/*************************************/

struct mAddCubeReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mAddCubeReturn *mAddCube(char *path, char *tblfile, char *template_file, char *output_file,
                                int shrink, int haveAreas, int coadd, int debug);

//-------------------

struct mAddReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mAddReturn *mAdd(char *path, char *tblfile, char *template_file, char *output_file,
                        int shrink, int haveAreas, int coadd, int debug);

//-------------------

struct mArchiveExecReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of images retrieved
   int    failed;        // Number of retrievals that failed
};

struct mArchiveExecReturn *mArchiveExec(char *tblfile, int nrestart, int timeout, int debug);

//-------------------

struct mArchiveGetReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Size of the file in bytes
};

struct mArchiveGetReturn *mArchiveGet(char *url, char *datafile, int timeout, int debug);

//-------------------

struct mArchiveListReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of lines in output file.
};

struct mArchiveListReturn *mArchiveList(char *survey, char *band, char *locstr, double width, double height, 
                                        char *outfile, int debug);

//-------------------

struct mBackgroundReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mBackgroundReturn *mBackground(char *input_file, char *output_file, double A, double B, double C,
                                      int noAreasin, int debug);

//-------------------

struct mBestImageReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   char   file[1024];    // 'Best' file name
   int    hdu;           // Best HDU (if any)
   char   url [1024];    // URL to best file (if any)
   double edgedist;      // Closest edge distance 
};

struct mBestImageReturn *mBestImage(char *tblfile, double ra, double dec, int debug);

//-------------------

struct mBgExecReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of images
   int    nocorrection;  // Number of images for which there was no correction 
   int    failed;        // Number of images where correction failed
};

struct mBgExecReturn *mBgExec(char *path, char *tblfile, char *fitfile, char *corrdir, int noAreas, int debug);

//-------------------

struct mBgModelReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
};

struct mBgModelReturn *mBgModel(char *imgfile, char *fitfile, char *corrtbl, int noslope, int useall, 
                                int niterations, int debug);

//-------------------

struct mCoverageCheckReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of images matching region
};

struct mCoverageCheckReturn *mCoverageCheck(char *path, char *infile, char *outfile, int mode, char *hdrfile, 
                                            int narray, double *array, int debug);

//-------------------

struct mDiffExecReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of differences                
   int    failed;        // Number of differences that failed
   int    warning;       // Number of fits to differences that produced warnings
};

struct mDiffExecReturn *mDiffExec(char *path, char *tblfile, char *template, char *diffdir, int noAreas, int debug);

//-------------------

struct mDiffFitExecReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of differences                
   int    diff_failed;   // Number of differences that failed
   int    fit_failed;    // Number of fits to differences that failed
   int    warning;       // Number of fits to differences that produced warnings
};

struct mDiffFitExecReturn *mDiffFitExec(char *path, char *tblfile, char *template, char *diffdir,
                                        char *fitfile, int keepAll, int levelOnly, int noAreas, int debug);

//-------------------

struct mDiffReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mDiffReturn *mDiff(char *input_file1, char *input_file2, char *output_file, 
                          char *template_file, int noAreas, double factor, int debug);

//-------------------

struct mExamineReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   char   proj  [32];    // Image projection.   
   char   csys  [16];    // Coordinate system.   
   double equinox;       // Coordinate system equinox.
   int    naxis;         // Number of axes.
   int    naxis1;        // First axis size.
   int    naxis2;        // Second axis size.
   int    naxis3;        // Third axis size (if it exists).
   int    naxis4;        // Fourth axis size (if it exists).
   double crval1;        // Axis 1 sky reference value.
   double crval2;        // Axis 2 sky reference value.
   double crpix1;        // Axis 1 reference pixel.
   double crpix2;        // Axis 2 reference pixel.
   double cdelt1;        // Axis 1 pixel scale.
   double cdelt2;        // Axis 2 pixel scale.
   double crota2;        // Image rotation on sky.
   double lonc;          // Longitude of the image center.
   double latc;          // Latitude of the image center.
   double ximgsize;      // Axis 1 size on the sky.
   double yimgsize;      // Axis 2 size on the sky.
   double rotequ;        // Rotation of image relative to Equatorial North
   double rac;           // RA of the image center.
   double decc;          // Dec of the image center.
   double ra1;           // RA of image corner 1
   double dec1;          // Dec of image corner 1
   double ra2;           // RA of image corner 2
   double dec2;          // Dec of image corner 2
   double ra3;           // RA of image corner 3
   double dec3;          // Dec of image corner 3
   double ra4;           // RA of image corner 4
   double dec4;          // Dec of image corner 4
   double radius;        // Radius of the examine radius (in degrees).
   double radpix;        // Radius of the examine radius (in pixels).
   int    npixel;        // Number of pixel in the examine area.
   double nnull;         // Number of null pixels in the examine area.
   double aveflux;       // Average flux.
   double rmsflux;       // RMS flux.
   double fluxref;       // Flux for center (reference) pixel.
   double sigmaref;      // Reference flux in units of RMS.
   double xref;          // Pixel X of reference.
   double yref;          // Pixel Y of reference.
   double raref;         // RA of reference.
   double decref;        // Dec of reference.
   double fluxmin;       // Flux of pixel with minimum value in examine area.
   double sigmamin;      // Min flux in units of RMS.
   double xmin;          // Pixel X of min.
   double ymin;          // Pixel Y of min.
   double ramin;         // RA of min pixel.
   double decmin;        // Dec of min pixel.
   double fluxmax;       // Flux of pixel with maximum value in examine area.
   double sigmamax;      // Max flux in units of RMS
   double xmax;          // Pixel X of max.
   double ymax;          // Pixel Y of max.
   double ramax;         // RA of max pixel.
   double decmax;        // Dec of m.n pixel.
   double totalflux;     // Aperture phtometry total flux.
};

struct mExamineReturn *mExamine(int areaMode, char *infile, int hdu, int plane3, int plane4,
                                double ra, double dec, double radius, int locinpix, int radinpix, int debug);

//-------------------

struct mFitExecReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of differences                
   int    failed;        // Number of fits to differences that failed
   int    warning;       // Number of fits to differences that produced warnings
   int    missing;       // Number of missing difference images
};

struct mFitExecReturn *mFitExec(char *tblfile, char *fitfile, char *diffdir, int levelOnly, int debug);

//-------------------

struct mFitplaneReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double a;             // Plane fit coefficient for X axis.
   double b;             // Plane fit coefficient for Y axis.
   double c;             // Plane fit constant offset.
   double crpix1;        // X-axis pixel coordinate for center of region.
   double crpix2;        // Y-axis pixel coordinate for center of region.
   double xmin;          // Minimum X-axis value.
   double xmax;          // Maximum X-axis value.
   double ymin;          // Minimum Y-axis value.
   double ymax;          // Maximum Y-axis value.
   double xcenter;       // Center X location.
   double ycenter;       // Center Y location.
   int    npixel;        // Total number of pixels fit.
   double rms;           // RMS of fit (pixels with large offset values were excluded in fit).
   double boxx;          // Rectanguar bounding box X center.
   double boxy;          // Rectanguar bounding box Y center.
   double boxwidth;      // Rectanguar bounding box width.
   double boxheight;     // Rectanguar bounding box height.
   double boxang;        // Rectanguar bounding box rotation angle.
};

struct mFitplaneReturn *mFitplane(char *input_file, int levelOnly, int border, int debug);

//-------------------

struct mFixNaNReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    rangeCount;    // Number of pixels found in the range(s) specified
   int    nanCount;      // Number of NaN pixels found
   int    boundaryCount; // Number of pixels found in "boundary" regions
};

struct mFixNaNReturn  *mFixNaN(char *input_file, char *output_file, int boundaries, int haveVal, double NaNvalue,
                               int nMinMax, double *minblank, int *ismin, double *maxblank, int *ismax, int debug);

//-------------------

struct mGetHdrReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    ncard;         // Number of lines in the header.
};

struct mGetHdrReturn *mGetHdr(char *infile, char *hdrfile, int hdu, int htmlMode, int debug);

//-------------------

struct mHdrReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of lines in output file.
};

struct mHdrReturn *mHdr(char *locstr, double width, double height, char *outfile, char *csys, double equinox,
                        double resolution, double rotation, char *band2MASS, int debug);

//-------------------

struct mHistogramReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double minval;        // Data value associated with histogram minimum.
   double minpercent;    // Percentile value of histogram minimum.
   double minsigma;      // "Sigma" level of histogram minimum.
   double maxval;        // Data value associated with histogram maximum.
   double maxpercent;    // Percentile value of histogram maximum.
   double maxsigma;      // "Sigma" level of histogram maximum.
   double datamin;       // Minimum data value in file.
   double datamax;       // Maximum data value in file.
};

struct mHistogramReturn *mHistogram(char *imgfile, char *histfile,
                                    char *yminstr, char *maxstr, char *stretchtype, int logpower, char *betastr, int debug);

//-------------------

struct mImgtblReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of images found with valid headers (may be more than one per file).
   int    nfile;         // Number of FITS files found.
   int    nhdu;          // Total number of HDSs in all files.
   int    badfits;       // Number of bad FITS files.
   int    badwcs;        // Number of images rejected because of bad WCS information.
};

struct mImgtblReturn *mImgtbl(char *pathnamein, char *tblname,
                              int recursiveMode, int processAreaFiles, int haveCubes, int noGZIP,
                              int showCorners, int showInfo, int showbad, char *imgListFile, char *fieldListFile,
                              int debug);

//-------------------

struct mMakeHdrReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   char   note[1024];    // Cautionary message (only there if needed).   
   int    count;         // Number of images in metadata table.
   int    ncube;         // Number of images that have 3/4 dimensions.
   int    naxis1;        // X axis pixel count in output template.
   int    naxis2;        // Y axis pixel count in output template.
   double clon;          // Center longitude for template.
   double clat;          // Center latitude for template.
   double lonsize;       // Template dimensions in X.
   double latsize;       // Template dimensions in Y.
   double posang;        // Rotation angle of template.
   double lon1;          // Image corners (lon of first corner).
   double lat1;          // Image corners (lat of first corner).
   double lon2;          // Image corners (lon of second corner).
   double lat2;          // Image corners (lat of second corner).
   double lon3;          // Image corners (lon of third corner).
   double lat3;          // Image corners (lat of third corner).
   double lon4;          // Image corners (lon of fourth corner).
   double lat4;          // Image corners (lat of fourth corner).
};

struct mMakeHdrReturn *mMakeHdr(char *tblfile, char *template, char *csys, double equinox, double pixelScale, 
                                int northAligned, double pad, int isPercentage, int maxPixel, int debug);

//-------------------

struct mMakeImgReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
};

struct mMakeImgReturn *mMakeImg(char *template_file, char *output_file, double noise, double bg1, double bg2, double bg3, double bg4,
                                int ncat, char **cat_file, char **colname, double *width, double *refmag, double *tblEpoch, int region,
                                int nimage, char **image_file, char *arrayfile, int replace, int indebug);

//-------------------

struct mOverlapsReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of overlaps.
};

struct mOverlapsReturn *mOverlaps(char *tblfile, char *difftbl, int quickmode, int debug);

//-------------------

struct mProjectReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mProjectReturn *mProject(char *input_file, char *output_file, char *template_file, int hdu,
                                char *weight_file, double fixedWeight, double threshold, char *borderstr,
                                double drizzle, double fluxScale, int energyMode, int expand, 
                                int fullRegion, int debug);

//-------------------

struct mProjectCubeReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mProjectCubeReturn *mProjectCube(char *input_file, char *output_file, char *template_file, 
                                        int hdu, char *weight_file, double fixedWeight, double threshold, 
                                        double drizzle, double fluxScale, int energyMode, int expand, 
                                        int fullRegion, int debug);

//-------------------

struct mProjectPPReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mProjectPPReturn *mProjectPP(char *input_file, char *output_file, char *template_file, int hdu,
                                    char *weight_file, double fixedWeight, double threshold, char *borderstr,
                                    char *altin, char *altout, double drizzle, double fluxScale, int energyMode,
                                    int expand, int fullRegion, int debug);

//-------------------

struct mProjectQLReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mProjectQLReturn *mProjectQL(char *input_file, char *output_file, char *template_file, int hdu, int interp,
                                    char *weight_file, double fixedWeight, double threshold, char *borderstr,
                                    double fluxScale, int expand, int fullRegion, int noAreas, int debug);

//-------------------

struct mProjExecReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of images reprojected
   int    failed;        // Number of reprojections that failed
   int    nooverlap;     // number of images that do not overlap the area of interest
};

struct mProjExecReturn *mProjExec(char *path, char *tblfile, char *template, char *projdir, int quickMode, 
                                  int exact, int wholeImages, int energyMode, char *border, char *scaleCol, 
                                  char *weightCol, int restart, char *stats, int debug);

//-------------------

struct mPutHdrReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
};

struct mPutHdrReturn *mPutHdr(char *input_file, char *output_file, char *template_file, int hdu, int debug);

//-------------------

struct mShrinkReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mShrinkReturn *mShrink(char *input_file, char *output_file, double shrinkFactor, 
                              int hdu, int fixedSize, int debug);

//-------------------

struct mShrinkCubeReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double time;          // Run time (sec)   
};

struct mShrinkCubeReturn *mShrinkCube(char *input_file, char *output_file, double shrinkFactor, int hdu, int mfactor, 
                                      int fixedSize, int debug);

//-------------------

struct mSubCubeReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg    [1024]; // Return message (for error return)
   char   json   [4096]; // Return parameters as JSON string
   char   content[1024]; // String giving an idea of output content (e.g., 'blank', 'flat', or 'normal'.   
   char   warning[1024]; // If warranted, warning message about CDELT, CRPIX, etc.   
};

struct mSubCubeReturn *mSubCube(int mode, char *infile, char *outfile, double xref, double yref, 
                                double xsize, double ysize, int hdu, int nowcs, char *d3constraint, char *d4constraint, 
                                int debug);

//-------------------

struct mSubimageReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg    [1024]; // Return message (for error return)
   char   json   [4096]; // Return parameters as JSON string
   char   content[1024]; // String giving an idea of output content (e.g., 'blank', 'flat', or 'normal'.   
};

struct mSubimageReturn *mSubimage(int mode, char *infile, char *outfile, double xref, double yref, 
                                  double xsize, double ysize, int hdu, int nowcs, int debug);

//-------------------

struct mSubsetReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   int    count;         // Number of images in the input table.
   int    nmatches;      // Number of matches.
};

struct mSubsetReturn *mSubset(char *tblfile, char *template, char *subtbl, int fastmode, int debugin);

//-------------------

struct mTANHdrReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double fwdxerr;       // Maximum error in X for forward transformation
   double fwdyerr;       // Maximum error in Y for forward transformation
   double fwditer;       // Number of iterations needed in forward transformation
   double revxerr;       // Maximum error in X for reverse transformation
   double revyerr;       // Maximum error in Y for reverse transformation
   double reviter;       // Number of iterations needed in reverse transformation
};

struct mTANHdrReturn *mTANHdr(char *origtmpl, char *newtmpl, int order, int maxiter, double tolerance, 
                              int useOffscl, int debugin);

//-------------------

struct mTransposeReturn
{
   int    status;        // Return status (0: OK, 1:ERROR)
   char   msg [1024];    // Return message (for error return)
   char   json[4096];    // Return parameters as JSON string
   double mindata;       // Minimum data value.
   double maxdata;       // Maximum data value.
};

struct mTransposeReturn *mTranspose(char *inputFile, char *outputFile, int norder, int *order, int debug);

//-------------------

struct mViewerReturn
{
   int     status;            // Return status (0: OK, 1:ERROR)
   char    msg [1024];        // Return message (for error return)
   char    json[4096];        // Return parameters as JSON string
   char    type[32];          // Whether the output is 'color' or 'grayscale'   
   int     nx;                // Width of the image in pixels
   int     ny;                // Height of the image in pixels
   double  grayminval;        // Minimum stretch data value (grayscale)
   double  grayminpercent;    // Percentile level in histogram for min stretch value (gray)
   double  grayminsigma;      // 'Sigma' level in histogram for min stretch value (gray)
   double  graymaxval;        // Maximum stretch data value (grayscale)
   double  graymaxpercent;    // Percentile level in histogram for min stretch value (gray)
   double  graymaxsigma;      // 'Sigma' level in histogram for min stretch value (gray)
   double  blueminval;        // Minimum stretch data value (color: blue image)
   double  blueminpercent;    // Percentile level in histogram for min stretch value (blue)
   double  blueminsigma;      // 'Sigma' level in histogram for min stretch value (blue)
   double  bluemaxval;        // Maximum stretch data value (color: blue image)
   double  bluemaxpercent;    // Percentile level in histogram for max stretch value (blue)
   double  bluemaxsigma;      // 'Sigma' level in histogram for min stretch value (blue)
   double  greenminval;       // Minimum stretch data value (color: green image)
   double  greenminpercent;   // Percentile level in histogram for min stretch value (green)
   double  greenminsigma;     // 'Sigma' level in histogram for min stretch value (green)
   double  greenmaxval;       // Maximum stretch data value (color: green image)
   double  greenmaxpercent;   // Percentile level in histogram for max stretch value (green)
   double  greenmaxsigma;     // 'Sigma' level in histogram for min stretch value (green)
   double  redminval;         // Minimum stretch data value (color: red image)
   double  redminpercent;     // Percentile level in histogram for min stretch value (red)
   double  redminsigma;       // 'Sigma' level in histogram for min stretch value (red)
   double  redmaxval;         // Maximum stretch data value (color: red image)
   double  redmaxpercent;     // Percentile level in histogram for max stretch value (red)
   double  redmaxsigma;       // 'Sigma' level in histogram for min stretch value (red)
   double  graydatamin;       // Minimum data value for grayscale image
   double  graydatamax;       // Maximum data value for grayscale image
   double  bdatamin;          // Minimum data value for blue image
   double  bdatamax;          // Maximum data value for blue image
   double  gdatamin;          // Minimum data value for green image
   double  gdatamax;          // Maximum data value for green image
   double  rdatamin;          // Minimum data value for red image
   double  rdatamax;          // Maximum data value for red image
   int     flipX;             // Boolean: whether the image X axis was flipped
   int     flipY;             // Boolean: whether the image Y axis was flipped
   int     colortable;        // Grayscale: pseudo-color lookup table index
   char    bunit[256];        // Flux units in data files (from BUNIT header keyword)   
};

struct mViewerReturn *mViewer(char *cmdstr, char *outFile, int mode, char *outFmt, char *fontFile, int debug);

//-------------------

int   montage_checkFile   (char *filename);
char *montage_checkHdr    (char *infile, int hdrflag, int hdu);
char *montage_getHdr      (void);
char *montage_checkWCS    (struct WorldCoor *wcs);
int   montage_debugCheck  (char *debugStr);
char *montage_filePath    (char *path, char *fname);
char *montage_fileName    (char *fname);

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#define mNaN(x) (isnan(x) || !isfinite(x))

#endif
