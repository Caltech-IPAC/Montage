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

   me.debug = true;

   me.controlDivName = controlDivName;

   me.controlDiv = document.getElementById(me.controlDivName);

   me.viewer = viewer;

   me.colorTbl    = 0;
   me.plane       = "blue";
   me.mode        = "grayscale";
   me.stretchMode = "gaussian-log";

   me.minStr;
   me.maxStr;
   me.minVal;
   me.maxVal;
   me.minUnit;
   me.maxUnit;

   me.min        =    0.;
   me.max        =    1.;
   me.percMin =    0.;
   me.percMax =  100.;
   me.sigmaMin   = -100.;
   me.sigmaMax   =  100.;
   me.dataMin    =  0.00;
   me.dataMax    =  1.00;
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

      if(me.debug) {
         console.log("DEBUG> ColorStretch.init()");
      }

      var data = me.viewer.imgData;
      if (data == null)
         return;

      
      if (data.grayFile != null) {
          
	  me.colorTbl = data.grayFile.colorTable;
          me.dataMin = data.grayFile.dataMin;
          me.dataMax = data.grayFile.dataMax;
          me.percMin = data.grayFile.percMin;
          me.percMax = data.grayFile.percMax;
          me.sigmaMin = data.grayFile.sigmaMin;
          me.sigmaMax = data.grayFile.sigmaMax;
	  me.min = data.grayFile.dispMin;
	  me.max = data.grayFile.dispMax;
	  me.bunit = data.grayFile.bunit;

          me.minStr = data.grayFile.stretchMin;
          me.maxStr = data.grayFile.stretchMax;
          me.Mode = data.grayFile.stretchMode;
      }
      else if (me.colorPlane == "red") {
      
          me.dataMin = data.redFile.dataMin;
          me.dataMax = data.redFile.dataMax;
          me.percMin = data.redFile.percMin;
          me.percMax = data.redFile.percMax;
          me.sigmaMin = data.redFile.sigmaMin;
          me.sigmaMax = data.redFile.sigmaMax;
	  me.min = data.redFile.dispMin;
	  me.max = data.redFile.dispMax;
	  me.bunit = data.redFile.bunit;

          me.minStr = data.redFile.stretchMin;
          me.maxStr = data.redFile.stretchMax;
          me.Mode = data.redFile.stretchMode;
      }
      else if (me.colorPlane == "green") {

	  me.colorTbl = data.greenFile.colorTable;
          me.dataMin = data.greenFile.dataMin;
          me.dataMax = data.greenFile.dataMax;
          me.percMin = data.greenFile.percMin;
          me.percMax = data.greenFile.percMax;
          me.sigmaMin = data.greenFile.sigmaMin;
          me.sigmaMax = data.greenFile.sigmaMax;
	  me.min = data.greenFile.dispMin;
	  me.max = data.greenFile.dispMax;
	  me.bunit = data.redFile.bunit;

          me.minStr = data.greenFile.stretchMin;
          me.maxStr = data.greenFile.stretchMax;
          me.Mode = data.greenFile.stretchMode;
      }
      else if (me.colorPlane == "blue") {

	  me.colorTbl = data.blueFile.colorTable;
          me.dataMin = data.blueFile.dataMin;
          me.dataMax = data.blueFile.dataMax;
          me.percMin = data.blueFile.percMin;
          me.percMax = data.blueFile.percMax;
          me.sigmaMin = data.blueFile.sigmaMin;
          me.sigmaMax = data.blueFile.sigmaMax;
	  me.min = data.blueFile.dispMin;
	  me.max = data.blueFile.dispMax;
	  me.bunit = data.redFile.bunit;

          me.minStr = data.blueFile.stretchMin;
          me.maxStr = data.blueFile.stretchMax;
          me.Mode = data.blueFile.stretchMode;
      }

        if (me.debug) {
            console.log ("stretchmin= " + me.minStr);
            console.log ("stretchmax= " + me.maxStr);
            console.log ("stretchmode= " + me.stretchMode);
        }

        me.minVal = parseStretchValue (me.minStr);
        me.maxVal = parseStretchValue (me.maxStr);
        me.minUnit = parseStretchUnit (me.minStr);
        me.maxUnit = parseStretchUnit (me.maxStr);

	if (me.debug) {
            console.log ("minval= " + me.minVal);
            console.log ("maxval= " + me.maxVal);
            console.log ("minunit= " + me.minUnit);
            console.log ("maxunit= " + me.maxUnit);
	}


      // Create the control div HTML content. This is 
      // just creating a block of text and using it to 
      // replace the (usually empty) target <div> contents.

      if(me.debug) {
         console.log("DEBUG> call me.makeControl");
      }

      me.makeControl();

      if(me.debug) {
         console.log("DEBUG> returned me.makeControl");
      }


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

      if(me.debug) {
         console.log("DEBUG> xxx1");
      }

      jQuery(me.controlDiv).find('.minSlider').mouseup(function(){ 
          
	  if(me.debug) {
             console.log("DEBUG> call me.stretchVals -- minslider");
          }

         me.stretchVals();
      });
      
      if(me.debug) {
         console.log("DEBUG> xxx2");
      }


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

      if(me.debug) {
         console.log("DEBUG> xxx3");
      }

      jQuery(me.controlDiv).find('.maxSlider').mouseup(function(){ 
	  
	  if(me.debug) {
             console.log("DEBUG> call me.stretchVals -- maxslider");
          }

         me.stretchVals();
      });


      if(me.debug) {
         console.log("DEBUG> xxx4");
      }

      // Min value string event handler 

      jQuery(me.controlDiv).find('.minValue').keydown(function(event){

         if(event.keyCode == 13)
         {
            event.preventDefault();

            var dataVal = jQuery(me.controlDiv).find('.minValue').val();

            dataVal = me.str2Data(dataVal, jQuery(me.controlDiv).find('.minUnits').val());

            jQuery(me.controlDiv).find('.minSlider').slider('value', dataVal);

	  if(me.debug) {
             console.log("DEBUG> call me.stretchVals -- minval");
          }

            me.stretchVals();
         }
      });


      if(me.debug) {
         console.log("DEBUG> xxx5");
      }

      // Max value string event handlers

      jQuery(me.controlDiv).find('.maxValue').keydown(function(event){

         if(event.keyCode == 13)
         {
            event.preventDefault();

            var dataVal = jQuery(me.controlDiv).find('.maxValue').val();

            dataVal = me.str2Data(dataVal, jQuery(me.controlDiv).find('.maxUnits').val());

            jQuery(me.controlDiv).find('.maxSlider').slider('value', dataVal);

	  if(me.debug) {
             console.log("DEBUG> call me.stretchVals -- maxval");
          }

            me.stretchVals();
         }
      });


      if(me.debug) {
         console.log("DEBUG> xxx6");
      }

      // Initialize stretch min

      var dataVal;

      jQuery(me.controlDiv).find('.minValue').val('-1');

      dataVal = me.str2Data(-1, 's');

      jQuery(me.controlDiv).find('.minSlider').slider('value', dataVal);

      if(me.debug) {
         console.log("DEBUG> xxx7");
      }


      // Initialize stretch max

      jQuery(me.controlDiv).find('.maxValue').val('max');

      dataVal = me.str2Data('max', 's');

      jQuery(me.controlDiv).find('.maxSlider').slider('value', dataVal);

      if(me.debug) {
         console.log("DEBUG> xxx8");
      }


      // Initial value updates from the viewer

