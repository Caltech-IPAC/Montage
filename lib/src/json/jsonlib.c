#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#include <json.h>

#define TRUE    1
#define FALSE  !TRUE
#define FOREVER TRUE

int json_debug = 0;


JSON *json_struct(char const *instr)
{
   int i, inlen, len, inquote, blev;
   char *str, *p, *begin, *end;
   char *sb, *se, *key, *peq, *val;
   char match;

   JSON *json;

   inlen = strlen(instr);

   if(instr == (char *)NULL)
      return((JSON *)NULL);

   if(instr[0] == '\0')
      return((JSON *)NULL);

   if(json_debug)
   {
      fprintf(stderr, "\nDEBUG: Input string: \"%s\"\n", instr);
      fflush(stderr);
   }


   /* Allocate initial space for object */

   json = (JSON *) malloc(sizeof(JSON));

   json->count  = 0;
   json->nalloc = JSON_JSONCNT;

   json->key = (char **) malloc(json->nalloc * sizeof(char *));
   json->val = (char **) malloc(json->nalloc * sizeof(char *));

   for(i=0; i<json->nalloc; ++i)
   {
      json->key[i] = (char *) malloc(inlen * sizeof(char));
      json->val[i] = (char *) malloc(inlen * sizeof(char));
   }

   if(json_debug)
   {
      fprintf(stderr, "\nDEBUG: Allocated JSON object and %d keyword/value pairs\n", 
         json->nalloc);
      fflush(stderr);
   }



   /* Strip off header and trailer */

   len = (int)strlen(instr);
   str = (char *) malloc((len + 1) * sizeof(char));
   strcpy(str, instr);

   p = str;
   p = json_stripblanks(p, len, 0);


   /* Check for (and skip over) object/array definition */

   match = ' ';
   if(*p == '{')
      match = '}';
   else if(*p == '[') 
      match = ']';
   
   if(*p != '{')
      ++p;
   else if(*p != '[')
      ++p;
   else
   {
      if(json_debug)
      {
         fprintf(stderr, "\nDEBUG: Invalid object start\n");
         fflush(stderr);
      }

      return((JSON *)NULL);
   }
   

   /* Check to see that the object/array ends properly */

   end = p + strlen(p) - 1;

   if(*end != '}' && *end != ']')
   {
      if(json_debug)
      {
         fprintf(stderr, "\nDEBUG: Invalid object end\n");
         fflush(stderr);
      }

      return((JSON *)NULL);
   }
   
   if(match == ' ' || *end != match)
   {
      if(json_debug)
      {
         fprintf(stderr, "\nDEBUG: Invalid object end match\n");
         fflush(stderr);
      }

      return((JSON *)NULL);
   }
   
   *end = '\0';


   /* Now step through the key : val (or val, val, ... sets for array) */

   if(json_debug)
   {
      fprintf(stderr, "\nDEBUG: Looking for elements in: \"%s\"\n", p);
      fflush(stderr);
   }

   blev = 0;
   begin = p;
   end = p;

   len = (int)strlen(p);



   /* Loop over object elements */

   while(FOREVER)
   {
      /* Search for closing comma */

      inquote = FALSE;
      while(FOREVER)
      {
         if(!inquote && blev == 0 && *end == ',')
            break;

         if(*end == '\0')
            break;

         if(end > p + len)
            break;

         if(*end == '"' && *(end-1) != '\\')
            inquote = !inquote;

         if(!inquote && (*end == '{' || *end == '['))
            ++blev;

         if(!inquote && (*end == '}' || *end == ']'))
            --blev;
         
         ++end;
      }
      if(inquote)
         return((JSON *)NULL);

      *end = '\0';



      /* Take the key : val expression apart */

      if(json_debug)
      {
         fprintf(stderr, "\nDEBUG: Taking apart: begin = \"%s\"\n", begin);
         fflush(stderr);
      }


      /* Strip off the leading and trailing blanks in key : value */

      sb = begin;
      sb = json_stripblanks(sb, strlen(sb), 0);

      if(json_debug)
      {
         fprintf(stderr, "\nDEBUG: Stripped: sb = \"%s\"\n", sb);
         fflush(stderr);
      }

      inquote = FALSE;
      key = sb;
      val = sb;


      /* Find ':' (if any) */

      peq = (char *) NULL;

      se = sb + strlen(sb);

      while(FOREVER)
      {
         if(!inquote && *val == ':')
         {
            peq = val;
            ++val;
            break;
         }

         if(*val == '"' && *(val-1) != '\\')
            inquote = !inquote;

         if(val >= se)
            break;

         ++val;
      }

      if(inquote)
         return((JSON *)NULL);


      /* Forget it if this unit is a object or array */

      if(*sb == '{' || *sb == '[')
      {
         peq = (char *)NULL;

         if(json_debug)
         {
            fprintf(stderr, "\nDEBUG: object or array\n");
            fflush(stderr);
         }
      }


      /* Reset if there was no colon (i.e., we have array, not object) */

      if(peq == (char *)NULL)
      {
         val = sb;

         if(json_debug)
         {
            fprintf(stderr, "\nDEBUG: array element (no colon found)\n");
            fflush(stderr);
         }
      }


      /* Assign the key, val to the return object */

      if(peq)
      {
         *peq = '\0';

         key = json_stripblanks(key, strlen(key), 1);
         strcpy(json->key[json->count], key);

         val = json_stripblanks(val, strlen(val), 1);
         strcpy(json->val[json->count], val);

         if(json_debug)
         {
            fprintf(stderr, "\nDEBUG: Found (keyword:value)  %4d: \"%s\" = \"%s\"\n", 
               json->count, key, val);
            fflush(stderr);
         }
      }
      else
      {
         sprintf(json->key[json->count], "%-d", json->count);

         key = json_stripblanks(key, strlen(key), 1);
         strcpy(json->val[json->count], key);

         if(json_debug)
         {
            fprintf(stderr, "\nDEBUG: Found (array element)  %4d: \"%s\" = \"%s\"\n", 
               json->count, json->key[json->count], val);
            fflush(stderr);
         }
      }


      /* If necessary, allocate more space for object */

      ++json->count;
      if(json->count >= json->nalloc)
      {
         json->nalloc += JSON_JSONCNT;

         json->key = (char **) realloc(json->key, json->nalloc * sizeof(char *));
         json->val = (char **) realloc(json->val, json->nalloc * sizeof(char *));

         for(i=json->nalloc - JSON_JSONCNT; i<json->nalloc; ++i)
         {
            json->key[i] = (char *) malloc(inlen * sizeof(char));
            json->val[i] = (char *) malloc(inlen * sizeof(char));
         }

         if(json_debug)
         {
            fprintf(stderr, "\nDEBUG: Allocated space for %d more keyword/value pairs\n",
               JSON_JSONCNT);
            fflush(stderr);
         }
      }


      /* Go on to the next subobject */

      begin = end + 1;
      end = begin;

      if(end >= p + len)
         break;
   }

   free(str);
   return(json);
}



