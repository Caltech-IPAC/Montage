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
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
        "10"
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


/*
    The following two routines are for process the pick result:
    
    inputs:  mViewer handle,
             divID of the text area to write the pick result.
*/
function processPickResult (viewer, textareaID)
{
    if (viewer.debug) {
        console.log ("From processPickResult: cmd= " + viewer.cmd);
    }

    if ((viewer.cmd != "impick") && 
        (viewer.cmd != "waveplot")) { 
        return;
    }
   
    var data = viewer.imgData;
   
    if (data == null)
        return;
    
    var xs = data.xs;
    var xe = data.xe;
    var ys = data.ys;
    var ye = data.ye;

    var pickval = data.pickValue;
        
    var xpick = data.xPick; 
    var ypick = data.yPick; 
    var rapick = data.raPick; 
    var decpick = data.decPick; 
    var sexrapick = data.sexraPick; 
    var sexdecpick = data.sexdecPick; 

    if (viewer.debug) {
        console.log (" pickval= " + pickval); 
        console.log (" xpick= " + xpick + " ypick= " + ypick);
        console.log (" rapick= " + rapick + " decpick= " + decpick);
        console.log (" sexrapick= " + sexrapick 
            + " sexdecpick= " + sexdecpick);
        console.log (" pickcsys= " + data.pickcsys);
        console.log ("nowcs= " + viewer.imgData.nowcs);
    }

/*
    Mark picked value and write x and y axis values in the text area
*/
    var datastr ="";
        
    if (viewer.imgData.nowcs == "0") { 
           
        var pickcsys;
        if (data.pickCsys != undefined)
            pickcsys = data.pickCsys; 
        else if (data.imCsys != undefined)
            pickcsys = data.imCsys; 
        
	if ((pickcsys == undefined) || (pickcsys.length == 0)) {
	    pickcsys = "EQ J2000.";
        }
    
        if (viewer.debug) {
                console.log (" pickcsys= " + data.pickcsys);
        }

        datastr =  "<b>Coord&nbsp;</b> (RA Dec in " + pickcsys + "):<br />" 
            + sexrapick + "&nbsp;&nbsp;" 
	    + sexdecpick + "&nbsp;&nbsp;<br />" 
	    + rapick + "&nbsp;&nbsp;" 
	    + decpick + "&nbsp; (deg)<br />"; 
    }

    datastr = datastr + "<b>Pixel value:</b> " + pickval + "<br />"; 

    if (viewer.debug) {
            console.log ("datastr= " + datastr);
    }

/*
    draw the image pick point on image
*/
    viewer.gc.clearDrawing ();
    viewer.gc.drawPlus (xpick, ypick, 4, "#ff0000");
	    
    if (viewer.debug) {
            console.log ("pick value marked");
    }

    
    var textarea = document.getElementById (textareaID);
       
    if (viewer.debug) {
	    console.log ("textarea= " + textarea);
    }

    if ((textarea == undefined) ||
	(textarea == null)) {
        
	if (viewer.zoomwinHandle != null) {

	    var doc = me.zoomwinHandle.document;
	    textarea = doc.getElementById("imcoord");
        	
	    if (viewer.debug) {
	            console.log ("(zoomwinHandle) textarea= " + textarea);
            }
        }
    }

     if	(textarea != null) {

	textarea.style.fontStyle="Times New Roman";
	textarea.style.fontSize="11px";
	textarea.style.color = "#0f0f0f";
	
	textarea.innerHTML = datastr;

	if (viewer.debug == 1) {
	        console.log ("textarea value set");
        } 
    }
}
 

function processClick (viewer)
{

    if (viewer.debug) {
	console.log ("From processClick");
    }

    var updateJSON = viewer.updateJSON;
    if (updateJSON == null)
        return;

    var xs = updateJSON.xs;
    var ys = updateJSON.ys;

    if (viewer.debug) {
	console.log ("xs= " + xs + " ys= " + ys);
    }

    var imcursormode = updateJSON.imcursorMode;

    if (viewer.debug) {
	console.log ("imcursormode= " + imcursormode);
    }

    if ((imcursormode == null) || (imcursormode == "imzoom")) {
        
        viewer.cmd = "impick"; 
        viewer.submitUpdateRequest ("impick"); 
    }
    else if (imcursormode == "waveplot") {
        
        viewer.cmd = "waveplot";
        
	var cubedata = updateJSON.imcubeFile;
	if (cubedata == null)
	    return;

        cubedata.waveplotType = "pix";

        viewer.submitUpdateRequest ("waveplot"); 
    }

}



function processBox (viewer) 
{
    if (viewer.debug) {
	console.log ("From processBox");
    }
    
    var updateJSON = viewer.updateJSON;
    if (updateJSON == null)
        return;

    var xs = updateJSON.xs;
    var xe = updateJSON.xe;
    var ys = updateJSON.ys;
    var ye = updateJSON.ye;

    if (viewer.debug) {
	console.log ("xs= " + xs + " ys= " + ys);
	console.log ("xe= " + xe + " ye= " + ye);
    }

    var imcursormode = updateJSON.imcursorMode;
    
    if (imcursormode == null) {
        
	viewer.cmd = "zoombox"; 
        viewer.submitUpdateRequest ("zoombox"); 
    }
    else if (imcursormode == "imzoom") {
        
        viewer.cmd = "zoombox"; 
        viewer.submitUpdateRequest ("zoombox"); 
    }
    else if (imcursormode == "waveplot") {
	
	var cubedata = updateJSON.imcubeFile;
	if (cubedata == null)
	    return;

	viewer.cmd = "waveplot"; 
        cubedata.waveplotType = "ave";

        viewer.submitUpdateRequest ("waveplot"); 
    }

}









