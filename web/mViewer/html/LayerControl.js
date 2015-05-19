/*********************************/
/* Layer Controls (Map Overlays) */
/*********************************/

function LayerControl(controlDivName, viewer)
{

   // This object encapsulates a control box that a user can use to
   // manage a set of image overlays.  This overlays can include:
   // 
   // - Coordinate grids
   // - Astronomical catalogs (scaled symbols)
   // - Image metadata (boxes)
   // - Labels
   // - Custom markers
   // 
   // The control allows the user to modify the details for each
   // layer (e.g. color of a grid or text of a label) and arrange
   // the order in which they will be drawn.  Layers can also be
   // temporarily hidden.
   //
   // If allowed, the control can delete layers.  Content for new layers 
   // is controlled from outside.

   var me = this;

   me.debug = false;

   me.controlDivName = controlDivName;

   me.controlDiv = document.getElementById(me.controlDivName);

   me.viewer = viewer;


   me.init = function()
   {
      // We separate this init() function from the object creation
      // in case there is information the calling application wants
      // to set in the control beyond the basic div/viewer info
      // passed in.  In our examples this is not the case but better
      // to be safe than sorry.  One use pattern where this might 
      // well be the case is someone wanting to turn on control 
      // debugging.

      if(me.debug)
         console.log("DEBUG> LayerControl.init()");

      // Create the control div HTML content. This is 
      // just creating a block of text and using it to 
      // replace the (usually empty) target <div> contents.

      me.makeControl();

      // Using the current viewer 'updateJSON' structure, 
      // populate the layer control with the current layer
      // info

      var nlayer = me.viewer.updateJSON.overlay.length;

      if(me.debug)
         console.log("DEBUG> LayerControl.init(): " + nlayer + " layers");

      for(i=0; i<nlayer; ++i)
      {
         var type     = me.viewer.updateJSON.overlay[i].type;
         var coordSys = me.viewer.updateJSON.overlay[i].coordSys;
         var color    = me.viewer.updateJSON.overlay[i].color;
         var dataFile = me.viewer.updateJSON.overlay[i].dataFile;
         var dataCol  = me.viewer.updateJSON.overlay[i].dataCol;
         var dataRef  = me.viewer.updateJSON.overlay[i].dataRef;
         var dataType = me.viewer.updateJSON.overlay[i].dataType;
         var symType  = me.viewer.updateJSON.overlay[i].symType;
         var symSize  = me.viewer.updateJSON.overlay[i].symSize;
         var location = me.viewer.updateJSON.overlay[i].location;
         var text     = me.viewer.updateJSON.overlay[i].text;
         var visible  = me.viewer.updateJSON.overlay[i].visible;

         switch(type)
         {
            case "grid":
               me.addGridRec(coordSys, color, visible);
            break;

            case "catalog":
               me.addCatalogRec(dataFile, dataCol, symType, symSize, dataRef, dataType, color, visible);
            break;

            case "imginfo":
               me.addImageRec(dataFile, color, visible);
            break;

            case "mark":
               me.addMarkRec(location, symType, symSize, color, visible);
            break;

            case "label":
               me.addLabelRec(location, text, color, visible);
            break;
            
            default:
            break;
         }
      }
   }


   // Add a "GRID" Row to the Layer Manager

   me.addGridRec = function(coordSys, color, gridVisible)
   {
      if(me.debug)
         console.log("DEBUG> LayerControl.addGridRec()");

      var index = jQuery(me.controlDiv).find(".overlayTbl").find("tr").size() - 1;

      jQuery(me.controlDiv).find(".overlayTbl tr:last").show();

      var clonedRow = jQuery(me.controlDiv).find(".overlayTbl tr:last").clone();

      jQuery(me.controlDiv).find(".overlayTbl").append(clonedRow);

      var row = jQuery(me.controlDiv).find(".overlayTbl").find("tr").eq(index);

      row.find(".dragOff").attr("class", "drag");

      row.find(".showOff").attr("class", "show");

      if(gridVisible == true)
         row.find(".show").html("<input type='checkbox' checked='checked' />"); 
      else
         row.find(".show").html("<input type='checkbox' />"); 

      row.find(".type").html("GRID");

      row.find(".source").html(
           "<center><select class='coordSys ovSelect'>\n"
         + "  <option value='eqj2000'>Equ J2000</option>\n"
         + "  <option value='eqb1950'>Equ B1950</option>\n"
         + "  <option value='gal'>Galactic</option>\n"
         + "  <option value='ecj2000'>Ecl J2000</option>\n"
         + "  <option value='ecb1950'>Ecl B1950</option>\n"
         + "</select></center>"
      );

      row.find(".symbol").html("&nbsp;");
      row.find(".scale").html("&nbsp;");

      row.find(".color").html("<input type='text' class='gridColor ovSelect'/>");

      row.find(".color").find(".gridColor").spectrum({
         preferredFormat: "hex",
         color:       color
      });

      row.find(".delOff").attr("class", "delete");

      row.find(".delete").click(function(){
         row.remove();
      });

      row.find(".source").find('.coordSys option[value="' + coordSys + '"]').prop("selected", true);

      jQuery(me.controlDiv).find(".overlayTbl tr:last").hide();
   }


   // Add a "SOURCE TABLE" Row to the Layer Manager

   me.addCatalogRec = function(dataFile, dataCol, symType, symSize, dataRef, dataType, catColor, catVisible)
   {
      if(me.debug)
         console.log("DEBUG> LayerControl.addCatalogRec()");

      var index = jQuery(me.controlDiv).find(".overlayTbl").find("tr").size() - 1;

      jQuery(me.controlDiv).find(".overlayTbl tr:last").show();

      var clonedRow = jQuery(me.controlDiv).find(".overlayTbl tr:last").clone();

      jQuery(me.controlDiv).find(".overlayTbl").append(clonedRow);

      var row = jQuery(me.controlDiv).find(".overlayTbl").find("tr").eq(index);

      row.find(".dragOff").attr("class", "drag");
      row.find(".showOff").attr("class", "show");


      // SHOW

      if(catVisible == true)
         row.find(".show").html("<input type='checkbox' checked='checked' />"); 
      else
         row.find(".show").html("<input type='checkbox' />"); 


      // TYPE

      row.find(".type").html("SOURCE<br/>TABLE");


      // SOURCE

      value = "<table class='ovlyCellTbl'><tr><td><b>File:</b></td>\n";

      value += "<td><span class='fileName'>" + dataFile + "</span></td></tr>\n"

      value += "<tr><td><b>Column:</b></td>\n";

      value += "<td><span class='colName'>" + dataCol + "</span></td></tr></table>\n"

      row.find(".source").html(value);


      // SYMBOL

      value = "<table class='ovlyCellTbl'><tr><td><b>Symbol:</b></td>\n";

      value += "<td><select class='symType ovSelect'>\n"
         + "   <option value='triangle'>Triangle</option>\n"
         + "   <option value='box'>Box</option>\n"
         + "   <option value='square'>Square</option>\n"
         + "   <option value='diamond'>Diamond</option>\n"
         + "   <option value='pentagon'>Pentagon</option>\n"
         + "   <option value='hexagon'>Hexagon</option>\n"
         + "   <option value='septagon'>Septagon</option>\n"
         + "   <option value='octagon'>Octagon</option>\n"
         + "   <option value='circle'>Circle</option>\n"
         + "</select></td></tr>"

         + "<tr><td><b>Size:</b></td>\n"
         + "<td><input size='8' class='symSize ovInput' value='" + symSize + "' /></td></tr></table>";

      row.find(".symbol").html(value);


      // SCALE

      value = "<table class='ovlyCellTbl'><tr><td><b>Type:</b></td>\n";

      value += "<td><select class='dataType ovSelect'>\n"
         + "   <option value='mag'>Mag</option>\n"
         + "   <option value='lin'>Linear</option>\n"
         + "   <option value='log'>Log</option>\n"
         + "   <option value='loglog'>Log*log</option>\n"
         + "</select></td></tr>";

      value += "<tr><td><span style='white-space: nowrap;'><b>Ref val:</b></span></td>\n";

      value += "<td><input class='dataRef ovInput' size='8' value='" + dataRef + "'/></td></tr></table>";

      row.find(".scale").html(value);


      // COLOR

      row.find(".color").html("<input type='text' class='catColor ovSelect'/>");

      row.find(".color").find(".catColor").spectrum({
         preferredFormat: "hex",
         color:       catColor,
      });

      row.find(".delOff").attr("class", "delete");

      row.find(".delete").click(function(){
         row.remove();
      });

      row.find(".symbol").find('.symType  option[value="' + symType + '"]').prop("selected", true);
      row.find(".symbol").find('.symSize  option[value="' + symSize + '"]').prop("selected", true);
      row.find(".scale" ).find('.dataType option[value="' + dataRef + '"]').prop("selected", true);

      jQuery(me.controlDiv).find(".overlayTbl tr:last").hide();
   }


   // Add an "IMAGE OUTLINES" Row to the Layer Manager

   me.addImageRec = function(dataFile, color, imgVisible)
   {
      if(me.debug)
         console.log("DEBUG> LayerControl.addImageRec()");

      var index = jQuery(me.controlDiv).find(".overlayTbl").find("tr").size() - 1;

      jQuery(me.controlDiv).find(".overlayTbl tr:last").show();

      var clonedRow = jQuery(me.controlDiv).find(".overlayTbl tr:last").clone();

      jQuery(me.controlDiv).find(".overlayTbl").append(clonedRow);

      var row = jQuery(me.controlDiv).find(".overlayTbl").find("tr").eq(index);

      row.find(".dragOff").attr("class", "drag");
      row.find(".showOff").attr("class", "show");

      if(imgVisible == true)
         row.find(".show").html("<input type='checkbox' checked='checked' />"); 
      else
         row.find(".show").html("<input type='checkbox' />"); 

      row.find(".type").html("IMAGE<br/>OUTLINES");


      value = "<table class='ovlyCellTbl'><tr><td><b>File:</b></td>\n";

      value += "<td><span class='fileName'>" + dataFile + "</span></td></tr></table>\n"

      row.find(".source").html(value);


      row.find(".symbol").html("&nbsp;");
      row.find(".scale").html("&nbsp;");

      row.find(".color").html("<input type='text' class='imgColor ovSelect'/>");

      row.find(".color").find(".imgColor").spectrum({
         preferredFormat: "hex",
         color:       color
      });

      row.find(".delOff").attr("class", "delete");

      row.find(".delete").click(function(){
         row.remove();
      });

      jQuery(me.controlDiv).find(".overlayTbl tr:last").hide();
   }


   // Add a "MARK" Row to the Layer Manager

   me.addMarkRec = function(location, symType, symSize, color, markVisible)
   {
      if(me.debug)
         console.log("DEBUG> LayerControl.addMarkRec()");

      var index = jQuery(me.controlDiv).find(".overlayTbl").find("tr").size() - 1;

      jQuery(me.controlDiv).find(".overlayTbl tr:last").show();

      var clonedRow = jQuery(me.controlDiv).find(".overlayTbl tr:last").clone();

      jQuery(me.controlDiv).find(".overlayTbl").append(clonedRow);

      var row = jQuery(me.controlDiv).find(".overlayTbl").find("tr").eq(index);

      row.find(".dragOff").attr("class", "drag");
      row.find(".showOff").attr("class", "show");

      if(markVisible == true)
         row.find(".show").html("<input type='checkbox' checked='checked' />"); 
      else
         row.find(".show").html("<input type='checkbox' />"); 

      row.find(".type").html("MARK");

      row.find(".source").html("<center><input size='40' class='ovInput' value='" + location + "' /></center>");
      
      row.find(".symbol").html(
           " <center><select class='symType ovSelect'>\n"
         + "   <option value='triangle'>Triangle</option>\n"
         + "   <option value='box'>Box</option>\n"
         + "   <option value='square'>Square</option>\n"
         + "   <option value='diamond'>Diamond</option>\n"
         + "   <option value='pentagon'>Pentagon</option>\n"
         + "   <option value='hexagon'>Hexagon</option>\n"
         + "   <option value='septagon'>Septagon</option>\n"
         + "   <option value='octagon'>Octagon</option>\n"
         + "   <option value='circle'>Circle</option>\n"
         + "</select></center>"
      );

      row.find(".scale").html("<center><input size='8' class='symSize ovInput' value='" + symSize + "' /></center>");

      row.find(".color").html("<input type='text' class='markColor ovSelect'/>");

      row.find(".color").find(".markColor").spectrum({
         preferredFormat: "hex",
         color:       color
      });

      row.find(".delOff").attr("class", "delete");

      row.find(".delete").click(function(){
         row.remove();
      });

      row.find('.symbol .symType option[value="' + symType + '"]').prop("selected", true);

      jQuery(me.controlDiv).find(".overlayTbl tr:last").hide();
   }


   // Add a "LABEL" Row to the Layer Manager

   me.addLabelRec = function(location, text, color, labelVisible)
   {
      if(me.debug)
         console.log("DEBUG> LayerControl.addLabelRec()");

      var index = jQuery(me.controlDiv).find(".overlayTbl").find("tr").size() - 1;

      jQuery(me.controlDiv).find(".overlayTbl tr:last").show();

      var clonedRow = jQuery(me.controlDiv).find(".overlayTbl tr:last").clone();

      jQuery(me.controlDiv).find(".overlayTbl").append(clonedRow);

      var row = jQuery(me.controlDiv).find(".overlayTbl").find("tr").eq(index);

      row.find(".dragOff").attr("class", "drag");
      row.find(".showOff").attr("class", "show");

      if(labelVisible == true)
         row.find(".show").html("<input type='checkbox' checked='checked' />"); 
      else
         row.find(".show").html("<input type='checkbox' />"); 

      row.find(".type").html("LABEL");

      row.find(".source").html("<center><input size='40' class='ovInput' value='" + location + "' /></center>");

      row.find(".symbol").html("<center><input size='20' class='ovInput' value='" + text + "' /></center>");

      row.find(".scale").html("&nbsp;");

      row.find(".color").html("<input type='text' class='labelColor ovSelect'/>");

      row.find(".color").find(".labelColor").spectrum({
         preferredFormat: "hex",
         color:       color
      });

      row.find(".delOff").attr("class", "delete");

      row.find(".delete").click(function(){
         row.remove();
      });

      jQuery(me.controlDiv).find(".overlayTbl tr:last").hide();
   }


   // "Redraw" button pressed

   me.updateLayers = function()
   {
      var nLayer, nOverlay;
      var ovShow, ovType, ovCsys, ovLocation, ovTable, ovSymbol;
      var ovText, ovSize, ovColumn, ovDataType, ovDataRef, ovColor;

      if(me.debug)
         console.log("DEBUG> LayerControl.updateLayers()");


      nLayer = jQuery(me.controlDiv).find(".sortable .fixed").length-1;

      if(me.debug)
         console.log(nLayer + " layers");

      nOverlay = 0;

      me.viewer.updateJSON.overlay = [];

      for(var i=0; i<nLayer; ++i)
      {
         ovShow = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".show :checkbox").is(":checked");

         if(ovShow == false) ovShow = 0;
         if(ovShow == true)  ovShow = 1;


         ovType = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".type").text();

         if(me.debug)
            console.log("Layer " + i + " type: [" + ovType + "]  checked: " + ovShow);


         // GRID 

         if(ovType == "GRID")
         {
            ovCsys  = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".source").find("select option:selected").val();

            ovColor = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".gridColor").spectrum('get').toHexString();

            if(me.debug)
               console.log("GRID:  color = " + ovColor + ", grid = " + ovCsys);

            me.viewer.updateJSON.overlay[nOverlay] = {};

            me.viewer.updateJSON.overlay[nOverlay].type     = "grid";
            me.viewer.updateJSON.overlay[nOverlay].coordSys = ovCsys;
            me.viewer.updateJSON.overlay[nOverlay].color    = ovColor;
            me.viewer.updateJSON.overlay[nOverlay].visible  = ovShow;

            ++nOverlay;
         }


         // CATALOG 

         else if(ovType == "SOURCETABLE")
         {
            ovTable    = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".source .fileName").text();

            ovColumn   = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".source .colName").text();

            ovSymbol   = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".symbol").find(".symType").find("option:selected").val();
             
            ovSize     = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".symbol").find("input").val();
             
            ovDataType = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".scale").find(".dataType").find("option:selected").val();

            ovDataRef  = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".scale").find(".dataRef").val();
             
            ovColor    = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".catColor").spectrum('get').toHexString();

            if(me.debug)
               console.log("CATALOG: color = " + ovColor  + ", csys = " + ovCsys + ", symbol = " + ovSymbol + ", table = " + ovTable + ", column = " + ovColumn + ", size = " + ovSize + ", type = " + ovDataType   + ", ref = " + ovDataRef);


            me.viewer.updateJSON.overlay[nOverlay] = {};

            me.viewer.updateJSON.overlay[nOverlay].type     = "catalog";
            me.viewer.updateJSON.overlay[nOverlay].coordSys = ovCsys;
            me.viewer.updateJSON.overlay[nOverlay].color    = ovColor;
            me.viewer.updateJSON.overlay[nOverlay].dataFile = ovTable;
            me.viewer.updateJSON.overlay[nOverlay].dataCol  = ovColumn;
            me.viewer.updateJSON.overlay[nOverlay].dataRef  = ovDataRef;
            me.viewer.updateJSON.overlay[nOverlay].dataType = ovDataType;
            me.viewer.updateJSON.overlay[nOverlay].symType  = ovSymbol;
            me.viewer.updateJSON.overlay[nOverlay].symSize  = ovSize;
            me.viewer.updateJSON.overlay[nOverlay].visible  = ovShow;

            ++nOverlay;
         }


         // IMAGES 

         else if(ovType == "IMAGEOUTLINES")
         {
            ovTable = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".source .fileName").text();

            ovColor = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".imgColor").spectrum('get').toHexString();

            if(me.debug)
               console.log("IMAGES: color = " + ovColor + ", table = " + ovTable );

            me.viewer.updateJSON.overlay[nOverlay] = {};

            me.viewer.updateJSON.overlay[nOverlay].type     = "imginfo";
            me.viewer.updateJSON.overlay[nOverlay].color    = ovColor;
            me.viewer.updateJSON.overlay[nOverlay].dataFile = ovTable;
            me.viewer.updateJSON.overlay[nOverlay].visible  = ovShow;

            ++nOverlay;
         }


         // MARK  

         else if(ovType == "MARK")
         {
            ovLocation = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".source").find("input").val();

            ovSymbol    = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".symbol").find("select option:selected").val();

            ovSize      = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".scale").find("input").val();

            ovColor     = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".markColor").spectrum('get').toHexString();

            if(me.debug)
               console.log("MARK: color = " + ovColor + ", location = " + ovLocation + ", symbol = " + ovSymbol + ", size = " + ovSize );

            me.viewer.updateJSON.overlay[nOverlay] = {};

            me.viewer.updateJSON.overlay[nOverlay].type     = "mark";
            me.viewer.updateJSON.overlay[nOverlay].color    = ovColor;
            me.viewer.updateJSON.overlay[nOverlay].symType  = ovSymbol;
            me.viewer.updateJSON.overlay[nOverlay].symSize  = ovSize;
            me.viewer.updateJSON.overlay[nOverlay].location = ovLocation;
            me.viewer.updateJSON.overlay[nOverlay].visible  = ovShow;

            ++nOverlay;
         }


         // LABEL 

         else if(ovType == "LABEL")
         {
            ovLocation = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".source").find("input").val();

            ovText     = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".symbol").find("input").val();

            ovColor    = jQuery(me.controlDiv).find(".sortable .fixed").eq(i).find(".labelColor").spectrum('get').toHexString();

            if(me.debug && ovText.length > 0)
               console.log("LABEL: color = " + ovColor + ", location = " + ovLocation + ", text = " + ovText );

            me.viewer.updateJSON.overlay[nOverlay] = {};

            me.viewer.updateJSON.overlay[nOverlay].type     = "label";
            me.viewer.updateJSON.overlay[nOverlay].color    = ovColor;
            me.viewer.updateJSON.overlay[nOverlay].location = ovLocation;
            me.viewer.updateJSON.overlay[nOverlay].text     = ovText;
            me.viewer.updateJSON.overlay[nOverlay].visible  = ovShow;

            ++nOverlay;
         }
      }

      me.viewer.submitUpdateRequest();
   }



   // Build the control div contents

   me.makeControl = function()
   {
      if(me.debug)
         console.log("DEBUG> LayerControl.makeControl()");

      var controlHTML = ""

      + "<center><input type='submit' class='updateBtn' value='Update Display'></center><br/>"

      + "<div class='layerDiv'>"
      + "<fieldset class='fieldset'>"
      + "  <div>"
      + "   <table class='overlayTbl'>"
      + "      <thead>"
      + "         <tr>"
      + "            <td class='ovlyhdSmall'>move</td>"
      + "            <td class='ovlyhdSmall'>show</td>"
      + "            <td class='ovlyhd'>Overlay Type</td>"
      + "            <td class='ovlyhd'>Data Source / Location</td>"
      + "            <td class='ovlyhd'>Symbol / Text</td>"
      + "            <td class='ovlyhd'>Scale</td>"
      + "            <td class='ovlyhd'>Color</td>"
      + "            <td class='ovlyhdSmall'>del</td>"
      + "         </tr>"
      + "      </thead>"
      + "      <tbody class='sortable'>"
      + "         <tr class='fixed'>"
      + "            <td class='dragOff'>&nbsp;</td>"
      + "            <td class='showOff'>&nbsp;</td>"
      + "            <td class='type'>&nbsp;</td>"
      + "            <td class='source'> &nbsp; </td>"
      + "            <td class='symbol'> &nbsp; </td>"
      + "            <td class='scale'> &nbsp; </td>"
      + "            <td class='color'> &nbsp; </td>"
      + "            <td class='delOff'>&nbsp;</td>"
      + "         </tr>"
      + "      </tbody>"
      + "   </table>"
      + "</div>"
      + "</fieldset>"
      + "</div>";

      if(me.debug)
         console.log("DEBUG> LayerControl.makeControl(): setting HTML");

      jQuery(me.controlDiv).html(controlHTML);

      jQuery(me.controlDiv).find(".updateBtn").click(me.updateLayers);

      jQuery(me.controlDiv).find(".sortable").sortable({ axis: "y", tolerance: "pointer", containment: "parent", cursor: "move", delay: 150, handle: ".drag" });
   }
}
