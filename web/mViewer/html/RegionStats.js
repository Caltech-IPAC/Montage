/*****************************/
/* Region Statistics Display */
/*****************************/

function RegionStats(statsDivName, viewer)
{
   var me = this;

   me.statsDivName = statsDivName;

   me.statsDiv = document.getElementById(me.statsDivName);

   me.viewer = viewer;

   me.workspace;
   me.fitsFile;

   me.plane = 'blue';

   me.radius = 20;

   me.statsResponse;

   me.statsJSON = {};


   me.init = function()
   {
      me.workspace = viewer.getWorkspace();
      me.jsonData  = viewer.getJSON();

      if(typeof me.jsonData.blueFile != 'undefined')
         me.fitsFile = me.jsonData.blueFile.fitsFile;
      
      else if(typeof me.jsonData.grayFile != 'undefined')
         me.fitsFile = me.jsonData.grayFile.fitsFile;

      else
      {
         alert("No image file references found in JSON structure.");
         return;
      }
      
      me.makeControl();


      // Borrow a little space in the viewer object
      // to maintain state between invocations

      if(typeof(me.viewer.statsState) == "undefined")
      {
         me.viewer.statsState = {};

         me.viewer.statsState.plane  = me.plane;
         me.viewer.statsState.file   = me.fitsFile;
         me.viewer.statsState.radius = me.radius;

         me.viewer.statsState.x = (viewer.imgData.xmin + viewer.imgData.xmax) / 2.;
         me.viewer.statsState.y = (viewer.imgData.ymin + viewer.imgData.ymax) / 2.;

         viewer.addClickCallback(me.getStats);
      }

      // (Re)initialize the plane (if used) and file name 

      me.fitsFile = me.viewer.statsState.file;

      jQuery(me.statsDiv).find(".fileName").html("File:&nbsp;<b>" + me.fitsFile + "</b>");

      var plane = jQuery(me.statsDiv).find('.colorPlane option:selected').val();

      jQuery(me.statsDiv).find('.colorPlane option[value="' + plane    + '"]').prop("selected", false);
      jQuery(me.statsDiv).find('.colorPlane option[value="' + me.plane + '"]').prop("selected", true);

      jQuery(me.statsDiv).find('.radius').val(me.radius);


      // (Re)set the values

      me.getStats(me.viewer.statsState.x, me.viewer.statsState.y);
   }

   
   // Load retrieve and load the selected header

   me.getStats = function(x, y)
   {
      me.radius = jQuery(me.statsDiv).find('.radius').val();

      var listURL = "/cgi-bin/mViewer/nph-mViewerStats"
                  + "?ws="     + me.workspace
                  + "&file="   + me.fitsFile
                  + "&x="      + x
                  + "&y="      + y
                  + "&radius=" + me.radius;


      // Create an XML HTTP request object
      
      var xmlhttp;

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

      if(xmlhttp == false)
      {
         jQuery(me.statsDiv).html("<div>Cannot create web request object.</div>");
         return;
      }


      // Send the request and put the 
      // response in the waiting <div>

      xmlhttp.open("GET", listURL);

      xmlhttp.onreadystatechange = function()
      {
         if (xmlhttp.readyState==4 && xmlhttp.status==200)
         {
            var xml = xmlhttp.responseXML;

            if(xml && xml.getElementsByTagName("error").length > 0)
            {
               var error = xml.getElementsByTagName("error")[0].childNodes[0].nodeValue;

               alert("Server Error: " + error);
            }

            else
            {
               me.statsResponse = xmlhttp.responseText;

               me.statsJSON = jQuery.parseJSON(me.statsResponse);

               jQuery(me.statsDiv).find(".fluxref" ).html(me.statsJSON.fluxref);
               jQuery(me.statsDiv).find(".sigmaref").html(me.statsJSON.sigmaref);
               jQuery(me.statsDiv).find(".xref"    ).html(me.statsJSON.xref);
               jQuery(me.statsDiv).find(".yref"    ).html(me.statsJSON.yref);
               jQuery(me.statsDiv).find(".raref"   ).html(me.statsJSON.raref);
               jQuery(me.statsDiv).find(".decref"  ).html(me.statsJSON.decref);
               jQuery(me.statsDiv).find(".fluxmin" ).html(me.statsJSON.fluxmin);
               jQuery(me.statsDiv).find(".sigmamin").html(me.statsJSON.sigmamin);
               jQuery(me.statsDiv).find(".xmin"    ).html(me.statsJSON.xmin);
               jQuery(me.statsDiv).find(".ymin"    ).html(me.statsJSON.ymin);
               jQuery(me.statsDiv).find(".ramin"   ).html(me.statsJSON.ramin);
               jQuery(me.statsDiv).find(".decmin"  ).html(me.statsJSON.decmin);
               jQuery(me.statsDiv).find(".fluxmax" ).html(me.statsJSON.fluxmax);
               jQuery(me.statsDiv).find(".sigmamax").html(me.statsJSON.sigmamax);
               jQuery(me.statsDiv).find(".xmax"    ).html(me.statsJSON.xmax);
               jQuery(me.statsDiv).find(".ymax"    ).html(me.statsJSON.ymax);
               jQuery(me.statsDiv).find(".ramax"   ).html(me.statsJSON.ramax);
               jQuery(me.statsDiv).find(".decmax"  ).html(me.statsJSON.decmax);
               jQuery(me.statsDiv).find(".decmax"  ).html(me.statsJSON.decmax);
               jQuery(me.statsDiv).find(".aveflux" ).html(me.statsJSON.aveflux);
               jQuery(me.statsDiv).find(".rmsflux" ).html(me.statsJSON.rmsflux);
               jQuery(me.statsDiv).find(".radius"  ).html(me.statsJSON.radius);
               jQuery(me.statsDiv).find(".radpix"  ).html(me.statsJSON.radpix);
               jQuery(me.statsDiv).find(".npixel"  ).html(me.statsJSON.npixel);
               jQuery(me.statsDiv).find(".nnull"   ).html(me.statsJSON.nnull);
            }
         }
      }

      xmlhttp.send(null);


      // Save the location so we can reinit the control 
      // with the same location

      me.viewer.statsState.x = x;
      me.viewer.statsState.y = y;

      me.viewer.statsState.radius = me.radius;
   }


   // Build the control div contents

   me.makeControl = function()
   {
      if(me.debug)
         console.log("DEBUG> makeControl()");

      var controlHTML = ""

      + "<div class='statDiv'>"
      + "   <p>"
      + "      <span>"
      + "            <select class='colorPlane'>"
      + "               <option value='blue' selected='selected'>Blue Plane</option>"
      + "               <option value='green'                   >Green Plane</option>"
      + "               <option value='red'                     >Red Plane</option>"
      + "            </select>"
      + "      </span>"

      + "      &nbsp;&nbsp;&nbsp;"
      + "      <span class='fileName'></span>"
      + "      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
      + "      Radius: <input class='radius' value='20' size='5'/> pixels"

      + "   </p>"

      + "   <table class='stat'>"
      + "     <tbody>"
      + "       <tr><th>&nbsp;</th><th>Flux</th><th>Sigma</th><th>X</th><th>Y</th><th>RA</th><th>Dec</th></tr>"
      + "       <tr><td><b>Center:</b></td><td class='fluxref'></td><td class='sigmaref'></td><td class='xref'></td><td class='yref'></td><td class='raref'></td><td class='decref'></td></tr>"
      + "       <tr><td><b>Max:</b></td><td class='fluxmax'></td><td class='sigmamax'></td><td class='xmax'></td><td class='ymax'></td><td class='ramax'></td><td class='decmax'></td></tr>"
      + "       <tr><td><b>Min:</b></td><td class='fluxmin'></td><td class='sigmamin'></td><td class='xmin'></td><td class='ymin'></td><td class='ramin'></td><td class='decmin'></td></tr>"
      + "     </tbody>"
      + "   </table><br/>"
      + "   <center>"
      + "   <table class='substat'>"
      + "     <tr><td><b>Avg:</b></td><td><span class='aveflux'></span> &plusmn; <span class='rmsflux'></span></td></tr>"
      + "     <tr><td><b>Region:</b></td><td> Radius <span class='radius'></span> deg&nbsp;&nbsp;(<span class='radpix'></span> pixels)</td></tr>"
      + "     <tr><td>&nbsp;</td><td> <span class='npixel'></span> pixels (<span class='nnull'></span> nulls)</td></tr>"
      + "   </table>"
      + "   </center>"
      + "</div>";

      if(me.debug)
         console.log("DEBUG> makeControl() setting HTML");

      jQuery(me.statsDiv).html(controlHTML);

      if(typeof me.jsonData.blueFile == 'undefined')
         jQuery(me.statsDiv).find('.colorPlane').hide();


      // Callback when new plane selected

      jQuery(me.statsDiv).find('.colorPlane').change(function(){

         me.plane = jQuery(me.statsDiv).find('.colorPlane option:selected').val();

         if(me.plane == "blue")
            me.fitsFile = me.jsonData.blueFile.fitsFile;
         else if(me.plane == "green")
            me.fitsFile = me.jsonData.greenFile.fitsFile;
         else if(me.plane == "red")
            me.fitsFile = me.jsonData.redFile.fitsFile;

         me.viewer.statsState.plane = me.plane;
         me.viewer.statsState.file  = me.fitsFile;

         jQuery(me.statsDiv).find(".fileName").html("File:&nbsp;<b>" + me.fitsFile + "</b>");

         me.getStats(me.viewer.statsState.x, me.viewer.statsState.y);
      });


      // Callback when new radius selected

      jQuery(me.statsDiv).find('.radius').change(function(){
         me.getStats(me.viewer.statsState.x, me.viewer.statsState.y);
      });
   }
}
