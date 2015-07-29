// mVIEWER Javascript library.  
//
// This code defines an mViewer Javascript "object".  Any
// GUI can have as many of this in a single browser window
// as we like.  Each object is tied to a specific set of
// files in a disk "workspace" area and to specific browser 
// window <div> so the required creation parameters are a 
// (relative) pointer to the workspace, an initial display
// JSON file in that directory, and the ID of the <div> in
// in the current window that mViewer will use.

function mViewer(workspace, jsonFile, imgDivID)
{
    var me = this;

    me.debug = false;

    if (me.debug) {
	console.log ("Enter mViewer: workspace= " + workspace);
	console.log ("jsonFile= " + jsonFile);
        console.log ("imgDivID= " + imgDivID);
    }

    me.cmd;

    me.workspace  = workspace;
    me.jsonFile   = jsonFile;
    me.imgDivID   = imgDivID;

    me.imgDiv = document.getElementById(imgDivID);

    me.plotjsonFile;
    me.plotDivID;
    
    me.showButtons = true;
    
    me.supressGrayout = true;

    me.timeoutValue = 150; // milliseconds

    me.updateInProcess = false;
    
    me.initJSON   = {};
    me.updateJSON = {};
    me.imgData    = {};

    me.updateCallbacks = [];
    me.clickCallbacks  = [];
    me.boxCallbacks  = [];
    me.waveplotCallbacks  = [];

    me.display;
    me.gc;

    me.xminImg;
    me.xmaxImg;
    me.yminImg;
    me.ymaxImg;

    me.scale;
    me.downloadMode = false;

    me.hdrWin;


   // MAIN SETUP Function.  The parameters above are 
   // essentially "instance variables" and the functions
   // defined below are class methods, tied to the current
   // object ("this" above) by use of the "me" convention
   // that keeps everything instance-specific.

   // The initialization proceeds in three steps:  First
   // we create any GUI objects needed inside the display
   // <div> specific to this instance (e.g. image display
   // and drawing overlay canvases, optional control buttons,
   // etc.)  Then, we asynchronously retrieve the initial
   // display JSON file and use this to request the intial
   // plot (after updating the size in it to match the 
   // current window).  Finally, we retrieve the generated
   // JPEG/PNG and paint it into the display window.

   // Since the above involves two asynchronous requests
   // to a server, we have to break the processing into
   // separate steps with callbacks at each stage to carry
   // the processing forward.

   // The sequence is 
   //
   //   init() -> getJSONFile() -> submitUpdateRequest() -> processUpdate()
   //                                   (callback)            (callback)
   //
   // The "submitUpdateRequest() -> processUpdate()" pair
   // will also be used whenever we decide to update the
   // display (e.g. with a new color stretch).

    me.init = function()
    {
        if (me.debug) {
            console.log("DEBUG> mViewer.init()");
        }

        me.initJSON   = {};
        me.updateJSON = {};

        me.display = null;
        me.cmd = "init";
        me.updateInProgress = false;

        me.xminImg = 0;
        me.xmaxImg = 1;
        me.yminImg = 0;
        me.ymaxImg = 1;

        me.scale   = 1;

/*
    Ask John about this
*/
        jQuery('body').on('contextmenu', 'img',    
	    function(e){ return false; });
        jQuery('body').on('contextmenu', 'canvas', 
	    function(e){ return false; });


      // We prevent colliding and redundant remote requests in two ways
      // First, resize events are handled with a short timeout to prevent
      // them firing in rapid succession as you resize the container 
      // (i.e., we try to act only on the final resized value).  Second,
      // once an remote update request is made, further updates are ignored.
      // This also mainly affects resizing since we try to disable (gray out)
      // all controls during the update processing.

        me.updateInProcess = false;


      // Create the internals mViewer needs inside the display <div>
      // This consists of a canvas for the image itself and another
      // one overlaying it for graphics.

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

	    console.log ("init.call getJSONFile");
        }

        me.getJSONFile();
	
    }


   // The initial JSON file was created by whoever set up the
   // workspace and will be used both to generate the intial
   // display and to configure various controls.

    me.getJSONFile = function()
    {
        var xmlhttp;
        var paramURL;

        if (!me.suppressGrayout) {
        
	    me.grayOut(true, {'opacity':'50'});
	    me.grayOutMessage(true);
        }

        paramURL = "/cgi-bin/FileDownload/nph-download"
                 + "?url=" + me.workspace + "/" + me.jsonFile;

        if (me.debug) {
	    console.log ("mViewer.getJSONFile: paramURL= " + paramURL);
        }

        try {
            xmlhttp = new XMLHttpRequest();
        }
        catch (e) {
            xmlhttp=false;
        }

        if (!xmlhttp && window.createRequest)
        {
            try {
                xmlhttp = window.createRequest();
            }
            catch (e) {
                xmlhttp=false;
            }
        }

        xmlhttp.open("GET", paramURL);

        xmlhttp.send(null);

        xmlhttp.onreadystatechange = function()
        {
            if (xmlhttp.readyState==4 && xmlhttp.status==200)
            {
                var xml = xmlhttp.responseXML;

                if(xml && xml.getElementsByTagName("error").length > 0)
                {
                    var obj = xml.getElementsByTagName("error")[0];
                    var error = obj.childNodes[0].nodeValue;

                    me.grayOutMessage(false);
                    me.grayOut(false);

                    alert("Server Error: " + error);
                }
                else
                {
                    me.paramText = xmlhttp.responseText;
                    
		    me.initJSON = jQuery.parseJSON(xmlhttp.responseText);
                    
		    me.updateJSON = me.initJSON;


/*
    Initialize graphic canvas
    
    Do this only once in init to avoid multiple gc which shows 
    when window resizes.
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

                    me.grayOutMessage(false);
                    me.grayOut(false);

	           if (me.debug) {
	               console.log ("call submitUpdateRequest");
                   }

                    me.submitUpdateRequest("init");
	        }
            }
            else if(xmlhttp.status != 200)
            {
                me.grayOutMessage(false);
                me.grayOut(false);

                alert("Remote service error [retrieving initial JSON].");
            } 
	}
    }



   // Change to a new JSON structure 
   // (set by an outside application)

   me.replaceJSON = function(json)
   {
      if (me.debug)
         console.log("DEBUG> mViewer.replaceJSON()");

      me.updateJSON = json;

      me.submitUpdateRequest();
   }



   // Send a request off for an updated display JPEG
   // The POST call names "processUpdate()" as its 
   // callback.

   me.submitUpdateRequest = function(cmd)
   {
        me.cmd = cmd;

        if ((me.cmd != "init") && (me.updateInProgress))
            return;


        if(me.debug) {
            console.log("DEBUG> mViewer.submitUpdateRequest()");
            console.log ("DEBUG> me.cmd= " + me.cmd);
        }

        if (!me.suppressGrayout) {
        
            me.grayOut(true, {'opacity':'50'});
            me.grayOutMessage(true);
        }

        me.updateInProgress = true;

	if (me.debug) {
            console.log ("canvasWidth= " + me.imgDiv.style.width);
            console.log ("canvasHeight= " + me.imgDiv.style.height);
            
	    console.log ("canvasWidth(jquery)= " 
	        + jQuery("#imgDivID").width());
            console.log ("canvasHeight(jquery)= " 
	        + jQuery("#imgDivID").height());
            
	    console.log ("canvasWidth= " + me.updateJSON.canvasWidth);
            console.log ("canvasHeight= " + me.updateJSON.canvasHeight);
            
            var overlay = me.updateJSON.overlay;
            var noverlay;

	    if (overlay != null) {
	        noverlay = overlay.length;
	        for (var l=0; l<noverlay; l++) { 

	            console.log ("l= " + l + " type= " + overlay[l].type);
	            console.log ("visible= " + overlay[l].visible);
	        }
	    }
	}


        if (me.cmd != null) 
        {
	    if (me.debug) {
                console.log ("here1");
            }

            jQuery.post("/cgi-bin/mViewer/nph-mViewer", "workspace=" 
                + me.workspace + "&cmd=" + me.cmd + "&json=" 
	        + encodeURIComponent(JSON.stringify(me.updateJSON)), 
	        me.processUpdate, "json");

            if (me.debug) {
	        console.log ("query URL = " 
                    + "/cgi-bin/mViewer/nph-mViewer?workspace=" 
                    + me.workspace + "&cmd=" + me.cmd + "&json=" 
	            + encodeURIComponent(JSON.stringify(me.updateJSON))); 
	    }
	}
	else {
	    if (me.debug) {
                console.log ("here2");
            }
 
	    jQuery.post("/cgi-bin/mViewer/nph-mViewer", "workspace=" 
                + me.workspace + "&json=" 
	        + encodeURIComponent(JSON.stringify(me.updateJSON)), 
	        me.processUpdate, "json");
	    
            if (me.debug) {
	        console.log ("query URL = " 
                    + "/cgi-bin/mViewer/nph-mViewer?workspace=" 
                    + me.workspace + "&json=" 
	            + encodeURIComponent(JSON.stringify(me.updateJSON))); 
	    }
     
        }
   
    }



    me.processUpdate = function(data, stat, jqXHR)
    {
	if (me.debug) {
            console.log ("From processUpdate");
            console.log ("stat= " + stat);

	    if (jqXHR != undefined) {
                console.log ("jqXHR.statusText= " + jqXHR.statusText);
                console.log ("jqXHR.readyState= " + jqXHR.readyState);
                console.log ("jqXHR.status= " + jqXHR.status);
                console.log ("jqXHR.responseJSON= " + jqXHR.responseJSON);
            }	
	}

        if (jqXHR == undefined) 
	    return;

        if (jqXHR.status != 200) {

            me.grayOutMessage(false);
            me.grayOut(false);

            alert ("Remote service error");
      
/*
    Execute callbacks (at least for the parent window to turn off its 
    blocker.
*/
	    for(i=0; i<me.updateCallbacks.length; ++i)
            {
                me.updateCallbacks[i]();
            }

            me.grayOutMessage(false);
            me.grayOut(false);
            me.updateInProgress = false;
	}

        me.imgData = data;

	if (me.debug) {
            console.log ("here1");
            console.log ("typeof(me.imgData.error)= " 
	        + typeof(me.imgData.error));
            console.log ("typeof(me.imgData.status)= " 
	        + typeof(me.imgData.status));
            console.log ("me.imgData.status= " + me.imgData.status);
            console.log ("me.imgData.error= " + me.imgData.error);
        }    
	    
        
	if (typeof(me.imgData.error) != "undefined")
        {
            me.requestSubmitted = false;

            me.grayOutMessage(false);
            me.grayOut(false);

            alert(me.imgData.error);

            me.updateInProgress = false;

/*
    Execute callbacks (at least for the parent window to turn off its 
    blocker.
*/
	    for(i=0; i<me.updateCallbacks.length; ++i)
            {
                me.updateCallbacks[i]();
            }

            return;
        }

/*
    Successful: update JSON
*/
	me.updateJSON = jqXHR.responseJSON;

	if (me.updateJSON === jqXHR.responseJSON) {

	    if (me.debug) {
                console.log ("updateJSON = responseJSON");
            }
	}
        
	var noverlay = 0;
        if (me.imgData.overlay != null) {	
	    noverlay = me.imgData.overlay.length;
        }

	if (me.debug) {
	    console.log ("me.imgData.file= " 
	        + me.imgData.file);
	    console.log ("me.imgData.imageName= " 
	        + me.imgData.imageName);
            console.log ("me.imgData.imageType= " 
	        + me.imgData.imageType);
            console.log ("me.imgData.canvasWidth= " 
	        + me.imgData.canvasWidth);
            console.log ("me.imgData.canvasHeight= " 
	        + me.imgData.canvasHeight);
            console.log ("me.imgData.grayFile.fitsFile= " 
	        + me.imgData.grayFile.fitsFile);
            console.log ("me.imgData.grayFile.cutoutFile= " 
	        + me.imgData.grayFile.cutoutFile);
            console.log ("me.imgData.grayFile.shrunkFile= " 
	        + me.imgData.grayFile.shrunkFile);
	    console.log ("me.imgData.grayFile.colorTable= " 
	        + me.imgData.grayFile.colorTable);
            console.log ("me.imgData.grayFile.stretchMin= " 
	        + me.imgData.grayFile.stretchMin);
            console.log ("me.imgData.grayFile.stretchMax= " 
	        + me.imgData.grayFile.stretchMax);
            console.log ("me.imgData.grayFile.stretchMode= " 
	        + me.imgData.grayFile.stretchMode);
          
	    console.log ("noverlay= " + noverlay);

	    for (var l=0; l<noverlay; l++) {
	    
	        console.log ("me.imgData.overlay[l].type= " 
	            + me.imgData.overlay[l].type);
                console.log ("me.imgData.overlay[l].color= " 
	            + me.imgData.overlay[l].color);
                console.log ("me.imgData.overlay[l].visible= " 
	            + me.imgData.overlay[l].visible);
           
	        if (me.imgData.overlay[l].type == "grid") {

	            console.log ("me.imgData.overlay[l].coordSys= " 
	                + me.imgData.overlay[l].coordSys);
	        }
	        else if (me.imgData.overlay[l].type == "marker") {
	
                    console.log ("me.imgData.overlay[l].symType= "
	                + me.imgData.overlay[l].symType);
                    console.log ("me.imgData.overlay[l].symSize= "
	                + imgDme.imgData.overlay[l].symSize);
                    console.log ("me.imgData.overlay[l].location= "
	                + me.imgData.overlay[l].location);
                } 
	    }
	    
	    console.log ("me.downloadMode= " + me.downloadMode);
	}


        if (me.downloadMode)
        {
            me.downloadMode = false;

            if (!window.location.origin)
                window.location.origin 
	            = window.location.protocol+"//"+window.location.host;

            var url = window.location.origin 
	         + "/cgi-bin/FileDownload/nph-download?forceDownload=1&url=" 
	             + me.imgData.file;

            window.open(url, "_self");
        }
        else
        {
	    if ((me.cmd != "waveplot") &&
	        (me.cmd != "impick")) {
                
	        if (me.debug) {
	            console.log ("get new JPEG url");
                }

                me.gc.clear();

                me.gc.refitCanvas();

                if(me.debug)
                    console.log("DEBUG> Retrieving new JPEG.");

                me.xminImg = me.imgData.xmin;
                me.xmaxImg = me.imgData.xmax;
                me.yminImg = me.imgData.ymin;
                me.ymaxImg = me.imgData.ymax;
                me.scale = me.imgData.scale;

                if(me.debug) {
                    console.log("DEBUG> xminImg= " + me.xminImg);
                    console.log("DEBUG> xmaxImg= " + me.xmaxImg);
                    console.log("DEBUG> yminImg= " + me.yminImg);
                    console.log("DEBUG> ymaxImg= " + me.ymaxImg);
                    console.log("DEBUG> scale= " + me.scale);
                }

	        var newjpg = me.imgData.file;
	        me.gc.setImage(newjpg + "?seed=" + (new Date()).valueOf());
                
		if(me.debug) {
                    console.log("DEBUG> newjpg= " + newjpg);
                }
            
            }
        }

      
      // If there are any registered update callbacks, 
      // call them now.

        if(me.debug) {
            console.log("DEBUG> cmd= " + me.cmd);
        }

        for(i=0; i<me.updateCallbacks.length; ++i)
        {
            me.updateCallbacks[i]();
        }

        me.grayOutMessage(false);
        me.grayOut(false);

        me.updateInProgress = false;
    
	if (me.debug) {
	    console.log ("processUpdate done");
        }
       
    }


   // Other objects (mostly controls but we'll leave it open)
   // want to be notified when an image display update occurs.
   // Here we manage a list of these object.methods.

   me.addUpdateCallback = function(method)
   {
      me.updateCallbacks.push(method);
   }


   // Display area resize event callback.  This is a pair
   // of functions:  The first one (the entry point) starts
   // a timer and eventually calls the second (used by
   // no one else) to do the real work.

   me.resizeTimeout = 0;

   me.resize = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.resize()");

      if(me.resizeTimeout)
         clearTimeout(me.resizeTimeout);

      me.resizeTimeout = setTimeout(me.resizeImageRequest, me.timeoutValue);
   }


   me.resizeImageRequest = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.resizeImageRequest()");

      me.resizeTimeout = 0;

      var areaHeight = jQuery(me.imgDiv).height();
      var areaWidth  = jQuery(me.imgDiv).width();

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
        
	var cubedata = me.imgData.imcubeFile;
	if (cubedata == null) {
        
            datastr = "workspace=" + me.workspace + "&file=" 
	        + me.imgData.grayFile.fitsFile;
        }
	else {
            if (me.debug)
                console.log ("mViewer.showFitshdr: cube data");

            var cubeOrig = me.imgData.imcubeFile.fitsFileOrig;

	    if (cubeOrig == null) {
                datastr = "workspace=" + me.workspace + "&file=" 
	            + me.imgData.imcubeFile.fitsFile;
	    }
	    else {
                datastr = "workspace=" + me.workspace + "&file=" 
	            + me.imgData.imcubeFile.fitsFileOrig;
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
	        "toolbar=no,directories=no,location=no,status=yes,menubar=no,resizeable=yes,scrollbars=yes,width=640,height=600");

            if (me.debug) {
	        console.log ("here1");
	    }

	    return;
	}

    }


   
   
   // Zoom in

   me.zoomIn = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.zoomIn()");

      if (!me.suppressGrayout) {
        
          me.grayOutMessage(true);
          me.grayOut(true);
      }

      if(me.debug)
      {
         console.log("DEBUG> updateJSON.grayFile.xs: " 
	     + me.updateJSON.grayFile.xs);
         console.log("DEBUG> updateJSON.grayFile.xe: " 
	     + me.updateJSON.grayFile.xe);
         console.log("DEBUG> updateJSON.grayFile.ys: " 
	     + me.updateJSON.grayFile.ys);
         console.log("DEBUG> updateJSON.grayFile.ye: " 
	     + me.updateJSON.grayFile.ye);
      }

      me.submitUpdateRequest("zoomin");
   }


   // Zoom out

   me.zoomOut = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.zoomOut()");

      if (!me.suppressGrayout) {
        
          me.grayOutMessage(true);
          me.grayOut(true);
      }

      if(me.debug)
      {
         console.log("DEBUG> updateJSON.grayFile.xs: " 
	     + me.updateJSON.grayFile.xs);
         console.log("DEBUG> updateJSON.grayFile.xe: " 
	     + me.updateJSON.grayFile.xe);
         console.log("DEBUG> updateJSON.grayFile.ys: " 
	     + me.updateJSON.grayFile.ys);
         console.log("DEBUG> updateJSON.grayFile.ye: " 
	     + me.updateJSON.grayFile.ye);
      }

      me.submitUpdateRequest("zoomout");
   }


   // Zoom reset

   me.zoomReset = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.zoomReset()");

      if (!me.suppressGrayout) {
        
          me.grayOutMessage(true);
          me.grayOut(true);
      }

      me.updateJSON.xmin = 0.;
      me.updateJSON.xmax = me.updateJSON.grayFile.imageWidth;
      me.updateJSON.ymin = 0.;
      me.updateJSON.ymax = me.updateJSON.grayFile.imageHeight;

      me.submitUpdateRequest("resetzoom");
   }


