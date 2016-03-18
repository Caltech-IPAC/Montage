/* Module: mCatSearch.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.0      John Good        15Nov15  Baseline code

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
#include <time.h>
#include <math.h>

#include <svc.h>

#define STRLEN 1024

int  tcp_connect(char *hostname, int port);
int  readline   (int fd, char *line) ;
void parseUrl   (char *urlStr, char *hostStr, int *port);
void printError (char *msg);

int  debug;


/*************************************************************************/
/*                                                                       */
/*  mCatSearch                                                           */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mCatSearch, can be used to retrieve a set of sources    */
/*  from a remote catalog, suitable for overlays or for image            */
/*  calibration.                                                         */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int  i, socket, port, pport, count;

   char base        [STRLEN];
   char server      [STRLEN];
   char pserver     [STRLEN];
   char constraint  [STRLEN];
   char request     [STRLEN];
   char line        [STRLEN];

   char input_file  [STRLEN];
   char output_file [STRLEN];

   char cmd         [STRLEN];
   char status      [STRLEN];

   char ra1         [STRLEN];
   char dec1        [STRLEN];
   char ra2         [STRLEN];
   char dec2        [STRLEN];
   char ra3         [STRLEN];
   char dec3        [STRLEN];
   char ra4         [STRLEN];
   char dec4        [STRLEN];

   char  *proxy;

   FILE  *fout;


   /***************************************/
   /* Process the command-line parameters */
   /***************************************/

   debug = 0;

   if (argc < 3) 
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: mCatSearch [-d] in.fits out.tbl\"]\n");
      exit(1);
   }

   if(strcmp(argv[1], "-d") == 0)
   {
      debug = 1;
      ++argv;
      --argc;
   }

   if (argc < 3) 
   {
      printf ("[struct stat=\"ERROR\", msg=\"Usage: mCatSearch [-d] in.fits out.tbl\"]\n");
      exit(1);
   }

   strcpy(input_file,  argv[1]);
   strcpy(output_file, argv[2]);

   if(debug)
   {
      printf("input_file       = [%s]\n", input_file);
      printf("output_file      = [%s]\n", output_file);

      svc_debug(stdout);
   }


   /***************************************/
   /* Get the corners for the input image */
   /***************************************/

   sprintf(cmd, "mExamine %s", input_file);
   svc_run(cmd);

   strcpy(status, svc_value("stat"));

   if(strcmp(status, "OK") != 0)
      printError(svc_value("msg"));

   strcpy(ra1,  svc_value("ra1"));
   strcpy(dec1, svc_value("dec1"));
   strcpy(ra2,  svc_value("ra2"));
   strcpy(dec2, svc_value("dec2"));
   strcpy(ra3,  svc_value("ra3"));
   strcpy(dec3, svc_value("dec3"));
   strcpy(ra4,  svc_value("ra4"));
   strcpy(dec4, svc_value("dec4"));


   /************************/
   /* Run a catalog search */
   /************************/

   strcpy(server, "irsa.ipac.caltech.edu");

   port = 80;

   strcpy(base, "/cgi-bin/Gator/nph-query?");

   sprintf(constraint, "catalog=usno_b1&selcols=usno_b1,ra,dec,b1_mag,b2_mag,r1_mag,r2_mag,i_mag&spatial=polygon&polygon=%s+%s,+%s+%s,+%s+%s,+%s+%s&order=b1_mag&outfmt=1\" %s", ra1, dec1, ra2, dec2, ra3, dec3, ra4, dec4, output_file);

   fout = fopen(output_file, "w+");

   if(fout == (FILE *)NULL)
   {
      printf("[struct stat=\"ERROR\", msg=\"Can't open output file %s\"]\n",
         argv[6]);
      exit(0);
   }


   /* Connect to the port on the host we want */
   proxy = getenv("http_proxy");

   if(proxy) 
   {
      parseUrl(proxy, pserver, &pport);

      if(debug)
      {
         printf("DEBUG> proxy = [%s]\n", proxy);
         printf("DEBUG> pserver = [%s]\n", pserver);
         printf("DEBUG> pport = [%d]\n", pport);
         fflush(stdout);
      }

      socket = tcp_connect(pserver, pport);
   }
   else
      socket = tcp_connect(server, port);


   /* Send a request for the file we want */

   if(proxy) 
     sprintf(request, "GET http://%s:%d%s%s HTTP/1.0\r\n\r\n",
             server, port, base, constraint);
   else
    sprintf(request, "GET %s%s HTTP/1.0\r\nHOST: %s:%d\r\n\r\n",
            base, constraint, server, port);

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

      if(readline (socket, line) == 0)
         break;

      if(debug)
      {
         printf("DEBUG> return; [%s]\n", line);
         fflush(stdout);
      }

      if(strstr(line, "\r\n") != 0)
         continue;
      
      fprintf(fout, "%s", line);
      fflush(fout);

      if(line[0] != '|' && line[0] != '\\')
         ++count;
   }

   fclose(fout);


   printf("[struct stat=\"OK\", count=%d]\n", count);
   fflush(stdout);

   exit(0);
}


/***********************************************/
/* This is the basic "make a connection" stuff */
/***********************************************/

int tcp_connect(char *hostname, int port)
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
      printf("[struct stat=\"ERROR\", msg=\"Couldn't create socket()\"]\n");
      fflush(stdout);
      return(0);
   }

   sin.sin_family = AF_INET;
   sin.sin_port = htons(port);
   bcopy(host->h_addr_list[0], &sin.sin_addr, host->h_length);

   if(connect(sock_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s: connect failed.\"]\n", hostname);
      fflush(stdout);
      return(0);
   }

   return sock_fd;
}


/***************************************/
/* This routine reads a line at a time */
/* from a raw file descriptor          */
/***************************************/

int readline (int fd, char *line)
{
   int n, rc = 0;
   char c ;

   for (n = 1 ; n < STRLEN ; n++)
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


/******************************/
/*                            */
/*  Parse the URL             */
/*                            */
/******************************/

void parseUrl(char *urlStr, char *hostStr, int *port) {

   char  *hostPtr;
   char  *portPtr;
   char  *dataref;
   char   save;

   if(strncmp(urlStr, "http://", 7) != 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid URL string (must start 'http://')\n");
      exit(0);
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
         printf("[struct stat=\"ERROR\", msg=\"Illegal port number in URL\"]\n");
         exit(0);
      }
   }
}


/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void printError(char *msg)
{
   fprintf(stderr, "[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
   exit(1);
}
