/*
Theme: This routine look up proper Hex code color string.

Date: January 19, 2011 (Mihseh Kong)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>


static char Colorval[][30] = {
        "black",
	"white",
	"grey",
	"red",
	"green",
	"blue",
	"yellow",
	"magenta",          
	"cyan",          
	"lightgrey",        
        "darkgrey",
	"deepred",
        "cornflowerblue",   
        "dodgerblue",       
        "lightskyblue",     
        "darkgreen",        
        "lightgreen",       
        "deepred",          
        "pink",             
        "hotpink",          
        "magenta4",         
        "purple",           
        "orange",           
        "lemonchiffon"
};

static char Hexval[][30] = {
        "000000",
	"FFFFFF",
	"7F7F7F",
	"FF0000",
	"00FF00",
	"0000FF",
	"FFFF00",
	"FF00FF", 
	"00FFFF", 
	"D3D3D3",
        "474747", 
        "8B0000",
        "6495ED",  
        "104E8B", 
        "87CEFA",  
        "006400", 
        "09F911", 
        "8B0000", 
        "FFC0CB",   
        "FF69B4", 
        "8B008B",  
        "7D26CD",  
        "FF7F00",
        "FFFACD"  
};


int colorLookup (char *color, char *colorstr, char *errmsg)
{
    int   l, n, found;
    

    n = 24;
    found = 0;
    for (l=0; l<n; l++) {
    
	if ((strcasecmp (color, Colorval[l]) == 0) ||
	    (strcasecmp (color, Hexval[l]) == 0)) {
	    
            strcpy (colorstr, Colorval[l]);
	    found = 1;

	    break;
	}
    }
        
    if (found)
        return (0);
    else
        return (-1);
}


int hexLookup (char *color, char *colorstr, char *errmsg)
{
    int   l, n, found;
    

    n = 24;
    found = 0;
    for (l=0; l<n; l++) {
    
	if ((strcasecmp (color, Colorval[l]) == 0) ||
	    (strcasecmp (color, Hexval[l]) == 0)) {
	    
            strcpy (colorstr, Hexval[l]);
	    found = 1;

	    break;
	}
    }
        
    if (found)
        return (0);
    else
        return (-1);
}




