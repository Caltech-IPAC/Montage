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
   
   me.workspace  = workspace;
   me.jsonFile   = jsonFile;
   me.imgDivID   = imgDivID;

   me.imgDiv = document.getElementById(imgDivID);

   me.showButtons = true;

   me.timeoutValue = 150; // milliseconds

   me.updateInProcess = false;

   me.initJSON   = {};
   me.updateJSON = {};
   me.imgData    = {};

   me.updateCallbacks = [];
   me.clickCallbacks  = [];

   me.display;
   me.gc;

   me.xminImg;
   me.xmaxImg;
   me.yminImg;
   me.ymaxImg;

   me.scale;


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
      if(me.debug)
         console.log("DEBUG> mViewer.init()");

      me.initJSON   = {};
      me.updateJSON = {};

      me.display = null;
      me.gc      = null;

      me.xminImg = 0;
      me.xmaxImg = 1;
      me.yminImg = 0;
      me.ymaxImg = 1;

      me.scale   = 1;

      jQuery('body').on('contextmenu', 'img',    function(e){ return false; });
      jQuery('body').on('contextmenu', 'canvas', function(e){ return false; });


      // We prevent colliding and redundant remote requests in two ways
      // First, resize events are handled with a short timeout to prevent
      // them firing in rapid succession as you resize the container 
      // (i.e., we try to act only on the final resized value).  Second,
      // once an remote update request is made, further updates are ignored.
      // This also mainly affects resizing since we try to disable (gray out)
      // all controls during the update processing.

      me.updateInProcess = false;


      // Sync the image div size up with it's container
      // The resize function usually does an automatic 
      // update request but at this point the canvas 
      // graphics context doesn't exist yet, so it won't
      // (which is a good thing since we don't have the 
      // JSON info yet).

      me.resize();


      // Create the internals mViewer needs inside the display <div>
      // This consists of a canvas for the image itself and another
      // one overlaying it for graphics.

      me.display = jQuery('<div/>').appendTo(me.imgDiv);

      me.display = jQuery(me.display).get(0);

      var rect = me.display.getBoundingClientRect();

      me.getJSONFile();
   }


   // The initial JSON file was created by whoever set up the
   // workspace and will be used both to generate the intial
   // display and to configure various controls.

   me.getJSONFile = function()
   {
      var xmlhttp;
      var paramURL;

      if(me.debug)
         console.log("DEBUG> mViewer.getJSONFile()");


      me.grayOut(true, {'opacity':'50'});
      me.grayOutMessage(true);

      paramURL = "/cgi-bin/FileDownload/nph-download"
                 + "?url=" + me.workspace + "/" + me.jsonFile;

      if(me.debug)
         console.log("DEBUG> mViewer.getJSONFile(): paramURL = [" + paramURL + "]");

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
               var error = xml.getElementsByTagName("error")[0].childNodes[0].nodeValue;

               me.grayOutMessage(false);
               me.grayOut(false);

               alert("Server Error: " + error);
            }

            else
            {
               me.paramText = xmlhttp.responseText;

               me.initJSON = jQuery.parseJSON(xmlhttp.responseText);

               me.updateJSON = me.initJSON;

               if(me.debug)
                  console.log("DEBUG> JSON download complete; updateJSON initialized.");

               me.gc = new iceGraphics(me.display);

               me.gc.boxCallback        = me.zoomCallback;
               me.gc.clickCallback      = me.clickCallback;
               me.gc.rightClickCallback = me.rightClickCallback;

               me.grayOutMessage(false);
               me.grayOut(false);

               me.submitUpdateRequest();
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
      if(me.debug)
         console.log("DEBUG> mViewer.replaceJSON()");

      me.updateJSON = json;

      me.submitUpdateRequest();
   }



   // Send a request off for an updated display JPEG
   // The POST call names "processUpdate()" as its 
   // callback.

   me.submitUpdateRequest = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.submitUpdateRequest()");

      me.grayOut(true, {'opacity':'50'});
      me.grayOutMessage(true);

      if(me.updateInProgress)
         return;

      me.updateInProgress = true;

      me.updateJSON.canvasWidth  = jQuery("#imgArea").width();
      me.updateJSON.canvasHeight = jQuery("#imgArea").height();

      jQuery.post("/cgi-bin/mViewer/nph-mViewer", "workspace=" + me.workspace + "&json=" + encodeURIComponent(JSON.stringify(me.updateJSON)), me.processUpdate, "json");
   }



   me.processUpdate = function(data, stat, jqXHR)
   {
      if(me.debug)
         console.log("DEBUG> processUpdate()");

      me.imgData = data;

      if(typeof(data.error) != "undefined")
      {
         me.requestSubmitted = false;

         me.grayOutMessage(false);
         me.grayOut(false);

         alert(data.error);

         me.updateInProgress = false;

         return;
      }

      if(me.downloadMode)
      {
         me.downloadMode = false;

         if (!window.location.origin)
            window.location.origin = window.location.protocol+"//"+window.location.host;

         var url = window.location.origin + "/cgi-bin/FileDownload/nph-download?forceDownload=1&url=" + data.file;

         window.open(url, "_self");
      }
      else
      {
         me.gc.clear();

         me.gc.refitCanvas();

         if(me.debug)
            console.log("DEBUG> Retrieving new JPEG.");

         me.xminImg = data.xmin;
         me.xmaxImg = data.xmax;
         me.yminImg = data.ymin;
         me.ymaxImg = data.ymax;

         me.scale = data.scale;

         me.gc.setImage(data.file + "?seed=" + (new Date()).valueOf());
      }

      
      // If there are any registered update callbacks, 
      // call them now.

      for(i=0; i<me.updateCallbacks.length; ++i)
      {
         me.updateCallbacks[i]();
      }


      me.grayOutMessage(false);
      me.grayOut(false);

      me.updateInProgress = false;
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

   // Zoom in

   me.zoomIn = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.zoomIn()");

      me.grayOutMessage(true);
      me.grayOut(true);

      xcenter = me.updateJSON.canvasWidth/2.;
      ycenter = me.updateJSON.canvasHeight/2.;

      xsize = me.updateJSON.canvasWidth/4.;
      ysize = me.updateJSON.canvasHeight/4.;

      xmin = xcenter - xsize;
      xmax = xcenter + xsize;

      ymin = ycenter - ysize;
      ymax = ycenter + ysize;

      if(me.debug)
      {
         console.log("DEBUG> min (screen): " + xmin + ", " + ymin);
         console.log("DEBUG> max (screen): " + xmax + ", " + ymax);
      }

      xminOrig = xmin * me.scale + me.xminImg;
      xmaxOrig = xmax * me.scale + me.xminImg;

      yminOrig = ymin * me.scale + me.yminImg;
      ymaxOrig = ymax * me.scale + me.yminImg;

      if(me.debug)
      {
         console.log("DEBUG> min (FITS image): " + xminOrig + ", " + yminOrig);
         console.log("DEBUG> max (FITS image): " + xmaxOrig + ", " + ymaxOrig);
      }

      me.updateJSON.xmin = xminOrig;
      me.updateJSON.xmax = xmaxOrig;
      me.updateJSON.ymin = yminOrig;
      me.updateJSON.ymax = ymaxOrig;


      me.submitUpdateRequest();
   }


   // Zoom out

   me.zoomOut = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.zoomOut()");

      me.grayOutMessage(true);
      me.grayOut(true);

      xcenter = me.updateJSON.canvasWidth/2.;
      ycenter = me.updateJSON.canvasHeight/2.;

      xsize = me.updateJSON.canvasWidth;
      ysize = me.updateJSON.canvasHeight;

      xmin = xcenter - xsize;
      xmax = xcenter + xsize;

      ymin = ycenter - ysize;
      ymax = ycenter + ysize;

      if(me.debug)
      {
         console.log("DEBUG> min (screen): " + xmin + ", " + ymin);
         console.log("DEBUG> max (screen): " + xmax + ", " + ymax);
      }

      xminOrig = xmin * me.scale + me.xminImg;
      xmaxOrig = xmax * me.scale + me.xminImg;

      yminOrig = ymin * me.scale + me.yminImg;
      ymaxOrig = ymax * me.scale + me.yminImg;

      if(me.debug)
      {
         console.log("DEBUG> min (FITS image): " + xminOrig + ", " + yminOrig);
         console.log("DEBUG> max (FITS image): " + xmaxOrig + ", " + ymaxOrig);
      }

      me.updateJSON.xmin = xminOrig;
      me.updateJSON.xmax = xmaxOrig;
      me.updateJSON.ymin = yminOrig;
      me.updateJSON.ymax = ymaxOrig;

      me.submitUpdateRequest();
   }


   // Zoom reset

   me.zoomReset = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.zoomReset()");

      me.grayOutMessage(true);
      me.grayOut(true);

      delete me.updateJSON.xmin;
      delete me.updateJSON.xmax;
      delete me.updateJSON.ymin;
      delete me.updateJSON.ymax;

      me.submitUpdateRequest();
   }


   // Download the image

   me.downloadImage = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.downloadImage()");

      if (!window.location.origin)
         window.location.origin = window.location.protocol+"//"+window.location.host;

      var url = window.location.origin + "/cgi-bin/FileDownload/nph-download?forceDownload=1&url=" + me.imgData.file;

      window.open(url, "_self");
   }


   // GRAPHICS CALLBACKS

   me.zoomCallback = function(xmin, ymin, xmax, ymax)
   {
      var tmp;

      if(me.debug)
         console.log("DEBUG> mViewer.zoomCallback()");

      me.grayOutMessage(true);
      me.grayOut(true);

      if(xmin > xmax)
      {
         tmp  = xmin;
         xmin = xmax;
         xmax = tmp;
      }

      if(xmin == xmax)
         xmax = xmin + 1.e-9;


      ymin = (me.updateJSON.canvasHeight - ymin);
      ymax = (me.updateJSON.canvasHeight - ymax);

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

      xminOrig = xmin * me.scale + me.xminImg;
      xmaxOrig = xmax * me.scale + me.xminImg;

      yminOrig = ymin * me.scale + me.yminImg;
      ymaxOrig = ymax * me.scale + me.yminImg;

      if(me.debug)
      {
         console.log("DEBUG> min (FITS image): " + xminOrig + ", " + yminOrig);
         console.log("DEBUG> max (FITS image): " + xmaxOrig + ", " + ymaxOrig);
      }

      me.updateJSON.xmin = xminOrig;
      me.updateJSON.xmax = xmaxOrig;
      me.updateJSON.ymin = yminOrig;
      me.updateJSON.ymax = ymaxOrig;

      me.submitUpdateRequest();
   }



   // If there are any registered update callbacks, 
   // we call them here.

   me.clickCallback = function(x, y)
   {
      y = (me.updateJSON.canvasHeight - y);

      var xOrig = x * me.scale + me.xminImg;
      var yOrig = y * me.scale + me.yminImg;

      for(i=0; i<me.clickCallbacks.length; ++i)
         me.clickCallbacks[i](xOrig, yOrig);
   }
   


   // Other objects (mostly controls but we'll leave it open)
   // want to be notified when the user clicks on the image.
   // Here we manage a list of these object.methods.

   me.addClickCallback = function(method)
   {
      me.clickCallbacks.push(method);
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
}
