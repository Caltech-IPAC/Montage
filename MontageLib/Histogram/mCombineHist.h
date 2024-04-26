#ifndef MHISTOGRAM_H
#define MHISTOGRAM_H

#define  VALUE       0
#define  PERCENTILE  1
#define  SIGMA       2

#define  POWER       0
#define  GAUSSIAN    1
#define  GAUSSIANLOG 2
#define  ASINH       3

int    mCombineHist_readHist         (char *histfile, unsigned long long *hist, double *datamin, double *datamax);
int    mCombineHist_parseRange       (char const *str, char const *kind, double *val, double *extra, int *type);
int    mCombineHist_getRange         (int type, char *minstr, char *maxstr, char *betastr,
                                      double *rangemin, double *rangemax, double *rangebeta, 
                                      double *median, double *sigma, double *dataval);
double mCombineHist_percentileLevel  (double percentile);
double mCombineHist_valuePercentile  (double value);
double mCombineHist_snpinv           (double p);
double mCombineHist_erfinv           (double p);

#endif
