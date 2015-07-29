/*
    Input might be index or colortbl name,
    the return is the colortbl name.
*/
function colortblLookup (str)
{
    var colortbl = [
        "greyscale",
        "reversegreyscale",
        "grayscale",
        "reversegrayscale",
        "thermal",
        "reversethermal",
        "logarithmicthermal",
        "velocity",
        "redramp",
        "greenramp",
        "blueramp"
    ];

    var colortblIndx = [
        "0",
        "1",
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8"
    ];

    var n = colortbl.length;
    var retval = "greyscale";

    for (var l=0; l<n; l++) {
   
	if ((str == colortblIndx[l]) || 
	    (str == colortbl[l])) {
	    
            retval = colortbl[l];
	    break;
	}
    }
        
    return (retval);
}


function colorLookup (hexcolor)
{
    var colorval = [ 
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
    ];

    var hexval = [
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
    ];

    
    var n = colorval.length;
   
    color = hexcolor;

    var found = -1;
    for (var l=0; l<n; l++) {
   
	if ((hexcolor == colorval[l]) || 
	    (hexcolor == hexval[l])) {
	    
            color = colorval[l];
	    found = l;
	    break;
	}
    }
        
    return (color);
}


