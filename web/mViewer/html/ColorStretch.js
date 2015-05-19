/**************************/
/* Color Stretch Controls */
/**************************/

function ColorStretch(controlDivName, viewer)
{
   // This object encapsulates a control box that a user can use to:
   //
   // - Select a grayscale image color table.
   // - Select blue/green/red planes in a full-color image.
   // - Set the stretch mode (linear, log, log-log, gaussian histogram 
   //   equalization or log-scaled gaussian histogram equalization) 
   //   and range (by value, percentile, or "sigma" value).
   // - The selected range is echoed back on the form in all 
   //   three of the scales above.
   //
   // The control works within a div and has to know the mViewer instance that is 
   // it's parent.  
   //
   // While this control can be used statically (always up) we have structured it 
   // so that it can be deleted and recreated as is a common use mode for pop-ups.  
   // To facilitate this, we cheat a little bit and create a small state object 
   // as a child of the parent "viewer".  This is safe so long as the no two controls 
   // try to use the same state name and we don't really expect that to ever happen.
   // 
   // There is an added complication for this control in that it needs to deal
   // with either a single grayscale/psuedocolor image or three blue/green/red
   // planes in a full-color image.  Most of the control is the same, though we
   // have to track the state of the three planes separately in that case and
   // this adds the need for maintaining a little state.

   var me = this;

   me.debug = false;

   me.controlDivName = controlDivName;

   me.controlDiv = document.getElementById(me.controlDivName);

   me.viewer = viewer;

   me.colorTbl    = 0;
   me.plane       = "blue";
   me.mode        = "grayscale";
   me.stretchMode = "gaussian-log";

   me.dataMin = 0.0;
   me.dataMax = 1.0;

   me.min        =    0.;
   me.max        =    1.;
   me.minpercent =    0.;
   me.maxpercent =  100.;
   me.minsigma   = -100.;
   me.maxsigma   =  100.;
   me.datamin    =  0.00;
   me.datamax    =  1.00;
   me.bunit      =    "";


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
         console.log("DEBUG> ColorStretch.init()");

      if(typeof(me.viewer.colorState) == "undefined")
         me.initColorState();


      // Create the control div HTML content. This is 
      // just creating a block of text and using it to 
      // replace the (usually empty) target <div> contents.

      me.makeControl();


      // The control HTML has a bunch of sliders, text input
      // fields, and pulldowns.  We need to attach processing
      // to all of them.


      // Create Min slider and set event handler 
      
      jQuery(me.controlDiv).find('.minSlider').slider({ 
         max:     500,
         min:     0,
         slide:   function(event, ui) {
            
            var dataVal = ui.value;

            dataVal = me.data2Str(dataVal, jQuery(me.controlDiv).find('.minUnits').val());

            jQuery(me.controlDiv).find('.minValue').val(dataVal);
         }
      });

      jQuery(me.controlDiv).find('.minSlider').mouseup(function(){ 
         me.stretchVals();
      });
      

      // Create Max slider and set event handler 

      jQuery(me.controlDiv).find('.maxSlider').slider({ 
         max: 500,
         min: 0,
         slide: function(event, ui) {

            var dataVal = ui.value;

            dataVal = me.data2Str(dataVal, jQuery(me.controlDiv).find('.maxUnits').val());

            jQuery(me.controlDiv).find('.maxValue').val(dataVal);
         }
      });

      jQuery(me.controlDiv).find('.maxSlider').mouseup(function(){ 
         me.stretchVals();
      });


      // Min value string event handler 

      jQuery(me.controlDiv).find('.minValue').keydown(function(event){

         if(event.keyCode == 13)
         {
            event.preventDefault();

            var dataVal = jQuery(me.controlDiv).find('.minValue').val();

            dataVal = me.str2Data(dataVal, jQuery(me.controlDiv).find('.minUnits').val());

            jQuery(me.controlDiv).find('.minSlider').slider('value', dataVal);

            me.stretchVals();
         }
      });


      // Max value string event handlers

      jQuery(me.controlDiv).find('.maxValue').keydown(function(event){

         if(event.keyCode == 13)
         {
            event.preventDefault();

            var dataVal = jQuery(me.controlDiv).find('.maxValue').val();

            dataVal = me.str2Data(dataVal, jQuery(me.controlDiv).find('.maxUnits').val());

            jQuery(me.controlDiv).find('.maxSlider').slider('value', dataVal);

            me.stretchVals();
         }
      });


      // Initialize stretch min

      var dataVal;

      jQuery(me.controlDiv).find('.minValue').val('-1');

      dataVal = me.str2Data(-1, 's');

      jQuery(me.controlDiv).find('.minSlider').slider('value', dataVal);


      // Initialize stretch max

      jQuery(me.controlDiv).find('.maxValue').val('max');

      dataVal = me.str2Data('max', 's');

      jQuery(me.controlDiv).find('.maxSlider').slider('value', dataVal);


      // Initial value updates from the viewer

      if(typeof(me.viewer.colorState) != "undefined")
      {
         me.resetColorTblPulldown();
         me.resetColorPlanePulldown();
         me.resetStretchModePulldown();
      }

      me.processUpdate();

      me.resetStretchUnits();
   }


   // Callback to register with a viewer instance so the control can update
   // its internals based on information only the viewer knows (e.g. the 
   // relationship between data stretch values and percentiles / sigma levels)

   me.processUpdate = function()
   {
      me.colorTbl = me.viewer.imgData.colortable;

      me.mode = "grayscale";

      if(typeof(me.colorTbl) == "undefined")
         me.mode = "color";

      if(typeof(me.viewer.colorState) != "undefined")
      {
         me.viewer.colorState.mode        = me.mode;
         me.viewer.colorState.colorTbl    = me.colorTbl;
         me.viewer.colorState.plane       = me.plane;

         me.stretchMode = jQuery(me.controlDiv).find('.stretchMode option:selected').val();
      }

      if(me.mode == "color")
      {
         jQuery(me.controlDiv).find('.colorTbl'  ).hide();
         jQuery(me.controlDiv).find('.colorPlane').show();

         if(me.plane == "blue")
         {
            me.min        = me.viewer.imgData.bmin;
            me.max        = me.viewer.imgData.bmax;
            me.minpercent = me.viewer.imgData.bminpercent;
            me.maxpercent = me.viewer.imgData.bmaxpercent;
            me.minsigma   = me.viewer.imgData.bminsigma;
            me.maxsigma   = me.viewer.imgData.bmaxsigma;
            me.datamin    = me.viewer.imgData.bdatamin;
            me.datamax    = me.viewer.imgData.bdatamax;

            if(typeof(me.viewer.colorState) != "undefined")
            {
               me.viewer.colorState.blueStretchMode = me.stretchMode;
            }
         }
         else if(me.plane == "green")
         {
            me.min        = me.viewer.imgData.gmin;
            me.max        = me.viewer.imgData.gmax;
            me.minpercent = me.viewer.imgData.gminpercent;
            me.maxpercent = me.viewer.imgData.gmaxpercent;
            me.minsigma   = me.viewer.imgData.gminsigma;
            me.maxsigma   = me.viewer.imgData.gmaxsigma;
            me.datamin    = me.viewer.imgData.gdatamin;
            me.datamax    = me.viewer.imgData.gdatamax;

            if(typeof(me.viewer.colorState) != "undefined")
            {
               me.viewer.colorState.greenStretchMode = me.stretchMode;
            }
         }
         else if(me.plane == "red")
         {
            me.min        = me.viewer.imgData.rmin;
            me.max        = me.viewer.imgData.rmax;
            me.minpercent = me.viewer.imgData.rminpercent;
            me.maxpercent = me.viewer.imgData.rmaxpercent;
            me.minsigma   = me.viewer.imgData.rminsigma;
            me.maxsigma   = me.viewer.imgData.rmaxsigma;
            me.datamin    = me.viewer.imgData.rdatamin;
            me.datamax    = me.viewer.imgData.rdatamax;

            if(typeof(me.viewer.colorState) != "undefined")
            {
               me.viewer.colorState.redStretchMode = me.stretchMode;
            }
         }
      }
      else
      {
         jQuery(me.controlDiv).find('.colorPlane').hide();
         jQuery(me.controlDiv).find('.colorTbl'  ).show();

         me.min        = me.viewer.imgData.min;
         me.max        = me.viewer.imgData.max;
         me.minpercent = me.viewer.imgData.minpercent;
         me.maxpercent = me.viewer.imgData.maxpercent;
         me.minsigma   = me.viewer.imgData.minsigma;
         me.maxsigma   = me.viewer.imgData.maxsigma;
         me.datamin    = me.viewer.imgData.datamin;
         me.datamax    = me.viewer.imgData.datamax;

         if(typeof(me.viewer.colorState) != "undefined")
         {
            me.viewer.colorState.grayStretchMode = me.stretchMode;
         }
      }

      me.bunit = me.viewer.imgData.bunit;

      jQuery("#" + me.controlDivName + " .minDN" ).html(me.min);
      jQuery("#" + me.controlDivName + " .maxDN" ).html(me.max);
      jQuery("#" + me.controlDivName + " .minPct").html(me.minpercent + " %");
      jQuery("#" + me.controlDivName + " .maxPct").html(me.maxpercent + " %");
      jQuery("#" + me.controlDivName + " .minSig").html(me.minsigma + " &sigma;");
      jQuery("#" + me.controlDivName + " .maxSig").html(me.maxsigma + " &sigma;");

      jQuery("#" + me.controlDivName + " .dataMin").html(me.datamin);
      jQuery("#" + me.controlDivName + " .dataMax").html(me.datamax + " " + me.bunit);

      if(me.mode == "grayscale")
      {
         var opVal = jQuery(me.controlDiv).find('.colorTbl option:selected').val();

         jQuery(me.controlDiv).find('.colorTbl option[value="' + opVal       + '"]').prop("selected", false);
         jQuery(me.controlDiv).find('.colorTbl option[value="' + me.colorTbl + '"]').prop("selected", true);
      }
   }


   me.initColorState = function()
   {
      // We decided to "borrow" space in the parent object 
      // to retain state for this control.  That way we remove 
      // and recreate the control without having to otherwise 
      // manage (hide, show, etc.)  That makes things much 
      // simpler and overall more stateless.
      
      // This is the first time this control has been 
      // instantiated, so set up the state info.  We don't know 
      // at this point what details we will be using, so
      // we'll just set up defaults for everything.

      if(typeof(me.viewer.colorState) != "undefined")
         return;

      me.viewer.colorState = {};

      me.viewer.colorState.mode        = me.mode;
      me.viewer.colorState.colorTbl    = me.colorTbl;
      me.viewer.colorState.plane       = me.plane;

      me.viewer.colorState.grayMinUnits  = "s";
      me.viewer.colorState.grayMaxUnits  = "%";
      me.viewer.colorState.blueMinUnits  = "s";
      me.viewer.colorState.blueMaxUnits  = "%";
      me.viewer.colorState.greenMinUnits = "s";
      me.viewer.colorState.greenMaxUnits = "%";
      me.viewer.colorState.redMinUnits   = "s";
      me.viewer.colorState.redMaxUnits   = "%";

      me.viewer.colorState.grayStretchMode  = me.stretchMode;
      me.viewer.colorState.blueStretchMode  = me.stretchMode;
      me.viewer.colorState.greenStretchMode = me.stretchMode;
      me.viewer.colorState.redStretchMode   = me.stretchMode;
   }


   // When switching planes, we need to check if the units pulldowns
   // have been left in a non-default state by a previous instance
   // of the control.  Also used when we reinstantiate the control.

   me.resetStretchUnits = function()
   {
      var oldMin = jQuery(me.controlDiv).find('.minUnits option:selected').val();
      var oldMax = jQuery(me.controlDiv).find('.maxUnits option:selected').val();

      var newMin, newMax;

      if(me.mode == "grayscale")
      {
         newMin = me.viewer.colorState.grayMinUnits;
         newMax = me.viewer.colorState.grayMaxUnits;
      }

      else  // Color
      {
         if(me.plane == "blue")
         {
            newMin = me.viewer.colorState.blueMinUnits;
            newMax = me.viewer.colorState.blueMaxUnits;
         }
         else if(me.plane == "green")
         {
            newMin = me.viewer.colorState.greenMinUnits;
            newMax = me.viewer.colorState.greenMaxUnits;
         }
         else if(me.plane == "red")
         {
            newMin = me.viewer.colorState.redMinUnits;
            newMax = me.viewer.colorState.redMaxUnits;
         }
      }

      jQuery(me.controlDiv).find('.minUnits option[value="' + oldMin + '"]').prop("selected", false);
      jQuery(me.controlDiv).find('.minUnits option[value="' + newMin + '"]').prop("selected", true);

      jQuery(me.controlDiv).find('.maxUnits option[value="' + oldMax + '"]').prop("selected", false);
      jQuery(me.controlDiv).find('.maxUnits option[value="' + newMax + '"]').prop("selected", true);

      me.setMinUnits();
      me.setMaxUnits();
   }


   // When we reinstantiate the control, we need to check the color table,
   // color plane, and stretch mode pulldowns and reset them if they were
   // in another state when a previous instantiation was closed.

   me.resetColorTblPulldown = function()
   {
      var oldColorTbl = jQuery(me.controlDiv).find('.colorTbl    option:selected').val();

      var newColorTbl = me.viewer.colorState.colorTbl;

      jQuery(me.controlDiv).find('.colorTbl    option[value="' + oldColorTbl    + '"]').prop("selected", false);
      jQuery(me.controlDiv).find('.colorTbl    option[value="' + newColorTbl    + '"]').prop("selected", true);

      me.colorTbl = newColorTbl;
   }

   me.resetColorPlanePulldown = function()
   {
      var oldColorPlane = jQuery(me.controlDiv).find('.colorPlane  option:selected').val();

      var newColorPlane = me.viewer.colorState.plane;

      jQuery(me.controlDiv).find('.colorPlane  option[value="' + oldColorPlane  + '"]').prop("selected", false);
      jQuery(me.controlDiv).find('.colorPlane  option[value="' + newColorPlane  + '"]').prop("selected", true);

      me.plane = newColorPlane;
   }

   me.resetStretchModePulldown = function()
   {
      var oldStretchMode = jQuery(me.controlDiv).find('.stretchMode option:selected').val();

      var newStretchMode;

      if(me.mode == "grayscale")
         newStretchMode = me.viewer.colorState.grayStretchMode;
      else
      {
         if(me.plane == "blue")
            newStretchMode = me.viewer.colorState.blueStretchMode;

         else if(me.plane == "green")
            newStretchMode = me.viewer.colorState.greenStretchMode;

         else if(me.plane == "red")
            newStretchMode = me.viewer.colorState.redStretchMode;
      }

      jQuery(me.controlDiv).find('.stretchMode option[value="' + oldStretchMode + '"]').prop("selected", false);
      jQuery(me.controlDiv).find('.stretchMode option[value="' + newStretchMode + '"]').prop("selected", true);

      me.stretchMode = newStretchMode;
   }


   /* Convert slider value to range string */

   me.data2Str = function(data, units)
   {
      if(me.debug)
         console.log("DEBUG> data2Str(\"" + data + "\",\"" + units + "\")");

      var val;

      if(units == '%')
         val = data/5.;

      else if(units == 'DN')
         val = (1.*data)/500. * (me.datamax - me.datamin) + me.datamin;

      else if(units == 's')
      {
         if(data == 0)
            val = "min";

         else if(data ==  500)
            val = "max";

         else if(data <= 250.)
         {
            val = -2. + (250. - data)/250. * 4.;

            val = -Math.pow(10., val);

            if(val <= -10.)
               val = val.toFixed(0);
            else if(val <= -3.)
               val = val.toFixed(1);
            else
               val = val.toFixed(2);
         }

         else if(data > 250.) 
         {
            val = -2. + (data - 250.)/250. * 4.;

            val = Math.pow(10., val);

            if(val >=  10.)
               val = val.toFixed(0);
            else if(val >= 3.)
               val = val.toFixed(1);
            else
               val = val.toFixed(2);
         }
      }

      else
         val = 0.;

      return val;
   }

   
   /* Convert range string to slider value */

   me.str2Data = function(str, units)
   {
      if(me.debug)
         console.log("DEBUG> str2Data(\"" + str + "\",\"" + units + "\")");

      var val;
      var data;

      val = str;

      if(str == "min")
         data = 0;

      else if(str == "max")
         data = 500;

      else if(units == '%')
         data = val * 5.;

      else if(units == 'DN')
         data = 500. * (val - me.datamin) / (me.datamax - me.datamin); 

      else if(units == 's')
      {
         if(val < 0)
         {
            val = 0.434294482 * Math.log(-val);

            data = 250. - (val + 2.) / 4. * 250.;
         }
         else if(val > 0)
         {
            val = 0.434294482 * Math.log(val);

            data = 250. + (val + 2.) / 4. * 250.;
         }
         else
            data = 250.;
      }

      if(data <   0.) data =   0.;
      if(data > 500.) data = 500.;

      return data;
   }


   // Set the color table 

   me.setColorTable = function()
   {
      if(me.debug)
         console.log("DEBUG> setColorTable()");

      me.colorTbl = jQuery(me.controlDiv).find('.colorTbl option:selected').val();

      me.stretchVals();
   }


   // Set the stretch mode

   me.setStretchMode = function()
   {
      if(me.debug)
         console.log("DEBUG> setStretchMode()");

      me.stretchVals();
   }


   // Set the stretch min units

   me.setMinUnits = function()
   {
      if(me.debug)
         console.log("DEBUG> setMinUnits()");

      var dataStr;
      var dataVal;

      var minUnits= jQuery(me.controlDiv).find('.minUnits option:selected').val();

           if(minUnits == "s")  dataStr = me.minsigma;
      else if(minUnits == "%")  dataStr = me.minpercent;
      else if(minUnits == "DN") dataStr = me.min;

      dataVal = me.str2Data(dataStr, minUnits);

      jQuery(me.controlDiv).find('.minSlider').slider('value', dataVal);

      if(dataStr == "min" || dataStr == "max")
         jQuery(me.controlDiv).find('.minValue').val(dataStr);
      else
         jQuery(me.controlDiv).find('.minValue').val((1.*dataStr).toPrecision(3));
   }


   // Set the stretch max units

   me.setMaxUnits = function()
   {
      if(me.debug)
         console.log("DEBUG> setMaxUnits()");

      var dataStr;
      var dataVal;

      var maxUnits= jQuery(me.controlDiv).find('.maxUnits option:selected').val();

           if(maxUnits == "s")  dataStr = me.maxsigma;
      else if(maxUnits == "%")  dataStr = me.maxpercent;
      else if(maxUnits == "DN") dataStr = me.max;

      dataVal = me.str2Data(dataStr, maxUnits);

      jQuery(me.controlDiv).find('.maxSlider').slider('value', dataVal);

      if(dataStr == "min" || dataStr == "max")
         jQuery(me.controlDiv).find('.maxValue').val(dataStr);
      else
         jQuery(me.controlDiv).find('.maxValue').val((1.*dataStr).toPrecision(3));
   }

      

   /* Package up all the stretch info */
   /* and submit for processing       */

   me.stretchVals = function()
   {
      if(me.debug)
         console.log("DEBUG> stretchVals()");


      // Get the GUI settings for stretch 

      var minValue = jQuery(me.controlDiv).find('.minValue').val();
      var maxValue = jQuery(me.controlDiv).find('.maxValue').val();

      var minUnits= jQuery(me.controlDiv).find('.minUnits option:selected').val();
      var maxUnits= jQuery(me.controlDiv).find('.maxUnits option:selected').val();

      me.stretchMode = jQuery(me.controlDiv).find('.stretchMode option:selected').val();

      
      // A few sanity checks

      if(minValue == 'min')
      {
         if(minUnits == "s") minValue = -99999.;

         if(minUnits == "%" ) minValue =      0.;

         if(minUnits == "DN"  )
         {
            minValue = 0.;
            minUnits = "%";
         }
      }

      if(minValue == 'max') 
      {
         if(minUnits == "s") minValue = 99999.;

         if(minUnits == "%" ) minValue =   100.;

         if(minUnits == "DN"  )
         {
            minValue = 100.;
            minUnits = "%";
         }
      }

      if(maxValue == 'min') 
      {
         if(maxUnits == "s") maxValue = -99999.;

         if(maxUnits == "%" ) maxValue =      0.;

         if(maxUnits == "DN"  )
         {
            maxValue = 0.;
            maxUnits = "%";
         }
      }

      if(maxValue == 'max') 
      {
         if(maxUnits == "s") maxValue = 99999.;

         if(maxUnits == "%" ) maxValue =   100.;

         if(maxUnits == "DN"  )
         {
            maxValue = 100.;
            maxUnits = "%";
         }
      }

      if(minUnits == "%" && minValue < 0.)
         minValue = 0.;

      if(minUnits == "%" && minValue > 100.)
         minValue = 100.;

      if(maxUnits == "%" && maxValue < 0.)
         maxValue = 0.;

      if(maxUnits == "%" && maxValue > 100.)
         maxValue = 100.;


      // Send the new parameters to the viewer and request an update
      // Don't forget to keep a record of the control state since
      // the user may kill the control at any time.


      if(minUnits == "DN") minUnits = "";
      if(maxUnits == "DN") maxUnits = "";

      if(me.mode == "grayscale")
      {
         me.viewer.updateJSON.grayFile.colorTable  = me.colorTbl;
         me.viewer.updateJSON.grayFile.stretchMode = me.stretchMode;

         me.viewer.updateJSON.grayFile.stretchMin  = minValue + minUnits;
         me.viewer.updateJSON.grayFile.stretchMax  = maxValue + maxUnits;

         me.viewer.colorState.grayMinUnits = minUnits;
         me.viewer.colorState.grayMaxUnits = maxUnits;

         // me.viewer.colorState.grayStretchMode = me.stretchMode;
      }
      else if(me.plane == "blue")
      {
         me.viewer.updateJSON.blueFile.colorTable  = me.colorTbl;
         me.viewer.updateJSON.blueFile.stretchMode = me.stretchMode;

         me.viewer.updateJSON.blueFile.stretchMin  = minValue + minUnits;
         me.viewer.updateJSON.blueFile.stretchMax  = maxValue + maxUnits;

         me.viewer.colorState.blueMinUnits = minUnits;
         me.viewer.colorState.blueMaxUnits = maxUnits;

         // me.viewer.colorState.blueStretchMode = me.stretchMode;
      }
      else if(me.plane == "green")
      {
         me.viewer.updateJSON.greenFile.colorTable  = me.colorTbl;
         me.viewer.updateJSON.greenFile.stretchMode = me.stretchMode;

         me.viewer.updateJSON.greenFile.stretchMin  = minValue + minUnits;
         me.viewer.updateJSON.greenFile.stretchMax  = maxValue + maxUnits;

         me.viewer.colorState.greenMinUnits = minUnits;
         me.viewer.colorState.greenMaxUnits = maxUnits;

         // me.viewer.colorState.greenStretchMode = me.stretchMode;
      }
      else if(me.plane == "red")
      {
         me.viewer.updateJSON.redFile.colorTable  = me.colorTbl;
         me.viewer.updateJSON.redFile.stretchMode = me.stretchMode;

         me.viewer.updateJSON.redFile.stretchMin  = minValue + minUnits;
         me.viewer.updateJSON.redFile.stretchMax  = maxValue + maxUnits;

         me.viewer.colorState.redMinUnits = minUnits;
         me.viewer.colorState.redMaxUnits = maxUnits;

         // me.viewer.colorState.redStretchMode = me.stretchMode;
      }

      me.viewer.submitUpdateRequest();
   }


   // Build the control div contents

   me.makeControl = function()
   {
      if(me.debug)
         console.log("DEBUG> makeControl()");

      var controlHTML = ""

      + "<div class='stretchDiv'>"
      + "   <center>"

      + "      <fieldset class='fieldset'>"
      + "      <div class='sliderSet'>"

      + "      <table class='sliderTable' border='0'>"
      + "         <tr>"
      + "            <td>"
      + "               <div class='littleLabel'>Stretch&nbsp;Min:</div>"
      + "            </td>"
      + "            <td>"
      + "               <div class='minSlider' style='width:200px;'></div>"
      + "            </td>"
      + "            <td>"
      + "               <input class='minValue' class='sliderVal' value='0'  style='width: 75px;'/>"
      + "            </td>"
      + "            <td>"
      + "               <select class='minUnits'>"
      + "                  <option value='s' selected='selected'>&sigma;</option>"
      + "                  <option value='%' >%</option>"
      + "                  <option value='DN'>DN</option>"
      + "               </select>"
      + "            </td>"
      + "         </tr>"
      + "         <tr>"
      + "            <td>"
      + "               <div class='littleLabel'>Stretch&nbsp;Max:</div>"
      + "            </td>"
      + "            <td>"
      + "               <div class='maxSlider' style='width:200px;'></div>"
      + "            </td>"
      + "            <td>"
      + "               <input class='maxValue' class='sliderVal' value='100' style='width: 75px;' />"
      + "            </td>"
      + "            <td>"
      + "               <select class='maxUnits'>"
      + "                  <option value='s'>&sigma;</option>"
      + "                  <option value='%' selected='selected'>%</option>"
      + "                  <option value='DN'>DN</option>"
      + "               </select>"
      + "            </td>"
      + "         </tr>"
      + "      </table>"
      + "      </div>"
      + "      </fieldset><br/>"

      + "      <div style='height:10px;'/>"
      + "      <table>"
      + "      <tr valign='top'>"
      + "      <td valign='bottom' align='left'>"
      + "         <div class='littleLabel'>"

      + "         Data: <span class='dataMin'>0</span> to <span class='dataMax'>100</span><p/>"

      + "         Color Table<br/>"
      + "         <select class='colorPlane'>"
      + "            <option value='blue' selected='selected'>Blue Plane</option>"
      + "            <option value='green'                   >Green Plane</option>"
      + "            <option value='red'                     >Red Plane</option>"
      + "         </select><p/>"

      + "         <select class='colorTbl'>"
      + "            <option value='0' >Grey Scale</option>"
      + "            <option value='1' selected='selected'>Reverse Grey Scale</option>"
      + "            <option value='3' >Thermal</option>"
      + "            <option value='5' >Reverse Thermal</option>"
      + "            <option value='4' >Logarithmic Thermal</option>"
      + "            <option value='7' >Velocity</option>"
      + "            <option value='8' > Red Ramp</option>"
      + "            <option value='9' >Green Ramp</option>"
      + "            <option value='10'>Blue Ramp</option>"
      + "         </select><p/>"

      + "         Stretch Mode<br/>"
      + "         <select class='stretchMode'>"
      + "            <option value='lin'>Linear</option>"
      + "            <option value='log'>Log</option>"
      + "            <option value='loglog'>Log-log</option>"
      + "            <option value='gaussian'>Gaussian histogram equalization</option>"
      + "            <option value='gaussian-log' selected='selected'>Gaussian histogram equalization (log)</option>"
      + "         </select>"
      + "         </div>"
      + "      </td>"

      + "      <td align='right'><div class='littleLabel'>Stretch&nbsp;in&nbsp;&nbsp;<br/>other&nbsp;&nbsp;<br/>units:&nbsp;&nbsp;</div></td>"
      + "      <td>"
      + "      <table class='dataVals'>"
      + "      <tr>"
      + "         <th>Min</th>"
      + "         <th>Max</th>"
      + "      </tr>"

      + "      <tr>"
      + "         <td class='minDN'>101.157</td>"
      + "         <td class='maxDN'>199.335</td>"
      + "      </tr>"

      + "      <tr>"
      + "         <td class='minPct'>79.37&nbsp;%</td>"
      + "         <td class='maxPct'>98.46&nbsp;%</td>"
      + "      </tr>"

      + "      <tr>"
      + "         <td class='minSig'>-1&nbsp;&sigma;</td>"
      + "         <td class='maxSig'>20.01&nbsp;&sigma;</td>"
      + "      </tr>"
      + "      </table>"
      + "      </tr>"
      + "      </table>"
      + "   </center>"
      + "</div>";

      if(me.debug)
         console.log("DEBUG> makeControl() setting HTML");

      jQuery(me.controlDiv).html(controlHTML);

      jQuery(me.controlDiv).find('.minUnits').change(function(){ 
         me.setMinUnits();
      });

      jQuery(me.controlDiv).find('.maxUnits').change(function(){ 
         me.setMaxUnits();
      });

      jQuery(me.controlDiv).find('.colorPlane').change(function(){ 
      
         me.plane = jQuery(me.controlDiv).find('.colorPlane option:selected').val();

         me.resetStretchModePulldown();

         me.processUpdate();

         me.resetStretchUnits();
      });

      jQuery(me.controlDiv).find('.colorTbl').change(function(){ 
         me.setColorTable();
      });

      jQuery(me.controlDiv).find('.stretchMode').change(function(){ 
         me.setStretchMode();
      });
   }
}
