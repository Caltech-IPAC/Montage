#define NGRIDMAX 2;
#define NIMINFOMAX 10;
#define NSRCTBLMAX 20;


/*
#ifndef COLORTABLES 
#define COLORTABLES

int ncolortbl = 9;
static char colortbl[][30] = {
    "greyscale",
    "reversegreyscale",
    "thermal",
    "reversethermal",
    "logarithmicthermal",
    "velocity",
    "redramp",
    "greenramp",
    "blueramp"
};

#endif


#ifndef DEFAULTCOLORS 
#define DEFAULTCOLORS

int ndefaultcolor = 24;;
static char defaultcolor[][30] = {
    "black",
    "white",
    "grey",
    "lightgrey",
    "darkgrey",
    "green",
    "darkgreen",
    "lightgreen",
    "yellow",
    "lemonchiffon",
    "orange",
    "red",
    "deepred",
    "pink",
    "hotpink",
    "magenta",
    "magenta4",
    "purple",
    "cyan",
    "blue",
    "aliceblue",
    "cornflowerblue",
    "dodgerblue",
    "lightskyblue"
};
    
static char defaultHexcolor[][30] = {
    "#000000",
    "#FFFFFF",
    "#7F7F7F",
    "#D3D3D3",
    "#474747",
    "#00FF00",
    "#008800",
    "#09F911",
    "#FFFF00",
    "#FF7F00",
    "#FFFACD",
    "#FF0000",
    "#8b0000",
    "#FFC0CB",
    "#FF69B4",
    "#FF00FF",
    "#8B008B",
    "#7D26CD",
    "#00FFFF",
    "#0000FF",
    "#F0F8FF",
    "#6495ED",
    "#104E8B",
    "#87CEFA"
};


int ncolors_symbol = 18;
static char defaultSymbolcolor[][30] = {
	
    "darkgreen",
    "deepred",
    "cornflowerblue",
    "magenta",
    "cyan",
    "yellow",
    "hotpink",
    "green",
    "red",
    "aliceblue",
    "magenta4",
    "lightgreen",
    "orange",
    "pink",
    "blue",
    "lemonchiffon",
    "dodgerblue",
    "lightskyblue",
    "purple"
};

#endif
*/



#ifndef IMINFOREC 
#define IMINFOREC

struct IminfoRec 
{
    char     filename[1024];
    int      nrec;
  
    int      *recno;
    
    double   *cra;
    double   *cdec;
    
    double   *ra0;
    double   *dec0;
    double   *ra1;
    double   *dec1;
    double   *ra2;
    double   *dec2;
    double   *ra3;
    double   *dec3;

/*
    pixel coord on image
*/
    int      *ix0;      
    int      *iy0;      
    int      *ix1;      
    int      *iy1;      
    int      *ix2;      
    int      *iy2;      
    int      *ix3;      
    int      *iy3;      
};

#endif


#ifndef MVIEWER 
#define MVIEWER

struct Mviewer
{
    char    errmsg[1024];
    char    status[40];

    int     nkey;

/*
    System config parameters
*/
    char    workDir[1024];
    char    directory[1024];
    char    baseURL[1024];

/*
    Input parameters
*/
    char    cmd[1024];
    char    workspace[1024];
   
    char    jsonFile[1024];
    
    char    jsonOrig[30000];
    char    jsonStr[30000];

    int     nowcs;
   
    char    helphtml[1024];
    char    imname[20];
    char    imageType[20];
    char    imageFile[1024];
    
    char    objname[40];
    char    filter[40];
    char    pixscale[100];

/*
    imcube 
*/
    int     isimcube;
    int     nfitsplane;
    int     nplaneave;
    char    nplaneavestr[20];
    
    int     centerplane;
    int     startplane;
    int     endplane;
   
    char    ctype3[40];
   
    double  crval3;
    double  cdelt3;

    char    imcubepathOrig[1024];
    char    imcubefile[1024];
    char    imcubepath[1024];
    
    char    redcubefile[1024];
    char    redcubepath[1024];
    char    grncubefile[1024];
    char    grncubepath[1024];
    char    bluecubefile[1024];
    char    bluecubepath[1024];

/*
    imcubemode: ave or median 
*/
    char    imcubemode[20];

/*
    imcursormode: imzoom  or waveplot
*/
    char    imcursormode[20];


/*
    Original image dimension
*/
    int     imageWidth;
    int     imageHeight;
    int     cutoutWidth;
    int     cutoutHeight;
    
    char     canvasWidthStr[40];
    char     canvasHeightStr[40];
    char     refWidthStr[40];
    char     refHeightStr[40];

    int     canvasWidth;
    int     canvasHeight;
    int     refWidth;
    int     refHeight;

    char    grayFile[1024];
    char    colorTable[40];
    char    stretchMode[40];
    char    stretchMin[40];
    char    stretchMax[40];

    char    stretchMinval[40];
    char    stretchMaxval[40];
    char    stretchMinunit[40];
    char    stretchMaxunit[40];
    
    
    char    subsetimfile[1024];
    char    shrunkimfile[1024];
    char    shrunkRefimfile[1024];


    char    redFile[1024];
    char    redMode[40];
    char    redMin[40];
    char    redMax[40];

    char    greenFile[1024];
    char    greenMode[40];
    char    greenMin[40];
    char    greenMax[40];