/*
    stretch reset
*/
   me.stretchReset = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.stretchReset()");

	me.updateJSON.grayFile.stretchMin = me.initJSON.grayFile.stretchMin;
        me.updateJSON.grayFile.stretchMax = me.initJSON.grayFile.stretchMax;
        me.updateJSON.grayFile.stretchMode = me.initJSON.grayFile.stretchMode;

        me.submitUpdateRequest();
   }

   

   // Download the image

    me.downloadImage = function(fileurl)
    {
        if(me.debug) {
            console.log("DEBUG> mViewer.downloadImage()");
            console.log ("me.imgData.file= " + me.imgData.file);
            console.log ("fileurl= " + fileurl);
        }
        
	var imurl;
	if (fileurl == null) 
	    imurl = me.imgData.file;
        else
	    imurl = fileurl;

        if (!window.location.origin)
            window.location.origin 
	        = window.location.protocol+"//"+window.location.host;

        var url = window.location.origin 
            + "/cgi-bin/FileDownload/nph-download?forceDownload=1&url=" 
	    + imurl; 

        window.open(url, "_self");
    }


   // GRAPHICS CALLBACKS

   me.zoomCallback = function(xmin, ymin, xmax, ymax)
   {
      var tmp;

      if(me.debug)
         console.log("DEBUG> mViewer.zoomCallback()");

      if (!me.suppressGrayout) {
        
          me.grayOutMessage(true);
          me.grayOut(true);
      }

      if(xmin > xmax)
      {
         tmp  = xmin;
         xmin = xmax;
         xmax = tmp;
      }

      if(xmin == xmax)
         xmax = xmin + 1.e-9;

      if(ymin > ymax)
      {
         tmp  = ymin;
         ymin = ymax;
         ymax = tmp;
      }

      if(ymin == ymax)
         ymax = ymin + 1.e-9;


      if(me.debug)
      {
         console.log("DEBUG> min (screen): " + xmin + ", " + ymin);
         console.log("DEBUG> max (screen): " + xmax + ", " + ymax);
      }

      me.updateJSON.xmin = xmin;
      me.updateJSON.xmax = xmax;
      me.updateJSON.ymin = ymin;
      me.updateJSON.ymax = ymax;

      me.submitUpdateRequest("zoombox");
       
   }



   // If there are any registered update callbacks, 
   // we call them here.

    me.clickCallback = function(x, y)
    {
        if(me.debug)
        {
            console.log("DEBUG> me.clickCallback: x= " + x + " y= " + y);
        }
        
	me.updateJSON.grayFile.xs = x;
	me.updateJSON.grayFile.ys = y;
	me.updateJSON.grayFile.xe = x;
	me.updateJSON.grayFile.ye = y;
        
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

	me.updateJSON.grayFile.xs = xs;
	me.updateJSON.grayFile.ys = ys;
	me.updateJSON.grayFile.xe = xe;
	me.updateJSON.grayFile.ye = ye;
        
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



   /* Gray-Out Functions */

   me.grayOut = function(vis, options)
   {
      // Pass true to gray out screen, false to ungray.  Options are optional.
      // This is a JSON object with the following (optional) properties:
      //
      //    opacity:0-100         // Lower number = less grayout higher = more of a blackout
      //    zindex: #             // HTML elements with a higher zindex appear on top of the gray out
      //    bgcolor: (#xxxxxx)    // Standard RGB Hex color code
      //
      // e.g.,  me.grayOut(true, {'zindex':'50', 'bgcolor':'#0000FF', 'opacity':'70'});
      //
      // Because 'options' is JSON, opacity/zindex/bgcolor are all optional and can appear
      // in any order.  Pass only the properties you need to set.

      var options = options || {};
      var zindex  = options.zindex || 5000;
      var opacity = options.opacity || 70;
      var opaque  = (opacity / 100);
      var bgcolor = options.bgcolor || '#000000';

      var dark = document.getElementById('darkenScreenObject');

      if(!dark)
      {
         // The dark layer doesn't exist, it's never been created.  So we'll
         // create it here and apply some basic me.styles.

         var tbody = document.getElementsByTagName("body")[0];

         var tnode = document.createElement('div');

         tnode.style.position ='absolute';             // Position absolutely
         tnode.style.top      ='0px';                  // In the top
         tnode.style.left     ='0px';                  // Left corner of the page
         tnode.style.overflow ='hidden';               // Try to avoid making scroll bars
         tnode.style.display  ='none';                 // Start out Hidden
         tnode.id             ='darkenScreenObject';   // Name it so we can find it later

         tbody.appendChild(tnode);                     // Add it to the web page

         dark = document.getElementById('darkenScreenObject');  // Get the object.
      }

      if(vis)
      {
         // Calculate the page width and height

         if( document.body && ( document.body.scrollWidth || document.body.scrollHeight ) )
         {
            var pageWidth  =  document.body.scrollWidth+'px';
            var pageHeight = (document.body.scrollHeight+1000)+'px';
         }
         else if( document.body.offsetWidth )
         {
            var pageWidth  =  document.body.offsetWidth+'px';
            var pageHeight = (document.body.offsetHeight+1000)+'px';
         }
         else
         {
            var pageWidth  = '100%';
            var pageHeight = '100%';
         }


         // Set the shader to cover the entire page and make it visible.

         dark.style.opacity         = opaque;
         dark.style.MozOpacity      = opaque;
         dark.style.filter          = 'alpha(opacity='+opacity+')';
         dark.style.zIndex          = zindex;
         dark.style.backgroundColor = bgcolor;
         dark.style.width           = pageWidth;
         dark.style.height          = pageHeight;
         dark.style.display         = 'block';
      }

      else
         dark.style.display='none';
   }



   /* "Please Wait" message with clock */

   me.grayOutMessage =function(vis)
   {
      var msg = document.getElementById('messageScreenObject');

      if(!msg)
      {
         var tbody = document.getElementsByTagName("body")[0];

         var tnode = document.createElement('div');

         tnode.style.width           = '230px';
         tnode.style.height          = '40px';
         tnode.style.position        = 'absolute';
         tnode.style.top             = '50%';
         tnode.style.left            = '50%';
         tnode.style.zIndex          = '5500';
         tnode.style.margin          = '-50px 0 0 -100px';
         tnode.style.backgroundColor = '#ffffff';
         tnode.style.display         = 'none';
         tnode.innerHTML             = '<img src="/applications/mViewer/waitClock.gif"/> &nbsp; Loading.  Please wait ... ';
         tnode.id                    = 'messageScreenObject';

         tbody.appendChild(tnode);

         msg = document.getElementById('messageScreenObject');
      }

      if(vis)
         msg.style.display = 'block';
      else
         msg.style.display = 'none';
   }


   me.closeChildWin = function () {
       
       if ((imDebugWin != null) &&
           (!imDebugWin.closed)) {
           imDebugWin.close(); 
       }
   
       if ((me.hdrWin != null) &&
           (!me.hdrWin.closed)) {
           me.hdrWin.close(); 
       }
   
   }


}
