#ifndef MHISTOGRAM_H
#define MHISTOGRAM_H

#define  VALUE       0
#define  PERCENTILE  1
#define  SIGMA       2

#define  POWER       0
#define  GAUSSIAN    1
#define  GAUSSIANLOG 2
#define  ASINH       3

int    mHistogram_getPlanes         (char *, int *);
double mHistogram_percentileLevel   (double percentile);
double mHistogram_valuePercentile   (double value);
double mHistogram_snpinv            (double p);
double mHistogram_erfinv            (double p);
void   mHistogram_printFitsError    (int);

void    mHistogram_parseRange       (char const *str, char const *kind,
                                     double *val, double *extra, int *type);

int     mHistogram_getRange         (fitsfile *fptr, char *minstr, char *maxstr,
                                     double *rangemin, double *rangemax, 
                                     int type, char *betastr, double *rangebeta, 
                                     double *dataval, int imnaxis1, int imnaxis2,
                                     double *datamin, double *datamax,
                                     double *median, double *sigma,
                                     int count, int *planes);

#endif