    char    blueFile[1024];
    char    blueMode[40];
    char    blueMin[40];
    char    blueMax[40];

    char    subsetredfile[1024];
    char    shrunkredfile[1024];
    char    shrunkRefredfile[1024];

    char    subsetgrnfile[1024];
    char    shrunkgrnfile[1024];
    char    shrunkRefgrnfile[1024];

    char    subsetbluefile[1024];
    char    shrunkbluefile[1024];
    char    shrunkRefbluefile[1024];
    
    struct 
    {
/*
    overlay type: srctbl, iminfo, marker, label, and grid
*/
        char    type[40];
        char    coordSys[40]; 
        char    color[40]; 
        char    dataFile[1024]; 
        char    visible[40]; 

/*
    srctbl parameters
*/
        char    dataCol[40];
        char    dataType[40];
        char    dataRef[40]; 
        char    symType[40]; 
        char    symSize[40]; 
        char    symSide[40]; 
        char    location[200]; 
        char    text[200]; 
    }
    overlay[50];

    int    noverlay;
    int    nim;    
    int    nsrctbl;
    int    niminfo;    

/*
    Input pick paramters 
*/
    int     srcpick;

    double  xpix;
    double  ypix;
    int     ixpix;
    int     iypix;
    char    pickcsys[40];
    
/*
    subsetImage boundary on the original image file 
*/
    double     xmin;
    double     ymin;
    double     xmax;
    double     ymax;

/*
    Variables used in the program
*/
    char    imcsys[40];
    
    int     width;
    int     height;

    double  zoomfactorOrig;
    
    int     iscolor;

    int     ns_shrunk;
    int     nl_shrunk;

    char    jpgfile[1024];
    char    refjpgfile[1024];

/*
    subset imate dimension
*/
    int        ns;
    int        nl;
    double     ss;
    double     sl;

/*
    box on refimage
*/
    int     zoomxmin;
    int     zoomxmax;
    int     zoomymin;
    int     zoomymax;

/*
    param retrieved from mViewer return struct
*/
    char    xflipstr[40];
    char    yflipstr[40];
    int     xflip;
    int     yflip;
    
    char    zoomfactorstr[40];
    char    refzoomfactorstr[40];
    double  zoomfactor;
    double  refzoomfactor;

    char    datamin[40];
    char    datamax[40];
    char    bunit[40];

    char    minstr[40];
    char    maxstr[40];
    char    percminstr[40];
    char    percmaxstr[40];
    char    sigmaminstr[40];
    char    sigmamaxstr[40];

    char    reddatamin[40];
    char    reddatamax[40];
    char    redminstr[40];
    char    redmaxstr[40];
    char    redpercminstr[40];
    char    redpercmaxstr[40];
    char    redsigmaminstr[40];
    char    redsigmamaxstr[40];

    char    grndatamin[40];
    char    grndatamax[40];
    char    grnminstr[40];
    char    grnmaxstr[40];
    char    grnpercminstr[40];
    char    grnpercmaxstr[40];
    char    grnsigmaminstr[40];
    char    grnsigmamaxstr[40];

    char    bluedatamin[40];
    char    bluedatamax[40];
    char    blueminstr[40];
    char    bluemaxstr[40];
    char    bluepercminstr[40];
    char    bluepercmaxstr[40];
    char    bluesigmaminstr[40];
    char    bluesigmamaxstr[40];
    
/*
    srcpick to find table row matched with the picked source
*/
    char    mintbl[1024];
    int     minrecno;
    int     minix;
    int     miniy;
    double  minra;
    double  mindec;

/*
    niminfotbl: total iminfo tbl in the overlay layers
    nimfinarr:  number of iminfotbl picked by the "pick" command
    iminfoArr is initially malloc to be niminfotbl size but only niminfoarr
    of them have the internal arrays malloced.
*/
    int     niminfotbl;
    int     niminfoarr;
    
    struct IminfoRec **iminfoArr;

    char retstr[10000];


/*
    if third dimension of imcube is wavelength:
    waveplottype: pix or ave (of a dragged area)
*/
    char    waveplottype[20];
    char    showplot[20];
    char    detachplot[20];

    char    plottype[20];
    char    plotfile[1024];
    char    plotpath[1024];
    char    plotjsonfile[1024];
    char    plotjsonpath[1024];

    char    plottitle[100];
    char    plotxaxis[20];
    char    plotyaxis[20];
    
    char    plotxlabel[40];
    char    plotylabel[40];
    
    char    plotxlabeloffset[40];
    char    plotylabeloffset[40];
    
    char    plotbgcolor[40];
    
    char    plotsymbol[40];
    char    plotcolor[40];
    
    char    plotlinestyle[40];
    char    plotlinecolor[40];

    char    plothistvalue[40];

    int     plothiststyle;

    int     plotwidth;
    int     plotheight;

    double  xs;
    double  ys;
    double  xe;
    double  ye;

/*
    click result parameters
*/
    int    xpick;
    int    ypick;

    double  rapick;
    double  decpick;

    char    sexrapick[100];
    char    sexdecpick[100];
    
    double  pickval;

/*
    Returned pick values 
*/
    double  pickrval;
    double  pickgval;
    double  pickbval;
};

#endif
