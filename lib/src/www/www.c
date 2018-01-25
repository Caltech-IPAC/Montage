#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>


#include <www.h>

#define MAX_ENTRIES  4096
#define MAXGET       4096
#define MAXSTR       4096
#define MAXIN       16384
#define HTML_HEADER "/irsa/cm/ws/laity/irsa/web/html/include/header.html"
#define HTML_FOOTER "/irsa/cm/ws/laity/irsa/web/html/include/footer.html"

typedef struct 
{
   char *name;
   char *val;
   char *fname;
   int   isfile;
}  entry;

static entry entries[MAX_ENTRIES];
static int   nentry;

static FILE *pcontent;

int    printDebug    ();
char * fmakeword     (FILE *f, char stop, int *cl);
char * makeword      (char *line, char stop);
void   plustospace   (char *str);
int    create_tmpfile(char const *str);
char   x2c           (char const *what);

#define METHOD_UNKNOWN 0
#define METHOD_POST    1
#define METHOD_GET     2
#define METHOD_MULTI   3

FILE *keydebug = (FILE *)NULL;

char  debugFile[1024];

char  tmpdir[1024] = "/tmp";

int   keyStdout;

int   flag, showChars = 0;

int   KeyLibHasBeenInitialized = 0;

static char  keyword_stripped[MAXSTR];
static int   keyword_type;


/*****************/
/* KEYWORD_DEBUG */
/*****************/

int keylib_initialized(void)
{
   return KeyLibHasBeenInitialized;
}

void keyword_debug(FILE *dbg)
{
   keydebug = dbg;
}


/*******************/
/* KEYWORD_WORKDIR */
/*******************/

void keyword_workdir(char *workdir)
{
   strcpy(tmpdir, workdir);
}


/****************/
/* KEYWORD_INIT */
/****************/

