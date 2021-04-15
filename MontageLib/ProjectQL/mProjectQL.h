#ifndef MPROJECTQL_H
#define MPROJECTQL_H

#define NEAREST 0
#define LANCZOS 1


/*****************************************/
/* Define mProjectQL function prototypes */
/*****************************************/

void  mProjectQL_UpdateBounds (double oxpix, double oypix,
                               double *oxpixMin, double *oxpixMax,
                               double *oypixMin, double *oypixMax);

int   mProjectQL_readFits     (char *filename, char *weightfile);
void  mProjectQL_cleanup      ();
void  mProjectQL_fixxy        (double *x, double *y, int *offscl);

int  mProjectQL_parseLine     (char *linein);
int  mProjectQL_stradd        (char *header, char *card);
int  mProjectQL_readTemplate  (char *filename);

int  mProjectQL_BorderSetup   (char *strin);
int  mProjectQL_BorderRange   (int jrow, int maxpix, int *imin, int *imax);

void mProjectQL_printFitsError(int);
void mProjectQL_printError    (char *);

#endif
