#ifndef ISIS_JSON
#define ISIS_JSON

#include <stdio.h>

#define JSON_OK     0
#define JSON_ERROR -1

#define JSON_MAXJSON   32
#define JSON_JSONCNT  128

typedef struct
{
   int  nalloc;
   int  count;
   char **key;
   char **val;
}
   JSON;

JSON *json_struct(char const *instr);
char *json_stripblanks(char *ptr, int len, int quotes);
int json_free(JSON *json);
char *json_val(char const *structstr, char const *key, char *val);

#endif /* ISIS_JSON */