int keyword_init(int argc, char **argv) 
{
   char *method, *count, *content;
   char  none[256];
   char  valbuf1[MAXIN];
   char  valbuf2[MAXIN];
   int   i, cl, len, ismap, total;
   int   ix, iy;
   int   ch, chb, boffset, btrans;
   char *endptr, *ptr;
 
   char  buffb[4096];
   char  buffe[4096];

   int   blenb, blene;
 
   char *form, *pboundary;
 
   char  line[MAXIN];
   char  fline[MAXIN];
   char  endline[32];
   char  boundary[1024]     = "";
   char  end_boundary[1024] = "";
 
   int   in_block             = 0;
   int   tmp_file_created     = 0;
   int   reading_block_header = 0;
   int   reading_block_data   = 0;
   int   have_name            = 0;
   int   have_value           = 0;
 
   static char  xismap[MAXSTR], yismap[MAXSTR];
   static char *x, *y, val[256];


   /* Set up temporary cache for debug output */
   /* to avoid 8K I/O buffer problem          */

   keyStdout = 0;

   if(keydebug == stdout)
   {
      int tmpfd;

      keyStdout = 1;

      sprintf(debugFile, "%s/KEYDBGXXXXXX", tmpdir);
      tmpfd = mkstemp(debugFile);

      keydebug = fdopen(tmpfd, "w+");
      chmod(debugFile, 0666);

      printf("Debug output in %s\n", debugFile);
      fflush(stdout);
   }


   /* Echo command-line arguments */

   if(keydebug)
   {
      fprintf(keydebug, "argc = %d <br>\n", argc);
 
      for(i=0; i<argc; ++i)
         fprintf(keydebug, "argv[%3d]: [%s] <br>\n", i, argv[i]);

      fprintf(keydebug, "<p>\n");
      fflush(keydebug);
   }


   /* SETUP BY FORM TYPE */

   strcpy(none, "NONE");
   strcpy(boundary, "--dummy");

   content = (char *)NULL; 
   pcontent = (FILE *)NULL;

   cl = 0;

   keyword_type = METHOD_UNKNOWN;


   form = getenv("CONTENT_TYPE");

   if(!form)
     form = none;

   method = getenv("REQUEST_METHOD");

   if(!method)
      method = none;

   if(strcmp(method, "NONE") == 0)
     method = none;

   if(keydebug)
   {
      fprintf(keydebug, "CONTENT_TYPE   = [%s]<br>\n", form);
      fprintf(keydebug, "REQUEST_METHOD = [%s]<br>\n", method);
      fflush(keydebug);
   }


   /* SETUP: multi-part data (get boundary string) */

   if(strncmp(form, "multipart/form-data", 19) == 0 && method != none)
   {
      keyword_type = METHOD_MULTI;
  
      pboundary = strstr(form, "boundary=");

      if(pboundary)
      {
         pboundary += 9;
         strcpy(boundary, "--");
         strcat(boundary, pboundary);

         if(keydebug)
         {
            fprintf(keydebug, "CONTENT_TYPE   = [multipart/form-data]<br>\n");
            fprintf(keydebug, "boundary       = [%s]<br>\n", boundary);
            fflush(keydebug);
         }
      }
      else
      {
         keyword_type = METHOD_UNKNOWN;

         if(keydebug)
         {
            fprintf(keydebug, "CONTENT_TYPE   = [multipart/form-data] (bad)<br>\n");
            fflush(keydebug);
         }
      }
   }


   /* SETUP: POST (Get content length) */

   else if(strncmp(method, "POST", 4) == 0)
   {
      keyword_type = METHOD_POST;

      count = getenv("CONTENT_LENGTH");

      cl = 0;
      if(count)
      {
         cl = atoi(count);

         if(keydebug)
         {
            fprintf(keydebug, "CONTENT_TYPE   = [POST]<br>\n");
            fprintf(keydebug, "length         = [%d]<br>\n", cl);
            fflush(keydebug);
         }
      }
      else
      {
         keyword_type = METHOD_UNKNOWN;

         if(keydebug)
         {
            fprintf(keydebug, "CONTENT_TYPE   = [POST] (bad)<br>\n");
            fflush(keydebug);
         }
      }
   }


   /* SETUP: GET (Get query string) */

   else if(strncmp(method, "GET", 3) == 0)
   {
      content = getenv("QUERY_STRING");

      if(content)
      {
         keyword_type = METHOD_GET;

         if(keydebug)
         {
            fprintf(keydebug, "CONTENT_TYPE   = [GET]<br>\n");
            fprintf(keydebug, "QUERY_STRING = [%s]<br>\n", content);
            fflush(keydebug);
         }
      }
      else
      {
         keyword_type = METHOD_UNKNOWN;

         if(keydebug)
         {
            fprintf(keydebug, "CONTENT_TYPE   = [GET] (bad)<br>\n");
            fflush(keydebug);
         }
      }
   }


   /* SETUP: Default(command-line treated like GET) */

   else
   {
      keyword_type = METHOD_GET;

      if(argc > 1)
      {
         content = argv[1];

         if(keydebug)
         {
            fprintf(keydebug, "Assuming CONTENT_TYPE   = [GET]<br>\n");
            fprintf(keydebug, "content = [%s]<br>\n", content);
            fflush(keydebug);
         }
      }
      else
      {
         if(keyStdout)
            printDebug();

         return(-1);
      }
   }



   /* PROCESSING BY FORM TYPE */

   /* PROCESSING: multi-part message data */

   if(keyword_type == METHOD_MULTI)
   {
      nentry     = 0;
      have_name  = 0;
      have_value = 0;

      strcpy(end_boundary, boundary);
      strcat(end_boundary, "--");

      if(keydebug)
      {
         fprintf(keydebug, "Got here<br>\n");
         fflush(keydebug);
      }

      /* Read and process a line from stdin.  This gets bypassed */
      /* temporarily when we find a file to be "uploaded"        */

      total = 0;

      while(1)
      {
         if(fgets(line, MAXIN, stdin) == (char *)NULL)
         {
            if(keydebug)
            {
               fprintf(keydebug, "EOF on stdin\n");
               fflush(keydebug);
            }

            break;
         }

         total += strlen(line);


         /* Analyze and strip the end-of-line charaters */

         if(strncmp(line+strlen(line)-2, "\r\n", 2) == 0)
            strcpy(endline, "-CRLF-");

         else if(strncmp(line+strlen(line)-1, "\r", 1) == 0)
            strcpy(endline, "-CR-");

         else if(strncmp(line+strlen(line)-1, "\n", 1) == 0)
            strcpy(endline, "-LF-");

         else 
            sprintf(endline, "-BAD(%d)-", *(line+strlen(line)-1));

         for(i=0; i<strlen(line); ++i)
         {
            if(line[i] == '\n' || line[i] == '\r' || line[i] == '\0')
            {
               line[i] = '\0';
               break;
            }
         }

         if(keydebug)
         {
            fprintf(keydebug, "stdin: %s%s\n", line, endline);
            fflush(keydebug);
         }

         if(strncmp(endline, "-BAD", 4) == 0)
         {
            if(keydebug)
            {
               fprintf(keydebug, "Improper line ending; exiting\n");
               fflush(keydebug);
            }

            break;
         }


         /* The line read is an "end of data" boundary */

         if(strcmp(line, end_boundary) == 0)
         {
            if(tmp_file_created)
            {
               if(pcontent)
               {
                  fflush(pcontent);
                  fclose(pcontent);
               }

               pcontent = (FILE *)NULL;
            }

            break;
         }


         /* The line read is a data block boundary */

         if(strncmp(line, boundary, strlen(boundary)) == 0)
         {
            if(tmp_file_created)
            {
               if(pcontent)
               {
                  fflush(pcontent);
                  fclose(pcontent);
               }

               pcontent = (FILE *)NULL;
            }

            in_block             = 1;
            reading_block_data   = 0;
            reading_block_header = 0;
            tmp_file_created     = 0;
            have_name            = 0;
            have_value           = 0;
            pcontent             = (FILE *)NULL;
         }


         /* Inside a data block, we process the information into   a */
         /* keyword name, a file for the value contents, and the     */
         /* original file name (if the contents are an uploaded file */

         if(in_block && (strncmp(line, "Content-Disposition: ", 21) == 0))
         {
            reading_block_header = 1;

            entries[nentry].isfile = 0;
            entries[nentry].fname  = (char *)NULL;
            entries[nentry].name   = (char *)NULL;
            entries[nentry].val    = (char *)NULL;

            have_name  = 0;
            have_value = 0;

            if(keydebug)
            {
               fprintf(keydebug, "\n");
               fflush(keydebug);
            }                    

            strcpy(fline, line);

            ptr = makeword(line, '=');

            if(line[strlen(line)-1] == '"')
               line[strlen(line)-1] = '\0';

            have_name = 1;

            entries[nentry].name = malloc(sizeof(char) * (strlen(line)+1));
            if(keydebug)
            {
               fprintf(keydebug, "mallocked %lu for entries[%d].name (%s)<br>\n",
                  (unsigned long)(strlen(line)+1), nentry, entries[nentry].name);
               fflush(keydebug);
            }

            strcpy(entries[nentry].name, line + 1);

            ptr = entries[nentry].name;

            while(*ptr != ' ' && *ptr != ';' && *ptr != '"' && *ptr != '\0')
               ++ptr;
      
            *ptr = '\0';

            if(keydebug)
            {
               fprintf(keydebug, "entries[%d].name -> [%s] (%lu) (file upload)\n\n", 
                  nentry, entries[nentry].name, (unsigned long)(strlen(line + 1)+1));
               fflush(keydebug);
            }                    

            tmp_file_created = create_tmpfile(fline);
         }

 
         /* Inside a data block, we hit the "null line" */
         /* that indicates the end of the HTTP header   */

         if(in_block && reading_block_header && (line[0] == '\0'))
         {
            in_block             = 0;
            reading_block_header = 0;
            reading_block_data   = 1;


            /* OK, if we are uploading a file and past the     */
            /* HTTP header we need to switch to 'binary'       */
            /* mode to get the data.  The trick here is to     */
            /* properly identify the end boundary (both types) */

            if(tmp_file_created)
            {
               if(keydebug)
               {
                  fprintf(keydebug, "Binary file transfer started\n");
                  fflush(keydebug);
               }                    

               boffset = 0;
               btrans  = 0;

               strcpy(buffb, "\r\n");
               strcat(buffb, boundary);
               strcat(buffb, "\r\n");

               strcpy(buffe, "\r\n");
               strcat(buffe, end_boundary);
               strcat(buffe, "\r\n");

               blenb = strlen(buffb);
               blene = strlen(buffe);

               while(1)
               {
                  ch = fgetc(stdin);

                  if(showChars)
                  {
                     if(ch >= 32 && ch <=126)
                        fprintf(keydebug, "In       [%c](%d)\n", ch, ch);
                     else
                        fprintf(keydebug, "In       [](%d)\n", ch);
                     fflush(keydebug);
                  }                    

                  if(ch == -1)
                  {
                     if(keyStdout)
                        printDebug();

                     return(-1);
                  }

                  if(ch == buffb[boffset]
                  || ch == buffe[boffset])
                     ++boffset;
                  else
                  {
                     if(boffset > 0)
                     {
                        for(i=0; i<boffset; ++i)
                        {
                           have_value = 1;

                           chb = buffe[i];
                           flag = fputc(chb, pcontent);

                           if(showChars && flag == EOF)
                           {
                              fprintf(keydebug, "Error from fputc()\n");
                              fflush(keydebug);
                           }

                           if(showChars)
                           {
                              if(ch >= 32 && ch <=126)
                                 fprintf(keydebug, "From buf [%c](%d)\n", chb, chb);
                              else
                                 fprintf(keydebug, "From buf [](%d)\n", chb);
                              fflush(keydebug);
                           }                    

                           ++btrans;
                        }

                        boffset = 0;
                     }

                     have_value = 1;

                     if(ch != buffb[0])
                     {
                        flag = fputc(ch, pcontent);

                        if(showChars && flag == EOF)
                        {
                           fprintf(keydebug, "Error from fputc()\n");
                           fflush(keydebug);
                        }

                        if(showChars)
                        {
                           if(ch >= 32 && ch <=126)
                              fprintf(keydebug, "Out      [%c](%d)\n", ch, ch);
                           else
                              fprintf(keydebug, "Out      [](%d)\n", ch);
                           fflush(keydebug);
                        }                    

                        ++btrans;
                     }

                     else
                        ++boffset;
                  }
                  
                  if(boffset == blene)
                  {
                     if(keydebug)
                     {
                        fprintf(keydebug, "Binary file transfer found EOF boundary\n");
                        fflush(keydebug);
                     }                    

                     fflush(pcontent);
                     fclose(pcontent);

                     if(keydebug)
                     {
                        fprintf(keydebug, "Closing temporary file\n");
                        fflush(keydebug);
                     }                    

                     pcontent = (FILE *)NULL;

                     break;
                  }
                  
                  if(boffset == blenb && ch != buffe[boffset-1])
                  {
                     if(keydebug)
                     {
                        fprintf(keydebug, "Binary file transfer found block boundary\n");
                        fflush(keydebug);
                     }                    

                     fflush(pcontent);
                     fclose(pcontent);

                     if(keydebug)
                     {
                        fprintf(keydebug, "Closing Temporary file\n");
                        fflush(keydebug);
                     }                    

                     in_block             = 1;
                     reading_block_data   = 0;
                     reading_block_header = 0;
                     tmp_file_created     = 0;

                     pcontent = (FILE *)NULL;

                     break;
                  }
               }

               if(keydebug)
               {
                  fprintf(keydebug, "Binary file transfer complete\n");
                  fprintf(keydebug, "%d bytes transferred (nentry = %d, have_name = %d, have_value = %d)\n", 
                     btrans, nentry, have_name, have_value);
                  fflush(keydebug);
               }                    

               if(btrans == 0)
                  have_value = 1;

               if(boffset == blene)
               {
                  if(have_name && have_value)
                  {
                     ++nentry;

                     if(keydebug)
                     {
                        fprintf(keydebug, "nentry -> %d (EOF after file transfer)\n", nentry); 
                        fflush(keydebug);
                     }                    
                  }

                  break;
               }
            }
         }

         if(have_name && have_value)
         {
            ++nentry;

            if(keydebug)
            {
               fprintf(keydebug, "nentry -> %d\n", nentry); 
               fflush(keydebug);
            }                    
         }
      }


      /* Print out the keyword/value (and file      */
      /* upload) info for this METHOD_MULTI dataset */

      for(i=0; i<nentry; ++i)
      {
         if(entries[i].fname && !entries[i].isfile)
         {
            pcontent = fopen(entries[i].fname, "r");

            if(pcontent)
            {
               if(fgets(valbuf1, MAXIN, pcontent) == (char *)NULL)
               {
                  if(keydebug)
                  {
                     fprintf(keydebug, "EOF reading first line from %s => null it out\n",
                        entries[i].fname);
                     fflush(keydebug);
                  }

                  fflush(pcontent);
                  fclose(pcontent);

                  pcontent = (FILE *)NULL;

                  entries[i].val = malloc(2 * sizeof(char));

                  strcpy(entries[i].val, "");

                  unlink(entries[i].fname);

                  free(entries[i].fname);

                  entries[i].fname = (char *)NULL;
               }
               else
               {
                  if(fgets(valbuf2, MAXIN, pcontent) == (char *)NULL)
                  {
                     if(keydebug)
                     {
                        fprintf(keydebug, "EOF reading second line from %s => string value\n",
                           entries[i].fname);
                        fflush(keydebug);
                     }

                     fflush(pcontent);
                     fclose(pcontent);

                     pcontent = (FILE *)NULL;

                     entries[i].val = malloc((strlen(valbuf1)+1) * sizeof(char));

                     strcpy(entries[i].val, valbuf1);

                     unlink(entries[i].fname);

                     free(entries[i].fname);

                     entries[i].fname = (char *)NULL;
                  }
                  else
                  {
                     fflush(pcontent);
                     fclose(pcontent);

                     pcontent = (FILE *)NULL;
                  }
               }
            }
         }
      }


      /* Print out the keyword/value (and file      */
      /* upload) info for this METHOD_MULTI dataset */

      if(keydebug)
      {
         fprintf(keydebug, "\n\n<p>%d Name/Value pairs:\n\n", nentry);
         fflush(keydebug);

         for(i=0; i<nentry; ++i)
         {
            if(entries[i].val)
            {
               if(entries[i].fname)
                  fprintf(keydebug, "%4d: [%s] = [%s] (%s)\n", 
                     i, entries[i].name, entries[i].val, entries[i].fname);
               else
                  fprintf(keydebug, "%4d: [%s] = [%s] <no file>\n", 
                     i, entries[i].name, entries[i].val);
            }
            else
            {
               if(entries[i].fname)
                  fprintf(keydebug, "%4d: [%s] = <no val> (%s)\n", 
                     i, entries[i].name, entries[i].fname);
               else
                  fprintf(keydebug, "%4d: [%s] = <no val> <no file>\n", 
                     i, entries[i].name);
            }
                    
            fflush(keydebug);
         }
      }
   }


   /* PROCESSING: POST (parse stdin line) */

   else if(keyword_type == METHOD_POST)
   {
      nentry = 0;

      if(keydebug)
      {
         fprintf(keydebug, "<p>POST: <br>\n");
         fflush(keydebug);
      }

      for(i=0; cl && (!feof(stdin)); ++i)
      {
         entries[i].val = fmakeword(stdin, '&', &cl);

         plustospace(entries[i].val);
         unescape_url(entries[i].val);

         entries[i].name = makeword(entries[i].val, '=');
         ++nentry;
      }

      if(keydebug)
      {
         fprintf(keydebug, "<p>\n");

         for(i=0; i<nentry; ++i)
         {
            fprintf(keydebug, "%4d: [%s] = [%s]<br>\n", 
                    i, entries[i].name, entries[i].val);
            fflush(keydebug);
         }
      }
   }


   /* PROCESSING: GET (parse query string) */

   else if(keyword_type == METHOD_GET)
   {
      nentry = 0;

      if(keydebug)
      {
         fprintf(keydebug, "<p>GET:<br>\n%s<br>\n", content);
         fflush(keydebug);
      }

      for(i=0; content[0] != '\0'; ++i) 
      {
         entries[i].fname = (char *)NULL;

         entries[i].val = makeword(content, '&');

         plustospace(entries[i].val);
         unescape_url(entries[i].val);

         entries[i].name = makeword(entries[i].val, '=');

         if(strlen (entries[i].val) > 10
         && strncmp(entries[i].val, "localfile:", 10) == 0)
         {
            entries[i].val += 10;
            entries[i].fname = entries[i].val;
         }

         ++nentry;
      }

      if(keydebug)
      {
         fprintf(keydebug, "<p>\n");

         for(i=0; i<nentry; ++i)
         {
            if(entries[i].fname)
               fprintf(keydebug, "%4d: [%s] = [%s] (%s)\n",
                  i, entries[i].name, entries[i].val, entries[i].fname);
            else
               fprintf(keydebug, "%4d: [%s] = [%s] <no file>\n",
                  i, entries[i].name, entries[i].val);

            fflush(keydebug);
         }
      }


      /* Special case 1: image coordinates */

      if(nentry == 1 
         && strlen(entries[0].val) == 0
         && strlen(entries[0].name) <= 256) 
      {
         strcpy(val, entries[0].name);
         x = val;
         y = val;

         ismap = 1;
         len = strlen(val);
         for(i=0; i<len-1; ++i)
         {
            if(val[i] == ',')
            {
               y = val + i + 1;
               val[i] = '\0';
               break;
            }
         }

         if(keydebug)
         {
            fprintf(keydebug, "x = [%s]<br>\n", x);
            fprintf(keydebug, "y = [%s]<br>\n", y);
            fflush(keydebug);
         }

         ismap = 1;
         if(i == len-1)
            ismap = 0;

         ix = strtol(x, &endptr, 10);
         if(endptr < x+(int)strlen(x))
            ismap = 0;

         iy = strtol(y, &endptr, 10);
         if(endptr < y+(int)strlen(y))
            ismap = 0;

         if(ismap)
         {
            strcpy(xismap, "xismap");
            strcpy(yismap, "yismap");
            entries[0].name = xismap;
            entries[1].name = yismap;
            entries[0].val  = x;
            entries[1].val  = y;
            nentry = 2;

            if(keydebug)
            {
               fprintf(keydebug, "ISMAP:<br>\n");

               for(i=0; i<nentry; ++i)
               {
                  fprintf(keydebug, "%4d: [%s] = [%s]<br>\n", 
                          i, entries[i].name, entries[i].val);
                  fflush(keydebug);
               }
            }                    
         }
      }
   }


   /* PROCESSING: ERROR (unknown method) */

   else
   {
      if(keydebug)
      {
         fprintf(keydebug, "<p>Request method unknown<br>\n");
         fflush(keydebug);
      }                    

      if(keyStdout)
         printDebug();

      return(-1);
   }
 
   if(keyStdout)
      printDebug();

   if(pcontent)
   {
      fflush(pcontent);
      fclose(pcontent);
   }

   pcontent = (FILE *)NULL;
   
   KeyLibHasBeenInitialized = 1;

   return(nentry);
}


