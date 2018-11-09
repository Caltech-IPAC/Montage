/* Module: mArchiveList.c


Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        14Dec04  Baseline code

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>

#include <mArchiveList.h>
#include <montage.h>

#define MAXLEN 20000

static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/* mArchiveList -- Given a location on the sky, archive name, and size   */
/* in degrees contact the IRSA server to retreive a list of archive      */
/* images.  The list contains enough information to support mArchiveGet  */
/* downloads.                                                            */
/*                                                                       */
/*   char  *survey         Data survey to search (e.g. 2MASS, SDSS,      */
/*                         WISE, etc.)                                   */
/*                                                                       */
/*   char  *band           Wavelength band (e.g. J for 2MASS, g for      */
/*                         SDSS)                                         */
/*                                                                       */
/*   char  *locstr         A (quoted if necessary) string containing     */
/*                         a coordinate or the name of an object on      */
/*                         the sky                                       */
/*                                                                       */
/*   double width          Image width in degrees                        */
/*   double height         Image height in degrees                       */
/*                                                                       */
/*   char  *outfile        Output FITS header file                       */
/*                                                                       */
/*   int    debug          Debugging output flag                         */
/*                                                                       */
/*************************************************************************/


struct mArchiveListReturn *mArchiveList(char *survey, char *band, char *location, double width, double height,
                                        char *outfile, int debug)
{
   int    socket, port, count;
   double size;
  
   char   line      [MAXLEN];
   char   request   [MAXLEN];
   char   base      [MAXLEN];
   char   constraint[MAXLEN];
   char   server    [MAXLEN];
   char   source    [MAXLEN];

   FILE  *fout;

   char *ptr;
   char *proxy;
   char  pserver    [MAXLEN];

   int   pport;

   struct mArchiveListReturn *returnStruct;

   char *surveystr;
   char *bandstr;
   char *locstr;

   if(debug)
   {
      printf("DEBUG> survey:   [%s]\n", survey);
      printf("DEBUG> band:     [%s]\n", band);
      printf("DEBUG> location: [%s]\n", location);
      printf("DEBUG> width:    %-g\n",  width);
      printf("DEBUG> height:   %-g\n",  height);
      printf("DEBUG> outfile:  [%s]\n", outfile);
      fflush(stdout);
   }


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mArchiveListReturn *)malloc(sizeof(struct mArchiveListReturn));

   memset((void *)returnStruct, 0, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /* Process command-line parameters */

   strcpy(server, "montage-web.ipac.caltech.edu");

   port = 80;

   strcpy(base, "/cgi-bin/ArchiveList/nph-archivelist?");

   surveystr = mArchiveList_url_encode(survey);
   bandstr   = mArchiveList_url_encode(band);
   locstr    = mArchiveList_url_encode(location);

   size = sqrt(width*width + height*height);

   sprintf(constraint, "survey=%s+%s&location=%s&size=%.4f&units=deg&mode=TBL",
      surveystr, bandstr, locstr, size);

   free(surveystr);
   free(bandstr);
   free(locstr);

   fout = fopen(outfile, "w+");

   if(fout == (FILE *)NULL)
   {   
      sprintf(returnStruct->msg, "Can't open output file %s", outfile);
      return returnStruct;
   } 


   /* Connect to the port on the host we want */
   proxy = getenv("http_proxy");

   if(proxy) 
   {
      if(mArchiveList_parseUrl(proxy, pserver, &pport) > 0)
      {   
         strcpy(returnStruct->msg, montage_msgstr);
         return returnStruct;
      } 

      if(debug)
      {
         printf("DEBUG> proxy = [%s]\n", proxy);
         printf("DEBUG> pserver = [%s]\n", pserver);
         printf("DEBUG> pport = [%d]\n", pport);
         fflush(stdout);
      }

      socket = mArchiveList_tcp_connect(pserver, pport);
   }
   else
      socket = mArchiveList_tcp_connect(server, port);

   if(socket == 0)
   {   
      strcpy(returnStruct->msg, montage_msgstr);
      return returnStruct;
   }
   

   /* Send a request for the file we want */

   if(proxy) 
   {
      sprintf(request, "GET http://%s:%d%s%s HTTP/1.0\r\n\r\n",
            server, port, base, constraint);
   }
   else 
   {
      sprintf(request, "GET %s%s HTTP/1.0\r\nHOST: %s:%d\r\n\r\n",
            base, constraint, server, port);
   }

   if(debug)
   {
      printf("DEBUG> request = [%s]\n", request);
      fflush(stdout);
   }

   send(socket, request, strlen(request), 0);


   /* And read all the lines coming back */

   count = 0;

   while(1)
   {
      /* Read lines returning from service */

      if(mArchiveList_readline (socket, line) == 0)
         break;

      if(debug)
      {
         printf("DEBUG> return: [%s]\n", line);
         fflush(stdout);
      }

      if(count == 0 && strncmp(line, "HTTP", 4) == 0)
         continue;

      if(count == 0 && strncmp(line, "Content-type", 12) == 0)
         continue;

      if(count == 0 && strcmp(line, "\r\n") == 0)
         continue;

      if(count == 0 && strncmp(line, "{\"error\":\"", 10) == 0)
      {
         if(line[strlen(line)-1] == '\n')
            line[strlen(line)-1]  = '\0';

         ptr = line + 10;

         while(*ptr != '"' && *ptr != '\0')
            ++ptr;

         *ptr = '\0';

         strcpy(returnStruct->msg, line+10);
         return returnStruct;
      }
      else
      {
         fprintf(fout, "%s", line);
         fflush(fout);

         if(line[0] != '|'
         && line[0] != '\\')
            ++count;
      }
   }
      
   fclose(fout);

   returnStruct->status = 0;

   sprintf(returnStruct->msg,  "count=%d",       count);
   sprintf(returnStruct->json, "{\"count\":%d}", count);

   returnStruct->count = count;

   return returnStruct;
}


