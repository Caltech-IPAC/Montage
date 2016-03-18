/**
    \file       www.h
    \author     <a href="mailto:jcg@ipac.caltech.edu">John Good</a>
 */

/**
    \mainpage   libwww: WWW CGI Parameter Handling Library
    \htmlinclude docs/www.html
 */

#ifndef ISIS_WWW_LIB
#define ISIS_WWW_LIB

#include <stdio.h>

#define WWW_OK 0
#define WWW_BADFOUT 1
#define WWW_BADHEAD 2
#define WWW_BADFOOT 3

void keyword_debug(FILE *dbg);

void keyword_workdir(char *workdir);

int keyword_init(int argc, char **argv);

int keyword_count();

void keyword_close();

int keyword_exists(char const *key);

char *keyword_value(char const *key);

char *keyword_value_unsafe(char const *key);

char *keyword_value_stripped(char const *key);

char *keyword_instance(char const *key, int count);

char *keyword_instance_unsafe(char const *key, int count);

char *keyword_filename(char const *key);

int keyword_info(int index, char **keyname, char **keyval, char **fname);

int keyword_info_unsafe(int index, char **keyname, char **keyval, char **fname);

int keyword_safe_system(char const *str);

int initHTTP(FILE *fout, char const *cookiestr);

int wwwHeader(FILE *fout, char const *header, char const *title);

int keylib_initialized(void);

int wwwFooter(FILE *fout, char const *footer);

/* Utility Routines */

void unescape_url(char *url);

int is_blank(char const *s);

char *html_encode(char const *s);

char *url_encode(char const *s);

char *url_decode(char const *s);

void encodeOffsetURL(char *out, int start);


#endif /* ISIS_WWW_LIB */