int keyword_count()
{
   if(keydebug)
   {
      fprintf(keydebug, "keyword_count() returning %d <br>\n", nentry);
      fflush(keydebug);
   }

   return(nentry);
}


int printDebug()
{
   FILE *fp;
   char  line[MAXSTR];

   fclose(keydebug);

   keydebug = stdout;

   fp = fopen(debugFile, "r");

   while(1)
   {
      if(fgets(line, MAXSTR, fp) == (char *)NULL)
         break;

      printf("%s", line);
   }

   fclose(fp);

   unlink(debugFile);

   return 0;
}



/*****************/
/* KEYWORD_CLOSE */
/*****************/

void keyword_close()
{
   int i;

   if(pcontent)
   {
      fflush(pcontent);
      fclose(pcontent);

      pcontent = (FILE *)NULL;
   }

   if(!showChars && (keyword_type != METHOD_GET))
   {
      for(i=0; i<nentry; ++i)
      {
         if(entries[i].fname != (char *)NULL)
            unlink(entries[i].fname);
      }
   }
}



/******************/
/* KEYWORD_EXISTS */
/******************/

int keyword_exists(char const * key)
{
   int i;


   /* Find the value of the requested keyword */

   for(i=0; i<nentry; ++i)
   {
      if(strcmp(entries[i].name, key) == 0)
         return(1);
   }

   return(0);
}



