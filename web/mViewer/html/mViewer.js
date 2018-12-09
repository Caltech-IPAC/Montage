/*
Theme: mViewer Javascript library.  

This code defines an mViewer Javascript "object".  Any GUI can have 
as many of this in a single browser window as needed.  Each object is 
tied to a specific set of files in a disk "workspace" area and to 
specific <div> so the required creation parameters are a (relative) 
pointer to the workspace, the ID of the <div> in in the current window.

mViewer can be initialized by a JSON disk file or a plan JSON structure
string. 

Modified: August 3, 2017 (Mihseh Kong)

Project related functionality such as re-arranging data cube and
extracting project keywords from FITS header have been removed. 
This code is strictly project independent.
*/

function mViewer(workspace, imgDivID)
{
    var me = this;

    me.debug = 0;

    if (me.debug) {
	console.log ("Enter mViewer: workspace= " + workspace);
        console.log ("imgDivID= " + imgDivID);
    }
    
    me.cmd;

    me.workspace  = workspace;
    me.imgDivID   = imgDivID;

    me.imgDiv = document.getElementById(imgDivID);

    me.dnloadurl = "/cgi-bin/FileDownload/nph-download";;
    me.cgiurl = "/cgi-bin/mViewer/nph-mViewer";

    me.jsonfile;
    me.jsonstr;

    me.getjsonHandle;
    me.iceajaxHandle;

    me.showButtons = true;

    me.timeoutValue = 150; // milliseconds
    
    me.updateInProgress = false;

    me.initJSON   = {};
    me.updateJSON = {};
    me.imgData    = {};

    me.updateCallbacks = [];
    me.clickCallbacks  = [];
    me.boxCallbacks  = [];

    me.display;
    me.gc;

    me.xminImg = 0;
    me.xmaxImg = 1;
    me.yminImg = 0;
    me.ymaxImg = 1;

    me.scale;

    me.colorPlane;

    me.iceBlock;
    me.noBlocker = true;

    me.hdrWin;


/*
    Initialize mViewer with either 'initJsonfile' or 'initJsondata';
    when jsondata (updateJSON) is downloaded, the callback routine 
    'getjsonCallback' will construct paramurl and send the ajax call via
	    
	me.iceajaxHandle.sendCmd (paramurl, "json");

    to display the initial image. 
*/
    me.initJsonfile = function (jsonfile)
    {
        me.cmd = "initjsonfile";

	if (me.debug) {
            console.log("mViewer.initJsonfile(): cmd= " + me.cmd);
        }
        
	me.init();

        if (jsonfile != undefined) {
            
            if (me.getjsonHandle == undefined) {
        
	        me.getjsonHandle = new getJSON (me.workspace, me.dnloadurl);
	        me.getjsonHandle.addCallback (me.getjsonCallback);
	        
		if (me.debug) {
	            console.log ("getjsonHandle created");
                }
            }

	    me.getjsonHandle.submit(jsonfile);
	    
	    if (me.debug) {
                console.log("mViewer.getjsonHandle.submit called");
            }
        
        }

    }
   
    me.initJsondata = function(data)
    {
        me.cmd = "initjsondata";
        
	if (me.debug) {
            console.log("mViewer.initJsondata()");
        }

        if ((data == undefined) ||
            (data == null)) {
	    return;
	}

	me.init();

	me.initJSON = data;
        me.updateJSON = data;

        if (!me.noBlocker) {
            me.iceBlock.show(); 
        }

        if (me.debug) {
	    console.log ("submit a ajax request");
        }

        var paramurl = "&cmd=" + me.cmd + "&json="
	    + encodeURIComponent(JSON.stringify(me.updateJSON));
				                
        if (me.debug) {
	    console.log ("query URL = " + me.cgiurl + "?workspace="
            + me.workspace + "&" + paramurl);
        }

            
	me.iceajaxHandle.sendCmd (paramurl, "json");
    }


    me.init = function ()
    {
        if (me.debug) {
            console.log("DEBUG> mViewer.init()");
        }
  
	me.updateInProgress = false;

        me.initJSON   = {};
        me.updateJSON = {};
        
	me.xminImg = 0;
        me.xmaxImg = 1;
        me.yminImg = 0;
        me.ymaxImg = 1;

        me.scale   = 1;

/*
    Create the internals mViewer needs inside the display <div>
    This consists of a canvas for the image itself and another
    one overlaying it for graphics.
*/
        if (me.display == undefined) {
        
	    if (me.debug) {
	        console.log ("create display canvas area");
	    } 
	    
	    me.display = jQuery('<div/>').appendTo(me.imgDiv);
            me.display = jQuery(me.display).get(0);
        
            var rect = me.display.getBoundingClientRect();

	    if (me.debug) {
                if (rect != null) {
	            console.log ("rect.left= " + rect.left);
	            console.log ("rect.top= " + rect.top);
	            console.log ("rect.right= " + rect.right);
	            console.log ("rect.bottom= " + rect.bottom);
	        }
            }
        }

/*
        if (me.getjsonHandle == undefined) {
        
	    me.getjsonHandle = new getJSON (me.workspace, me.dnloadurl);
	    me.getjsonHandle.addCallback (me.getjsonCallback);

	    if (me.debug) {
	        console.log ("getjsonHandle created");
            }
        }
*/

        if (me.iceajaxHandle == undefined) {
        
	    me.iceajaxHandle = new iceAjax (me.workspace, me.cgiurl);
	    me.iceajaxHandle.addCallback (me.iceajaxCallback);

	    if (me.debug) {
	        console.log ("iceajaxHandle created");
            }
        }

/*
    Initialize graphic canvas
    
    Do this only once in init to avoid multiple gc which shows 
    when window resizes.

    mViewer only passes along the click/box coordinates to the
    applicaiton program, not taking any custom action internally.

    When the image click event occurs, me.clickCalback is executed;
    me.clickCallback calls the me.clickCallbacks[] which is defined
    by the custom code in the application program, and registered
    at the time when mViewer is initialized.
*/
        if (me.gc == undefined)  {
	
            if (me.debug) {
                console.log ("call iceGraphics");
            }     

            me.gc = new iceGraphics(me.imgDivID);
            me.gc.boxCallback        = me.boxCallback;
            me.gc.clickCallback      = me.clickCallback;
            me.gc.rightClickCallback = me.rightClickCallback;
        }	

        if ((!me.noBlocker) && (me.iceBlock == undefined))  {
            me.iceBlock = new IceBlock(me.imgDivID);
        }

    }


   // The initial JSON file was created by whoever set up the
   // workspace and will be used both to generate the intial
   // display and to configure various controls.

    me.getJSONFile = function(jsonfile)
    {
        var debugflag = 0;

        me.jsonfile = jsonfile;

        me.getjsonHandle.submit (me.jsonfile);
    
	if (debugflag) {
	    console.log ("From mViewer.getJSONFile: jsonfile= " + jsonfile);
        }

        me.getjsonHandle.submit (me.jsonfile);
    }


    me.getjsonCallback = function() 
    {
        var debug = 0;

	if (debug) {
            console.log ("From mViewer.getjsonCallback");
        }	

	me.initJSON = me.getjsonHandle.jsonData;
        me.updateJSON = me.initJSON;
/*
	if (debug) {
	    console.log ("imagefile= " 
	        + me.initJSON.image.imagefile);
	    console.log ("imagetype= " 
	        + me.initJSON.image.imagetype);
	    console.log ("fitsfile= " 
	        + me.initJSON.image.grayfile.fitsfile);
            console.log ("me.initJSON.image.grayfile.colortable= " 
                + me.initJSON.image.grayfile.colorTable);
            console.log ("me.initJSON.image.grayfile.stretchmin= " 
                + me.initJSON.image.grayfile.stretchmin);
            console.log ("me.initJSON.image.grayfile.stretchmax= " 
                + me.initJSON.image.grayfile.stretchmax);
            console.log ("me.initJSON.image.grayfile.stretchmode= " 
                + me.initJSON.image.grayfile.stretchmode);
	            
            var noverlay = me.initJSON.overlay.length;
	                
            console.log ("noverlay= " + noverlay);

	    for (var l=0; l<noverlay; l++) {
	    
	        console.log ("me.initJSON.overlay[l].type= " 
	            + me.initJSON.overlay[l].type);
                console.log ("me.initJSON.overlay[l].color= " 
	            + me.initJSON.overlay[l].color);
                console.log ("me.initJSON.overlay[l].visible= " 
	            + me.initJSON.overlay[l].visible);
           
	        if (me.initJSON.overlay[l].type == "grid") {

	            console.log ("me.initJSON.overlay[l].coordsys= "
	                + me.initJSON.overlay[l].coordsys);
	        }
	        if (me.initJSON.overlay[l].type == "catalog") {

                    console.log ("me.initJSON.overlay[l].datafile= "
	                + me.initJSON.overlay[l].datafile);
                    
		    console.log ("me.initJSON.overlay[l].symtype= "
	                + me.initJSON.overlay[l].symtype);
                    console.log ("me.initJSON.overlay[l].symsize= "
	                + me.initJSON.overlay[l].symsize);
	        }
	        if (me.initJSON.overlay[l].type == "iminfo") {

                    console.log ("me.initJSON.overlay[l].datafile= "
	                + me.initJSON.overlay[l].datafile);
	        }
	        else if (me.initJSON.overlay[l].type == "marker") {
	
                    console.log ("me.initJSON.overlay[l].symtype= "
	                + me.initJSON.overlay[l].symtype);
                    console.log ("me.initJSON.overlay[l].symsize= "
	                + me.initJSON.overlay[l].symsize);
                    console.log ("me.initJSON.overlay[l].location= "
	                + me.initJSON.overlay[l].location);
                } 
            }
	}
*/

	if (debug) {
	    console.log ("mViewer.cmd= " + me.cmd);
	    console.log ("mViewer.updateInProgress= " + me.updateInProgress);
	}


	if (!me.updateInProgress) {

            if (!me.noBlocker) {
                me.iceBlock.show(); 
            }

            if (debug) {
	        console.log ("submit a ajax request");
            }

            var paramurl = "&cmd=" + me.cmd + "&json="
		    + encodeURIComponent(JSON.stringify(me.updateJSON));
				                
	   if (debug) {
	       console.log ("query URL = " + me.cgiurl + "?workspace="
               + me.workspace + "&" + paramurl);
            }

            me.updateInProgress = true;
            
	    me.iceajaxHandle.sendCmd (paramurl, "json");
	
	}
    }

/*
    Change to a new JSON structure (set by an outside application)
    e.g.  replaceim operation
*/
    me.replaceJSON = function(cmd, jsondata)
    {
        if (me.debug)
            console.log("DEBUG> mViewer.replaceJSON()");

        me.cmd = cmd;
        me.initJSON = jsondata;
        me.updateJSON = jsondata;

        if (!me.updateInProgress) {

            if (!me.noBlocker) {
                me.iceBlock.show(); 
            }


            if (me.debug) {
	        console.log ("submit a ajax request");
            }

            var paramurl = "&cmd=" + me.cmd + "&json="
		    + encodeURIComponent(JSON.stringify(me.updateJSON));
				                
	   if (me.debug) {
	       console.log ("query URL = " + me.cgiurl + "?workspace="
               + me.workspace + "&" + paramurl);
            }

            me.updateInProgress = true;
            
	    me.iceajaxHandle.sendCmd (paramurl, "json");
        }	
    }


    me.iceajaxCallback = function()
    {
//        alert ("From mViewer.iceajaxCallback");
        
	var debugflag = 0;

	if (debugflag) {
            console.log ("From mViewer.iceajaxCallback");
            console.log ("retstatus= " + me.iceajaxHandle.retstatus);
	}
        
        if (me.iceajaxHandle.retstatus != "success") {
            me.updateInProgress = false;
            return;	
	}

/*
    Successful: save data and update JSON
*/
	me.imgData = me.iceajaxHandle.data;
	me.updateJSON = me.imgData;
        me.cmd = me.updateJSON.cmd;

	if (debugflag) {
            console.log ("me.cmd= " + me.cmd);
	}

        if ((me.cmd != "pick") && 
            (me.cmd != "impick")) { 
                
	    if (debugflag) {
	        console.log ("get new JPEG url");
            }

            me.gc.clear();

            me.gc.refitCanvas();

            if (debugflag) {
                console.log("mViewer.iceajaxCallback: Retrieving new JPEG.");
            }
            
	    me.xminImg = me.imgData.subimage.xmin;
            me.xmaxImg = me.imgData.subimage.xmax;
            me.yminImg = me.imgData.subimage.ymin;
            me.ymaxImg = me.imgData.subimage.ymax;
            me.scale = me.imgData.image.factor;

            if(debugflag) {
                console.log("DEBUG> xminImg= " + me.xminImg);
                console.log("DEBUG> xmaxImg= " + me.xmaxImg);
                console.log("DEBUG> yminImg= " + me.yminImg);
                console.log("DEBUG> ymaxImg= " + me.ymaxImg);
                console.log("DEBUG> scale= " + me.scale);
                console.log("DEBUG> baseurl= " + me.imgData.baseurl);
                console.log("DEBUG> jpgfile= " + me.imgData.image.file);
            }

	    var newjpg = me.imgData.baseurl + "/" + me.imgData.image.file;
	    me.gc.setImage(newjpg + "?seed=" + (new Date()).valueOf());
                
	    if(debugflag) {
                console.log("DEBUG> newjpg= " + newjpg);
            }
            
        }

/*
    If there are any registered update callbacks,  call them now.
*/
        if(debugflag) {
            console.log("DEBUG> cmd= " + me.cmd);
            console.log("DEBUG> mviewer.updateCallbacks.length= " 
	        + me.updateCallbacks.length);
        }

        me.updateInProgress = false;
    
        if (!me.noBlocker) {
            me.iceBlock.hide();
	}
	
	if (debugflag) {
	    console.log ("iceajaxCallback done: execute callbacks");
        }
        
	for(i=0; i<me.updateCallbacks.length; i++)
        {
            if(debugflag) {
                console.log("DEBUG> i= " + i + " call updateCallbacks");
	    }

            me.updateCallbacks[i]();
        }

    }


/*
    Other objects (mostly controls but we'll leave it open)
    want to be notified when an image display update occurs.
    Here we manage a list of these object.methods.
*/
   me.addUpdateCallback = function(method)
   {
      me.updateCallbacks.push(method);
	
        if (me.debug) {
	    console.log ("From mViewer.addUpdateCallback: ncallbacksd= "
	        + me.updateCallbacks.length);
	    console.log ("method= ");
	    console.log (method);
        }
   }

/*
    Display area resize event callback.  This is a pair
    of functions:  The first one (the entry point) starts
    a timer and eventually calls the second (used by
    no one else) to do the real work.
*/
   me.resizeTimeout = 0;

   me.resize = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.resize()");

      if(me.resizeTimeout)
         clearTimeout(me.resizeTimeout);

      me.resizeTimeout = setTimeout(me.resizeImageRequest, me.timeoutValue);
   }

