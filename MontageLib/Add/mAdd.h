#ifndef MADD_H
#define MADD_H

/***************************/
/* Define coaddition modes */
/***************************/

#define MEAN   0
#define MEDIAN 1
#define COUNT  2
#define SUM    3


/***********************************/
/* Define mAdd function prototypes */
/***********************************/

int   mAdd_readTemplate (char *filename);
void  mAdd_parseLine    (char *line);
int   mAdd_avg_count    (double data[], double area[], double *outdata, 
                         double *outarea, int count);
int   mAdd_avg_mean     (double data[], double area[], double *outdata, 
                         double *outarea, int count);
int   mAdd_avg_median   (double data[], double area[], double *outdata, 
                         double *outarea, int n, double nom_area);
int   mAdd_avg_sum      (double data[], double area[], double *outdata, 
                         double *outarea, int count);
void  mAdd_sort         (double *data, double *area, int n);
 
int  mAdd_listInit      ();    
int  mAdd_listAdd       (int value);
int  mAdd_listDelete    (int value);
int  mAdd_listCount     ();    
int  mAdd_listIndex     (int index);

int  mAdd_stradd        (char *header, char *card);
void mAdd_printFitsError(int); 
void mAdd_printError    (char *);
int  mAdd_allocError    (char *);

#endif
