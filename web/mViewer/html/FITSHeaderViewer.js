/**********************/
/* FITS Header Viewer */
/**********************/

function FITSHeaderViewer(fitsHeaderDivName, viewer)
{
   var me = this;

   me.fitsHeaderDivName = fitsHeaderDivName;

   me.fitsHeaderDiv = document.getElementById(me.fitsHeaderDivName);

   me.viewer = viewer;

   me.workspace;
   me.fitsFile;
   
   me.debug = 0;

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

      jQuery(me.fitsHeaderDiv).find(".fileName").html("File:&nbsp;<b>" + me.fitsFile + "</b>");

      me.loadHeader();
   }

   
   // Load retrieve and load the selected header

   me.loadHeader = function()
   {
      var listURL = "/cgi-bin/mViewer/nph-mViewerHdr"
                  + "?ws="   + me.workspace
                  + "&file=" + me.fitsFile;


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
         jQuery(me.fitsHeaderDiv).html("<div>Cannot create web request object.</div>");
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

               jQuery(me.fitsHeaderDiv).html("<div>Remote server error[1].</div>");
            }

            else
               jQuery(me.fitsHeaderDiv).find(".fitsHdr").html(xmlhttp.responseText);
         }
      }

      xmlhttp.send(null);
   }


   // Build the control div contents

   me.makeControl = function()
   {
      if(me.debug)
         console.log("DEBUG> makeControl()");

      var controlHTML = ""

      + "<div class='hdrDiv'>"
      + "   <p>"
      + "      <img class='titleLine' src='/applications/mViewer/icons/info.gif'/>"

      + "      &nbsp;&nbsp;&nbsp;"

      + "      <span>"
      + "            <select class='colorPlane'>"
      + "               <option value='blue' selected='selected'>Blue Plane</option>"
      + "               <option value='green'                   >Green Plane</option>"
      + "               <option value='red'                     >Red Plane</option>"
      + "            </select><p/>"
      + "      </span>"

      + "      &nbsp;&nbsp;&nbsp;"

      + "      <span class='fileName'></span>"

      + "   </p>"
      + "   <div class='fitsHdr'></div>"
      + "</div>";

      if(me.debug)
         console.log("DEBUG> makeControl() setting HTML");

      jQuery(me.fitsHeaderDiv).html(controlHTML);

      if(typeof me.jsonData.blueFile == 'undefined')
         jQuery(me.fitsHeaderDiv).find('.colorPlane').hide();

      jQuery(me.fitsHeaderDiv).find('.colorPlane').change(function(){

         var plane = jQuery(me.fitsHeaderDiv).find('.colorPlane option:selected').val();

         if(plane == "blue")
            me.fitsFile = me.jsonData.blueFile.fitsFile;
         else if(plane == "green")
            me.fitsFile = me.jsonData.greenFile.fitsFile;
         else if(plane == "red")
            me.fitsFile = me.jsonData.redFile.fitsFile;

         jQuery(me.fitsHeaderDiv).find(".fileName").html("File:&nbsp;<b>" + me.fitsFile + "</b>");

         me.loadHeader();
      });
   }
}