/*
   me.resizeImageRequest = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.resizeImageRequest()");

      me.resizeTimeout = 0;

      var areaHeight = jQuery(me.imgDiv).height();
      var areaWidth  = jQuery(me.imgDiv).width();

      if(me.debug) {
         console.log("DEBUG> areaWidth= " + areaWidth)
         console.log("DEBUG> areaHeight= " + areaHeight)
      } 
     
      me.updateJSON.canvasWidth = areaWidth;
      me.updateJSON.canvasHeight = areaHeight;

      jQuery(me.buttons).width (areaWidth);

      jQuery(me.display).height(areaHeight);
      jQuery(me.display).width (areaWidth);

      if(me.gc != null)
      {
         me.gc.clear();
         me.gc.refitCanvas();

         me.submitUpdateRequest();
      }
   }
*/


   me.getWorkspace = function()
   {
      return me.workspace;
   }


   me.getJSON = function()
   {
      return me.initJSON;
   }


   // CLICK ACTIONS


    me.showFitshdr = function()
    {
        if (me.debug)
            console.log ("mViewer.showFitshdr");

        var datastr;
        var datadir = me.imgData.image.grayfile.datadir;
        
	var cubedata = me.imgData.imcubeFile;
	if (cubedata == null) {
           

	    if (datadir.length > 0) {
                datastr = "workspace=" + me.workspace + "&file=" 
	            + datadir + "/" + me.imgData.image.grayfile.fitsfile;
	    }
	    else {
                datastr = "workspace=" + me.workspace + "&file=" 
	            + me.imgData.image.grayfile.fitsfile;
	    }
	    
	}
	else {
            if (me.debug)
                console.log ("mViewer.showFitshdr: cube data");

            var cubeOrig = me.imgData.imcubeFile.fitsFileOrig;

	    if (cubeOrig == null) {
	    
	        if (datadir.length > 0) {
                    datastr = "workspace=" + me.workspace + "&file=" 
	                + datadir + "/" + me.imgData.imcubeFile.fitsFile;
                }
		else {
		    datastr = "workspace=" + me.workspace + "&file=" 
	                + me.imgData.imcubeFile.fitsFile;
		}
	    }
	    else {
	        if (datadir.length > 0) {
                    datastr = "workspace=" + me.workspace + "&file=" 
	                + datadir + "/" + me.imgData.imcubeFile.fitsFileOrig;
                }
		else {
                    datastr = "workspace=" + me.workspace + "&file=" 
	                + me.imgData.imcubeFile.fitsFileOrig;
		}

	    }
	}

        if (me.debug)
            console.log ("datastr= " + datastr);
        
	jQuery.post ("/cgi-bin/mViewer/nph-mViewerHdr", datastr,
	    me.showFitshdrCallback, "json");

    }


    me.showFitshdrCallback = function(data, stat, jqXHR)
    {
	if (me.debug) {
            console.log ("From showFitshdrCallback\n");
            console.log ("stat= " + stat);
            console.log ("jqXHR.statusText= " + jqXHR.statusText);
            console.log ("jqXHR.readyState= " + jqXHR.readyState);
            console.log ("jqXHR.status= " + jqXHR.status);
            console.log ("jqXHR.responseText= " + jqXHR.responseText);
            console.log ("data= " + data);
	}

        if (jqXHR.status != 200) {

            alert ("Remote service error");
            return;
	}

        var url = data.url;
        if (me.debug) {
	    console.log ("url= " + url);
	}

	if ((me.hdrWin != undefined) && 
	    (!me.hdrWin.closed)) {

            if (me.debug) {
	        console.log ("here1");
	    }

            me.hdrWin.location.replace(data.url);
            me.hdrWin.focus();
	    return;
	}
	else {
	    me.hdrWin = window.open(data.url, "hdrwin", 
	        "toolbar=no,directories=no,location=no,status=yes,menubar=no,resizeable=yes,scrollbars=yes,width=800,height=600");

            if (me.debug) {
	        console.log ("here1");
	    }

	    return;
	}

    }
   
   
   // Zoom in

    me.zoomIn = function()
    {
        if (me.debug) {
            console.log("DEBUG> mViewer.zoomIn()");
            console.log("DEBUG> updateJSON.xmin: " + me.updateJSON.xmin);
            console.log("DEBUG> updateJSON.xmax: " + me.updateJSON.xmax);
            console.log("DEBUG> updateJSON.ymin: " + me.updateJSON.ymin);
            console.log("DEBUG> updateJSON.ymax: " + me.updateJSON.ymax);
        }

	me.cmd = "zoomin";

        var paramurl = "&cmd=zoomin&json="
	    + encodeURIComponent(JSON.stringify(me.updateJSON));

        if (me.debug)
            console.log("paramurl= " + paramurl);

	if (!me.noBlocker) {
            me.iceBlock.show(); 
        }

        me.iceajaxHandle.sendCmd (paramurl, "json"); 
        
	if (me.debug) {
            console.log("zoomout ajax call sent:");
            console.log("DEBUG> updateJSON.xmin: " + me.updateJSON.xmin);
            console.log("DEBUG> updateJSON.xmax: " + me.updateJSON.xmax);
            console.log("DEBUG> updateJSON.ymin: " + me.updateJSON.ymin);
            console.log("DEBUG> updateJSON.ymax: " + me.updateJSON.ymax);
	}
    }


   // Zoom out

    me.zoomOut = function()
    {
        if(me.debug)
            console.log("DEBUG> mViewer.zoomOut()");

        if(me.debug) {
            console.log("DEBUG> updateJSON.xmin: " + me.updateJSON.xmin);
            console.log("DEBUG> updateJSON.xmax: " + me.updateJSON.xmax);
            console.log("DEBUG> updateJSON.ymin: " + me.updateJSON.ymin);
            console.log("DEBUG> updateJSON.ymax: " + me.updateJSON.ymax);
        }

	me.cmd = "zoomout";

        var paramurl = "&cmd=zoomout&json="
	    + encodeURIComponent(JSON.stringify(me.updateJSON));

        if (me.debug)
            console.log("paramurl= " + paramurl);

	if (!me.noBlocker) {
            me.iceBlock.show(); 
        }

        me.iceajaxHandle.sendCmd (paramurl, "json"); 
        
        if (me.debug) {
            console.log("zoomout ajax call sent:");
            console.log("DEBUG> updateJSON.xmin: " + me.updateJSON.xmin);
            console.log("DEBUG> updateJSON.xmax: " + me.updateJSON.xmax);
            console.log("DEBUG> updateJSON.ymin: " + me.updateJSON.ymin);
            console.log("DEBUG> updateJSON.ymax: " + me.updateJSON.ymax);
        }
    }


   // Zoom reset

    me.zoomReset = function()
    {
        if (me.debug)
            console.log("DEBUG> mViewer.zoomReset()");

        me.updateJSON.xmin = 0.;
        me.updateJSON.xmax = me.updateJSON.imageWidth;
        me.updateJSON.ymin = 0.;
        me.updateJSON.ymax = me.updateJSON.imageHeight;
        
	me.cmd = "resetzoom";

        var paramurl = "&cmd=resetzoom&json="
	    + encodeURIComponent(JSON.stringify(me.updateJSON));

        if (me.debug)
            console.log("paramurl= " + paramurl);

	if (!me.noBlocker) {
            me.iceBlock.show(); 
        }

        me.iceajaxHandle.sendCmd (paramurl, "json"); 
        
        if (me.debug) {
            console.log("resetzoom ajax call sent:");
            console.log("DEBUG> updateJSON.xmin: " + me.updateJSON.xmin);
            console.log("DEBUG> updateJSON.xmax: " + me.updateJSON.xmax);
            console.log("DEBUG> updateJSON.ymin: " + me.updateJSON.ymin);
            console.log("DEBUG> updateJSON.ymax: " + me.updateJSON.ymax);
        }
   }