/*****************/
/* KEYWORD_VALUE */
/*****************/

char *keyword_value(char const *key)
{
   int i;


   /* Find the value of the requested keyword */

   for(i=0; i<nentry; ++i)
   {
      if(strcmp(entries[i].name, key) == 0)
         return(html_encode(entries[i].val));
   }

   return((char *) NULL);
}

   
char *keyword_value_unsafe(char const *key)
{
   int i;


   /* Find the value of the requested keyword */

   for(i=0; i<nentry; ++i)
   {
      if(strcmp(entries[i].name, key) == 0)
         return(entries[i].val);
   }

   return((char *) NULL);
}



/****************************/
/* "STRIPPED" KEYWORD_VALUE */
/****************************/

char *keyword_value_stripped(char const *key)
{
   int   i, j;
   char *ptr;


   /* Find the value of the requested keyword */

   for(i=0; i<nentry; ++i)
   {
      if(strcmp(entries[i].name, key) == 0)
      {
         ptr = entries[i].val;

         while(1)
         {
            if(*ptr != ' ')
               break;
            
            ++ptr;
         }

         strcpy(keyword_stripped, ptr);

         for(j=strlen(keyword_stripped)-1; j>=0; --j)
         {
            if(keyword_stripped[j] != ' ')
               break;
            
            keyword_stripped[j] = '\0';
         }

         return(html_encode(keyword_stripped));
      }
   }

   return((char *) NULL);
}



