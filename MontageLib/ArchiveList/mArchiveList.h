#ifndef MARCHIVELIST_H
#define MARCHIVELIST_H

/*******************************************/
/* Define mArchiveList function prototypes */
/*******************************************/

char  *mArchiveList_url_encode  (char *s);
int    mArchiveList_tcp_connect (char *hostname, int port);
int    mArchiveList_readline    (int fd, char *line);
int    mArchiveList_parseUrl    (char *urlStr, char *hostStr, int *port);

#endif
