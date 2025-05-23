#ifndef ISIS_COORD_LIB

struct COORD                /* Definition of coordinate structure            */
{                           /*                                               */ 
  char sys[3];              /* Coordinate system                             */
  char clon[25], clat[25];  /* Coordinates (when expressed as char string)   */
  double lon, lat;          /* Coordinates (when expressed as a real number) */
  char fmt[6];              /* Units                                         */
  char epoch[10];           /* Epoch type and year                           */
};


/* "sys" can be one of the following:                                        */
/*                                                                           */
/*    "EQ"   -   Equatorial                                                  */
/*    "GA"   -	 Galactic                                                    */
/*    "EC"   -	 Ecliptic                                                    */
/*    "SG"   -	 Supergalactic                                               */
/*                                                                           */
/* "fmt" can be any of the following:                                        */
/*                                                                           */
/*    "DD" or "DDR"     -  Decimal Degrees (expressed as a real number)      */
/*    "DDC"             -  Decimal Degrees (expressed as a char string)      */
/*    "SEXR"            -  Sexigesimal (expressed as a real number)          */
/*    "SEX" or "SEXC"   -  Sexigesimal (expressed as a char string)          */
/*    "RAD" or "RADR"   -  Radians (expressed as a real number)              */
/*    "RADC"            -  Radians (expressed as a char string)              */
/*    "MRAD" or "MRADR" -  Milliradians (expressed as a real number)         */
/*    "MRADC"           -  Milliradians	(expressed as a char string)         */
/*    "AS" or "ASR"     -  Arc-seconds (expressed as a real number)          */
/*    "ASC"             -  Arc-seconds (expressed as a char string)          */
/*    "MAS" or "MASR"   -  Milliarcseconds (expressed as a real number)      */
/*    "MASC"            -  Milliarcseconds (expressed as a char string)      */
/*                                                                           */
/* "epoch" must start with the characters "B" or "J" followed by a           */
/*    four-digit year (e.g. "J2000", "B1950").                               */



typedef enum {
	      DD = 0,      /* 0 */
	      SEX   ,      /* 1 */
	      RAD   ,      /* 2 */
	      MRAD  ,      /* 3 */
	      AS    ,      /* 4 */
	      MAS          /* 5 */
	     }
	      CoordUnit;


typedef enum {A = 0 ,      /* 0 */
	      T     ,      /* 1 */
	      H     ,      /* 2 */
	      M            /* 3 */
	     }
	      ArcPrec;





/* ERROR codes returned by ccalc()                                           */
/*                                                                           */
#define ERR_CCALC_INVETYPE  -1 /* Invalid epoch type was specified           */
#define ERR_CCALC_INVEYEAR  -2 /* Invalid epoch year was specified           */
#define ERR_CCALC_INVSYS    -3 /* Invalid coordinate system was specified    */
#define ERR_CCALC_INVDOUBL  -4 /* Couldn't convert a value to double         */
#define ERR_CCALC_SEXCONV   -5 /* Sexigesimal conversion failed              */
#define ERR_CCALC_SEXERR    -6 /* Internal error with sexigesimal conversion */
#define ERR_CCALC_INVFMT    -7 /* Invalid format was specified               */
#define ERR_CCALC_INVPREC   -8 /* Invalid precision was specified            */
#define ERR_CCALC_INVCOORD  -9 /* Invalid coordinate value was specified     */



/* Coordinate system codes           */
/* (used in transformation routines) */

#define EQUJ      0
#define EQUB      1
#define ECLJ      2
#define ECLB      3
#define GAL       4
#define SGAL      5

#define JULIAN    0
#define BESSELIAN 1



/* Prototypes of callable functions */

void
convertCoordinates(int  insys, double  inepoch, double   inlon, double   inlat,
                   int outsys, double outepoch, double *outlon, double *outlat,
		   double obstime);
void convertEclToEqu(double elon, double elat, double *ra, double *dec, 
		     double date, int besselian);
void convertEquToEcl(double ra, double dec, double *elon, double *elat, 
		     double date, int besselian);
void convertEquToGal(double ra, double dec, double *glon, double *glat);
void convertGalToEqu(double glon, double glat, double *ra, double *dec);
void convertGalToSgal(double glon, double glat, double *sglon, double *sglat);
void convertSgalToGal(double sglon, double sglat, double *glon, double *glat);

void convertBesselianToJulian(double equinoxin, double ra, double dec, 
                              double obsdatein, int ieflg, 
                              double *raout, double *decout);
void convertJulianToBesselian(double ra, double dec, 
                              double obsdatein, int ieflg, double equinoxout, 
                              double *raout, double *decout);
void precessBesselian(double epochin,  double  rain,  double  decin, 
                      double epochout, double *raout, double *decout);
void precessBesselianWithProperMotion
   (double epochin,  double  rain,  double  decin, 
    double epochout, double *raout, double *decout, 
    double pmain, double pmdin, double pin, double vin, 
    double *rapm, double *decpm);
void precessJulian(double epochin,  double  rain,  double  decin, 
                   double epochout, double *raout, double *decout);
void precessJulianWithProperMotion
   (double epochin,  double  rain,  double  decin, 
    double epochout, double *raout, double *decout, 
    double pmain, double pmdin, double pin, double vin, 
    double *rapm, double *decpm);
void
julianToBesselianFKCorrection(double ra, double dec, double xmag, double tobs, 
                              double *corra, double *corrd, double *corrpa, 
                              double *corrpd);
void
besselianToJulianFKCorrection(double ain, double d, double dmag, double epoch, 
                              double *corra, double *corrd, double *corrpa, 
                              double *corrpd);

int ccalc(struct COORD *from, struct COORD *to, char *longprec, char *latprec);
int degreeToDMS(double deg, int prec, int *neg, int *d, int *m, double *s);
int degreeToHMS(double deg, int prec, int *neg, int *h, int *m, double *s);
int degreeToSex(double lon, double lat, char *lonstr, char *latstr);
int sexToDegree(char *cra, char *cdec, double *ra, double *dec);
int parseCoordinateString(char *cmd, char *lonstr, char *latstr, 
	                  char *csys, char *cfmt, char *epoch);

double roundValue(double value, int precision);


#define ISIS_COORD_LIB
#endif