/********************/
/* KEYWORD_INSTANCE */
/********************/

char *keyword_instance(char const *key, int count)
{
   int i, found;


   /* Find the value of the requested keyword */

   found = 0;

   for(i=0; i<nentry; ++i)
   {

      if(strcmp(entries[i].name, key) == 0)
         ++found;

      if(found == count)
         return(html_encode(entries[i].val));
   }
   
   return((char *) NULL);
}


char *keyword_instance_unsafe(char const *key, int count)
{
   int i, found;


   /* Find the value of the requested keyword */

   found = 0;

   for(i=0; i<nentry; ++i)
   {

      if(strcmp(entries[i].name, key) == 0)
         ++found;

      if(found == count)
         return(entries[i].val);
   }
   
   return((char *) NULL);
}



/********************/
/* KEYWORD_FILENAME */
/********************/

char *keyword_filename(char const *key)
{
   int i;


   /* Find the file created for the requested keyword */
   /* (returns NULL if this keyword doesn't have one) */

   for(i=0; i<nentry; ++i)
   {
      if(strcmp(entries[i].name, key) == 0)
         return(html_encode(entries[i].fname));
   }

   return((char *) NULL);
}



/****************/
/* KEYWORD_INFO */
/****************/

int keyword_info(int index, char **keyname, char **keyval, char **fname)
{
   if(index < 0 || index >= nentry)
      return(1);

   *keyname = entries[index].name;
   *keyval  = html_encode(entries[index].val);
   *fname   = entries[index].fname;

   return(-1);
}