/*
      if(typeof(me.viewer.colorState) != "undefined")
      {
*/

	  if(me.debug) {
             console.log("DEBUG> call me.resetColorTblPulldown");
          }

         me.resetColorTblPulldown();
	  
	  if(me.debug) {
             console.log("DEBUG> call me.resetColorPanelPulldown");
          }

         me.resetColorPlanePulldown();
	  
	  if(me.debug) {
             console.log("DEBUG> call me.resetStretchModePulldown");
          }

         me.resetStretchModePulldown();
/*      
      }
*/

	  if(me.debug) {
             console.log("DEBUG> call me.processUpdate");
          }

      me.processUpdate();

	  if(me.debug) {
             console.log("DEBUG> call me.resetStretchUnits");
          }

      me.resetStretchUnits();
   }


me.parseStretchValue = function (stretchstr)
{
    if (me.debug) {
        console.log ("From parseStretchValue: stretchstr= " + stretchstr); 
    }

    var data = viewer.imgData; 
    if (data == null)
        return;

/*
    parse stretcmin and stretchmax to find stretch value 
*/
    var value = "";
    
    var ind;
    ind = stretchstr.indexOf ('%');
    if (ind != -1) {
        value = stretchstr.substring (0, ind);        
    }
    else {
        ind = stretchstr.indexOf ('s');
        if (ind != -1) {
            value = stretchstr.substring (0, ind);        
        }
	else {
            value = stretchstr;        
	}
    }

    if (me.debug) {
	console.log ("value= " + value);
    } 

    return (value);
}