/***********************************************/
/* This is the basic "make a connection" stuff */
/***********************************************/

int mArchiveList_tcp_connect(char *hostname, int port)
{
   int                 sock_fd;
   struct hostent     *host;
   struct sockaddr_in  sin;


   if((host = gethostbyname(hostname)) == NULL) 
   {
      printf("[struct stat=\"ERROR\", msg=\"Couldn't find host %s\"]\n", hostname);
      fflush(stdout);
      return(0);
   }

   if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
   {
      sprintf(montage_msgstr, "Couldn't create socket()");
      return(0);
   }

   sin.sin_family = AF_INET;
   sin.sin_port = htons(port);
   bcopy(host->h_addr_list[0], &sin.sin_addr, host->h_length);

   if(connect(sock_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
   {
      sprintf(montage_msgstr, "%s: connect failed.", hostname);
      return(0);
   }

   return sock_fd;
}




/***************************************/
/* This routine reads a line at a time */
/* from a raw file descriptor          */
/***************************************/

int mArchiveList_readline (int fd, char *line) 
{
   int n, rc = 0;
   char c ;

   for (n = 1 ; n < MAXLEN ; n++)
   {
      if ((rc == read (fd, &c, 1)) != 1)
      {
         *line++ = c ;
         if (c == '\n')
            break ;
      }

      else if (rc == 0)
      {
         if (n == 1)
            return 0 ; /* EOF */
         else
            break ;    /* unexpected EOF */
      }
      else 
         return -1 ;
   }

   *line = 0 ;
   return n ;
}




/**************************************/
/* This routine URL-encodes a string  */
/**************************************/

static unsigned char hexchars[] = "0123456789ABCDEF";

char *mArchiveList_url_encode(char *s)
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


int mArchiveList_parseUrl(char *urlStr, char *hostStr, int *port) 
{
   char  *hostPtr;
   char  *portPtr;
   char  *dataref;
   char   save;

   if(strncmp(urlStr, "http://", 7) != 0)
   {
      sprintf(montage_msgstr, "Invalid URL string (must start 'http://')");
      return 1;
   }

   hostPtr = urlStr + 7;

   dataref = hostPtr;

   while(1)
   {
      if(*dataref == ':' || *dataref == '/' || *dataref == '\0')
         break;
      
      ++dataref;
   }

   save = *dataref;

   *dataref = '\0';

   strcpy(hostStr, hostPtr);

   *dataref = save;


   if(*dataref == ':')
   {
      portPtr = dataref+1;

      dataref = portPtr;

      while(1)
      {
         if(*dataref == '/' || *dataref == '\0')
            break;
         
         ++dataref;
      } 

      *dataref = '\0';

      *port = atoi(portPtr);

      *dataref = '/';

      if(*port <= 0)
      {
         sprintf(montage_msgstr, "Illegal port number in URL");
         return 1;
      }
   }

   return 0;
}
