/* Module: mHdr.c


Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        21Jul06  Baseline code

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
#include <errno.h>

#include <mHdr.h>
#include <montage.h>

#define MAXLEN 20000


static char montage_msgstr[1024];


/*-***********************************************************************/
/*                                                                       */
/* mHdr -- Create a header template file from location, size, resolution */
/* and rotation inputs. In order to support object name lookup for the   */
/* location string argument, this service must have a network connection */
/* available.                                                            */
/*                                                                       */
/*   char  *locstr         A (quoted if necessary) string containing     */
/*                         a coordinate or the name of an object on      */
/*                         the sky                                       */
/*                                                                       */
/*   double width          Image width in degrees                        */
/*   double height         Image height in degrees                       */
/*                                                                       */
/*   char  *csys           Coordinate system string (e.g. 'EquJ',        */
/*                         'Galactic', etc. Fairly forgiving.            */
/*                                                                       */
/*   double equinox        Coordinate system equinox (e.g. 2000.0)       */
/*   double resolution     Image pixel resolution (in arcsec)            */
/*   double rotation       Image rotation on the sky                     */
/*                                                                       */
/*   char  *band2MASS      Optional argument when mosaicking 2MASS, adds */
/*                         proper MAGZP value                            */
/*                                                                       */
/*   char  *outfile        Output FITS header file                       */
/*                                                                       */
/*   int    debug          Debugging output level                        */
/*                                                                       */
/*************************************************************************/

struct mHdrReturn *mHdr(char *locstr, double width, double height, char *csys, double equinox, 
                        double resolution, double rotation, char *band2MASS, char *outfile, int debug)
{
   int    sock, port, count;
  
   char   line      [MAXLEN];
   char   request   [MAXLEN];
   char   base      [MAXLEN];
   char   constraint[MAXLEN];
   char   server    [MAXLEN];

   char  *locstr_encode;
   char  *csys_encode;

   FILE  *fout;

   char  *proxy;
   char   pserver   [MAXLEN];
   int    pport;

   struct mHdrReturn *returnStruct;


   /*******************************/
   /* Initialize return structure */
   /*******************************/

   returnStruct = (struct mHdrReturn *)malloc(sizeof(struct mHdrReturn));

   bzero((void *)returnStruct, sizeof(returnStruct));


   returnStruct->status = 1;

   strcpy(returnStruct->msg, "");


   /* Process command-line parameters */

   strcpy(server, "montage.ipac.caltech.edu");

   port = 80;

   strcpy(base, "/cgi-bin/HdrTemplate/nph-hdr?");

   locstr_encode = mHdr_url_encode(locstr);
   csys_encode   = mHdr_url_encode(csys);

   sprintf(constraint, "location=%s&width=%.10f&height=%.10f&system=%s&equinox=%.2f&resolution=%.12f&rotation=%.6f&band=%s",
      locstr_encode, width, height, csys_encode, equinox, resolution, rotation, band2MASS);

   free(locstr_encode);
   free(csys_encode);


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
      if(mHdr_parseUrl(proxy, pserver, &pport) > 0)
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

      sock = mHdr_tcp_connect(pserver, pport);
   } 
   else 
   {
      sock = mHdr_tcp_connect(server, port);
   }

   if(sock == 0)
   {
      strcpy(returnStruct->msg, "Cannot open network socket to header service.");
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

   send(sock, request, strlen(request), 0);


   /* And read all the lines coming back */

   count = 0;

   while(1)
   {
      /* Read lines returning from service */

      if(mHdr_readline (sock, line) == 0)
         break;

      if(debug)
      {
         printf("DEBUG> return; [%s]\n", line);
         fflush(stdout);
      }

      if(strncmp(line, "ERROR: ", 7) == 0)
      {
         if(line[strlen(line)-1] == '\n')
            line[strlen(line)-1]  = '\0';

         sprintf(returnStruct->msg, "%s", line+7);
         return returnStruct;
      }
      else
      {
         fprintf(fout, "%s", line);
         fflush(fout);

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

int mHdr_tcp_connect(char *hostname, int port)
{
   int                 sock_fd;
   struct hostent     *host;
   struct sockaddr_in  sin;


   if((host = gethostbyname(hostname)) == NULL) 
   {
      sprintf(montage_msgstr, "Couldn't find host %s", hostname);
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

int mHdr_readline (int fd, char *line) 
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

char *mHdr_url_encode(char *s)
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


int mHdr_parseUrl(char *urlStr, char *hostStr, int *port) 
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
