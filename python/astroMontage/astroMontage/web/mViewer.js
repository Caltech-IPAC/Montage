function mViewer(client, imgDivID)
{
   var me = this;

   me.debug = false;

   me.client = client;

   me.timeoutVal = 150;


   me.imgDiv = document.getElementById(imgDivID);

   me.display = jQuery('<div/>').appendTo(me.imgDiv);

   me.display = jQuery(me.display).get(0);


   me.gc = new iceGraphics(me.display);

   me.canvasWidth  = jQuery(me.imgDiv).width();
   me.canvasHeight = jQuery(me.imgDiv).height();


   var resizeTimeout = 0;

   me.resize = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.resize()");

      if(resizeTimeout)
         clearTimeout(resizeTimeout);

      resizeTimeout = setTimeout(me.resizeFinal, me.timeoutVal);
   }


   me.resizeFinal = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.resizeFinal()");

      areaWidth  = jQuery(me.imgDiv).width();
      areaHeight = jQuery(me.imgDiv).height();

      jQuery(me.display).height(areaHeight);
      jQuery(me.display).width (areaWidth);

      if(me.gc != null)
      {
         me.gc.clear();
         me.gc.refitCanvas();
      }

      me.canvasWidth  = jQuery(me.imgDiv).width();
      me.canvasHeight = jQuery(me.imgDiv).height();

      var cmd = "resize "
              + me.canvasWidth  + " "
              + me.canvasHeight + " ";

      if(me.debug)
         console.log("DEBUG> cmd: " + cmd);

      me.client.send(cmd);
   }


   me.processUpdate = function(cmd)
   {
      if(me.debug)
         console.log("DEBUG> Processing: " + cmd);

      args = cmd.split(" ");

      var cmd = args[0];

      if(cmd == "image")
      {
         me.gc.clear();

         me.gc.refitCanvas();

         if(me.debug)
            console.log("DEBUG> Retrieving new JPEG.");

         me.gc.setImage(args[1] + "?seed=" + (new Date()).valueOf());
      }
   }


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


      me.canvasWidth  = jQuery(me.imgDiv).width();
      me.canvasHeight = jQuery(me.imgDiv).height();

      ymin = (me.canvasHeight - ymin);
      ymax = (me.canvasHeight - ymax);

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

      var cmd = "zoom "
              + xmin + " "
              + xmax + " "
              + ymin + " "
              + ymax;

      if(me.debug)
         console.log("DEBUG> cmd: " + cmd);

      me.client.send(cmd);
   }


   me.resetZoom = function()
   {
      var tmp;

      if(me.debug)
         console.log("DEBUG> mViewer.resetZoom()");

      me.grayOutMessage(true);
      me.grayOut(true);

      var cmd = "zoomReset"

      if(me.debug)
         console.log("DEBUG> cmd: " + cmd);

      me.client.send(cmd);
   }


   // If there are any registered update callbacks, 
   // we call them here.

   me.clickCallback = function(x, y)
   {
      me.canvasWidth  = jQuery(me.imgDiv).width();
      me.canvasHeight = jQuery(me.imgDiv).height();

      y = (me.canvasHeight - y);

      var cmd = "pick "
              + x + " "
              + y;

      if(me.debug)
         console.log("DEBUG> cmd: " + cmd);

      me.client.send(cmd);
   }


   // For now, we will stub out right click

   me.rightClickCallback = function(x, y)
   {
      // Do nothing.
   }


   me.gc.boxCallback        = me.zoomCallback;
   me.gc.clickCallback      = me.clickCallback;
   me.gc.rightClickCallback = me.rightClickCallback;



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
         // create it here and apply some basic styles.

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
         tnode.innerHTML             = '<img src="waitClock.gif"/> &nbsp; Loading.  Please wait ... ';
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