/*
    stretch reset
*/
   me.stretchReset = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.stretchReset()");

	me.updateJSON.image.grayfile.stretchmin 
	    = me.initJSON.image.grayfile.stretchmin;
	me.updateJSON.image.grayfile.stretchmax 
	    = me.initJSON.image.grayfile.stretchmax;
	me.updateJSON.image.grayfile.stretchmode 
	    = me.initJSON.image.grayfile.stretchmode;
	me.updateJSON.image.grayfile.colortable 
	    = me.initJSON.image.grayfile.colortable;
        
	me.cmd = "resetstretch";

        var paramurl = "&cmd=resetstretch&json="
	    + encodeURIComponent(JSON.stringify(me.updateJSON));

        if (me.debug)
            console.log("paramurl= " + paramurl);

	if (!me.noBlocker) {
            me.iceBlock.show(); 
        }

        me.iceajaxHandle.sendCmd (paramurl, "json"); 
   
   }

   

   // Download the image

    me.downloadImage = function(fileurl)
    {
        if(me.debug) {
            console.log("DEBUG> mViewer.downloadImage()");
            console.log ("me.imgData.image.file= " + me.imgData.image.file);
            console.log ("fileurl= " + fileurl);
        }
        
	var imurl;
	if (fileurl == null) 
	    imurl = me.imgData.baseurl + "/" + me.imgData.image.file;
        else
	    imurl = fileurl;

        if (!window.location.origin)
            window.location.origin 
	        = window.location.protocol+"/"+window.location.host;

        var url = window.location.origin 
            + "/cgi-bin/FileDownload/nph-download?forceDownload=1&url=" 
	    + imurl; 

        window.open(url, "_self");
    }


