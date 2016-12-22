#ifndef MIMGTBL_H
#define MIMGTBL_H

struct Hdr_rec
{
   int       cntr;
   char      fname[1024];
   int       hdu;
   off_t     size;
   char      ctype1[10];
   char      ctype2[10];
   int       ns;
   int       nl;
   float     crpix1;
   float     crpix2;
   double    crval1;
   double    crval2;
   double    cdelt1;
   double    cdelt2;
   double    crota2;
   double    ra2000;
   double    dec2000;
   double    ra1, dec1;
   double    ra2, dec2;
   double    ra3, dec3;
   double    ra4, dec4;
   double    radius;

   double    equinox;
};


/**************************************/
/* Define mImgtbl function prototypes */
/**************************************/

int  mImgtbl_get_files   (char*);
int  mImgtbl_get_list    (char*, int);

int  mImgtbl_get_hdr     (char*, struct Hdr_rec*, char*);
void mImgtbl_print_rec   (struct Hdr_rec*);

int  mImgtbl_update_table(char *tblname);


#endif
