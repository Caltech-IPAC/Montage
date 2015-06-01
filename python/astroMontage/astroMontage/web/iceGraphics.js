function iceGraphics(divID)
{
   var me = this;


   var mode  = "DRAW";
   var shape = "BOX";

   me.boxCallback        = null;
   me.clickCallback      = null;
   me.rightClickCallback = null;

   me.moved = false;

   var iceDiv = document.getElementById(divID);

   if(iceDiv == null)
      iceDiv = divID;

   var rect   = iceDiv.getBoundingClientRect();

   var width  = rect.right  - rect.left;
   var height = rect.bottom - rect.top;

   var x0 = 0, y0 = 0;
   var x1 = 0, y1 = 0;

   var xminRef = 0, yminRef = 0;
   var xmaxRef = 0, ymaxRef = 0;

   var mouseIsDown = false;


   var color;
   var image;

   var iceImCanvas = document.createElement("canvas");

   iceImCanvas.style.position = "absolute";
   iceImCanvas.style.left     = 0;
   iceImCanvas.style.top      = 0;
   iceImCanvas.style.width    = width;
   iceImCanvas.style.height   = height;
   iceImCanvas.style.zIndex   = 0;

   iceImCanvas.width          = width;
   iceImCanvas.height         = height;

   iceDiv.appendChild(iceImCanvas);

   var dcimg = iceImCanvas.getContext("2d");


   var iceCanvas = document.createElement("canvas");

   iceCanvas.style.position = "absolute";
   iceCanvas.style.left     = 0;
   iceCanvas.style.top      = 0;
   iceCanvas.style.width    = width;
   iceCanvas.style.height   = height;
   iceCanvas.style.zIndex   = 1;

   iceCanvas.width          = width;
   iceCanvas.height         = height;

   iceDiv.appendChild(iceCanvas);


   var dc = iceCanvas.getContext("2d");

   color          = "#ff0000";

   dc.strokeStyle = color;
   dc.globalAlpha = 1.0;
   dc.lineWidth   = 1;
   dc.lineCap     = "round";
   dc.lineJoin    = "round";


   me.setImage = function(inImage)
   {
      var img = new Image();

      img.onload = function() {
         dcimg.drawImage(img, 0, 0);
      }

      img.src = inImage;

      image = inImage;
   }


   me.setMode = function(inMode)
   {
      mode = inMode;
   }


   me.clear = function()
   {
      clearDrawing();
      clearImage();
   }


   me.redraw = function()
   {
      var parentWidth  = iceDiv.parentNode.style.width;
      var parentHeight = iceDiv.parentNode.style.height;
      
      iceDiv.style.width       = parentWidth
      iceDiv.style.height      = parentHeight;

      iceImCanvas.style.width  = parentWidth;
      iceImCanvas.style.height = parentHeight;

      iceImCanvas.width        = parentWidth;
      iceImCanvas.height       = parentHeight;

      iceImCanvas.setAttribute("width", parentWidth);
      iceImCanvas.setAttribute("height", parentHeight);

      iceCanvas.style.width    = parentWidth;
      iceCanvas.style.height   = parentHeight;

      iceCanvas.width          = parentWidth;
      iceCanvas.height         = parentHeight;

      iceCanvas.setAttribute("width", parentWidth);
      iceCanvas.setAttribute("height", parentHeight);

      var oldRect = rect;

      rect   = iceDiv.getBoundingClientRect();

      width  = rect.right  - rect.left;
      height = rect.bottom - rect.top;

      var dx = rect.left - oldRect.left;
      var dy = rect.top  - oldRect.top;

      var img = new Image();

      img.onload = function() {
         dcimg.drawImage(img, 0, 0);
      }

      img.src = image;

      if(shape == "BOX")
      {
         dc.strokeStyle = color;

         dc.beginPath();

         dc.moveTo(xminRef, yminRef);
         dc.lineTo(xmaxRef, yminRef);
         dc.lineTo(xmaxRef, ymaxRef);
         dc.lineTo(xminRef, ymaxRef);
         dc.lineTo(xminRef, yminRef);

         dc.stroke();
      }
   }


   me.refitCanvas = function()
   {
      var divWidth  = iceDiv.style.width;
      var divHeight = iceDiv.style.height; 
      
      iceImCanvas.style.width  = divWidth;
      iceImCanvas.style.height = divHeight;

      iceImCanvas.width        = divWidth;
      iceImCanvas.height       = divHeight;

      iceImCanvas.setAttribute("width",  divWidth);
      iceImCanvas.setAttribute("height", divHeight);

      iceCanvas.style.width    = divWidth;
      iceCanvas.style.height   = divHeight;

      iceCanvas.width          = divWidth;
      iceCanvas.height         = divHeight;

      iceCanvas.setAttribute("width",  divWidth);
      iceCanvas.setAttribute("height", divHeight);
   }



   me.setColor = function(inColor)
   {
      color = inColor;

      dc.strokeStyle = inColor;
   }


   me.drawLine = function(xmin, ymin, xmax, ymax)
   {
      dc.beginPath();

      dc.moveTo(xmin, ymin);
      dc.lineTo(xmax, ymax);

      dc.stroke();

      xminRef = xmin;
      yminRef = ymin;
      xmaxRef = xmax;
      ymaxRef = ymax;
   }


   me.drawBox = function(xmin, ymin, xmax, ymax)
   {
      dc.beginPath();

      dc.moveTo(xmin, ymin);
      dc.lineTo(xmax, ymin);
      dc.lineTo(xmax, ymax);
      dc.lineTo(xmin, ymax);
      dc.lineTo(xmin, ymin);

      dc.stroke();

      xminRef = xmin;
      yminRef = ymin;
      xmaxRef = xmax;
      ymaxRef = ymax;
   }



   me.getBox = function()
   {
      var box = new Object();

      box.xmin = xminRef;
      box.ymin = yminRef;
      box.xmax = xmaxRef;
      box.ymax = ymaxRef;

      return box;
   }



   iceCanvas.addEventListener("mousedown", mouseDown, false);
   iceCanvas.addEventListener("mousemove", mouseMove, false);
   iceCanvas.addEventListener("mouseup",   mouseUp,   false);


   function mouseDown(e)
   {
      clearDrawing();

      if(e.preventDefault != undefined)
         e.preventDefault();
      if(e.stopPropagation != undefined)
         e.stopPropagation();

      me.moved = false;

      var posx = 0;
      var posy = 0;

      if (!e) var e = window.event;

      if (e.pageX || e.pageY)
      {
         posx = e.pageX - document.body.scrollLeft - document.documentElement.scrollLeft;
         posy = e.pageY - document.body.scrollTop  - document.documentElement.scrollTop;
      }
      else if (e.clientX || e.clientY)
      {
         posx = e.clientX + document.body.scrollLeft + document.documentElement.scrollLeft;
         posy = e.clientY + document.body.scrollTop  + document.documentElement.scrollTop;
      }

      rect = iceDiv.getBoundingClientRect();

      x0 = posx - rect.left;
      y0 = posy - rect.top;

      x1 = posx - rect.left;
      y1 = posy - rect.top;

      mouseIsDown = true;

      if(mode == "DRAG")
      {
         if(shape == "BOX")
         {
            var xoff = (xminRef + xmaxRef)/2. - x1;
            var yoff = (yminRef + ymaxRef)/2. - y1;

            xminRef -= xoff;
            yminRef -= yoff;

            xmaxRef -= xoff;
            ymaxRef -= yoff;

            dc.beginPath();

            dc.moveTo(xminRef, yminRef);
            dc.lineTo(xmaxRef, yminRef);
            dc.lineTo(xmaxRef, ymaxRef);
            dc.lineTo(xminRef, ymaxRef);
            dc.lineTo(xminRef, yminRef);

            dc.stroke();
         }
      }

      return false;
   }


   function mouseMove(e)
   {
      if(!mouseIsDown) 
         return;

      if(e.preventDefault != undefined)
         e.preventDefault();
      if(e.stopPropagation != undefined)
         e.stopPropagation();

      me.moved = true;

      clearDrawing();

      var posx = 0;
      var posy = 0;

      if (!e) var e = window.event;

      if (e.pageX || e.pageY)
      {
         posx = e.pageX - document.body.scrollLeft - document.documentElement.scrollLeft;
         posy = e.pageY - document.body.scrollTop  - document.documentElement.scrollTop;
      }
      else if (e.clientX || e.clientY)
      {
         posx = e.clientX + document.body.scrollLeft + document.documentElement.scrollLeft;
         posy = e.clientY + document.body.scrollTop  + document.documentElement.scrollTop;
      }

      if(mode == "DRAW")
      {
         x1 = posx - rect.left;
         y1 = posy - rect.top;

         dc.strokeStyle = color;
         dc.globalAlpha = 1.0;
         dc.lineWidth   = 2;
         dc.lineCap     = "round";
         dc.lineJoin    = "round";

         dc.beginPath();

         dc.moveTo(x0, y0);
         dc.lineTo(x1, y0);
         dc.lineTo(x1, y1);
         dc.lineTo(x0, y1);
         dc.lineTo(x0, y0);

         dc.stroke();

         xminRef = x0;
         yminRef = y0;
         xmaxRef = x1;
         ymaxRef = y1;
      }
      else if(mode == "DRAG")
      {
         x1 = posx - rect.left;
         y1 = posy - rect.top;

         if(shape == "BOX")
         {
            var xoff = (xminRef + xmaxRef)/2. - x1;
            var yoff = (yminRef + ymaxRef)/2. - y1;

            xminRef -= xoff;
            yminRef -= yoff;

            xmaxRef -= xoff;
            ymaxRef -= yoff;

            dc.beginPath();

            dc.moveTo(xminRef, yminRef);
            dc.lineTo(xmaxRef, yminRef);
            dc.lineTo(xmaxRef, ymaxRef);
            dc.lineTo(xminRef, ymaxRef);
            dc.lineTo(xminRef, yminRef);

            dc.stroke();
         }
      }

      return false;
   }


   function mouseUp(e)
   {
      if(e.preventDefault != undefined)
         e.preventDefault();
      if(e.stopPropagation != undefined)
         e.stopPropagation();

      mouseIsDown = false;

      if(me.moved == false)
      {
        if(e.which > 1)
        {
           if(me.rightClickCallback != null)
              me.rightClickCallback(x0, y0);
        }
        else
        {
           if(me.clickCallback != null)
              me.clickCallback(x0, y0);
        }
      }
      else
      {
         me.moved = false;

         if(me.boxCallback != null)
           me.boxCallback(xminRef, yminRef, xmaxRef, ymaxRef);
      }

      return(false);
   }


   function clearDrawing()
   {
      dc.clearRect(0, 0, iceCanvas.width, iceCanvas.height);
   }


   function clearImage()
   {
      dcimg.clearRect(0, 0, iceCanvas.width, iceCanvas.height);
   }

   return me;
}
