/* Module: debugCheck.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.1      John Good        25Aug03  Implemented status file processing
1.0      John Good        13Mar03  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**********************************************************/
/*                                                        */
/*  debugCheck                                            */
/*                                                        */
/*  This routine checks a debug level string to see if it */
/*  represents a valid positive integer.                  */
/*                                                        */
/**********************************************************/

int montage_debugCheck(char *debugStr)
{
   int   debug;
   char *end;

   debug = strtol(debugStr, &end, 0);

   if(end - debugStr < (int)strlen(debugStr))
      return -1;

   if(debug < 0)
      return -1;

   return debug;
}
