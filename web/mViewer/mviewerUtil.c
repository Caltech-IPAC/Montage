#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>


#include <password.h>


extern FILE *fp_debug;


void printerr(char *errmsg)
{
    printf("HTTP/1.1 200 OK\n");
    printf("Content-type: text/html\r\n");
    printf ("\r\n");

    printf ("ERROR: %s\n", errmsg);
    fflush (stdout);
    
    exit(0);
}



void printHtmlErr(char *status, char *statusMsg, char *msg)
{
    printf("HTTP/1.1 %s %s\r\n", status, statusMsg);
    printf("Content-type: text/html\r\n\r\n");
    fflush(stdout);

    printf ("<html>\n");
    printf ("<body bgcolor=\"#ffff88\">\n");

    printf ("<center><h1>ERROR</h1><p>\n");
    printf ("<b>%s</b>\n", msg);
    printf ("</center>\n");
    
    printf ("</body>\n");
    printf ("</html>\n");
    fflush (stdout);
    exit(0);
}




int printHtml (char *cookieName, char *cookieStr, char *htmlpath, char *errmsg) 
{

    FILE    *fp;
    char    cmd[1024];
    char    timeout[256];

    int     debugfile = 0;

/*
    push HTML to browser
*/
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "From printHtml: htmlpath= [%s]\n", htmlpath);
        fprintf (fp_debug, "cookieName= [%s]\n", cookieName);
        fprintf (fp_debug, "cookieStr= [%s]\n", cookieStr);
	fflush (fp_debug);
    }

    fp = fopen(htmlpath, "r");
    if (fp == (FILE *)NULL) {
        sprintf (errmsg, "Failed to open HTML file [%s].", htmlpath);
        return (-1);
    }
    
    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "push result to browser\n");
	fflush (fp_debug);
    }


/*
    expires function is in password library, it converts the number of
    days to a time string for the cookie expiration datetime.
    The first parameter is 'double days'
*/
    expires (7.0, timeout);
        
    printf("HTTP/1.0 200 OK\r\n");
    printf ("Set-Cookie: %s=%s;path=/;expires=%s\r\n", 
        cookieName, cookieStr, timeout);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "cookie set: timeout= [%s]\n", timeout);
	fflush (fp_debug);
    }
        
    printf("Content-type: text/html\r\n\r\n");
    fflush(stdout);
      
    
    while(1) {
        if(fgets(cmd, 1024, fp) == (char *)NULL)
            break;

        if(cmd[strlen(cmd) - 1] == '\n')
            cmd[strlen(cmd) - 1]  = '\0';
        
        puts(cmd);
    }
    fflush (stdout);
    fclose (fp);

    if ((debugfile) && (fp_debug != (FILE *)NULL)) {
        fprintf (fp_debug, "done returning html\n");
	fflush (fp_debug);
    }
    return (0);
}



int fileExist (char *fname)
{
    struct stat buf;
    
    int fileExist, istatus;


    fileExist = 0;
    istatus = stat (fname, &buf);
    if (istatus >= 0) {
	fileExist = 1;
    }

    return (fileExist);
}


int str2Integer (char *strval, int *intval, char *errmsg)
{
    char *endptr, str[40];
    int  i, len;
    long value;

/*
    detect the first leading non-zero digit
*/
    i = 0;
    len = (int)strlen(strval);
    while ((i < len) && (strval[i] == '0')) {
        
	if (strval[i] != '0')
	    break;

	i++;
    }
    
    if (i >= len) {
        *intval = 0;
	return (0);
    }

    strcpy (str, &strval[i]); 


    endptr = (char *)NULL;
    value = strtol (str, &endptr, 0);
    
    if (endptr < str + (int)strlen(str)) {

        sprintf (errmsg, "Failed to convert [%s] to an integer.", strval);
	return (-1);
    }

    *intval = value;
    
    return (0);
}


int str2Double (char *strval, double *dblval, char *errmsg)
{
    char *endptr;

    double  value;

    value = strtod (strval, &endptr);
                
    if (endptr < strval + strlen(strval)) {

        sprintf (errmsg, "Failed to convert [%s] to a double.", strval);
	return (-1);
    }

    *dblval = value;
    
    return (0);
}


int isDouble (char *str)
{
    char    *ptr;
    double  d;

    if (str == (char *)NULL)
	return (-1);
   
    errno = 0;
    d = strtod (str, &ptr);

    if (ptr != (str + strlen(str)))
	return (-1);

    if (errno != 0)
	return (-1);
	
    return (1);
}

void upper(char *str)
{
    char *c = str;
    while (*c != '\0') 
    {
        *c = toupper(*c);
        ++c;
    }
}

void lower(char *str)
{
    char *c = str;

    while (*c != '\0')
    {
        *c = tolower(*c);
        ++c;
    }
}


char *strtrim (char * s)
{
    char * t;

    if (s == (char *)NULL)
        return ((char *)NULL);
    while (isspace((int)((unsigned char)*s)))
        ++s;
    t = s + strlen(s);
    while (t > s)
    {
        if (isspace((int)((unsigned char)*(t - 1))) == 0 )
        {
            *t = '\0';
            break;
        }
        --t;
    }
    return (s);
}


