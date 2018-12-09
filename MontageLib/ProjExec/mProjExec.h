#ifndef MPROJEXEC_H
#define MPROJEXEC_H

/******************************/
/* Define tangent plane modes */
/******************************/

#define INTRINSIC 0
#define COMPUTED  1
#define FAILED    2

int   mProjExec_readTemplate(char *filename);
int   mProjExec_stradd      (char *header, char *card);

#endif
