#ifndef MVIEWER_H
#define MVIEWER_H

#define JSONFILE 0
#define JSONMODE 1
#define CMDMODE  2

int    mViewer_parseSymbol        (char *symbolstr, int *symNPnt, int *symNMax, int *symType, double *symRotAngle);
int    mViewer_colorLookup        (char *colorin, double *ovlyred, double *ovlygreen, double *ovlyblue);
int    mViewer_hexVal             (char c);

int    mViewer_getPlanes          (char *, int *);
double mViewer_percentileLevel    (double percentile);
double mViewer_valuePercentile    (double value);
int    mViewer_fits_comment       (char *header, char *comment);
double mViewer_snpinv             (double p);
double mViewer_erfinv             (double p);
void   mViewer_printFitsError     (int);

void   mViewer_createColorTable   (int itable);
int    mViewer_vamp_comment       (char *comment);

int    mViewer_parseRange         (char const *str, char const *kind,
                                   double *val, double *extra, int *type);

int    mViewer_getRange           (fitsfile *fptr, char *minstr, char *maxstr,
                                   double *rangemin, double *rangemax, 
                                   int type, char *betastr, double *rangebeta, 
                                   double *dataval, int imnaxis1, int imnaxis2,
                                   double *datamin, double *datamax,
                                   double *median, double *sigma,
                                   int count, int *planes);

int    checkHdr                   (char *infile, int hdrflag, int hdu);
int    checkWCS                   (struct WorldCoor *wcs, int action);

void   mViewer_fixxy              (double *x, double *y, int *offscl);
int    mViewer_stradd             (char *header, char *card);

struct WorldCoor *mViewer_wcsfake (int naxis1, int naxis2);

int    mViewer_setPixel           (int i, int j, double brightness, double red, double green, double blue, int force);
int    mViewer_getPixel           (int i, int j, int color);

double mViewer_label_length       (char *face_path, int fontsize, char *text);

void   mViewer_addOverlay         ();

void   mViewer_labeledCurve      (char *face_path, int fontsize, int showLine,
                                   double *xcurve, double *ycurve, int npt,
                                   char *text, double offset,
                                   double red, double green, double blue);

void   mViewer_curve              (double *xcurve, double *ycurve, int npt,
                                   double red, double green, double blue);

void   mViewer_makeGrid           (struct WorldCoor *wcs,
                                   int csysimg,  double epochimg,
                                   int csysgrid, double epochgrid,
                                   double red, double green, double blue,
                                   char *fontfile, double fontscale);

void   mViewer_coord_label        (char *face_path, int fontsize, 
                                   double lonlab, double latlab, char *label,
                                   int csysimg, double epochimg, int csysgrid, double epochgrid,
                                   double red, double green, double blue);

void   mViewer_latitude_line      (double lat, double lonmin, double lonmax, 
                                   int csysimg, double epochimg, int csysgrid, double epochgrid,
                                   double red, double green, double blue);

void   mViewer_longitude_line     (double lon, double latmin, double latmax, 
                                   int csysimg, double epochimg, int csysgrid, double epochgrid,
                                   double red, double green, double blue);

void   mViewer_draw_boundary      (double red, double green, double blue);

void   mViewer_great_circle       (struct WorldCoor *wcs, int flipY,
                                   int csysimg,  double epochimg,
                                   int csysgrid, double epochgrid,
                                   double lon0,  double lat0, double lon1, double lat1,
                                   double red,   double green, double blue);

void   mViewer_symbol             (struct WorldCoor *wcs, int flipY,
                                   int csysimg,  double epochimg,
                                   int csyssym, double epochsym,
                                   double clon,  double clat, int inpix, 
                                   double symSize, int symnpnt, int symNMax, int symType, double symRotAngle,
                                   double red,   double green, double blue);

void   mViewer_draw_label         (char *fontfile, int fontsize, 
                                   int xlab, int ylab, char *text, 
                                   double red, double green, double blue);

int    mViewer_writePNG           (const char* filename, const unsigned char* image, 
                                   unsigned w, unsigned h, char *comment);

int    mViewer_readHist           (char *histfile,  double *minval,  double *maxval, double *dataval, 
                                   double *datamin, double *datamax, double *median, double *sigma, int *type);

void   mViewer_parseCoordStr      (char *coordStr, int *csys, double *epoch);

#endif
