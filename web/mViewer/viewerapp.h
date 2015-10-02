#ifndef VIEWERAPP 
#define VIEWERAPP

#define NGRIDMAX 2;
#define NTBLMAX 20;
#define NIMINFOMAX 4;
#define NSRCTBLMAX 20;
#define NLABELLMAX 100;

/*
int ndefaultcolor;

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
*/


struct ViewerApp
{
    char    errmsg[1024];
    char    retstr[10000];
    char    project[40];

    char    objname[40];
    char    filter[40];
    char    pixscale[40];


/*
    System config parameters
*/
    char    http_srvr[1024];
    char    baseurl[1024];

    char    workdir[1024];
    char    workspace[1024];
    char    directory[1024];


    char    cookiename[1024];
    char    cookiestr[1024];
    char    timeout[1024];


/*
    Parameters from init param file
*/
    char    paramfile[1024];
    char    parampath[1024];

    char    datadir[1024];
    char    immode[10];

    char    viewtemplate[1024];
    char    viewhtml[1024];
    char    viewhtmlpath[1024];
    char    helphtml[1024];
    char    helphtmlpath[1024];
    
    char    viewcgiurl[1024];
    char    tblcgiurl[1024];

    char    **impath;
    char    **imfile;

    char    **rimpath;
    char    **rimfile;
    char    **gimpath;
    char    **gimfile;
    char    **bimpath;
    char    **bimfile;
    
    char    imlist_gray[1024];
    char    imlist_color[1024];
    char    tbllist[1024];
    char    iminfolist[1024];


    char    imcubefile[1024];
    char    imcubepath[1024];
    
    char    graypath[1024];

    char    redcubefile[1024];
    char    redcubepath[1024];
    char    grncubefile[1024];
    char    grncubepath[1024];
    char    bluecubefile[1024];
    char    bluecubepath[1024];

    char    ctype3[40];
   
    double  crval3;
    double  cdelt3;


/*
    Input stretch param
*/
    char    colortbl[40];
    
    char    stretchmode[40];
    char    minstretch[40];
    char    maxstretch[40];

    char    redminstretch[40];
    char    redmaxstretch[40];
    char    redstretchmode[40];

    char    grnminstretch[40];
    char    grnmaxstretch[40];
    char    grnstretchmode[40];

    char    blueminstretch[40];
    char    bluemaxstretch[40];
    char    bluestretchmode[40];


    
    int     nim_gray;
    int     *imExist_gray;

    int     nim_color;
    int     *imExist_color;

    int     cuberewritten;
    int     isimcube;

    int     nfitsplane;
    int     nplaneave;
    char    nplaneavestr[20];
    
    int     centerplane;
    int     startplane;
    int     endplane;

    char    imcubemode[20];
    char    imcursormode[20];


    int     iscolor;

    int     tblwidth;
    int     tblheight;

    int     refwidth;
    int     refheight;

/*
    Input parameters
*/
    char    cmd[1024];
    char    mode[40];
    
    int     impick;
    int     srcpick;

    int     nogrid;
    int     nowcs;

    int     isInit;
    int     update;
    
    char    title[1024];
    char    winname[1024];
    
    char    infiletype[40];
    char    outimtype[40];
    
    char    divname[1024];
    char    imname[1024];
    char    imroot[1024];
    char    grayfile[1024];

    char    redroot[1024];
    char    grnroot[1024];
    char    blueroot[1024];
    
    char    redfile[1024];
    char    grnfile[1024];
    char    bluefile[1024];
    
    char    jsonfile[1024];
    char    jsonpath[1024];
    
    int     canvaswidth;
    int     canvasheight;
    
    int     nimMax;
    int     nim;

    
/*
    Overlay
*/
    int    nlabelMax;
    int    ngridMax;
    int    nsrctblMax;
    int    niminfoMax;
    int    nmarkerMax;

    int     nmarker;
    int     *markervis;
    char    **markersize;
    char    **markertype; 
    char    **markerlocstr; 
    char    **markercolor; 

    int     nlabel;
    int     *labelvis;
    char    **labelcolor; 
    char    **labeltext; 
    char    **labellocstr; 

    int     ngrid;
    int     *gridvis;
    char    **gridcolor; 
    char    **gridcsys; 
    
    int     niminfo;
    int     *iminfovis;
    char    **iminfopath;
    char    **iminfofile;
    char    **iminfocolor;
    
    int     nsrctbl;
    int     *srctblvis;
    char    **srctblpath;
    char    **srctblfile;
    
    char    **srctblcolor;
    
    char    **tblsymtype;
    double  *tblsymsize;
    int     *tblsymside;
    char    **datacol;
    double  *dataref;
    char    **datafluxmode;
  

/*
    Wave plot parameters
*/
    char    plotjsonfile[1024];
    char    plotjsonpath[1024];
    
    char    waveplottype[20];

    char    plottype[20];
    char    waveplotfile[1024];
    char    waveplotpath[1024];
    char    wavejsonfile[1024];
    char    wavejsonpath[1024];

    char    plottitle[100];
    char    plotxaxis[20];
    char    plotyaxis[20];
    
    char    plotxlabel[40];
    char    plotylabel[40];
    
    char    plotxlabeloffset[40];
    char    plotylabeloffset[40];
    
    char    plotbgcolor[40];
    char    plotaxescolor[40];
    char    plotlabelcolor[40];
    
    char    plotsymbol[40];
    char    plotcolor[40];
    
    char    plotlinestyle[40];
    char    plotlinecolor[40];

    char    plothistvalue[40];

    int     haveplot;
    int     plothiststyle;

    int     plotwidth;
    int     plotheight;

/*
    average plot region boundary input
    if it is pick than xe=xs and ye=ys
*/
    double  xs;
    double  xe;
    double  ys;
    double  ye;

/*
    click result parameters
*/
    int    xpick;
    int    ypick;

    double  rapick;
    double  decpick;

    char    sexrapick[40];
    char    sexdecpick[40];
    char    pickcsys[20];
    
    double  pickval;

};

#endif
