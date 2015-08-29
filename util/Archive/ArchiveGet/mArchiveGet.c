/* Module: mArchiveGet.c

Version  Developer        Date     Change
-------  ---------------  -------  -----------------------
1.1      John Good        14Feb05  Got rid of IRSA-specific code
1.0      John Good        15Dec04  Baseline code

*/
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <errno.h>
#include <montage.h>

#define MAXLEN 16384

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int tcp_connect(char *hostname, int port);

int checkHdr(char *infile, int hdrflag, int hdu);

void parseUrl(char *urlStr, char *hostStr, int *port, char **dataref);

void archive_sigset();

void archive_sighandler(int sig);

char archive_msg[1024];

int debug;


/*************************************************************************/
/*                                                                       */
/*  mArchiveGet                                                          */
/*                                                                       */
/*  Montage is a set of general reprojection / coordinate-transform /    */
/*  mosaicking programs.  Any number of input images can be merged into  */
/*  an output FITS file.  The attributes of the input are read from the  */
/*  input files; the attributes of the output are read a combination of  */
/*  the command line and a FITS header template file.                    */
/*                                                                       */
/*  This module, mArchiveGet, retrieve a single FITS image from          */
/*  a remote archive.  The supported data sets are retrieved through a   */
/*  basic URL GET.                                                       */
/*                                                                       */
/*************************************************************************/

