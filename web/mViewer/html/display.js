/***********************/
/* mViewer Constructor */
/***********************/

function mViewer(workspace, json)
{
   me = this;

   me.debug = false;

   if(me.debug)
      console.log("DEBUG> mViewer() constructor  <<<<<<<<<<");

   me.workspace;
   me.initJSON;
   me.form;

   me.userGuideURL = null;

   me.displayDiv = "mViewer";

   me.autoResize   = true;
   me.westResizing = false;

   me.pageLayout;
   me.containerDiv;
   me.gc;
   me.sc;

   me.prec = 4;

   me.timeoutVal = 150;

   me.workspace = workspace;
   me.initJSON  = json;

   if(me.debug)
   {
      console.log("DEBUG> workspace: " + me.workspace);
      console.log("DEBUG> intJSON:   " + me.initJSON);
   }


   // MAIN SETUP Function. 

   me.init = function()
   {
      if(me.debug)
         console.log("DEBUG> mViewer.init()  <<<<<<<<<<");

      if(me.userGuideURL)
         jQuery("#userGuideMsg").html("<a href='" + me.userGuideURL + "'>User's Guide</a>");

      me.initialized = false;

      me.updateMode = "initial";
      
      if(me.debug)
         console.log("DEBUG> 'me.initialized' set to false");

      jQuery('body').on('contextmenu', 'img',    function(e){ return false; });
      jQuery('body').on('contextmenu', 'canvas', function(e){ return false; });

      me.pageLayout = jQuery("#plotLayout").layout({

         west_onresize_end: function () 
         {
            me.westResizing = false; 
         },

         center__onresize: function () 
         {
            if(me.initialized && me.autoResize)
               me.resizeImage(); 
         },

         west__onresize:   function () 
         {
            var currentTab;
            var currentPlot;

            me.westResizing = true;

            var width = jQuery("#controlDiv").parent().parent().parent().parent().width();

            jQuery("#controlDiv").parent().parent().parent().width(width);
            jQuery("#controlDiv").parent().parent().width(width);
            jQuery("#controlDiv").parent().width(width);
            jQuery("#controlDiv").width(width);
               
            me.westResizing = false;
         }
      });

      me.gc = new iceGraphics(me.displayDiv);

      me.gc.boxCallback        = me.zoomCallback;
      me.gc.clickCallback      = me.clickCallback;
      me.gc.rightClickCallback = me.rightClickCallback;
      
      me.resizeImageFinal();
   }


   // We started (above) with getting the
   // list of datasets in the workspace.
   // This function starts the plot parameter
   // (JSON) retrieval, telling the retrieval
   // function to call the form initialization
   // when done.  This cascading of processing
   // is necessary because each step involves an
   // asynchronous remote retrieval.

   me.getdDisplayJSON = function()
   {
      var xmlhttp;
      var paramURL;

      if(me.debug)
         console.log("DEBUG> getParamInit()");


      me.grayOut(true, {'opacity':'50'});
      me.grayOutMessage(true);

      if(me.prefix.length > 0)
         paramURL = "/cgi-bin/FileDownload/nph-download"
                    + "?url=" + me.workspace + "/" + me.prefix + "_params.json";
      else
         paramURL = "/cgi-bin/FileDownload/nph-download"
                    + "?url=" + me.workspace + "/params.json";

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

               me.initParam = jQuery.parseJSON(xmlhttp.responseText);

               me.initParam.jpeg.width  = jQuery("#iceImg").width();
               me.initParam.jpeg.height = jQuery("#iceImg").height();

               me.currentParam = me.initParam;

               me.ndataset = me.initParam.pointset.length;

               me.plotContext = "getPlotJSON";

               me.submitPlotRequest();

               me.initializeForms();
            }
         }

         else if(xmlhttp.status != 200)
         {
            me.grayOutMessage(false);
            me.grayOut(false);

            alert("Remote service error[1].");
         }
      }
   }


   // This concludes the initialization.  We started
   // by getting the list of available data files, then
   // got the JSON structure defining the initial plot.
   // Here we use all this to populate the GUI forms
   // and then generate the initial plot.

   me.initializeForms = function()
   {
      if(me.debug)
         console.log("DEBUG> me.initializeForms()");

      me.setParams(me.initParam);
   }



   // Reset div me.sizes based on window size

   var resizeTimeout = 0;

   me.resizeImage = function()
   {
      if(me.debug)
         console.log("DEBUG> me.resizeImage()");

      if(resizeTimeout)
         clearTimeout(resizeTimeout);

      resizeTimeout = setTimeout(me.resizeImageFinal, me.timeoutVal);
   }

   me.resizeImageFinal = function() 
   {
      if(me.debug)
         console.log("DEBUG> me.resizeImageFinal()");


      // layoutCenter size (resized automatically by jQuery layout) 

      var bannerHeight = jQuery("#bannerArea"  ).height();

      var layoutHeight = jQuery("#layoutCenter").height();
      var layoutWidth  = jQuery("#layoutCenter").width();

      if(me.debug)
      {
         console.log("DEBUG> layoutHeight = " + layoutHeight);
         console.log("DEBUG> layoutWidth  = " + layoutWidth);
      }

      var imgHeight = layoutHeight - bannerHeight - 70;
      var imgWidth  = layoutWidth;

      if(me.form == "minimal")
         imgHeight = layoutHeight;

      if(me.debug)
      {
         console.log("DEBUG> imgHeight    = " + imgHeight);
         console.log("DEBUG> imgWidth     = " + imgWidth);
      }

      jQuery("#imgContainer").css("height", imgHeight + "px");
      jQuery("#imgContainer").css("width",  imgWidth  + "px");

      jQuery("#iceImg").css("height", imgHeight + "px");
      jQuery("#iceImg").css("width",  imgWidth  + "px");

      me.updateMode = "resize";

      if(me.initialized)
      {
         if(me.debug)
            console.log("DEBUG> updatePlot() from resizeImageFinal()");

         me.updatePlot();
      }

      me.gc.refitCanvas();
   }


   me.hideDatasetsDialog = function()
   {
      if(me.debug)
         console.log("DEBUG> me.hideDatasetsDialog()");

      jQuery("#datasetsDiv").parent().hide();

      me.multiControls = false;
   }


   // Zoom (Box Drawing) Callback

   me.zoomCallback = function(xmin, ymin, xmax, ymax)
   {
      var tmp;

      if(me.debug)
         console.log("DEBUG> me.zoomCallback()");


      if(xmin > xmax)
      {
         tmp  = xmin;
         xmin = xmax;
         xmax = tmp;
      }

      if(xmin == xmax)
         xmax = xmin + 1.e-9;


      if(ymin < ymax)
      {
         tmp  = ymin;
         ymin = ymax;
         ymax = tmp;
      }

      if(ymin == ymax)
         ymax = ymin + 1.e-9;


      if(me.debug)
      {
         console.log("DEBUG> min (pixels): " + xmin + ", " + ymin);
         console.log("DEBUG> max (pixels): " + xmax + ", " + ymax);
      }

      var x0 = xmin*sc.xpscale + sc.xpoffset;
      var y0 = ymin*sc.ypscale + sc.ypoffset;

      x0 = x0*sc.xscale + sc.xoffset;
      y0 = y0*sc.yscale + sc.yoffset;

      if(sc.xtype == "LOG")
         x0 = Math.pow(10.,x0);

      if(sc.ytype == "LOG")
         y0 = Math.pow(10.,y0);


      if(me.debug)
         console.log("DEBUG> min (data): " + x0 + ", " + y0);


      x1 = xmax*sc.xpscale + sc.xpoffset;
      y1 = ymax*sc.ypscale + sc.ypoffset;

      x1 = x1*sc.xscale + sc.xoffset;
      y1 = y1*sc.yscale + sc.yoffset;

      if(sc.xtype == "LOG")
         x1 = Math.pow(10.,x1);

      if(sc.ytype == "LOG")
         y1 = Math.pow(10.,y1);


      if(me.debug)
         console.log("DEBUG> max (data): " + x1 + ", " + y1);


      if(x0 > x1)
      {
         if(me.debug)
            console.log("DEBUG> flip x ");

         var tmp = x0;

         x0 = x1;

         x1 = tmp;
      }


      if(y0 > y1)
      {
         if(me.debug)
            console.log("DEBUG> flip y ");

         var tmp = y0;

         y0 = y1;

         y1 = tmp;
      }


      if(me.plotMode == "scatter")
      {
         jQuery("#xScaleMin").val(x0.toPrecision(me.prec));
         jQuery("#xScaleMax").val(x1.toPrecision(me.prec));

         jQuery("#xScaleMin").show();
         jQuery("#xScaleMax").show();
         
         jQuery("#xScaleMinLbl").show();
         jQuery("#xScaleMaxLbl").show();
         

         jQuery("#yScaleMin").val(y0.toPrecision(me.prec));
         jQuery("#yScaleMax").val(y1.toPrecision(me.prec));
         
         jQuery("#yScaleMin").show();
         jQuery("#yScaleMax").show();

         jQuery("#yScaleMinLbl").show();
         jQuery("#yScaleMaxLbl").show();


         me.changePulldown("xType", "manual");
         me.changePulldown("yType", "manual");

         me.plotContext = "zoomCallback";
      }
      else  // me.plotMode == "histogram"
      {
         me.changePulldown("histDataType", "manual");

         jQuery("#histDataScaleMin").val(x0.toPrecision(me.prec));
         jQuery("#histDataScaleMax").val(x1.toPrecision(me.prec));
         
         jQuery("#histDataScaleMin").show();
         jQuery("#histDataScaleMax").show();

         jQuery("#histDataScaleMinLbl").show();
         jQuery("#histDataScaleMaxLbl").show();
         
         var ymode = jQuery("#histCountType").val();
         
         if(ymode == "manual")
         {
            jQuery("#histCountScaleMin").val(y0.toPrecision(me.prec));
            jQuery("#histCountScaleMax").val(y1.toPrecision(me.prec));

            jQuery("#histCountScaleMin").show();
            jQuery("#histCountScaleMax").show();

            jQuery("#histCountScaleMinLbl").show();
            jQuery("#histCountScaleMaxLbl").show();
         }

         me.plotContext = "zoomCallback";
      }

      me.updateMode = "zoomCallback";

      me.checkHistLink();

      me.updatePlot();

      me.manualScaleWarning();
   }


   // Click Callback (not used yet)

   me.clickCallback = function(x, y)
   {
      console.log("DEBUG> DEBUG> click: (" + x + "," + y + ")");
   }


   // Right Click - PNG Download

   me.rightClickCallback = function(x, y)
   {
      me.updateMode = "rightClick";

      me.downloadMode = true;
      
      me.updatePlot();
   }


   // Zoom buttons

   me.zoomIn = function()
   {
      var xcenter, ycenter;
      var xsize, ysize;

      var xdatamin;
      var xdatamax;

      var xscalemin;
      var xscalemax;

      var xscale;
      var xmode ;


      var ydatamin;
      var ydatamax;

      var yscale;
      var ymode ;

      var yscalemin;
      var yscalemax;


      if(me.plotMode == "scatter")
      {
         xdatamin = jQuery("#xDataMin").html();
         xdatamax = jQuery("#xDataMax").html();

         var tmpval = jQuery("#xScaleMin").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#xScaleMin").val(xdatamin.toPrecision(me.prec));

         tmpval = jQuery("#xScaleMax").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#xScaleMax").val(xdatamax.toPrecision(me.prec));

         xscalemin = jQuery("#xScaleMin").val();
         xscalemax = jQuery("#xScaleMax").val();

         xscale = jQuery("#xScaling").val();
         xmode  = jQuery("#xType").val();

         //-----

         ydatamin = jQuery("#yDataMin").html();
         ydatamax = jQuery("#yDataMax").html();

         tmpval = jQuery("#yScaleMin").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#yScaleMin").val(ydatamin.toPrecision(me.prec));

         tmpval = jQuery("#yScaleMax").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#yScaleMax").val(ydatamax.toPrecision(me.prec));

         yscale = jQuery("#yScaling").val();
         ymode  = jQuery("#yType").val();

         yscalemin = jQuery("#yScaleMin").val();
         yscalemax = jQuery("#yScaleMax").val();
      }

      else  // me.plotMode == "histogram"
      {
         xdatamin = jQuery("#histDataMin").html();
         xdatamax = jQuery("#histDataMax").html();

         var tmpval = jQuery("#histDataScaleMin").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#histDataScaleMin").val(xdatamin.toPrecision(me.prec));

         tmpval = jQuery("#histDataScaleMax").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#histDataScaleMax").val(xdatamax.toPrecision(me.prec));

         xscalemin = jQuery("#histDataScaleMin").val();
         xscalemax = jQuery("#histDataScaleMax").val();

         xscale = jQuery("#histDataScaling").val();
         xmode  = jQuery("#histDataType").val();

         //-----

         me.checkHistCount();

         yscalemin = jQuery("#histCountScaleMin").val();
         yscalemax = jQuery("#histCountScaleMax").val();

         yscale = jQuery("#histCountScaling").val();
         ymode  = jQuery("#histCountType").val();

         me.checkHistLink();
      }

      
      // This mode common to both modes

      if(xscale == "log"
      || xscale == "logFlip")
      {
         xdatamin  = Math.log(xdatamin);
         xdatamax  = Math.log(xdatamax);

         xscalemin = Math.log(xscalemin);
         xscalemax = Math.log(xscalemax);
      }

      xcenter = xscalemin/2. + xscalemax/2.;
      xsize   = xscalemax/2. - xscalemin/2.;

      xsize = xsize / 2.;

      xscalemin = xcenter - xsize;
      xscalemax = xcenter + xsize;

      if(xscale == "log"
      || xscale == "logFlip")
      {
         xscalemin = Math.exp(xscalemin);
         xscalemax = Math.exp(xscalemax);
      }

      //-----

      if(yscale == "log"
      || yscale == "logFlip")
      {
         ydatamin  = Math.log(ydatamin);
         ydatamax  = Math.log(ydatamax);

         yscalemin = Math.log(yscalemin);
         yscalemax = Math.log(yscalemax);
      }

      ycenter = yscalemin/2. + yscalemax/2.;
      ysize   = yscalemax/2. - yscalemin/2.;

      ysize = ysize / 2.;

      yscalemin = ycenter - ysize;
      yscalemax = ycenter + ysize;

      if(yscale == "log"
      || yscale == "logFlip")
      {
         yscalemin = Math.exp(yscalemin);
         yscalemax = Math.exp(yscalemax);
      }


      // Update the GUI

      if(me.plotMode == "scatter")
      {
         me.changePulldown("xType", "manual");
         
         jQuery("#xScaleMinLbl").show();
         jQuery("#xScaleMaxLbl").show();
         
         jQuery("#xScaleMin").show();
         jQuery("#xScaleMax").show();

         jQuery("#xScaleMin").val(xscalemin.toPrecision(me.prec));
         jQuery("#xScaleMax").val(xscalemax.toPrecision(me.prec));

         me.changePulldown("yType", "manual");
         
         jQuery("#yScaleMinLbl").show();
         jQuery("#yScaleMaxLbl").show();
         
         jQuery("#yScaleMin").show();
         jQuery("#yScaleMax").show();

         jQuery("#yScaleMin").val(yscalemin.toPrecision(me.prec));
         jQuery("#yScaleMax").val(yscalemax.toPrecision(me.prec));
      }

      else  // me.plotMode == "histogram"
      {
         me.changePulldown("histDataType", "manual");
         
         jQuery("#histDataScaleMinLbl").show();
         jQuery("#histDataScaleMaxLbl").show();
         
         jQuery("#histDataScaleMin").show();
         jQuery("#histDataScaleMax").show();

         jQuery("#histDataScaleMin").val(xscalemin.toPrecision(me.prec));
         jQuery("#histDataScaleMax").val(xscalemax.toPrecision(me.prec));

         var ymode = jQuery("#histCountType").val();
         
         if(ymode == "manual")
         {
            jQuery("#histCountScaleMin").val(yscalemin.toPrecision(me.prec));
            jQuery("#histCountScaleMax").val(yscalemax.toPrecision(me.prec));
         }
      }

      me.updateMode = "zoomIn";

      if(me.debug)
         console.log("DEBUG> updatePlot() from zoomIn()");
      
      me.checkHistLink();

      me.updatePlot();

      me.manualScaleWarning();
   }


   me.zoomOut = function()
   {
      var xcenter, ycenter;
      var xsize, ysize;

      var xdatamin;
      var xdatamax;

      var xscalemin;
      var xscalemax;

      var xscale;
      var xmode ;


      var ydatamin;
      var ydatamax;

      var yscale;
      var ymode ;

      var yscalemin;
      var yscalemax;


      if(me.plotMode == "scatter")
      {
         xdatamin = jQuery("#xDataMin").html();
         xdatamax = jQuery("#xDataMax").html();

         var tmpval = jQuery("#xScaleMin").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#xScaleMin").val(xdatamin.toPrecision(me.prec));

         tmpval = jQuery("#xScaleMax").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#xScaleMax").val(xdatamax.toPrecision(me.prec));

         xscalemin = jQuery("#xScaleMin").val();
         xscalemax = jQuery("#xScaleMax").val();

         xscale = jQuery("#xScaling").val();
         xmode  = jQuery("#xType").val();

         //-----

         ydatamin = jQuery("#yDataMin").html();
         ydatamax = jQuery("#yDataMax").html();

         tmpval = jQuery("#yScaleMin").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#yScaleMin").val(ydatamin.toPrecision(me.prec));

         tmpval = jQuery("#yScaleMax").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#yScaleMax").val(ydatamax.toPrecision(me.prec));

         yscale = jQuery("#yScaling").val();
         ymode  = jQuery("#yType").val();

         yscalemin = jQuery("#yScaleMin").val();
         yscalemax = jQuery("#yScaleMax").val();
      }

      else  // me.plotMode == "histogram"
      {
         xdatamin = jQuery("#histDataMin").html();
         xdatamax = jQuery("#histDataMax").html();

         var tmpval = jQuery("#histDataScaleMin").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#histDataScaleMin").val(xdatamin.toPrecision(me.prec));

         tmpval = jQuery("#histDataScaleMax").val();

         if(isNaN(tmpval) || tmpval.trim() == "")
            jQuery("#histDataScaleMax").val(xdatamax.toPrecision(me.prec));

         xscalemin = jQuery("#histDataScaleMin").val();
         xscalemax = jQuery("#histDataScaleMax").val();

         xscale = jQuery("#histDataScaling").val();
         xmode  = jQuery("#histDataType").val();

         //-----

         me.checkHistCount();

         yscalemin = jQuery("#histCountScaleMin").val();
         yscalemax = jQuery("#histCountScaleMax").val();

         yscale = jQuery("#histCountScaling").val();
         ymode  = jQuery("#histCountType").val();

         me.checkHistLink();
      }

      
      // This code common to both modes

      if(xscale == "log"
      || xscale == "logFlip")
      {
         xdatamin  = Math.log(xdatamin);
         xdatamax  = Math.log(xdatamax);

         xscalemin = Math.log(xscalemin);
         xscalemax = Math.log(xscalemax);
      }

      xcenter = xscalemin/2. + xscalemax/2.;
      xsize   = xscalemax/2. - xscalemin/2.;

      xsize = 2. * xsize;

      xscalemin = xcenter - xsize;
      xscalemax = xcenter + xsize;

      if(xscale == "log"
      || xscale == "logFlip")
      {
         xscalemin = Math.exp(xscalemin);
         xscalemax = Math.exp(xscalemax);
      }

      //-----

      if(yscale == "log"
      || yscale == "logFlip")
      {
         ydatamin  = Math.log(ydatamin);
         ydatamax  = Math.log(ydatamax);

         yscalemin = Math.log(yscalemin);
         yscalemax = Math.log(yscalemax);
      }

      ycenter = yscalemin/2. + yscalemax/2.;
      ysize   = yscalemax/2. - yscalemin/2.;

      ysize = 2. * ysize;

      yscalemin = ycenter - ysize;
      yscalemax = ycenter + ysize;

      if(yscale == "log"
      || yscale == "logFlip")
      {
         yscalemin = Math.exp(yscalemin);
         yscalemax = Math.exp(yscalemax);
      }


      // Update the GUI

      if(me.plotMode == "scatter")
      {
         me.changePulldown("xType", "manual");
         
         jQuery("#xScaleMinLbl").show();
         jQuery("#xScaleMaxLbl").show();
         
         jQuery("#xScaleMin").show();
         jQuery("#xScaleMax").show();

         jQuery("#xScaleMin").val(xscalemin.toPrecision(me.prec));
         jQuery("#xScaleMax").val(xscalemax.toPrecision(me.prec));

         me.changePulldown("yType", "manual");
         
         jQuery("#yScaleMinLbl").show();
         jQuery("#yScaleMaxLbl").show();
         
         jQuery("#yScaleMin").show();
         jQuery("#yScaleMax").show();

         jQuery("#yScaleMin").val(yscalemin.toPrecision(me.prec));
         jQuery("#yScaleMax").val(yscalemax.toPrecision(me.prec));
      }

      else  // me.plotMode == "histogram"
      {
         me.changePulldown("histDataType", "manual");
         
         jQuery("#histDataScaleMinLbl").show();
         jQuery("#histDataScaleMaxLbl").show();
         
         jQuery("#histDataScaleMin").show();
         jQuery("#histDataScaleMax").show();

         jQuery("#histDataScaleMin").val(xscalemin.toPrecision(me.prec));
         jQuery("#histDataScaleMax").val(xscalemax.toPrecision(me.prec));

         var ymode = jQuery("#histCountType").val();
         
         if(ymode == "manual")
         {
            jQuery("#histCountScaleMin").val(yscalemin.toPrecision(me.prec));
            jQuery("#histCountScaleMax").val(yscalemax.toPrecision(me.prec));
         }
      }

      me.updateMode = "zoomOut";

      if(me.debug)
         console.log("DEBUG> updatePlot() from zoomIn()");
      
      me.checkHistLink();

      me.updatePlot();

      me.manualScaleWarning();
   }


   // Resetting the zoom is essentially redoing the 
   // me.form intialization, but just for the axes params.

   me.resetZoom = function()
   {
      if(me.debug)
         console.log("DEBUG> zoomReset()");

      if(me.plotMode == "scatter")
      {
         me.changePulldown("xType", "automatic");

         jQuery("#xScaleMinLbl").hide();
         jQuery("#xScaleMaxLbl").hide();

         jQuery("#xScaleMin").hide();
         jQuery("#xScaleMax").hide();

         me.changePulldown("yType", "automatic");

         jQuery("#yScaleMinLbl").hide();
         jQuery("#yScaleMaxLbl").hide();

         jQuery("#yScaleMin").hide();
         jQuery("#yScaleMax").hide();
      }
      else  // me.plotMode == "histogram"
      {
         me.changePulldown("histDataType", "automatic");

         jQuery("#histDataScaleMinLbl").hide();
         jQuery("#histDataScaleMaxLbl").hide();

         jQuery("#histDataScaleMin").hide();
         jQuery("#histDataScaleMax").hide();

         jQuery("#histDataScaleMin").val((jQuery("#histDataMin").html()*1.).toPrecision(me.prec));
         jQuery("#histDataScaleMax").val((jQuery("#histDataMax").html()*1.).toPrecision(me.prec));


         me.changePulldown("histCountType", "automatic");

         jQuery("#histCountScaleMinLbl").hide();
         jQuery("#histCountScaleMaxLbl").hide();

         jQuery("#histCountScaleMin").hide();
         jQuery("#histCountScaleMax").hide();
     
         jQuery("#histCountScaleMin").val(histCountDataMin);
         jQuery("#histCountScaleMax").val((histCountDataMax*1.025).toPrecision(me.prec));
      }

      me.updateMode = "resetZoom";

      if(me.debug)
         console.log("DEBUG> updatePlot() from resetZoom()");
      
      me.checkHistLink();

      me.updatePlot();

      me.manualScaleWarning();
   }



   // Gather the parameters and submit a new plot request.
   // Because there are potentially two asynchronous calls
   // in a row, we need to do this processing stepwise; 
   // first calling nph-icePlotHistogram if needed, then 
   // nph-icePlotter (remoteHistogram() and remotePlot() 
   // are asynchronous calls with processHistogram() and 
   // processUpdate() as callbacks). 

   // So the sequence is either
   //
   //    updatePlot -> submitPlotRequest 
   //               -> remoteHistogram -> processHistogram 
   //               -> remotePlot -> processUpdate
   // or just
   //
   //    updatePlot -> submitPlotRequest 
   //               -> remotePlot -> processUpdate
   //
   //
   // The first thing done in updatePlot() is to run
   // getParams(), which gathers all the plot parameters
   // and in the process decides whether the histogram
   // data on the server needs to be regenerated.
   //
   // When the plotter is in histogram mode (just showing
   // a single histogram) the processing is the same, 
   // though only one histogram is generated and the
   // parameters for the plot come from the histogram 
   // control tabs.


   me.redrawBtn = function()
   {
      me.updateMode = "redraw";

      me.checkHistLink();

      me.updatePlot();
   }

   me.updatePlot = function() 
   {
      if(me.debug)
         console.log("DEBUG> updatePlot()");

      if(me.requestSubmitted)
         return;
      
      me.requestSubmitted = true;

      me.grayOut(true, {'opacity':'50'});
      me.grayOutMessage(true);

      me.getParams();

      if(me.downloadMode)
      {
         me.saveParams = me.params;

         me.params.jpeg.width  = jQuery("#iceImg").width();
         me.params.jpeg.height = jQuery("#iceImg").height();

         var aspect = me.params.jpeg.height / me.params.jpeg.width;

         if(aspect < 1.)
         {
            me.params.jpeg.width  = 3000;
            me.params.jpeg.height = Math.round(aspect * me.params.jpeg.width);
         }
         else
         {
            me.params.jpeg.height  = 3000;
            me.params.jpeg.width  = Math.round(me.params.jpeg.height / aspect);
         }
      }

      me.currentParam = me.params;

      me.plotContext = "updatePlot";

      me.remotePlot();
   }


   // Called by me.updatePlot()

   me.remotePlot = function()
   {
      if(me.debug)
         console.log("DEBUG> Submit remotePlot() request (plotContext: " + me.plotContext + ")");

      if(!me.initialized)
      {
         me.initialized = true;
         
         if(me.debug)
            console.log("DEBUG> 'initialized' set to true");
      }


      // First add in any histograms requested
      // (obviously not in "histogram" mode)  

      if(me.plotMode == "scatter")
      {
         var histFactor;
         var histMax;
         var histLoc;
         var color;

         var npointset = me.ndataset;

         xShowAll     = jQuery("#xShowAll"      ).is(":checked");
         xShowVisible = jQuery("#xShowVisible"  ).is(":checked");

         if(me.histXAll != null && me.histXAll != "" && xShowAll == true)
         {
            histMax = me.histXAllMax;
            histLoc = me.histXAllLoc;

            if(histMax == 0 || histMax == null)
            {
               histMax = me.histXFilteredMax;
               histLoc = me.histXFilteredLoc;
            }

            histFactor = jQuery("#xHistoScale").val();

            if(histFactor.trim() == "")
               histFactor = 0.25;

            if(isNaN(histFactor) == true)
               histFactor = 0.25;

            me.currentParam.pointset[npointset]        = {};
            me.currentParam.pointset[npointset].yaxis  = {};
            me.currentParam.pointset[npointset].ygrid  = {};
            me.currentParam.pointset[npointset].source = {};
            me.currentParam.pointset[npointset].boxes  = {};

            me.currentParam.pointset[npointset].yaxis.scaling  = "linear";
            me.currentParam.pointset[npointset].yaxis.min      = 0.;
            me.currentParam.pointset[npointset].yaxis.max      = histMax / histFactor;

            me.currentParam.pointset[npointset].ygrid.min      = 0.0;
            me.currentParam.pointset[npointset].ygrid.max      = histMax;
            me.currentParam.pointset[npointset].ygrid.loc      = histLoc;

            me.currentParam.pointset[npointset].source.table   = "xAll.hist";

            me.currentParam.pointset[npointset].source.xmincolumn = "minval";
            me.currentParam.pointset[npointset].source.xmaxcolumn = "maxval";

            me.currentParam.pointset[npointset].source.yminval    = "0.";
            me.currentParam.pointset[npointset].source.ymaxcolumn = "count";

            color = tinycolor(jQuery("#xHistBorderColorPicker").val());

            me.currentParam.pointset[npointset].boxes.borderColor = color.toHex8();

            color = tinycolor(jQuery("#xHistFillColorPicker").val());

            me.currentParam.pointset[npointset].boxes.fillColor = color.toHex8();

            ++npointset;
         }

         if(me.histXFiltered != null && me.histXFiltered != "" && xShowVisible == true)
         {
            if(me.histXAll.trim() == "" || xShowAll == false)
            {
               histMax = me.histXFilteredMax;
               histLoc = me.histXFilteredLoc;
            }

            histFactor = jQuery("#xHistoScale").val();

            if(isNaN(histFactor) == true)
               histFactor = 0.25;

            me.currentParam.pointset[npointset]        = {};
            me.currentParam.pointset[npointset].yaxis  = {};

            if(me.histXAll.trim() == "" || xShowAll == false)
               me.currentParam.pointset[npointset].ygrid = {};

            me.currentParam.pointset[npointset].source = {};
            me.currentParam.pointset[npointset].boxes  = {};

            me.currentParam.pointset[npointset].yaxis.scaling  = "linear";
            me.currentParam.pointset[npointset].yaxis.min      = 0.;
            me.currentParam.pointset[npointset].yaxis.max      = histMax / histFactor;

            if(me.histXAll.trim() == "" || xShowAll == false)
            {
               me.currentParam.pointset[npointset].ygrid.min   = 0.0;
               me.currentParam.pointset[npointset].ygrid.max   = histMax;
               me.currentParam.pointset[npointset].ygrid.loc   = histLoc;
            }

            me.currentParam.pointset[npointset].source.table   = "xFiltered.hist";

            me.currentParam.pointset[npointset].source.xmincolumn = "minval";
            me.currentParam.pointset[npointset].source.xmaxcolumn = "maxval";

            me.currentParam.pointset[npointset].source.yminval    = "0.";
            me.currentParam.pointset[npointset].source.ymaxcolumn = "count";

            color = tinycolor(jQuery("#xHistBorderColorPicker").val());

            me.currentParam.pointset[npointset].boxes.borderColor = color.toHex8();

            color = tinycolor(jQuery("#xHistFillColorPicker").val());

            me.currentParam.pointset[npointset].boxes.fillColor = color.toHex8();

            ++npointset;
         }


         yShowAll     = jQuery("#yShowAll"      ).is(":checked");
         yShowVisible = jQuery("#yShowVisible"  ).is(":checked");

         if(me.histYAll != null && me.histYAll != "" && yShowAll == true)
         {
            histMax = me.histYAllMax;
            histLoc = me.histYAllLoc;

            histFactor = jQuery("#yHistoScale").val();

            if(histFactor.trim() == "")
               histFactor = 0.25;

            if(isNaN(histFactor) == true)
               histFactor = 0.25;

            me.currentParam.pointset[npointset]        = {};
            me.currentParam.pointset[npointset].xaxis  = {};
            me.currentParam.pointset[npointset].xgrid  = {};
            me.currentParam.pointset[npointset].source = {};
            me.currentParam.pointset[npointset].boxes  = {};

            me.currentParam.pointset[npointset].xaxis.scaling  = "linear";
            me.currentParam.pointset[npointset].xaxis.min      = 0.;
            me.currentParam.pointset[npointset].xaxis.max      = histMax / histFactor;

            me.currentParam.pointset[npointset].xgrid.min      = 0.0;
            me.currentParam.pointset[npointset].xgrid.max      = histMax;
            me.currentParam.pointset[npointset].xgrid.loc      = histLoc;

            me.currentParam.pointset[npointset].source.table   = "yAll.hist";

            me.currentParam.pointset[npointset].source.ymincolumn = "minval";
            me.currentParam.pointset[npointset].source.ymaxcolumn = "maxval";

            me.currentParam.pointset[npointset].source.xminval    = "0.";
            me.currentParam.pointset[npointset].source.xmaxcolumn = "count";

            color = tinycolor(jQuery("#yHistBorderColorPicker").val());

            me.currentParam.pointset[npointset].boxes.borderColor = color.toHex8();

            color = tinycolor(jQuery("#yHistFillColorPicker").val());

            me.currentParam.pointset[npointset].boxes.fillColor = color.toHex8();

            ++npointset;
         }

         if(me.histYFiltered != null && me.histYFiltered != "" && yShowVisible == true)
         {
            if(me.histYAll.trim() == "" || yShowAll == false)
            {
               histMax = me.histYFilteredMax;
               histLoc = me.histYFilteredLoc;
            }

            histFactor = jQuery("#yHistoScale").val();

            if(isNaN(histFactor) == true)
               histFactor = 0.25;

            me.currentParam.pointset[npointset]        = {};
            me.currentParam.pointset[npointset].xaxis  = {};
            me.currentParam.pointset[npointset].source = {};
            me.currentParam.pointset[npointset].boxes  = {};

            me.currentParam.pointset[npointset].xaxis.scaling  = "linear";
            me.currentParam.pointset[npointset].xaxis.min      = 0.;
            me.currentParam.pointset[npointset].xaxis.max      = histMax / histFactor;

            if(me.histYAll.trim() == "" || yShowAll == false)
               me.currentParam.pointset[npointset].xgrid = {};

            if(me.histYAll.trim() == "" || yShowAll == false)
            {
               me.currentParam.pointset[npointset].xgrid.min   = 0.0;
               me.currentParam.pointset[npointset].xgrid.max   = histMax;
               me.currentParam.pointset[npointset].xgrid.loc   = histLoc;
            }

            me.currentParam.pointset[npointset].source.table   = "yFiltered.hist";

            me.currentParam.pointset[npointset].source.ymincolumn = "minval";
            me.currentParam.pointset[npointset].source.ymaxcolumn = "maxval";

            me.currentParam.pointset[npointset].source.xminval    = "0.";
            me.currentParam.pointset[npointset].source.xmaxcolumn = "count";

            color = tinycolor(jQuery("#yHistBorderColorPicker").val());

            me.currentParam.pointset[npointset].boxes.borderColor = color.toHex8();

            color = tinycolor(jQuery("#yHistFillColorPicker").val());

            me.currentParam.pointset[npointset].boxes.fillColor = color.toHex8();

            ++npointset;
         }
      }

      if(me.debug)
         console.log("DEBUG> URL: /cgi-bin/mViewer/nph-icePlotter workspace=" + me.workspace + "&params=" + JSON.stringify(me.currentParam));

      jQuery.post("/cgi-bin/mViewer/nph-icePlotter", "caller=" + me.plotContext + "&workspace=" + me.workspace + "&params=" + encodeURIComponent(JSON.stringify(me.currentParam)), me.processUpdate, "json");
   }



   // When the remote icePlotter call returns, we need to
   // update the user interface, both the image and some
   // of the GUI field values.


   me.processUpdate = function(data, stat, jqXHR)
   {
      if(me.debug)
         console.log("DEBUG> processUpdate()");

      if(typeof(data.error) != "undefined")
      {
         me.requestSubmitted = false;

         me.grayOutMessage(false);
         me.grayOut(false);

         alert(data.error);

         return;
      }

      if(!me.initialized)
      {
         me.initialized = true;
         
         if(me.debug)
            console.log("DEBUG> 'initialized' set to true");
      }


      if(!me.downloadMode)
      {
         sc = data;

         if(me.plotMode == "scatter")
         {
            jQuery("#xDataMin").html(data.xminData.toPrecision(me.prec));
            jQuery("#xDataMax").html(data.xmaxData.toPrecision(me.prec));

            jQuery("#yDataMin").html(data.yminData.toPrecision(me.prec));
            jQuery("#yDataMax").html(data.ymaxData.toPrecision(me.prec));

            var tmpval = jQuery("#xScaleMin").val();

            if(isNaN(tmpval) || tmpval.trim() == "")
               jQuery("#xScaleMin").val(data.xminData.toPrecision(me.prec));

            tmpval = jQuery("#xScaleMax").val();

            if(isNaN(tmpval) || tmpval.trim() == "")
               jQuery("#xScaleMax").val(data.xmaxData.toPrecision(me.prec));

            tmpval = jQuery("#yScaleMin").val();

            if(isNaN(tmpval) || tmpval.trim() == "")
               jQuery("#yScaleMin").val(data.yminData.toPrecision(me.prec));

            tmpval = jQuery("#yScaleMax").val();

            if(isNaN(tmpval) || tmpval.trim() == "")
               jQuery("#yScaleMax").val(data.ymaxData.toPrecision(me.prec));

            jQuery("#xHistoDataMin").html(data.xminData.toPrecision(me.prec));
            jQuery("#xHistoDataMax").html(data.xmaxData.toPrecision(me.prec));

            jQuery("#yHistoDataMin").html(data.yminData.toPrecision(me.prec));
            jQuery("#yHistoDataMax").html(data.ymaxData.toPrecision(me.prec));

            if(me.debug)
               console.log("DEBUG> updateMode = [" + me.updateMode + "]");

            if(me.updateMode == "zoom")
            {
               me.changePulldown("xType", "manual");
               
               jQuery("#xScaleMinLbl").show();
               jQuery("#xScaleMaxLbl").show();
               
               jQuery("#xScaleMin").show();
               jQuery("#xScaleMax").show();

               jQuery("#xScaleMin").val(sc.xmin.toPrecision(me.prec));
               jQuery("#xScaleMax").val(sc.xmax.toPrecision(me.prec));

               me.changePulldown("yType", "manual");
               
               jQuery("#yScaleMinLbl").show();
               jQuery("#yScaleMaxLbl").show();
               
               jQuery("#yScaleMin").show();
               jQuery("#yScaleMax").show();

               jQuery("#yScaleMin").val(sc.ymin.toPrecision(me.prec));
               jQuery("#yScaleMax").val(sc.ymax.toPrecision(me.prec));
            }
         }
         else  // me.plotMode == 'histogram'
         {
            jQuery("#histCountDataMin").html(data.yminData.toFixed(0));
            jQuery("#histCountDataMax").html(data.ymaxData.toFixed(0));


            var ymode = jQuery("#histCountType").val();
               
            if(ymode == "automatic")
            {
               jQuery("#histCountScaleMin").val(data.yminData.toPrecision(me.prec));
               jQuery("#histCountScaleMax").val((data.ymaxData*1.025).toPrecision(me.prec));
            }

            if(me.debug)
               console.log("DEBUG> updateMode = [" + me.updateMode + "]");

            if(me.updateMode == "zoom")
            {
               me.changePulldown("histDataType", "manual");
               
               jQuery("#histDataScaleMinLbl").show();
               jQuery("#histDataScaleMaxLbl").show();
               
               jQuery("#histDataScaleMin").show();
               jQuery("#histDataScaleMax").show();

               jQuery("#histDataScaleMin").val(sc.xmin.toPrecision(me.prec));
               jQuery("#histDataScaleMax").val(sc.xmax.toPrecision(me.prec));

               if(ymode == "automatic")
               {
                  jQuery("#histCountScaleMinLbl").show();
                  jQuery("#histCountScaleMaxLbl").show();
                  
                  jQuery("#histCountScaleMin").show();
                  jQuery("#histCountScaleMax").show();

                  jQuery("#histCountScaleMin").val(yscalemin.toPrecision(me.prec));
                  jQuery("#histCountScaleMax").val((yscalemax*1.025).toPrecision(me.prec));
               }

               me.checkHistLink();
            }
         }


         me.ntotal = data.npts;

         var nbin = jQuery("#xHistoNBin").val();

         if(nbin.trim() == "")
            nbin = Math.floor(Math.sqrt(me.ntotal));

         jQuery("#xHistoNBin").val(nbin);


         nbin = jQuery("#yHistoNBin").val();

         if(nbin.trim() == "")
            nbin = Math.floor(Math.sqrt(me.ntotal));

         jQuery("#yHistoNBin").val(nbin);


         nbin = jQuery("#histoNBin").val();

         if(isNaN(nbin) || nbin.trim() == "" || nbin < 1)
            nbin = me.nbin;

         jQuery("#histoNBin").val(nbin);
      }


      if(me.downloadMode)
      {
         me.downloadMode = false;

         me.params = me.saveParams;

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
            console.log("DEBUG> Retrieving new PNG.");

         me.gc.setImage(data.file + "?seed=" + (new Date()).valueOf());
      }


      me.grayOutMessage(false);
      me.grayOut(false);

      me.requestSubmitted = false;

      me.manualScaleWarning();
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
      var zindex  = options.zindex || 50;
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
         tnode.style.zIndex          = '100';
         tnode.style.margin          = '-50px 0 0 -100px';
         tnode.style.backgroundColor = '#ffffff';
         tnode.style.display         = 'none';
         tnode.innerHTML             = '<img src="/applications/IceTable/waitClock.gif"/> &nbsp; Loading.  Please wait ... ';
         tnode.id                    = 'messageScreenObject';

         tbody.appendChild(tnode);

         msg = document.getElementById('messageScreenObject');
      }

      if(vis)
         msg.style.display = 'block';
      else
         msg.style.display = 'none';
   }



   // Parameter List Functions

   me.setParams = function(params)
   {
      if(me.debug)
         console.log("DEBUG> setParams()");

      // Axes parameters

      jQuery("#plotLabel"    ).val(params.axes.title);
      jQuery("#histPlotLabel").val(params.axes.title);

      jQuery("#yLabel").val(params.axes.yaxis.label);

      me.changePulldown("yScaling", params.axes.yaxis.scaling);

      if(params.axes.yaxis.autoscale == false)
      {
         me.changePulldown("yType", "manual");

         jQuery("#yScaleMinLbl").show();
         jQuery("#yScaleMaxLbl").show();

         jQuery("#yScaleMin").show();
         jQuery("#yScaleMax").show();

         jQuery("#yScaleMin").val(params.axes.yaxis.min.toPrecision(me.prec));
         jQuery("#yScaleMax").val(params.axes.yaxis.max.toPrecision(me.prec));
      }
      else
      {
         me.changePulldown("yType", "automatic");

         jQuery("#yScaleMinLbl").hide();
         jQuery("#yScaleMaxLbl").hide();

         jQuery("#yScaleMin").hide();
         jQuery("#yScaleMax").hide();
      }


      jQuery("#xLabel").val(params.axes.xaxis.label);

      me.changePulldown("xScaling", params.axes.xaxis.scaling);

      if(params.axes.xaxis.autoscale == false)
      {
         me.changePulldown("xType", "manual");

         jQuery("#xScaleMinLbl").show();
         jQuery("#xScaleMaxLbl").show();

         jQuery("#xScaleMin").show();
         jQuery("#xScaleMax").show();

         jQuery("#xScaleMin").val(params.axes.xaxis.min.toPrecision(me.prec));
         jQuery("#xScaleMax").val(params.axes.xaxis.max.toPrecision(me.prec));
      }
      else
      {
         me.changePulldown("xType", "automatic");

         jQuery("#xScaleMinLbl").hide();
         jQuery("#xScaleMaxLbl").hide();

         jQuery("#xScaleMin").hide();
         jQuery("#xScaleMax").hide();
      }


      jQuery("#histLabel").val(params.axes.xaxis.label);

      me.changePulldown("histDataScaling", params.axes.xaxis.scaling);

      if(params.axes.xaxis.autoscale == false)
      {
         me.changePulldown("histDataType", "manual");

         jQuery("#histDataScaleMinLbl").show();
         jQuery("#histDataScaleMaxLbl").show();

         jQuery("#histDataScaleMin").show();
         jQuery("#histDataScaleMax").show();

         jQuery("#histDataScaleMin").val(params.axes.xaxis.min.toPrecision(me.prec));
         jQuery("#histDataScaleMax").val(params.axes.xaxis.max.toPrecision(me.prec));

         me.checkHistLink();
      }
      else
      {
         me.changePulldown("histDataType", "automatic");

         jQuery("#histDataScaleMinLbl").hide();
         jQuery("#histDataScaleMaxLbl").hide();

         jQuery("#histDataScaleMin").hide();
         jQuery("#histDataScaleMax").hide();
      }


      me.changePulldown("histCountType", "automatic");

      jQuery("#histCountScaleMinLbl").hide();
      jQuery("#histCountScaleMaxLbl").hide();

      jQuery("#histCountScaleMin").hide();
      jQuery("#histCountScaleMax").hide();


      // This first set is just in case we are using the minimal form, which
      // doesn't implement the (hidden) color selectors

      jQuery("#axesColor"      ).val(params.axes.colors.axes);
      jQuery("#labelColor"     ).val(params.axes.colors.labels);
      jQuery("#backgroundColor").val(params.jpeg.background);

      jQuery("#histAxesColor"      ).val(params.axes.colors.axes);
      jQuery("#histLabelColor"     ).val(params.axes.colors.labels);
      jQuery("#histBackgroundColor").val(params.jpeg.background);


      // And here the setting for the forms that do use the color selectors

      jQuery("#axesColor"      ).spectrum("set", params.axes.colors.axes);
      jQuery("#labelColor"     ).spectrum("set", params.axes.colors.labels);
      jQuery("#backgroundColor").spectrum("set", params.jpeg.background);

      jQuery("#histAxesColor"      ).spectrum("set", params.axes.colors.axes);
      jQuery("#histLabelColor"     ).spectrum("set", params.axes.colors.labels);
      jQuery("#histBackgroundColor").spectrum("set", params.jpeg.background);


      // Datasets parameters

      jQuery("#currentSymbol").attr("src", "/applications/IcePlotter/symbols/"+params.pointset[0].points.symbol+".png");

      jQuery("#currentColor").val(params.pointset[0].points.color);
      jQuery("#currentColor").spectrum("set", params.pointset[0].points.color);

      var size = params.pointset[0].points.size;

      var str;

      if(size <= 0.2) str = "s0.2";
      if(size >  0.2) str = "s0.5";
      if(size >  0.5) str = "s0.7";
      if(size >  0.7) str =  "s1";
      if(size >  1. ) str =  "s2";
      if(size >  2. ) str =  "s3";
      if(size >  3. ) str =  "s4";
      if(size >  4. ) str =  "s5";
      if(size >  5. ) str = "s10";

      jQuery("#currentSize").attr("src", "/applications/IcePlotter/sizes/"+str+".png");

      for(var i=0; i<me.ndataset; ++i)
      {
         var file     = null;
         var xcol     = null;
         var ycol     = null;
         var xpcol    = null;
         var xmcol    = null;
         var ypcol    = null;
         var ymcol    = null;
         var symColor = null;
         var symType  = null;
         var symSize  = null;
         var lnColor  = null;
         var lnType   = null;
         var lnWidth  = null;
         var npts     = null;
         
         try{file     = params.pointset[i].source.table;  } catch(e){}
         try{xcol     = params.pointset[i].source.xcolumn;} catch(e){}
         try{ycol     = params.pointset[i].source.ycolumn;} catch(e){}
         try{xpcol    = params.pointset[i].source.xerrp;  } catch(e){}
         try{xmcol    = params.pointset[i].source.xerrm;  } catch(e){}
         try{ypcol    = params.pointset[i].source.yerrp;  } catch(e){}
         try{ymcol    = params.pointset[i].source.yerrm;  } catch(e){}
         try{symColor = params.pointset[i].points.color;  } catch(e){}
         try{symType  = params.pointset[i].points.symbol; } catch(e){}
         try{symSize  = params.pointset[i].points.size;   } catch(e){}
         try{lnColor  = params.pointset[i].lines.color;   } catch(e){}
         try{lnType   = params.pointset[i].lines.type;    } catch(e){}
         try{lnWidth  = params.pointset[i].lines.width;   } catch(e){}
         try{npts     = params.pointset[i].source.npts;   } catch(e){}

         if(symType == null)
            symType = "none";

         if(lnType == null)
            lnType = "none";

         me.addTable(file, xcol, ycol, xpcol, xmcol, ypcol, ymcol, symColor, symType, symSize, lnColor, lnType, lnWidth, npts);
      }
   }


   // Add a Row to the Dataset List

   me.addTable = function(file, xcol, ycol, xpcol, xmcol, ypcol, ymcol, symColor, symType, symSize, lnColor, lnType, lnWidth, npts)
   {
      if(me.debug)
         console.log("DEBUG> addTable()");

      var sizeStr;

      if(symSize <= 0.2) sizeStr = "s0.2";
      if(symSize >  0.2) sizeStr = "s0.5";
      if(symSize >  0.5) sizeStr = "s0.7";
      if(symSize >  0.7) sizeStr =  "s1";
      if(symSize >  1. ) sizeStr =  "s2";
      if(symSize >  2. ) sizeStr =  "s3";
      if(symSize >  3. ) sizeStr =  "s4";
      if(symSize >  4. ) sizeStr =  "s5";
      if(symSize >  5. ) sizeStr = "s10";


      jQuery("#overlayTbl tr:last").show();

      var index = jQuery("#overlayTbl").find("tr").size() - 1;

      var clonedRow = jQuery("#overlayTbl tr:last").clone();

      jQuery("#overlayTbl").append(clonedRow);

      var row = jQuery("#overlayTbl").find("tr").eq(index);

      // row.find(".dragOff").attr("class", "drag");

      row.find(".showOff").attr("class", "show");



      // SHOW

      row.find(".show").html("<input type=\"checkbox\" checked=\"checked\" />"); 



      // TABLE FILE

      for(var i=0; i<me.catFiles.length; ++i)
      {
         if(me.catFiles[i].name == file)
            break;
      }

      var fileContent = "<span class=\"fileName\" hidden=\"hidden\">" + file + "</span> " + me.tableLookup[file];

      row.find(".file").html(fileContent);



      // PLOT COLUMNS

      var ifile;

      for(ifile=0; ifile<me.catFiles.length; ++ifile)
      {
         if(me.catFiles[ifile].name == file)
            break;
      }

      var value = "<nobr>X&nbsp;<select class=\"ovSelect xCol\">\n";

      for(var i=0; i<me.colLists[me.catFiles[ifile].name].length; ++i)
      {
         var colLbl = me.colLists[me.catFiles[ifile].name][i][0];

         if(typeof(me.remoteLists.labels) != "undefined" && typeof(me.remoteLists.labels[colLbl]) != "undefined")
            colLbl = me.remoteLists.labels[colLbl];

         if(me.colLists[me.catFiles[ifile].name][i][0] == xcol)
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\" selected=\"selected\">" + colLbl + "</option>\n";
         else
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\">" + colLbl + "</option>\n";
      }

      value += "</select></nobr><br/>";

      value += "<nobr>Y&nbsp;<select class=\"ovSelect yCol\">\n";

      for(var i=0; i<me.colLists[me.catFiles[ifile].name].length; ++i)
      {
         var colLbl = me.colLists[me.catFiles[ifile].name][i][0];

         if(typeof(me.remoteLists.labels) != "undefined" && typeof(me.remoteLists.labels[colLbl]) != "undefined")
            colLbl = me.remoteLists.labels[colLbl];

         if(me.colLists[me.catFiles[ifile].name][i][0] == ycol)
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\" selected=\"selected\">" + colLbl + "</option>\n";
         else
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\">" + colLbl + "</option>\n";
      }

      value += "</select></nobr>";

      row.find(".columns").html(value);



      // POINT PARAMETERS

      me.colorNames = Object.keys(me.colors);

      value  = "<table class=\"symSelect\">\n";
      value += "   <tr>\n";

      value += "      <td><img id=\"sym" 
               + me.datasetCount + "\" src=\"/applications/IcePlotter/symbols/" 
               + symType      + ".png\" onclick=\"me.showBalloon('sym" 
               + me.datasetCount + "', 'me.symbols');\" style=\"vertical-align: middle;\" /></td>\n";

      value += "      <td><img id=\"siz" 
               + me.datasetCount + "\" src=\"/applications/IcePlotter/sizes/" 
               + sizeStr      + ".png\" onclick=\"me.showBalloon('siz" 
               + me.datasetCount + "', 'me.sizes');\"   style=\"vertical-align: middle;\" /></td>\n";

      value += "      <td><input type=\"text\" class=\"ovSelect symColor\" /></td>\n";

      value += "   </tr>\n";
      value += "</table>\n";

      row.find(".points").html(value);

      me.showBalloon("sym" + me.datasetCount, 'me.symbols');
      me.showBalloon("siz" + me.datasetCount, 'me.sizes');
      
      row.find(".points").find(".symColor").spectrum({
         preferredFormat: "hex8",
         color:       symColor,
         showPalette: true,
         showAlpha:   true,
         showInitial: true,
         showInput:   true,
         change:      function(color) { me.colorUpdate("layer"); }
      });

      row.find(".points").find(".symColor").val(symColor);
      row.find(".points").find(".symColor").spectrum("set", symColor);



      // LINE PARAMETERS

      me.styleNames = Object.keys(me.styles);

      if(lnType == null)
         lnType = "none";

      value = "<nobr><table class=\"ovlyTbl\"><tr>";

      value += "<td><select class=\"ovSelect lnType\">\n";

      for(var i=0; i<me.styleNames.length; ++i)
      {
         if(lnType == me.styleNames[i])
            value += "   <option value=\"" + me.styleNames[i] + "\" selected=\"selected\"> " + me.styles[me.styleNames[i]] + " </option>\n";
         else
            value += "   <option value=\"" + me.styleNames[i] + "\"> " + me.styles[me.styleNames[i]] + " </option>\n";
      }

      value += "</select></td>\n";


      value += "<td rowspan=\"2\"><input type=\"text\" class=\"ovSelect lnColor\" /></td></tr>\n";


      value += "<tr><td><select class=\"ovSelect lnWidth\">\n";

      for(var i=0; i<me.widths.length; ++i)
      {
         if(lnWidth == me.widths[i])
            value += "   <option value=\"" + me.widths[i] + "\" selected=\"selected\"> " + me.widths[i] + " </option>\n";
         else
            value += "   <option value=\"" + me.widths[i] + "\"> " + me.widths[i] + " </option>\n";
      }

      value += "</select></td></tr></table>\n";
         
      row.find(".lines").html(value);

      row.find(".lines").find(".lnColor").spectrum({
         preferredFormat: "hex8",
         color:       lnColor,
         showPalette: true,
         showAlpha:   true,
         showInitial: true,
         showInput:   true,
         change:      function(color) { me.colorUpdate("layer"); }
      });

      row.find(".lines").find(".lnColor").val(lnColor);
      row.find(".lines").find(".lnColor").spectrum("set", lnColor);


      // XERR COLUMNS

      var ifile;

      for(ifile=0; ifile<me.catFiles.length; ++ifile)
      {
         if(me.catFiles[ifile].name == file)
            break;
      }

      value = "<nobr>+&nbsp;<select class=\"ovSelect xPlusCol\">\n";

      if(xpcol == null)
         value += "  <option value=\"(none)\" selected=\"selected\"> </option>\n";
      else
         value += "  <option value=\"(none)\"> </option>\n";

      for(var i=0; i<me.colLists[me.catFiles[ifile].name].length; ++i)
      {
         var colLbl = me.colLists[me.catFiles[ifile].name][i][0];

         if(typeof(me.remoteLists.labels) != "undefined" && typeof(me.remoteLists.labels[colLbl]) != "undefined")
            colLbl = me.remoteLists.labels[colLbl];

         if(me.colLists[me.catFiles[ifile].name][i][0] == xpcol)
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\" selected=\"selected\">" + colLbl + "</option>\n";
         else
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\">" + colLbl + "</option>\n";
      }

      value += "</select></nobr><br/>";

      value += "<nobr>-&nbsp;<select class=\"ovSelect xMinusCol\">\n";

      if(xmcol == null)
         value += "  <option value=\"(none)\" selected=\"selected\"> </option>\n";
      else
         value += "  <option value=\"(none)\"> </option>\n";

      for(var i=0; i<me.colLists[me.catFiles[ifile].name].length; ++i)
      {
         var colLbl = me.colLists[me.catFiles[ifile].name][i][0];

         if(typeof(me.remoteLists.labels) != "undefined" && typeof(me.remoteLists.labels[colLbl]) != "undefined")
            colLbl = me.remoteLists.labels[colLbl];

         if(me.colLists[me.catFiles[ifile].name][i][0] == xmcol)
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\" selected=\"selected\">" + colLbl + "</option>\n";
         else
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\">" + colLbl + "</option>\n";
      }

      value += "</select></nobr>";

      row.find(".xerr").html(value);



      // YERR COLUMNS

      var ifile;

      for(ifile=0; ifile<me.catFiles.length; ++ifile)
      {
         if(me.catFiles[ifile].name == file)
            break;
      }

      value = "<nobr>+&nbsp;<select class=\"ovSelect yPlusCol\">\n";

      if(ypcol == null)
         value += "  <option value=\"(none)\" selected=\"selected\"> </option>\n";
      else
         value += "  <option value=\"(none)\"> </option>\n";

      for(var i=0; i<me.colLists[me.catFiles[ifile].name].length; ++i)
      {
         var colLbl = me.colLists[me.catFiles[ifile].name][i][0];

         if(typeof(me.remoteLists.labels) != "undefined" && typeof(me.remoteLists.labels[colLbl]) != "undefined")
            colLbl = me.remoteLists.labels[colLbl];

         if(me.colLists[me.catFiles[ifile].name][i][0] == ypcol)
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\" selected=\"selected\">" + colLbl + "</option>\n";
         else
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\">" + colLbl + "</option>\n";
      }

      value += "</select></nobr><br/>";

      value += "<nobr>-&nbsp;<select class=\"ovSelect yMinusCol\">\n";

      if(ymcol == null)
         value += "  <option value=\"(none)\" selected=\"selected\"> </option>\n";
      else
         value += "  <option value=\"(none)\"> </option>\n";

      for(var i=0; i<me.colLists[me.catFiles[ifile].name].length; ++i)
      {
         var colLbl = me.colLists[me.catFiles[ifile].name][i][0];

         if(typeof(me.remoteLists.labels) != "undefined" && typeof(me.remoteLists.labels[colLbl]) != "undefined")
            colLbl = me.remoteLists.labels[colLbl];

         if(me.colLists[me.catFiles[ifile].name][i][0] == ymcol)
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\" selected=\"selected\">" + colLbl + "</option>\n";
         else
            value += "  <option title=\"" + colLbl + "\" value=\"" + me.colLists[me.catFiles[ifile].name][i][0] + "\">" + colLbl + "</option>\n";
      }

      value += "</select></nobr>";

      row.find(".yerr").html(value);



      //NPTS

      row.find(".npoints").html(npts);


      row.find(".delOff").attr("class", "delete");

      row.find(".delete").html("<img src=\"/applications/IcePlotter/imgs/delete.png\" class=\"delBtn\" onclick=\"jQuery(this).parent().parent().remove();\" />"); 

      ++me.datasetCount;


      jQuery("#overlayTbl tr:last").hide();
   }


   me.changePulldown = function(id, val)
   {
      if(me.debug)
         console.log("DEBUG> changePulldown(" + id + ") -> " + val);

      var opVal = jQuery("#" + id + " :selected").val();

      var selVal = "#" + id + " option[value='" + opVal + "']";

      jQuery(selVal).prop("selected", false);

      opVal = val;

      selVal = "#" + id + " option[value='" + opVal + "']";

      jQuery(selVal).prop("selected", true);
   }


   me.iconBase = function(str)
   {
      var substrs = str.split("/");

      var len = substrs.length;

      var ext = substrs[len-1].indexOf(".png");

      return substrs[len-1].substring(0,ext);
   }



   // Multi-Dataset Controls

   me.showMultiDatasetControls = function()
   {
      if(me.multiControls)
      {
         jQuery('#datasetsDiv').parent().hide();

         me.multiControls = false;
      }
      else
      {
         jQuery('#datasetsDiv').parent().show();

         me.multiControls = true;
      }
   }


   me.isNumeric = function(n, showAlert)
   {
      var goodvalue = true;

      if(n == undefined || n == null)
         goodValue = false;

      else if(n == '-' || n.trim() == "" || n == "-." || n == '.') 
         goodValue = true;

      else
         goodValue = !isNaN(parseFloat(n)) && isFinite(n);

      if(!goodValue && showAlert)
        alert("Bad value [" + n + "] for numeric field.\nPlease correct before continuing.");

      return goodValue;
   }
}