int keyword_info_unsafe(int index, char **keyname, char **keyval, char **fname)
{
   if(index < 0 || index >= nentry)
      return(1);

   *keyname = entries[index].name;
   *keyval  = entries[index].val;
   *fname   = entries[index].fname;

   return(-1);
}


/********************/
/* Utility Routines */
/********************/

char *fmakeword(FILE *f, char stop, int *cl) 
{
   int wsize;
   static char *word;
   int ll;

   ll=0;
   wsize = 1024;
   word = (char *) malloc(sizeof(char) * (wsize));

   while(1) 
   {
      if(ll >= wsize-1) 
      {
         wsize += 1024;
         word = (char *)realloc(word, sizeof(char)*(wsize));
      }

      word[ll] = (char)fgetc(f);

      if(keydebug)
      {
         putc(word[ll], keydebug);
         fflush(keydebug);
      }

      --(*cl);

      if((word[ll] == stop) || (feof(f)) || (!(*cl))) 
      {
         if(word[ll] != stop) 
              ++ll;

         word[ll] = '\0';
         return word;
      }

      ++ll;
   }
}



char *makeword(char *line, char stop) 
{
   int   i, j;
   static char *word;

   word = (char *) malloc(sizeof(char) * (strlen(line)+1));

   for(i=0; ((line[i]) && (line[i] != stop)); ++i)
      word[i] = line[i];

   word[i] = '\0';

   if(line[i]) 
      ++i;

   j=0;
   while( (line[j++] = line[i++]) ) ;

   return word;
}



void plustospace(char *str) 
{
   register int i;

   for(i=0; str[i]; ++i) 
      if(str[i] == '+') 
         str[i] = ' ';
}



void unescape_url(char *url) 
{
   register int i, j;

   for(i=0, j=0; url[j]; ++i, ++j) 
   {
      if((url[i] = url[j]) == '%') 
      {
         url[i] = x2c(&url[j+1]);
         j+=2;
      }
   }

   url[i] = '\0';
}



char x2c(char const *what) 
{
   register char digit;

   digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
   digit *= 16;
   digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));

   return(digit);
}



int create_tmpfile(char const *str)
{
   int  i, fd;
   char *p, *end, *fname;

   p  = strstr(str, "filename=\"");
   fname = (char *) NULL;

   if(p)
   {
      entries[nentry].isfile = 1;

      p += 10;

      end = p;

      while(*end != '"'
         && *end != '\0'
         && *end != '\n'
         && *end != '\r')
      {
         ++end;
      }

      *end = '\0';

      if(*p == '\0')
         fname = p;
      else
      {
         fname = p + strlen(p) - 1;

         while(*fname != '\\' && *fname != '/') 
         {
            if(fname == p)
            {
               --fname;
               break;
            }
           else
              --fname;
         }
         ++fname;
      }

      entries[nentry].val = malloc(sizeof(char) * (strlen(fname)+1));

      strcpy(entries[nentry].val, fname);

      for(i=0; i<strlen(entries[nentry].val); ++i)
      {
         if(entries[nentry].val[i] == ';')
         {
            strcpy(entries[nentry].val, "(semicolon)");
         }
      }
   }
   else
      entries[nentry].val = (char *)NULL;

   if(keydebug)
   {
      if(entries[nentry].val)
         fprintf(keydebug, "create_tmpfile: entries[%d].val   = [%s] (%lu)\n", 
            nentry, entries[nentry].val, (unsigned long)(strlen(fname)+1));
      else
         fprintf(keydebug, "create_tmpfile: entries[%d].val is null\n", 
            nentry);

      fflush(keydebug);
   }                    

   entries[nentry].fname = malloc(sizeof(char) * (MAXSTR));

   sprintf(entries[nentry].fname, "%s/UPLOAD", tmpdir);

   if(entries[nentry].isfile)
   {
      strcat(entries[nentry].fname, "_");
      strcat(entries[nentry].fname, entries[nentry].val);
      strcat(entries[nentry].fname, "_");
   }

   strcat(entries[nentry].fname, "XXXXXX");

   fd = mkstemp(entries[nentry].fname);

   if(keydebug)
   {
      fprintf(keydebug, "create_tmpfile: entries[%d].fname = [%s] (%d)\n", 
         nentry, entries[nentry].fname, MAXSTR);
      fflush(keydebug);
   }                    
   
   pcontent = fdopen(fd, "w+");

   if(pcontent == (FILE *)NULL)
   {
      printf("Error: upload file open failed [%s].\n", entries[nentry].fname);
      exit(0);
   }

   chmod(entries[nentry].fname, 0666);

   return 1;
}



int is_blank(char const *s)
{
   int i = 0;

   if(s == NULL)
      return 1;

   while(s[i] != '\n')
   {
      if(!isspace((int)((unsigned char)s[i])))
         return 0;

      ++i;
   }

   return 1;
}


char *html_encode(char const *s)
{
   int           len, special;
   register int  i, j;
   unsigned char *str;


   if(s == (char *)NULL)
      return((char *)NULL);


   // First scan the input for the number of 'special' characters.

   len = strlen(s);

   special = 0;

   for(i=0; i<len; ++i)
   {
      if(s[i] == '&'
      || s[i] == '<'
      || s[i] == '>'
      || s[i] == '\''
      || s[i] == '"')
         ++special;
   }


   // Allocate enough space for the encoded string.

   str = (unsigned char *)malloc((len+5*special+1)*sizeof(char));

   str[0] = '\0';


   // Copy the string, replacing the 'special' characters.
   // Note: quotes are allowed through for now.

   j = 0;

   for(i=0; i<len; ++i)
   {
           if(s[i] == '&' ) {strcat((char *)str, "&amp;");  j+=5;}
      else if(s[i] == '<' ) {strcat((char *)str, "&lt;");   j+=4;}
      else if(s[i] == '>' ) {strcat((char *)str, "&gt;");   j+=4;}
   // else if(s[i] == '\'') {strcat((char *)str, "&#39;");  j+=5;}
   // else if(s[i] == '"' ) {strcat((char *)str, "&quot;"); j+=6;}
      else                  {str[j] = s[i];                 j+=1;}

      str[j] = '\0';
   }

   return ((char *) str);
} 