int main(int argc, char **argv)
{
   int    socket, ihead, pastHeader;
   int    i, c, nread, count, port, raw;

   unsigned int timeout;

   struct timeval timer;
 
   char  *dataref;

   char   buf     [MAXLEN];
   char   lead    [MAXLEN];
   char   head    [MAXLEN];
   char   request [MAXLEN];
   char   urlStr  [MAXLEN];
   char   hostStr [MAXLEN];
   char   cmd     [MAXLEN];
   char   fileName[MAXLEN];
 
   char  *proxy;
   char   phostStr[MAXLEN];
   int    pport;
   char  *pdataref;
 
   int    fd;

   fd_set fdset;
 
   FILE  *fdebug;

   fdebug  = stdout;
   fstatus = stdout;

   strcpy(hostStr, "vaoweb3.ipac.caltech.edu");
   strcpy(archive_msg, "");

   archive_sigset();

   debug   =   0;
   opterr  =   0;
   port    =  80;
   raw     =   0;
   timeout = 300;
 
   while ((c = getopt(argc, argv, "drt:")) != EOF)
   {
      switch (c)
      {
         case 'd':
            debug = 1;
            break;

         case 'r':
            raw = 1;
            break;

         case 't':
            timeout = atoi(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage:  %s [-d][-r] remoteref localfile\"]\n",argv[0]);
            exit(0);
            break;
      }
   }

   if(argc - optind < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage:  %s [-d][-r] remoteref localfile\"]\n",argv[0]);
      exit(0);
   }


   /* Try to open the output file */
 
   if(debug)
   {
      fprintf(fdebug, "DEBUG> localfile = [%s]\n", argv[optind+1]);
      fflush(fdebug);
   }

   strcpy(fileName, argv[optind+1]);

   fd = open(fileName, O_WRONLY | O_CREAT, 0644);
 
   if(fd < 0)
   {
      fprintf(fdebug, "[struct stat=\"ERROR\", msg=\"Output file(%s) open failed\"]\n", 
         argv[optind+1]);
      exit(0);
   }
  
   /* Parse the reference string to get host and port info */

   strcpy(urlStr, argv[optind]);

   if(debug)
   {
      fprintf(fdebug, "DEBUG> urlStr  = [%s]\n", urlStr);
      fflush(fdebug);
   }

   parseUrl(urlStr, hostStr, &port, &dataref);

   if(*dataref == '\0')
   {
      printf("[struct stat=\"ERROR\", msg=\"No data reference given in URL\"]\n");
      exit(0);
   }

   proxy = getenv("http_proxy");
   
   if(proxy)
     parseUrl(proxy, phostStr, &pport, &pdataref);

   /* Connect to the port on the host we want */

   if(debug)
   {
      fprintf(fdebug, "DEBUG> hostStr = [%s]\n", hostStr);
      fprintf(fdebug, "DEBUG> port    =  %d\n",  port);
      fflush(fdebug);
   }
 
   strcpy(archive_msg, "Timeout connecting to remote host.");
   alarm(timeout);

   if(proxy) {
     socket = tcp_connect(phostStr, pport);
   } else {
     socket = tcp_connect(hostStr, port);
   }
 
   /* Send a request for the file we want */
 
   if(raw) {
     if(proxy)
       sprintf(request, "GET %s/%s\r\n\r\n",
               hostStr, dataref);
     else 
       sprintf(request, "GET %s\r\n\r\n",
               dataref);       
   } else {
     if(proxy)
       sprintf(request, "GET %s HTTP/1.0\r\n\r\n",
               urlStr);
     else 
       sprintf(request, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",
               dataref, hostStr);
   }
 
   if(debug)
   {
      fprintf(fdebug, "DEBUG> request = [%s]\n", request);
      fflush(fdebug);
   }

   send(socket, request, strlen(request), 0);
 

   /* Read the data coming back */
 
   count = 0;
   ihead = 0;

   pastHeader = 0;
 
   strcpy(archive_msg, "Data retrieval remote read timeout.");

   while(1)
   { 
      alarm(timeout);

      nread = read(socket, buf, MAXLEN);
 
      if(nread <= 0)
         break;

      if(debug)
      {
         fprintf(fdebug, "DEBUG> read %d bytes\n", nread);
         fflush(fdebug);
      }
      
      if(!pastHeader && ihead == 0 && strncmp(buf, "H", 1) != 0)
      {
         if(debug)
         {
            fprintf(fdebug, "DEBUG> No HTTP header on this one.\n");
            fflush(fdebug);

            for(i=0; i<40; ++i)
              lead[i] = buf[i];

            lead[40] = '\0';
            fprintf(fdebug, "DEBUG> Starts with: [%s]... \n", lead);
            fflush(fdebug);
         }

         pastHeader = 1;
      }

      if(!pastHeader)
      {
         for(i=0; i<nread; ++i)
         {
            head[ihead] = buf[i];
            ++ihead;
         }

         head[ihead] = '\0';

         if(debug)
         {
            fprintf(fdebug, "DEBUG> Header ->\n%s\nDEBUG> Length = %d\n",
               head, ihead);
            fflush(fdebug);
         }

         for(i=0; i<ihead-3; ++i)
         {
            if(strncmp(head+i, "\r\n\r\n", 4) == 0 && ihead-i-4 > 0)
            {
               if(debug)
               {
                  fprintf(fdebug, "DEBUG> End of header found: %d - %d\n",
                     i, i+3);

                  fprintf(fdebug, "DEBUG> Writing %d from header array\n",
                     ihead-i-4);

                  fflush(fdebug);
               }

               write(fd, head+i+4, ihead-i-4);

               pastHeader = 1;

               break;
            }
         }
      }
      else
      {
         count += nread;

         if(debug)
         {
            fprintf(fdebug, "DEBUG> Writing %d\n", nread);
            fflush(fdebug);
         }

        
         write(fd, buf, nread);
      }
   }

   close(fd);

   alarm(0);


   /* Unzip the file if necesary */

   if(strlen(fileName) > 4 && strcmp(fileName+strlen(fileName)-4, ".bz2") == 0)
   {
      sprintf(cmd, "bunzip2 %s", fileName);
      system(cmd);

      *(fileName+strlen(fileName)-4) = '\0';
   }


   checkHdr(fileName, 0, 0);
 
   printf("[struct stat=\"OK\", count=\"%d\"]\n", count);
   fflush(fdebug);
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
      fprintf(stderr, "Couldn't find host %s\n", hostname);
      return(0);
   }

   if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
   {
      fprintf(stderr, "Couldn't create socket()\n");
      return(0);
   }

   sin.sin_family = AF_INET;
   sin.sin_port = htons(port);
   bcopy(host->h_addr, &sin.sin_addr, host->h_length);

   if(connect(sock_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
   {
      fprintf(stderr, "%s: connect failed.\n", hostname);
      return(0);
   }

   return sock_fd;
}


void parseUrl(char *urlStr, char *hostStr, int *port, char **dataref) {
  
   char  *hostPtr;
   char  *portPtr;
   char   save;

   if(strncmp(urlStr, "http://", 7) != 0)
   {
      printf("[struct stat=\"ERROR\", msg=\"Invalid URL string (must start 'http://')\n"); 
      exit(0);
   }

   hostPtr = urlStr + 7;

   *dataref = hostPtr;

   while(1)
   {
      if(**dataref == ':' || **dataref == '/' || **dataref == '\0')
         break;
      
      ++*dataref;
   }

   save = **dataref;

   **dataref = '\0';

   strcpy(hostStr, hostPtr);

   **dataref = save;


   if(**dataref == ':')
   {
      portPtr = *dataref+1;

      *dataref = portPtr;

      while(1)
      {
         if(**dataref == '/' || **dataref == '\0')
            break;
         
         ++*dataref;
      } 

      **dataref = '\0';

      *port = atoi(portPtr);

      **dataref = '/';

      if(*port <= 0)
      {
         printf("[struct stat=\"ERROR\", msg=\"Illegal port number in URL\"]\n");
        exit(0);
      }
   }
}



/*********************************************/
/* Set up signal catching so child processes */
/* can be shut down gracefully               */
/*********************************************/

void archive_sigset()
{
   signal(SIGHUP,     archive_sighandler);
   signal(SIGINT,     archive_sighandler);
   signal(SIGQUIT,    archive_sighandler);
   signal(SIGILL,     archive_sighandler);
   signal(SIGTRAP,    archive_sighandler);
   signal(SIGABRT,    archive_sighandler);
   signal(SIGFPE,     archive_sighandler);
   signal(SIGBUS,     archive_sighandler);
   signal(SIGSEGV,    archive_sighandler);
   signal(SIGSYS,     archive_sighandler);
   signal(SIGPIPE,    archive_sighandler);
   signal(SIGALRM,    archive_sighandler);
   signal(SIGTERM,    archive_sighandler);
   signal(SIGUSR1,    archive_sighandler);
   signal(SIGUSR2,    archive_sighandler);
   signal(SIGWINCH,   archive_sighandler);
   signal(SIGURG,     archive_sighandler);
   signal(SIGTSTP,    archive_sighandler);
   signal(SIGCONT,    archive_sighandler);
   signal(SIGTTIN,    archive_sighandler);
   signal(SIGTTOU,    archive_sighandler);
   signal(SIGVTALRM,  archive_sighandler);
   signal(SIGPROF,    archive_sighandler);
   signal(SIGXCPU,    archive_sighandler);
   signal(SIGXFSZ,    archive_sighandler);
}



/*******************/
/* Process signals */
/*******************/

void archive_sighandler(int sig)
{
   char msg[1024];

   if(sig == SIGHUP
   || sig == SIGINT
   || sig == SIGQUIT
   || sig == SIGILL
   || sig == SIGTRAP
   || sig == SIGABRT
   || sig == SIGFPE
   || sig == SIGBUS
   || sig == SIGSEGV
   || sig == SIGSYS
   || sig == SIGPIPE
   || sig == SIGALRM
   || sig == SIGTERM
   || sig == SIGUSR1
   || sig == SIGUSR2
   || sig == SIGTSTP
   || sig == SIGTTIN
   || sig == SIGTTOU
   || sig == SIGVTALRM
   || sig == SIGPROF
   || sig == SIGXCPU)
   {
      signal(SIGHUP,     SIG_IGN);
      signal(SIGINT,     SIG_IGN);
      signal(SIGQUIT,    SIG_IGN);
      signal(SIGILL,     SIG_IGN);
      signal(SIGTRAP,    SIG_IGN);
      signal(SIGABRT,    SIG_IGN);
      signal(SIGFPE,     SIG_IGN);
      signal(SIGBUS,     SIG_IGN);
      signal(SIGSEGV,    SIG_IGN);
      signal(SIGSYS,     SIG_IGN);
      signal(SIGPIPE,    SIG_IGN);
      signal(SIGALRM,    SIG_IGN);
      signal(SIGTERM,    SIG_IGN);
      signal(SIGUSR1,    SIG_IGN);
      signal(SIGUSR2,    SIG_IGN);
      signal(SIGTSTP,    SIG_IGN);
      signal(SIGTTIN,    SIG_IGN);
      signal(SIGTTOU,    SIG_IGN);
      signal(SIGVTALRM,  SIG_IGN);
      signal(SIGPROF,    SIG_IGN);
      signal(SIGXCPU,    SIG_IGN);

           if(sig == SIGHUP   ) strcpy(msg, "SIGHUP:     Hangup (see termio(7I))");
      else if(sig == SIGINT   ) strcpy(msg, "SIGINT:     Interrupt (see termio(7I))");
      else if(sig == SIGQUIT  ) strcpy(msg, "SIGQUIT:    Quit (see termio(7I))");
      else if(sig == SIGILL   ) strcpy(msg, "SIGILL:     Illegal Instruction");
      else if(sig == SIGTRAP  ) strcpy(msg, "SIGTRAP:    Trace/Breakpoint Trap");
      else if(sig == SIGABRT  ) strcpy(msg, "SIGABRT:    Abort");
      else if(sig == SIGFPE   ) strcpy(msg, "SIGFPE:     Arithmetic Exception");
      else if(sig == SIGBUS   ) strcpy(msg, "SIGBUS:     Bus Error");
      else if(sig == SIGSEGV  ) strcpy(msg, "SIGSEGV:    Segmentation Fault");
      else if(sig == SIGSYS   ) strcpy(msg, "SIGSYS:     Bad System Call");
      else if(sig == SIGPIPE  ) strcpy(msg, "SIGPIPE:    Broken Pipe");
      else if(sig == SIGALRM  ) strcpy(msg,  archive_msg);
      else if(sig == SIGTERM  ) strcpy(msg, "SIGTERM:    Terminated");
      else if(sig == SIGUSR1  ) strcpy(msg, "SIGUSR1:    User Signal 1");
      else if(sig == SIGUSR2  ) strcpy(msg, "SIGUSR2:    User Signal 2");
      else if(sig == SIGTSTP  ) strcpy(msg, "SIGTSTP:    Stopped (user)");
      else if(sig == SIGCONT  ) strcpy(msg, "SIGCONT:    Continued");
      else if(sig == SIGTTIN  ) strcpy(msg, "SIGTTIN:    Stopped (tty input)");
      else if(sig == SIGTTOU  ) strcpy(msg, "SIGTTOU:    Stopped (tty output)");
      else if(sig == SIGVTALRM) strcpy(msg, "SIGVTALRM:  Virtual Timer Expired");
      else if(sig == SIGPROF  ) strcpy(msg, "SIGPROF:    Profiling Timer Expired");
      else if(sig == SIGXCPU  ) strcpy(msg, "SIGXCPU:    CPU time limit exceeded");

      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
      fflush(stdout);

      sleep(5);

      exit(0);
   }
}