/* This routine strips leading and trailing white space from   */
/* strings, returning the new starting location for the string */

char *json_stripblanks(char *ptr, int len, int quotes)
{
   char *begin, *end;


   /* First strip off the trailing white space */

   begin = ptr;
   end = begin + len - 1;
      
   while(FOREVER)
   {
      if(*end == ' '  || *end == '\t'
      || *end == '\r' || *end == '\n')
      {
         *end = '\0';
         --end;
      }
      else
         break;
      
      if(end <= begin)
         break;
   }


   /* Then move the begin pointer over the leading white space */

   while(FOREVER)
   {
      if(*begin == ' '  || *begin == '\t'
      || *begin == '\r' || *begin == '\n')
      {
         ++begin;
      }
      else
         break;

      if(begin >= end)
         break;
   }


   /* If desired, strip off leading/trailing qoutes */

   if(quotes)
   {
      if(*end == '"')
         *end = '\0';

      if(*begin == '"')
      {
         *begin = '\0';
         ++begin;
      }
   }

   return(begin);
}




int json_free(JSON *json)
{
   int i, nalloc, s;

   s = JSON_ERROR;
   if(json != (JSON *)NULL)
   { 
      nalloc = json->nalloc;

      for(i=0; i<nalloc; ++i)
      {
         free(json->key[i]);
         free(json->val[i]);
      }

      free(json->key);
      free(json->val);
      free(json);
      s = JSON_OK;
   }
   return s;
}




char *json_val(char const *structstr, char const *key, char *val)
{
   int  i, inlen, len, found;
   char *subkey, *tail, *subval;

   JSON *sv;

   inlen = strlen(structstr);

   subkey = (char *)malloc(inlen);
   tail   = (char *)malloc(inlen);
   subval = (char *)malloc(inlen);

   if(json_debug == 1)
   {
      printf("DEBUG> json_val() structstr = [%s], key = [%s]\n", structstr, key);
      fflush(stdout);
   }

   strcpy(subkey, key);
   len = strlen(subkey);

   for(i=0; i<len; ++i)
   {
      if(subkey[i] == '.' || subkey[i] == '[')
      {
         subkey[i] = '\0';

         break;
      }
   }

   if(subkey[strlen(subkey) - 1] == ']')
      subkey[strlen(subkey) - 1] = '\0';

   if(i >= len)
      tail[0] = '\0';
   else
      strcpy(tail, subkey + i + 1);
   
   if(json_debug == 1)
   {
      printf("DEBUG> json_val() subkey = [%s], tail = [%s]\n", subkey, tail);
      fflush(stdout);
   }

   len = strlen(tail);

   found = 0;
   
   if((sv = json_struct(structstr)) != (JSON *)NULL)
   {
      for(i=0; i<sv->count; ++i)
      {
         if(strcmp(sv->key[i], subkey) == 0)
         {
            if(!len)
            {
               strcpy(val, sv->val[i]);
               found = 1;
               break;
            }

            else if(json_val(sv->val[i], tail, subval))
            {
               strcpy(val, subval);
               found = 1;
               break;
            }

            else
               break;
         }
      }
   }

   json_free(sv);
   free(subkey);
   free(tail);
   free(subval);

   if(found)
      return val;
   else
      return((char *) NULL);
}