static unsigned char hexchars[] = "0123456789ABCDEF";

char *url_encode(char const *s)
{
   int      len;
   register int i, j;
   unsigned char *str;

   len = strlen(s);

   str = (unsigned char *) malloc(3 * strlen(s) + 1);

   j = 0;

   for (i=0; i<len; ++i) 
   {
      str[j] = (unsigned char) s[i];

      if (str[j] == ' ') 
      {
         str[j] = '+';
      }
      else if ((str[j] < '0' && str[j] != '-' && str[j] != '.') ||
               (str[j] < 'A' && str[j] > '9')                   ||
               (str[j] > 'Z' && str[j] < 'a' && str[j] != '_')  ||
               (str[j] > 'z')) 
      {
         str[j++] = '%';

         str[j++] = hexchars[(unsigned char) s[i] >> 4];

         str[j]   = hexchars[(unsigned char) s[i] & 15];
      }

      ++j;
   }

   str[j] = '\0';

   return ((char *) str);
}


char *url_decode(char const *s)
{
   int      len, index;
   register int i, j;
   unsigned char *str;
   char     index_str[5];
   char    *end;

   len = strlen(s);

   str = (unsigned char *) malloc(strlen(s) + 1);

   j = 0;

   for (i=0; i<len; ++i)
   {
      str[j] = (unsigned char) s[i];

      if (str[j] == '+')
      {
         str[j] = ' ';
      }
      else if (str[j] == '%' && i<len-2)
      {
         index_str[0] = '0';
         index_str[1] = 'x';
         index_str[2] = s[i+1];
         index_str[3] = s[i+2];
         index_str[4] = '\0';

         index = strtol(index_str, &end, 0);

         if(end < index_str + strlen(index_str)
         || index < 0 || index > 255)
         {
            str[j+1] = (unsigned char) s[i+1];
            str[j+2] = (unsigned char) s[i+2];

            j+=2;
         }
         else
            str[j] = (unsigned char)index;

         i+=2;
      }

      ++j;
   }

   str[j] = '\0';

   return ((char *) str);
}


void encodeOffsetURL(char *out, int start)
{
   int  i, j;
   char hexstr[8];

   char *in = (char *)malloc(strlen(out)+1);

   strcpy(in, out);

   for(i=strlen(in)-1; i>=0; --i)
   {
      if(in[i] != ' ')
         break;

      in[i] = '\0';
   }

   i = 0;

   for(j=0; j<strlen(in); ++j)
   {
      if(j<start)
      {
         out[i] = in[j];
         ++i;
      }

      else if(in[j] == ' ')
      {
         out[i] = '+';
         ++i;
      }

      else if(in[j] == '*'
           || in[j] == '-'
           || in[j] == '.'
           || in[j] == '_'
           || (in[j] >= '0' && in[j] <= '9')
           || (in[j] >= 'a' && in[j] <= 'z')
           || (in[j] >= 'A' && in[j] <= 'Z'))
      {
         out[i] = in[j];
         ++i;
      }

      else
      {
         sprintf(hexstr, "%02x", in[j]);

         out[i]   = '%';
         out[i+1] = (char)toupper((int)hexstr[0]);
         out[i+2] = (char)toupper((int)hexstr[1]);
         i+=3;
      }
   }

   out[i] = '\0';

   free(in);
}

/*********************************************************/
/* int initHTTP(FILE *fout, char *cookiestr)             */
/*                                                       */
/* Routine to print out a standard HTTP header.          */
/*                                                       */
/* INPUTS:                                               */
/* fout: file descriptor of output, ie stdout            */
/* cookiestr: if null or blank, no cookie will be set.   */
/*            To set a cookie, provide the full          */
/*            cookie string, ie "ISIS=TMP_XXXXXXX"       */
/*                                                       */
/* RETURN CODES:                                         */
/*    WWW_OK: webpage header successfully written to fout*/
/*    WWW_BADFOUT: file descriptor isn't open            */
/*                                                       */
/*********************************************************/