me.parseStretchUnit = function (stretchstr)
{
    if (me.debug) {
        console.log ("From parseStretchUnit: stretchstr= " + stretchstr); 
    }

    var data = viewer.imgData; 
    if (data == null)
        return;

/*
    parse stretcmin and stretchmax to find stretch unit
*/
    var unit = "";

    var ind;
    ind = stretchstr.indexOf ('%');
    if (ind != -1) {
        unit = "perc";
    }
    else {
        ind = stretchstr.indexOf ('s');
        if (ind != -1) {
            unit = "sigma";
        }
	else {
            if ((stretchstr == "min") ||
	        (stretchstr == "max")) {
	        unit = "perc";
	    }
	    else {
	        unit = "val";
	    }
	}
    }
    
    if (me.debug) {
	console.log ("unit= " + unit);
    } 

    return (unit) ;
}


   // Callback to register with a viewer instance so the control can update
   // its internals based on information only the viewer knows (e.g. the 
   // relationship between data stretch values and percentiles / sigma levels)

   me.processUpdate = function()
   {
        if(me.debug) {
            console.log("DEBUG> From  ColorStretch.processUpdate");
        }

        if ( me.viewer.imgData == null)
            return;
        
        me.mode = "grayscale";

        if (me.viewer.imgData.grayFile == null)
            me.mode = "color";

	if (me.viewer.imgData.grayFile != null) {
            
	    me.colorTbl = me.viewer.imgData.grayFile.colorTable;
	
	    if(me.debug) {
                console.log("DEBUG> me.colorTbl= " + me.colorTbl);
            }
	}
       

/*
      if(typeof(me.viewer.colorState) != "undefined")
      {
         me.viewer.colorState.mode        = me.mode;
         me.viewer.colorState.colorTbl    = me.colorTbl;
         me.viewer.colorState.plane       = me.plane;

	  if(me.debug) {
             console.log("DEBUG> me.mode= " + me.mode);
             console.log("DEBUG> me.plane= " + me.plane);
          }

         me.stretchMode = jQuery(me.controlDiv).find('.stretchMode option:selected').val();

	  if(me.debug) {
             console.log("DEBUG> me.stretchMode= " + me.stretchMode);
          }
      }
*/


      if(me.mode == "color")
      {
	  if(me.debug) {
             console.log("DEBUG>here1: mode== color");
          }
	    
	  me.viewer.colorPlane       = me.plane;

	  if(me.debug) {
              console.log("DEBUG> me.colorPlane= " + me.colorPlane);
          }

         jQuery(me.controlDiv).find('.colorTbl'  ).hide();
         jQuery(me.controlDiv).find('.colorPlane').show();

         if(me.plane == "blue")
         {
            me.min        = me.viewer.imgData.blueFile.dispMin;
            me.max        = me.viewer.imgData.blueFile.dispMax;
            me.minpercent = me.viewer.imgData.blueFile.percMin;
            me.maxpercent = me.viewer.imgData.blueFile.percMax;
            me.minsigma   = me.viewer.imgData.blueFile.sigmaMin;
            me.maxsigma   = me.viewer.imgData.blueFile.sigmaMax;
            me.datamin    = me.viewer.imgData.blueFile.dataMin;
            me.datamax    = me.viewer.imgData.blueFile.dataMax;
            me.stretchMode = me.viewer.imgData.blueFile.stretchMode;
	
	    if(me.debug) {
               console.log("DEBUG>me.plane == blue");
               console.log("DEBUG>datamin= " + me.datamin);
               console.log("DEBUG>datamax= " + me.datamax);
               console.log("DEBUG>minsigma= " + me.minsigma);
               console.log("DEBUG>maxsigma= " + me.maxsigma);
               console.log("DEBUG>minpercent= " + me.minpercent);
               console.log("DEBUG>maxpercent= " + me.maxpercent);
               console.log("DEBUG>min= " + me.min);
               console.log("DEBUG>max= " + me.max);
               console.log("DEBUG>stretchMode= " + me.stretchMode);
            }


         }
         else if(me.plane == "green")
         {
            me.min        = me.viewer.imgData.greenFile.dispMin;
            me.max        = me.viewer.imgData.greenFile.dispMax;
            me.minpercent = me.viewer.imgData.greenFile.percMin;
            me.maxpercent = me.viewer.imgData.greenFile.percMin;
            me.minsigma   = me.viewer.imgData.greenFile.sigmaMin;
            me.maxsigma   = me.viewer.imgData.greenFile.sigmaMax;
            me.datamin    = me.viewer.imgData.greenFile.dataMin;
            me.datamax    = me.viewer.imgData.greenFile.dataMax;
            me.stretchMode = me.viewer.imgData.greenFile.stretchMode;
	
	    if(me.debug) {
               console.log("DEBUG>me.plane == green");
               console.log("DEBUG>datamin= " + me.datamin);
               console.log("DEBUG>datamax= " + me.datamax);
               console.log("DEBUG>minsigma= " + me.minsigma);
               console.log("DEBUG>maxsigma= " + me.maxsigma);
               console.log("DEBUG>minpercent= " + me.minpercent);
               console.log("DEBUG>maxpercent= " + me.maxpercent);
               console.log("DEBUG>min= " + me.min);
               console.log("DEBUG>max= " + me.max);
               console.log("DEBUG>stretchMode= " + me.stretchMode);
            }

         }
         else if(me.plane == "red")
         {
            me.min        = me.viewer.imgData.redFile.dispMin;
            me.max        = me.viewer.imgData.redFile.dispMax;
            me.minpercent = me.viewer.imgData.redFile.percMin;
            me.maxpercent = me.viewer.imgData.redFile.percMax;
            me.minsigma   = me.viewer.imgData.redFile.sigmaMin;
            me.maxsigma   = me.viewer.imgData.redFile.sigmaMax;
            me.datamin    = me.viewer.imgData.redFile.dataMin;
            me.datamax    = me.viewer.imgData.redFile.dataMax;
            me.stretchMode = me.viewer.imgData.redFile.stretchMode;

	    if(me.debug) {
               console.log("DEBUG>me.plane == red");
               console.log("DEBUG>datamin= " + me.datamin);
               console.log("DEBUG>datamax= " + me.datamax);
               console.log("DEBUG>minsigma= " + me.minsigma);
               console.log("DEBUG>maxsigma= " + me.maxsigma);
               console.log("DEBUG>minpercent= " + me.minpercent);
               console.log("DEBUG>maxpercent= " + me.maxpercent);
               console.log("DEBUG>min= " + me.min);
               console.log("DEBUG>max= " + me.max);
               console.log("DEBUG>stretchMode= " + me.stretchMode);
            }

         }
      }
      else
      {
	  if(me.debug) {
             console.log("DEBUG>here2 -- mode=grayscale");
          }

         jQuery(me.controlDiv).find('.colorPlane').hide();
         jQuery(me.controlDiv).find('.colorTbl'  ).show();

         me.min        = me.viewer.imgData.grayFile.dispMin;
         me.max        = me.viewer.imgData.grayFile.dispMax;
         me.minpercent = me.viewer.imgData.grayFile.percMin;
         me.maxpercent = me.viewer.imgData.grayFile.percMax;
         me.minsigma   = me.viewer.imgData.grayFile.sigmaMin;
         me.maxsigma   = me.viewer.imgData.grayFile.sigmaMax;
         me.datamin    = me.viewer.imgData.grayFile.dataMin;
         me.datamax    = me.viewer.imgData.grayFile.dataMax;
         me.stretchMode = me.viewer.imgData.grayFile.stretchMode;

	if(me.debug) {
           console.log("DEBUG>datamin= " + me.datamin);
           console.log("DEBUG>datamax= " + me.datamax);
           console.log("DEBUG>minsigma= " + me.minsigma);
           console.log("DEBUG>maxsigma= " + me.maxsigma);
           console.log("DEBUG>minpercent= " + me.minpercent);
           console.log("DEBUG>maxpercent= " + me.maxpercent);
           console.log("DEBUG>min= " + me.min);
           console.log("DEBUG>max= " + me.max);
           console.log("DEBUG>stretchMode= " + me.stretchMode);
        }

      }

      me.bunit = me.viewer.imgData.bunit;

      jQuery("#" + me.controlDivName + " .minDN" ).html(me.min);
      jQuery("#" + me.controlDivName + " .maxDN" ).html(me.max);
      jQuery("#" + me.controlDivName + " .minPct").html(me.minpercent + " %");
      jQuery("#" + me.controlDivName + " .maxPct").html(me.maxpercent + " %");
      jQuery("#" + me.controlDivName + " .minSig").html(me.minsigma 
          + " &sigma;");
      jQuery("#" + me.controlDivName + " .maxSig").html(me.maxsigma 
          + " &sigma;");

      jQuery("#" + me.controlDivName + " .dataMin").html(me.datamin);
      jQuery("#" + me.controlDivName + " .dataMax").html(me.datamax + " " 
          + me.bunit);

      jQuery(me.controlDiv).find('.stretchMode option[value="' + opVal       
         + '"]').prop("selected", false);
      jQuery(me.controlDiv).find('.stretchMode option[value="' 
         + me.stretchMode + '"]').prop("selected", true);
      
      if(me.mode == "grayscale")
      {
         var opVal 
	     = jQuery(me.controlDiv).find('.colorTbl option:selected').val();

         jQuery(me.controlDiv).find('.colorTbl option[value="' + opVal       
	     + '"]').prop("selected", false);
         jQuery(me.controlDiv).find('.colorTbl option[value="' + me.colorTbl 
	     + '"]').prop("selected", true);
      
         var opVal 
	     = jQuery(me.controlDiv).find('.stretchMode option:selected').val();
/*
         jQuery(me.controlDiv).find('.stretchMode option[value="' + opVal       
	     + '"]').prop("selected", false);
         jQuery(me.controlDiv).find('.stretchMode option[value="' 
	     + me.stretchMode + '"]').prop("selected", true);
*/

      }
      else {
          me.resetColorPlanePulldown();

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

      if (debug) {
          console.log ("From ColorStretch.initColorState");
      }

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
        if(me.debug) {
            console.log("DEBUG> From  me.resetStretchUnits");
        }

      var oldMin 
          = jQuery(me.controlDiv).find('.minUnits option:selected').val();
      var oldMax 
          = jQuery(me.controlDiv).find('.maxUnits option:selected').val();

      var newMin, newMax;

      if(me.mode == "grayscale")
      {
        if (me.viewer.imgData != null) {
	    newMin = me.viewer.imgData.grayFile.minUnit;
            newMax = me.viewer.imgData.grayFile.maxUnit;
        }
	else {
	    newMin = me.minUnit;
            newMax = me.maxUnit;
        }
      }

      else  // Color
      {
/*
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
*/

      }

      jQuery(me.controlDiv).find('.minUnits option[value="' 
          + oldMin + '"]').prop("selected", false);
      jQuery(me.controlDiv).find('.minUnits option[value="' 
          + newMin + '"]').prop("selected", true);

      jQuery(me.controlDiv).find('.maxUnits option[value="' 
          + oldMax + '"]').prop("selected", false);
      jQuery(me.controlDiv).find('.maxUnits option[value="' 
          + newMax + '"]').prop("selected", true);

      me.setMinUnits();
      me.setMaxUnits();
   }


   // When we reinstantiate the control, we need to check the color table,
   // color plane, and stretch mode pulldowns and reset them if they were
   // in another state when a previous instantiation was closed.

   me.resetColorTblPulldown = function()
   {
       if(me.debug) {
          console.log("DEBUG> From  me.resetColorTblPulldown");
       }

       if (me.mode == "color")
	    return;
      
      var oldColorTbl = jQuery(me.controlDiv).find('.colorTbl    option:selected').val();

/*
      var newColorTbl = me.viewer.colorState.colorTbl;
*/

      if (me.viewer.imgData.grayFile != null) {
          
	  var newColorTbl = me.viewer.imgData.grayFile.colorTable;

          jQuery(me.controlDiv).find('.colorTbl    option[value="' + oldColorTbl    + '"]').prop("selected", false);
          jQuery(me.controlDiv).find('.colorTbl    option[value="' + newColorTbl    + '"]').prop("selected", true);

          me.colorTbl = newColorTbl;
      } 
   }

   me.resetColorPlanePulldown = function()
   {
	  if(me.debug) {
             console.log("DEBUG> From  me.resetColorPanelPulldown");
          }

        if (me.mode != "color")
	    return;

      var oldColorPlane = jQuery(me.controlDiv).find('.colorPlane  option:selected').val();

      var newColorPlane = me.viewer.colorPlane;

      jQuery(me.controlDiv).find('.colorPlane  option[value="' + oldColorPlane  + '"]').prop("selected", false);
      
      jQuery(me.controlDiv).find('.colorPlane  option[value="' + newColorPlane  + '"]').prop("selected", true);

      me.plane = newColorPlane;
   }

   me.resetStretchModePulldown = function()
   {
	  if(me.debug) {
             console.log("DEBUG> From  me.resetStretchModePulldown");
          }

      var oldStretchMode = jQuery(me.controlDiv).find('.stretchMode option:selected').val();

      var newStretchMode;

      if(me.mode == "grayscale") {
/*
         newStretchMode = me.viewer.colorState.grayStretchMode;
*/


      }
      else
      {
/*
         if(me.plane == "blue")
            newStretchMode = me.viewer.colorState.blueStretchMode;

         else if(me.plane == "green")
            newStretchMode = me.viewer.colorState.greenStretchMode;

         else if(me.plane == "red")
            newStretchMode = me.viewer.colorState.redStretchMode;
*/


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


      me.colorTbl 
          = jQuery(me.controlDiv).find('.colorTbl option:selected').val();

      if(me.debug)
         console.log("DEBUG> me.colorTbl= " + me.colorTbl);
      
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

      var minUnits
          = jQuery(me.controlDiv).find('.minUnits option:selected').val();

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

      if(me.debug)
         console.log("DEBUG> maxUnits= " + maxUnits);
	   
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

      var minUnits
          = jQuery(me.controlDiv).find('.minUnits option:selected').val();
      var maxUnits
          = jQuery(me.controlDiv).find('.maxUnits option:selected').val();

      me.stretchMode 
          = jQuery(me.controlDiv).find('.stretchMode option:selected').val();
      
      me.colorTbl 
          = jQuery(me.controlDiv).find('.colorTbl option:selected').val();

      if(me.debug) {
         console.log("DEBUG> minValue= " + minValue);
         console.log("DEBUG> maxValue= " + maxValue);
     
         console.log("DEBUG> minUnits= " + minUnits);
         console.log("DEBUG> maxUnits= " + maxUnits);
     
         console.log("DEBUG> me.stretchMode= " + me.stretchMode);
         console.log("DEBUG> me.colorTbl= " + me.colorTbl);
      }

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
      
          if(me.debug) {
             console.log("DEBUG> here1");
          } 
	 
	 me.viewer.updateJSON.grayFile.colorTable  = me.colorTbl;
         me.viewer.updateJSON.grayFile.stretchMode = me.stretchMode;

         me.viewer.updateJSON.grayFile.stretchMin  = minValue + minUnits;
         me.viewer.updateJSON.grayFile.stretchMax  = maxValue + maxUnits;

/*
         me.viewer.colorState.grayMinUnits = minUnits;
         me.viewer.colorState.grayMaxUnits = maxUnits;
*/

      }
      else if(me.plane == "blue")
      {
         me.viewer.updateJSON.blueFile.colorTable  = me.colorTbl;
         me.viewer.updateJSON.blueFile.stretchMode = me.stretchMode;

         me.viewer.updateJSON.blueFile.stretchMin  = minValue + minUnits;
         me.viewer.updateJSON.blueFile.stretchMax  = maxValue + maxUnits;

/*
         me.viewer.colorState.blueMinUnits = minUnits;
         me.viewer.colorState.blueMaxUnits = maxUnits;
*/

         // me.viewer.colorState.blueStretchMode = me.stretchMode;
      }
      else if(me.plane == "green")
      {
         me.viewer.updateJSON.greenFile.colorTable  = me.colorTbl;
         me.viewer.updateJSON.greenFile.stretchMode = me.stretchMode;

         me.viewer.updateJSON.greenFile.stretchMin  = minValue + minUnits;
         me.viewer.updateJSON.greenFile.stretchMax  = maxValue + maxUnits;

/*
         me.viewer.colorState.greenMinUnits = minUnits;
         me.viewer.colorState.greenMaxUnits = maxUnits;
*/

         // me.viewer.colorState.greenStretchMode = me.stretchMode;
      }
      else if(me.plane == "red")
      {
         me.viewer.updateJSON.redFile.colorTable  = me.colorTbl;
         me.viewer.updateJSON.redFile.stretchMode = me.stretchMode;

         me.viewer.updateJSON.redFile.stretchMin  = minValue + minUnits;
         me.viewer.updateJSON.redFile.stretchMax  = maxValue + maxUnits;

/*
         me.viewer.colorState.redMinUnits = minUnits;
         me.viewer.colorState.redMaxUnits = maxUnits;
*/

         // me.viewer.colorState.redStretchMode = me.stretchMode;
      }

      if(me.debug) {
          console.log("DEBUG> call me.viewer.submitUpdateRequest");
      } 


      me.viewer.grayOut(true, {'opacity':'50'});
      me.viewer.grayOutMessage(true);

      me.viewer.submitUpdateRequest();
   }


   // Build the control div contents

   me.makeControl = function()
   {
      if(me.debug) {
         console.log("DEBUG> ColorStretch.makeControl()");
      }


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
         
          if(me.debug)
             console.log("DEBUG> call me.setMinUnits");
	 
	 me.setMinUnits();
      });

      jQuery(me.controlDiv).find('.maxUnits').change(function(){ 
          
	  if(me.debug)
             console.log("DEBUG> call me.setMaxUnits");
         
	 me.setMaxUnits();
      });

      jQuery(me.controlDiv).find('.colorPlane').change(function(){ 
      
         me.plane = jQuery(me.controlDiv).find('.colorPlane option:selected').val();

         me.resetStretchModePulldown();

         me.processUpdate();

         me.resetStretchUnits();
      });

      jQuery(me.controlDiv).find('.colorTbl').change(function(){ 
	  
	  if(me.debug)
             console.log("DEBUG> call me.setColorTable");
         
         me.setColorTable();
      });

      jQuery(me.controlDiv).find('.stretchMode').change(function(){ 
	  
	  if(me.debug)
             console.log("DEBUG> call me.setStretchMode");
         
         me.setStretchMode();
      });
   }
}
