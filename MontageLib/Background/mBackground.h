#ifndef MBACKGROUND_H
#define MBACKGROUND_H

/******************************************/
/* Define mBackground function prototypes */
/******************************************/

void mBackground_printFitsError(int);
void mBackground_printError    (char *);
int  mBackground_readFits      (char *fluxfile, char *areafile);

#endif
