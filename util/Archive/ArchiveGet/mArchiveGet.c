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
#include <mcurl.h>
#include <montage.h>

#define MAXLEN 16384

extern char *optarg;
extern int optind, opterr;

extern int getopt(int argc, char *const *argv, const char *options);

int checkHdr(char *infile, int hdrflag, int hdu);

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
   unsigned int timeout;

   int    c, status;
   char   urlStr  [MAXLEN];
   char   fileName[MAXLEN];
   char   msg     [MAXLEN];
   char   cmd     [MAXLEN];

   double size;
 
   FILE  *fdebug;

   fdebug = stdout;

   strcpy(archive_msg, "");

   archive_sigset();

   debug   =   0;
   opterr  =   0;
   timeout = 300;
 
   while ((c = getopt(argc, argv, "dt:")) != EOF)
   {
      switch (c)
      {
         case 'd':
            debug = 1;
            break;

         case 't':
            timeout = atoi(optarg);
            break;

         default:
            printf("[struct stat=\"ERROR\", msg=\"Usage:  %s [-d][-t timeout] remoteref localfile\"]\n",argv[0]);
            exit(0);
            break;
      }
   }

   if(argc - optind < 2)
   {
      printf("[struct stat=\"ERROR\", msg=\"Usage:  %s [-d][-t timeout] remoteref localfile\"]\n",argv[0]);
      exit(0);
   }


   /* Get the URL */

   strcpy(urlStr, argv[optind]);

   if(debug)
   {
      fprintf(fdebug, "DEBUG> urlStr  = [%s]\n", urlStr);
      fflush(fdebug);
   }


   /* Try to open the output file */

   strcpy(fileName, argv[optind+1]);
 
   if(debug)
   {
      fprintf(fdebug, "DEBUG> fileName = [%s]\n", fileName);
      fflush(fdebug);
   }

  
   /* Use cURL library to retrieve file */

   status = mcurl(urlStr, fileName, timeout, &size, msg);

   if(status)
   {
      printf("[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
      fflush(stdout);
      exit(0);
   }


   /* Unzip the file if necesary */

   if(strlen(fileName) > 4 && strcmp(fileName+strlen(fileName)-4, ".bz2") == 0)
   {
      sprintf(cmd, "bunzip2 %s", fileName);

      *(fileName+strlen(fileName)-4) = '\0';

      unlink(fileName);

      system(cmd);
   }

   // checkHdr(fileName, 0, 0);
 
   printf("[struct stat=\"OK\", count=\"%-g\"]\n", size);
   fflush(fdebug);
   exit(0);
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