/*
GRAPHICS CALLBACKS:

Image click/box event are defined by the application;
if there are any registered update callbacks, we call them here.
*/
    me.clickCallback = function(x, y)
    {
        if(me.debug)
        {
            console.log("DEBUG> me.clickCallback: x= " + x + " y= " + y);
        }
        
	me.updateJSON.cursor.xs = x;
	me.updateJSON.cursor.ys = y;
	me.updateJSON.cursor.xe = x;
	me.updateJSON.cursor.ye = y;
        
        for(i=0; i<me.clickCallbacks.length; ++i) {
         
	    me.clickCallbacks[i]();
        }
    }
   
    me.boxCallback = function(xs, ys, xe, ye, div)
    {
        if(me.debug)
        {
            console.log("DEBUG> me.boxCallback: xs= " + xs + " ys= " + ys);
            console.log("DEBUG> xe= " + xe + " ye= " + ye);
            console.log("DEBUG> div= " + div);
        }

        if (xs > xe)
        {
            tmp  = xs;
            xs = xe;
            xe = tmp;
        }

        if(xs == xe)
            xe = xs + 1.e-9;

        if(ys > ye)
        {
            tmp  = ys;
            ys = ye;
            ye = tmp;
        }

        if(ys == ye)
            ye = ys + 1.e-9;


        if(me.debug)
        {
            console.log("DEBUG>  (screen): " + xs + ", " + ys);
            console.log("DEBUG>  (screen): " + xe + ", " + ye);
        }

	me.updateJSON.cursor.xs = xs;
	me.updateJSON.cursor.ys = ys;
	me.updateJSON.cursor.xe = xe;
	me.updateJSON.cursor.ye = ye;
        
	if(me.debug)
        {
            console.log("DEBUG>  (screen): " + me.updateJSON.cursor.xs + ", " 
	        + me.updateJSON.cursor.ys);
            console.log("DEBUG>  (screen): " + me.updateJSON.cursor.xe + ", " 
	        + me.updateJSON.cursor.ye);
        }

        
        for(i=0; i<me.boxCallbacks.length; ++i) {
         
	    me.boxCallbacks[i]();
        }
    }
   


   // Other objects (mostly controls but we'll leave it open)
   // want to be notified when the user clicks on the image.
   // Here we manage a list of these object.methods.

   me.addClickCallback = function(method)
   {
      me.clickCallbacks.push(method);
   }
   
   me.addBoxCallback = function(method)
   {
      me.boxCallbacks.push(method);
   }


   // For now, we will just use the right click for our
   // own purposes.  This could be made public if there 
   // is a reason to.

   me.rightClickCallback = function(x, y)
   {
      me.downloadImage();
   }

}