int initHTTP(FILE *fout, char const *cookiestr)
{

  int setcookie = 0;
  char timeout[256];
  struct tm *gmt;
  static time_t clock;
  char day[7][10] = {"Sunday",    "Monday",  "Tuesday",
                     "Wednesday", "Thursday", "Friday",  "Saturday"};
  char month[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


  /* The file descriptor has to already be open */
  if (fout == (FILE *)NULL)
    return WWW_BADFOUT;

  /* Was a cookie provided? */
  if ( (cookiestr != NULL) && (strlen(cookiestr) > 0) )
  {
    setcookie = 1;

    /* Set a default expiration */
    time(&clock);
    clock += (time_t) ((int)(14 * 24 * 60 * 60));
    gmt = gmtime(&clock);
    gmt->tm_year += 1900;
    sprintf(timeout, "%s,%02d-%s-%04d %02d:%02d:%02d GMT",
      day[gmt->tm_wday], gmt->tm_mday, month[gmt->tm_mon], gmt->tm_year,
      gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
  }
  
  if (keydebug)
  {
    fprintf(keydebug, "DEBUG> initHTTP: setcookie=[%d]<br>\n", setcookie);
    fprintf(keydebug, "DEBUG> initHTTP: cookiestr=[%s]<br>\n", cookiestr);
    fflush(keydebug);
  }


  /* Print standard HTTP header */
  fprintf(fout, "HTTP/1.0 200 OK\r\n");
  fprintf(fout, "Content-type: text/html\r\n");
  if (setcookie)
    fprintf(fout, "Set-Cookie: %s;path=/;expires=%s\r\n", cookiestr, timeout);
  fprintf(fout, "\r\n");
                                        
  return WWW_OK;
      
}

/*********************************************************/
/* int wwwHeader(FILE *fout, char *header, char *title)  */
/*                                                       */
/* Begins html portion of a web page (after HTTP header),*/
/* including head/title/body and the contents of the     */
/* file given (i.e., standard header.html)               */
/*                                                       */
/* Inputs:                                               */
/* fout: file descriptor of output, ie stdout            */
/* header: if given, file path to html header.  If null  */
/*         or blank, will be retrieved from ISIS.conf,   */
/*         then the environment, and finally defaults to */
/*         a define statement at the top of this code.   */
/*         To print out no header contents (you want a   */
/*         page that's blank except for what you put in),*/
/*         use the text "NOHEAD"                         */
/* title: Title of webpage.                              */
/*                                                       */
/* Return Codes:                                         */
/*    WWW_OK: header successfuly printed to fout.        */
/*    WWW_BADHEAD: Unable to open HTML_HEADER            */
/*                                                       */
/*********************************************************/
int wwwHeader(FILE *fout, char const *header, char const *title)
{
  char myheader[MAXSTR];
  FILE *fheader;
  char mytitle[MAXSTR];
  char str[MAXSTR];
  int printheader = 1;

  /* The file descriptor has to already be open */
  if (fout == (FILE *)NULL)
    return WWW_BADFOUT;

  /* Set a blank title if none was provided */
  if ( (title == NULL) || (strlen(title) == 0) )
    strcpy(mytitle, "");
  else
    strcpy(mytitle, title);

  /* Get file path of header include */
  if ( (header == NULL) || (strlen(header) == 0) )
  {
    if (getenv("HTML_HEADER") != (char *)NULL)
      strcpy(myheader, getenv("HTML_HEADER"));
    else
      strcpy(myheader, HTML_HEADER);
  }
  else
    strcpy(myheader, header);

  if (strcmp(myheader, "NOHEAD") == 0)
    printheader = 0;

  if (printheader)
  {
    fheader = fopen(myheader, "r");
    if (fheader == (FILE *)NULL)
      return WWW_BADHEAD;
  }

  /* Start webpage */
  fprintf(fout, "<html>\r\n");
  fprintf(fout, "<head>\r\n");
  fprintf(fout, "<title>%s</title>\r\n", mytitle);

  if (printheader)
  {
    /* Print HTML header */
    while (1)
    {
      if (fgets(str, MAXSTR, fheader) == (char *)NULL)
        break;
      fprintf(fout, "%s", str);
    }
    fclose(fheader);
  }
  else
    fprintf(fout, "</head><body bgcolor=\"#FFFFFF\">\n");

  fflush(fout);
  return WWW_OK;
}
                        
/*********************************************************/
/* int wwwFooter(FILE *fout, char *footer)               */
/*                                                       */
/* Ends a webpage by printing out footer (if given)      */
/* and closing the body/html tags.                       */
/*                                                       */
/* Inputs:                                               */
/* fout: file descriptor of output, ie stdout            */
/* footer: if given, file path to html footer.  If null  */
/*         or blank, will be retrieved from ISIS.conf,   */
/*         then the environment, and finally defaults to */
/*         a define statement at the top of this code.   */
/*         To print out no footer contents (you want a   */
/*         page that's blank except for what you put in),*/
/*         use the text "NOFOOT"                         */
/*                                                       */
/* Return Codes:                                         */
/*    WWW_OK: header successfuly printed to fout.        */
/*    WWW_BADFOOT: Unable to open HTML_FOOTER            */
/*                                                       */
/*********************************************************/
int wwwFooter(FILE *fout, char const *footer)
{
  char myfooter[MAXSTR];
  FILE *ffooter;
  char str[MAXSTR];
  int printfooter = 1;

  /* The file descriptor has to already be open */
  if (fout == (FILE *)NULL)
    return WWW_BADFOUT;

  /* Get file path of footer include */
  if ( (footer == NULL) || (strlen(footer) == 0) )
  {
    if (getenv("HTML_FOOTER") != (char *)NULL)
      strcpy(myfooter, getenv("HTML_FOOTER"));
    else
      strcpy(myfooter, HTML_FOOTER);
  }
  else
    strcpy(myfooter, footer);

  if (strcmp(myfooter, "NOFOOT") == 0)
    printfooter = 0;

  if (printfooter)
  {
    ffooter = fopen(myfooter, "r");
    if (ffooter == (FILE *)NULL)
      return WWW_BADFOOT;
  }

  /* End webpage */

  if (printfooter)
  {
    /* Print HTML footer */
    while (1)
    {
      if (fgets(str, MAXSTR, ffooter) == (char *)NULL)
        break;
      fprintf(fout, "%s", str);
    }
    fclose(ffooter);
  }
  else
    fprintf(fout, "</body></html>\n");

  fflush(fout);
  return WWW_OK;
}



