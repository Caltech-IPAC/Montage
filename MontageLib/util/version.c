/* Module: version.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        22Feb16  Baseline code

*/

#include <stdio.h>
#include <string.h>

/* Emit a version number */

char *montage_version(char *filename)
{
   static char version[1024];

   strcpy(version, "5.0.0");

   return version;
}
