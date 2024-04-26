#ifndef MADDMEM_H
#define MADDMEM_H


/**************************************/
/* Define mAddMem function prototypes */
/**************************************/

int  mAddMem_readTemplate  (char *filename);
int  mAddMem_parseLine     (char *line);
int  mAddMem_readFits      (char *fluxfile, char *areafile);
void mAddMem_printFitsError(int);
void mAddMem_printError    (char *);

#endif
