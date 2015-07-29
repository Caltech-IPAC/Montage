#ifndef FITSHDR
#define FITSHDR

struct FitsHdr
{
    char    errmsg[1024];

    char    datatype[10];
    char    bunit[20];

    char    ra[40];
    char    dec[40];

    int     nl;
    int     ns;
    int     nplane;
    int     naxis;
    int     naxes[3];
    int     axisIndx[3];

    int     bitpix;

    double  bscale;
    double  bzero;
    double  blank;
    
    int     bscaleExist;
    int     bzeroExist;
    int     blankExist;


    char    cunit[3][40];
    char    ctype[3][20];
    double  crval[3];
    double  crpix[3];
    double  cdelt[3];
    double  crota[3];
    double  cd[2][3];
    double  pc[2][3];
    
    char    csysstr[40];
    char    epochstr[40];
    char    equinoxstr[40];

    char    objname[100];
    char    filter[100];
    char    pixscale[100];

    int     sys;

    double  epoch;
    double  equinox;

    int     haveCunit[3];
    int     haveCtype[3];
    int     haveCrpix[3];
    int     haveCrval[3];
    int     haveCdelt[3];
    int     haveCd[3][3];
    int     havePc[3][3];
    int     haveCrota[3];
    int     haveEpoch;
    int     haveEquinox;

    int     nowcs;
};

#endif
