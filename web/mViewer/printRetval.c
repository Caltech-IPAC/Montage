#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *fdebug;

int printRetval (char *msg)
{
    int  debug = 1;

    if ((debug) && (fdebug != (FILE *)NULL)) {
        fprintf (fdebug, "printRetval: msg= [%s]\n", msg);
	fflush (fdebug);
    }

    printf("HTTP/1.1 200 OK\n");
    printf("Content-type: text/plain\r\n\r\n");
    printf ("%s", msg);
    fflush (stdout);
    
    return (0);
}

