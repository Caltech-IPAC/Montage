#ifndef MADDCUBE_H
#define MADDCUBE_H

/***************************/
/* Define coaddition modes */
/***************************/

#define MEAN   0
#define MEDIAN 1
#define COUNT  2


/******************************/
/* Define mAddCube prototypes */
/******************************/

int  mAddCube_listInit       ();
int  mAddCube_listAdd        (int value);
int  mAddCube_listDelete     (int value);
int  mAddCube_listCount      ();
int  mAddCube_listIndex      (int index);
int  mAddCube_stradd         (char *header, char *card);
void mAddCube_parseLine      (char *line);
void mAddCube_sort           (double *data, double *area, int n);
int  mAddCube_readTemplate   (char *filename);
int  mAddCube_avg_count      (double data[], double area[], double *outdata, 
                              double *outarea, int count);
int  mAddCube_avg_mean       (double data[], double area[], double *outdata, 
                              double *outarea, int count);
int  mAddCube_avg_median     (double data[], double area[], double *outdata, 
                              double *outarea, int n, double nom_area);
void mAddCube_printFitsError (int);
void mAddCube_printError     (char *);
int  mAddCube_allocError     (char *);

#endif
