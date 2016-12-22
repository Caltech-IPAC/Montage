#ifndef MHDR_H
#define MHDR_H

/***********************************/
/* Define mHdr function prototypes */
/***********************************/

char  *mHdr_url_encode  (char *s);
int    mHdr_tcp_connect (char *hostname, int port);
int    mHdr_readline    (int fd, char *line);
int    mHdr_parseUrl    (char *urlStr, char *hostStr, int *port);

#endif
