#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int printRetval (char *msg)
{
    printf("HTTP/1.1 200 OK\n");
    printf("Content-type: text/plain\r\n\r\n");
    printf ("%s", msg);
    fflush (stdout);
    
    return (0);
}

