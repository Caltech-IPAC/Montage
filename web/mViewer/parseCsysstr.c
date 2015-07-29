/*
Theme: Parse Csys string (e.g. eq j2000) to 
    csys (an integer) and epoch (double).
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>


char  *strtrim (char *str);

extern FILE *fp_debug;


int parseCsysstr (char *csysstrIn, int *sys, double *epoch, char *errmsg)
{
    char    *cptr;
    char    *endptr;

    char    str[1024], substr[1024];
    char    csysstr[40], epochstr[40];
    
    int     isys;
   
    double  value;

    int     debugfile = 0;

/*
    Determine coordinate system
*/
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "parseCsysstr= [%s]\n", csysstrIn);
        fflush (fp_debug);
    }

    strcpy (str, csysstrIn);
    
    cptr= (char *)NULL;
    cptr = strchr (str, ' ');
    if (cptr != (char *)NULL) {
        
	strcpy (substr, cptr+1);
	strcpy (epochstr, strtrim(substr));

	*cptr = '\0';
	strcpy (csysstr, strtrim(str));
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "csysstr= [%s] epochstr= [%s]\n", 
	    csysstr, epochstr);
        fflush (fp_debug);
    }

    
    if (strncasecmp (csysstr, "eq", 2) == 0) {
        
	if ((epochstr[0] == 'j') || (epochstr[0] == 'J')) 
	    isys = 0;
	else if ((epochstr[0] == 'b') || (epochstr[0] == 'B')) 
	    isys = 1;
    }
    else if (strncasecmp (csysstr, "ec", 2) == 0) {
        
	if ((epochstr[0] == 'j') || (epochstr[0] == 'J')) 
	    isys = 2;
	else if ((epochstr[0] == 'b') || (epochstr[0] == 'B')) 
	    isys = 3;
    }
    else if (strncasecmp (csysstr, "ga", 2) == 0) {
	    isys = 4;
    }
    else if (strcasecmp (csysstr, "sgal") == 0) {
	    isys = 5;
    }
    else {
        sprintf (errmsg, 
  "From parseCsysstr: failed to convert csys string [%s] to coordinate system.",
	    csysstr);
	return (-1);
    }


    strcpy (str, &epochstr[1]);
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "isys= [%d]\n", isys);
	fprintf (fp_debug, "str= [%s]\n", str);
        fflush (fp_debug);
    }


    value = strtod (str, &endptr); 

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
	fprintf (fp_debug, "value= [%lf]\n", value);
        fflush (fp_debug);
    }
 
    if (endptr < str + strlen(str)) {

        sprintf (errmsg, "Failed to convert epoch string [%s] to double.", str);
	return (-1);
    }
       
    *sys = isys;
    *epoch = value;
    
    return (0);
}


