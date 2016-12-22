#ifndef MFIXNAN_H
#define MFIXNAN_H

void mFixNaN_printFitsError(int err);
void mFixNaN_printError    (char *msg);
int  mFixNaN_readFits      (char *fluxfile, int boundaryFlag);
int  mFixNaN_checkHdr      (char *infile, int hdrflag, int hdu);

#endif
