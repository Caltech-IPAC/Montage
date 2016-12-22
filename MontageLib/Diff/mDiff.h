#ifndef MDIFF_H
#define MDIFF_H


/************************************/
/* Define mDiff function prototypes */
/************************************/

int  mDiff_readTemplate  (char *filename);
int  mDiff_parseLine     (char *line);
int  mDiff_readFits      (char *fluxfile, char *areafile);
void mDiff_printFitsError(int);
void mDiff_printError    (char *);

#endif
