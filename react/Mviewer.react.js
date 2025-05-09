import React, {useState, useRef, useEffect} from 'react';
import PropTypes from 'prop-types';
import './style.css';


const Mviewer = (props) => {
    const {id, img, imgWidth, imgHeight, 
           inset, insetWidth, insetHeight,
           winWidth, winHeight, cutoutDesc,
           zoombox, box, pick, cmdStr,
           setProps} = props;

    const componentDiv = useRef();

    const imgBtn       = useRef();

    const mainBox      = useRef();
    const mainImg      = useRef();
    const mainCanvas   = useRef();

    const insetBox     = useRef();
    const insetImg     = useRef();
    const insetCanvas  = useRef();

    const infoBox      = useRef();

    var cutoutInfo     = useRef();

    var zoomfact       = useRef(1.);

    var mouseIsDown    = useRef(false);
    var mouseHasMoved  = useRef(false);

    var boxXmin        = useRef(0.);
    var boxYmin        = useRef(0.);
    var boxXmax        = useRef(0.);
    var boxYmax        = useRef(0.);

    var screenXmin     = useRef(0.);
    var screenYmin     = useRef(0.);
    var screenDx       = useRef(0.);
    var screenDy       = useRef(0.);

    var zWidth  = zoombox.zoom * imgWidth;
    var zHeight = zoombox.zoom * imgHeight;

    var mWidth  = zWidth  + 'px';
    var mHeight = zHeight + 'px';

    var imgBtnHeight = 25
    var infoHeight   = 20

    var debug = useRef(true);

    window.mview = this;

    cutoutInfo.current = cutoutDesc;

    zoomfact.current = zoombox.zoom;

    try {
        if(debug.current == true)
            console.log('--------------------------------------------------------------------');
            console.log('DEBUG> Initialization: cutoutDesc:                   ', cutoutInfo.current);
            console.log('DEBUG> Initialization: zoombox:                      ', zoombox);
            console.log('DEBUG> Initialization: cmdStr:                       ', cmdStr);
            console.log('DEBUG> Initialization: Set mainImg/mainCanvas width: ', imgWidth*zoomfact.current);
            console.log('DEBUG> Initialization: Set mainImg/mainCanvas height:', imgHeight*zoomfact.current);
        
        mainImg.current.style.width  = imgWidth  * zoomfact.current + "px";
        mainImg.current.style.height = imgHeight * zoomfact.current + "px";

        mainImg.current.width  = imgWidth  * zoomfact.current;
        mainImg.current.height = imgHeight * zoomfact.current;

        mainCanvas.current.style.width  = imgWidth  * zoomfact.current + "px";
        mainCanvas.current.style.height = imgHeight * zoomfact.current + "px";

        mainCanvas.current.width  = imgWidth  * zoomfact.current;
        mainCanvas.current.height = imgHeight * zoomfact.current;


        if(debug.current == true)
            console.log('DEBUG> Initialization: Clear and redraw ctx.');

        var xmin = box.screenXmin;
        var ymin = box.screenYmin;
        var dx   = box.screenDx;
        var dy   = box.screenDy;

        if(dx > 0 && dy > 0)
        {
           var ctx = mainCanvas.current.getContext('2d');

           ctx.clearRect(0, 0, mainImg.current.offsetWidth, mainImg.current.offsetHeight);

           ctx.beginPath();
           ctx.rect(xmin, ymin, dx, dy);
           ctx.strokeStyle = '#ff8080';
           ctx.stroke();
        }
    }

    catch { 
        if(debug.current == true)
            console.log('DEBUG> FAILED (too early)..');
    }

    if(debug.current == true)
    {
        console.log('DEBUG> input props:', props);
        console.log('DEBUG> input props zoombox:', zoombox);
        console.log('DEBUG> zoomed width/height:', zWidth, 'x', zHeight);
        console.log('DEBUG> mainBox:            ', mWidth, 'x', mHeight);
    }


    /* Track resizing of main window box */

    useEffect(() => {

        var xmin = mainBox.current.scrollLeft   / zoomfact.current;
        var ymin = (mainImg.current.offsetHeight - (mainBox.current.scrollTop + mainBox.current.offsetHeight)) / zoomfact.current;

        var dx   = mainBox.current.offsetWidth  / zoomfact.current;
        var dy   = mainBox.current.offsetHeight / zoomfact.current;



       /* Set up box observer */

        const observer = new ResizeObserver((entries) => {

            const boxElement = entries[0];

            console.log('XXX> in ResizeObserver(), cutoutDesc = ' + cutoutInfo.current);
            console.log('XXX> in ResizeObserver(), zoomfact   = ' + zoomfact.current);


            /* Initialize info string at bottom */

            var infoStr =
                '<b>Window:</b> ' + boxElement.contentRect.width + ' x ' + boxElement.contentRect.height + '&emsp;|&emsp;' +
                '<b>Image:</b> ' + imgWidth + ' x ' + imgHeight + '&emsp;|&emsp;';

            if(cutoutInfo.current.length > 0)
                infoStr = infoStr + cutoutInfo.current + '&emsp;|&emsp;';

            if(zoomfact.current >= 0.9999)
                infoStr = infoStr + '<b>Zoom In:</b> ' + (zoomfact.current).toFixed(1);
            else
                infoStr = infoStr + '<b>Zoom Out:</b> ' + (1./zoomfact.current).toFixed(1);

            infoBox.current.innerHTML = infoStr


            /* Adjust some components style. These sometimes */
            /* get confused during React rerendering.        */

            infoBox.current.style.width    = mainBox.current.offsetWidth + 'px';

            mainCanvas.current.style.left  = mainBox.current.offsetLeft  + 'px';
            mainCanvas.current.style.top   = mainBox.current.offsetTop   + 'px';

            insetBox.current.style.left    = mainBox.current.offsetLeft  + 'px';
            insetBox.current.style.top     = mainBox.current.offsetTop   + 'px';

            insetCanvas.current.style.left = mainBox.current.offsetLeft  + 'px';
            insetCanvas.current.style.top  = mainBox.current.offsetTop   + 'px';


            /* For some reason, React turns on mouse event handling for elements */  
            /* besides the one where we actually have onMouse events registered. */  

            mainCanvas.current.style.pointerEvents = 'none';
            mainImg.current.style.pointerEvents    = 'none';


            /* Update zoombox after interactive window resize */
             
            var xmin = mainBox.current.scrollLeft   / zoomfact.current;
            var ymin = (mainImg.current.offsetHeight - (mainBox.current.scrollTop + mainBox.current.offsetHeight)) / zoomfact.current;

            var dx   = mainBox.current.offsetWidth  / zoomfact.current;
            var dy   = mainBox.current.offsetHeight / zoomfact.current;

            var new_zoombox = {
                zoom:   zoomfact.current,

                width:  mainBox.current.offsetWidth,
                height: mainBox.current.offsetHeight,

                xmin:   xmin,
                ymin:   ymin,

                dx:     dx,
                dy:     dy
            };


            if(debug.current == true)
                console.log({'DEBUG> ResizeObserver() setProps(zoombox)': new_zoombox});

            setProps({zoombox: new_zoombox});


            /* Draw the region box on the inset image */

            var insetXmin = insetWidth  * mainBox.current.scrollLeft   / mainImg.current.offsetWidth;
            var insetYmin = insetHeight * mainBox.current.scrollTop    / mainImg.current.offsetHeight;
            var deltaX    = insetWidth  * mainBox.current.offsetWidth  / mainImg.current.offsetWidth;
            var deltaY    = insetHeight * mainBox.current.offsetHeight / mainImg.current.offsetHeight;

            var insetCtx = insetCanvas.current.getContext('2d');

            if(debug.current == true)
                console.log('DEBUG> ResetObserver() Clear and redraw insetCtx.');

            insetCtx.clearRect(0, 0, insetWidth, insetHeight);

            insetCtx.beginPath();
            insetCtx.rect(insetXmin, insetYmin, deltaX, deltaY);
            insetCtx.lineWidth = 2;
            insetCtx.strokeStyle = '#8080ff';
            insetCtx.stroke();
        });


        /* Start size tracker */

        observer.observe(mainBox.current);

    }, []);   /* Empty element box means it won't be triggered  */
              /* by element changes, just the render/rerenders. */


    function runCmd(event) {

        /* Some commands can be processed directly */

        console.log('XXX> in runCmd(), cutoutDesc = ' + cutoutInfo.current);

        var cmd = event.target.id;

        var insetCtx = insetCanvas.current.getContext('2d');

        if(cmd == 'zoomin' || cmd == 'zoomout' 
        || cmd == 'actualPixels' || cmd == 'fitWindow')
        {
            var currWidth    = mainImg.current.offsetWidth;
            var currHeight   = mainImg.current.offsetHeight;
            var scrollLeft   = mainBox.current.scrollLeft;
            var scrollTop    = mainBox.current.scrollTop;
            var offsetWidth  = mainBox.current.offsetWidth;
            var offsetHeight = mainBox.current.offsetHeight;


            var dZoom;

            if(cmd == 'zoomin')
                dZoom = Math.sqrt(2.);

            else if(cmd == 'zoomout')
                dZoom = 1./Math.sqrt(2.);

            else if(cmd == 'actualPixels')
                dZoom = 1./zoomfact.current;

            else if(cmd == 'fitWindow')
            {
                var zoom_x = offsetWidth / imgWidth;
                var zoom_y = offsetHeight / imgHeight;

                var zoom = zoom_x;

                if(zoom_y < zoom)
                    zoom = zoom_y;

                dZoom = 1./zoomfact.current * zoom;
            }

            zoomfact.current = zoomfact.current * dZoom;


            mainImg.current.style.width  = imgWidth  * zoomfact.current + "px";
            mainImg.current.style.height = imgHeight * zoomfact.current + "px";

            mainImg.current.width  = imgWidth  * zoomfact.current;
            mainImg.current.height = imgHeight * zoomfact.current;

            if(debug.current == true)
                console.log('DEBUG> mainImg:', mainImg.current.width, 'x', mainImg.current.height);

            mainCanvas.current.style.width  = imgWidth  * zoomfact.current + "px";
            mainCanvas.current.style.height = imgHeight * zoomfact.current + "px";

            mainCanvas.current.width  = imgWidth  * zoomfact.current;
            mainCanvas.current.height = imgHeight * zoomfact.current;

            console.log('XXX> in ResizeObserver(), cutoutDesc = ' + cutoutInfo.current);
            console.log('XXX> in ResizeObserver(), zoomfact   = ' + zoomfact.current);

            var infoStr =
                '<b>Window:</b> ' + mainBox.current.offsetWidth + ' x ' + mainBox.current.offsetHeight + '&emsp;|&emsp;' +
                '<b>Image:</b> ' + imgWidth + ' x ' + imgHeight + '&emsp;|&emsp;';

            if(cutoutInfo.current.length > 0)
                infoStr = infoStr + cutoutInfo.current + '&emsp;|&emsp;';

            if(zoomfact.current >= 0.9999)
                infoStr = infoStr + '<b>Zoom In:</b> ' + (zoomfact.current).toFixed(1);
            else
                infoStr = infoStr + '<b>Zoom Out:</b> ' + (1./zoomfact.current).toFixed(1);

            infoBox.current.innerHTML = infoStr

            var xoffset = scrollLeft + offsetWidth/2.;
            var new_scrollLeft = dZoom*xoffset - offsetWidth/2.;

            mainBox.current.scrollLeft = new_scrollLeft;

            var yoffset = scrollTop + offsetHeight/2.;
            var new_scrollTop = dZoom*yoffset - offsetHeight/2.;

            mainBox.current.scrollTop = new_scrollTop;


            /* Draw the region box on the inset image */

            var insetXmin = insetWidth  * mainBox.current.scrollLeft   / mainImg.current.offsetWidth;
            var insetYmin = insetHeight * mainBox.current.scrollTop    / mainImg.current.offsetHeight;
            var deltaX    = insetWidth  * mainBox.current.offsetWidth  / mainImg.current.offsetWidth;
            var deltaY    = insetHeight * mainBox.current.offsetHeight / mainImg.current.offsetHeight;

            if(debug.current == true)
                console.log('DEBUG> runCmd() Clear and redraw insetCtx.');

            insetCtx.clearRect(0, 0, insetWidth, insetHeight);

            insetCtx.beginPath();
            insetCtx.rect(insetXmin, insetYmin, deltaX, deltaY);
            insetCtx.lineWidth = 2;
            insetCtx.strokeStyle = '#8080ff';
            insetCtx.stroke();
        }
        else
        {
            if(debug.current == true)
                console.log({'DEBUG> runCmd() setProps(cmdStr)': new_box});

            setProps({cmdStr: cmd})
        }

      
        var ctx = mainCanvas.current.getContext('2d');

        if(debug.current == true)
            console.log('DEBUG> runCmd() Clear ctx.');

        ctx.clearRect(0, 0, mainImg.current.offsetWidth, mainImg.current.offsetHeight);


        var new_box = {xmin: 0, ymin: 0, dx: 0, dy: 0,
                  screenXmin: 0, screenYmin: 0, screenDx: 0, screenDy: 0};

        if(debug.current == true)
            console.log({'DEBUG> runCmd() setProps(box)': new_box});

        setProps({box: new_box});
    }

    return (
        <div ref={componentDiv} id='componentDiv' style={{backgroundColor: '#2c3539', outline: 'none'}}>
         

            {/* Button bar */}
       
            <section ref={imgBtn} id='imgBtn' style={{display: 'flex', padding: '0px', height: imgBtnHeight + 'px', width: '100%'}}>
                <div
                    className='imgBtn'
                    style={{outline: 'none'}}
                >
                    <button id='zoomin'       className='cmdBtn' onClick={runCmd}> Zoom In       </button>
                    <button id='zoomout'      className='cmdBtn' onClick={runCmd}> Zoom Out      </button>
                    <button id='actualPixels' className='cmdBtn' onClick={runCmd}> Actual Pixels </button>
                    <button id='fitWindow'    className='cmdBtn' onClick={runCmd}> Fit to Window </button>
                </div>
            </section>



            {/* Div containing image / drawing overlay canvas */}


            {/*
            NOTES: The setProps() call appears to be doing damage to the minified JS for this code.
                   A workaround appears to be to reset the ten variables pointing to divs, canvases,
                   and graphics contexts.  Or it may have something to do with referring to the object
                   itself from within its own event handlers.

                   When we return coordinates to the Dash Python for processing, we convert the Y
                   values from top-down (as HTML paints them) to bottom, as other image processing
                   software we use has it.
                
                   For now we will remove

                        resize:     'both',

                   from the style below.
            */}

            <section
                ref={mainBox}
                id='mainBox'
                style={{
                    width:      '500px',
                    height:     '500px',
                    overflow:   'auto',
                    userSelect: 'none'
                }}


                onMouseDown={(event) => {

                    mainBox.current.style.overflow = 'auto';

                    mouseIsDown.current = true;

                    var rect = event.target.getBoundingClientRect();

                    var offsetX = event.clientX - rect.left;
                    var offsetY = event.clientY - rect.top;

                    if(debug.current == true)
                    {
                        console.log('------------------------------------------------------');
                        console.log('DEBUG> onMouseDown()');
                        console.log('DEBUG> X:', offsetX);
                        console.log('DEBUG> Y:', offsetY);
                    }

                    if(offsetX < mainBox.current.offsetWidth
                    && offsetY < mainBox.current.offsetHeight)
                        mainBox.current.style.overflow = 'hidden';

                    if(offsetX > mainBox.current.offsetWidth
                    || offsetX > mainBox.current.offsetHeight)
                        mouseIsDown.current = false;


                   {/* Save mainBox x, y */}

                    boxXmin.current = offsetX;
                    boxYmin.current = offsetY;

                    var ctx = mainCanvas.current.getContext('2d');

                    if(debug.current == true)
                        console.log('DEBUG> onMouseDown() Clear ctx.');

                    ctx.clearRect(0, 0, mainImg.current.offsetWidth, mainImg.current.offsetHeight);


                    var new_box = {xmin: 0, ymin: 0, dx: 0, dy: 0,
                              screenXmin: 0, screenYmin: 0, screenDx: 0, screenDy: 0};

                    if(debug.current == true)
                        console.log({'DEBUG> onMouseDown() setProps(box)': new_box});

                    setProps({box: new_box});
                }}


                onMouseMove={(event) => {

                    if(mouseIsDown.current == true)
                    {
                        var rect = event.target.getBoundingClientRect();

                        var offsetX = event.clientX - rect.left;
                        var offsetY = event.clientY - rect.top;
                    
                        boxXmax.current = offsetX;
                        boxYmax.current = offsetY;

                        var dx = boxXmax.current-boxXmin.current;
                        var dy = boxYmax.current-boxYmin.current;

                        mouseHasMoved.current = true;

                        var ctx = mainCanvas.current.getContext('2d');

                        if(debug.current == true)
                            console.log('DEBUG> onMouseMove() Clear and redraw ctx.');

                        ctx.clearRect(0, 0, mainImg.current.offsetWidth, mainImg.current.offsetHeight);

                        ctx.beginPath();
                        ctx.rect(boxXmin.current, boxYmin.current, dx, dy);
                        ctx.strokeStyle = '#ff8080';
                        ctx.stroke();

                        screenXmin.current = boxXmin.current;
                        screenYmin.current = boxYmin.current;
                        screenDx.current   = dx;
                        screenDy.current   = dy;

                        /*
                        if(debug.current == true)
                        {
                            console.log('DEBUG> screenXmin:', screenXmin.current);
                            console.log('DEBUG> screenYmin:', screenYmin.current);
                            console.log('DEBUG> screenDx:  ', screenDx.current);
                            console.log('DEBUG> screenDy:  ', screenDy.current);
                        }
                        */
                    }
                }}


                onMouseUp={(event) => {

                    if(mouseIsDown.current == true)
                    {
                        mouseIsDown.current = false;

                        mainBox.current.style.overflow = 'auto';

                        var rect = event.target.getBoundingClientRect();

                        var offsetX = event.clientX - rect.left;
                        var offsetY = event.clientY - rect.top;
                    
                        if(debug.current == true)
                        {
                            console.log('DEBUG> onMouseUp()');
                            console.log('DEBUG> X:', offsetX);
                            console.log('DEBUG> Y:', offsetY);
                        }

                        var x = offsetX
                        var y = offsetY


                        /* If the zoombox has changed, update the 'zoombox' property. */

                        var oldZoomBox = props.zoombox;

                        var width  = mainBox.current.offsetWidth;
                        var height = mainBox.current.offsetHeight;

                        var xmin   = mainBox.current.scrollLeft   / zoomfact.current;
                        var ymin   = (mainImg.current.offsetHeight 
                                   - (mainBox.current.scrollTop + mainBox.current.offsetHeight)) / zoomfact.current;

                        var dx     = mainBox.current.offsetWidth  / zoomfact.current;
                        var dy     = mainBox.current.offsetHeight / zoomfact.current;

                        if(oldZoomBox.zoom   != zoomfact.current
                        || oldZoomBox.width  != width
                        || oldZoomBox.height != height
                        || oldZoomBox.xmin   != xmin
                        || oldZoomBox.ymin   != ymin
                        || oldZoomBox.dx     != dx
                        || oldZoomBox.dy     != dy)
                        {
                            if(debug.current == true)
                                console.log('DEBUG> MouseUp() oldZoomBox: ', oldZoomBox);

                            var new_zoombox = {
                                zoom:   zoomfact.current,

                                width:  width,
                                height: height,

                                xmin:   xmin,
                                ymin:   ymin,

                                dx:     dx,
                                dy:     dy
                            };

                            if(debug.current == true)
                                console.log('DEBUG> onMouseUp() setProps(zoombox): ', new_zoombox);

                            setProps({zoombox: new_zoombox});
                        }


                        /* Update 'pick' or 'box' property */

                        else
                        {
                            boxXmax.current = x;

                            if(x < boxXmin.current)
                            {
                               boxXmax.current = boxXmin.current;
                               boxXmin.current = x;
                            }

                            boxYmax.current = y;

                            if(y < boxYmin.current)
                            {
                               boxYmax.current = boxYmin.current;
                               boxYmin.current = y;
                            }

                            var deltaX = boxXmax.current - boxXmin.current;
                            var deltaY = boxYmax.current - boxYmin.current;

                            if(debug.current == true)
                            {
                                console.log('DEBUG> deltaX:', deltaX);
                                console.log('DEBUG> deltaY:', deltaY);
                            }

                            if(deltaX == 0 || deltaY == 0 || (deltaX < 5 && deltaY < 5))
                            {
                                if(debug.current == true)
                                    console.log('--> PICK');

                                var xorig = (x + mainBox.current.scrollLeft) / zoomfact.current;
                                var yorig = (mainBox.current.scrollHeight - (y + mainBox.current.scrollTop))/ zoomfact.current;

                                var ctx =  mainCanvas.current.getContext('2d');

                                if(debug.current == true)
                                    console.log('DEBUG> onMouseUp() Clear ctx.');

                                ctx.clearRect(0, 0, mainImg.current.offsetWidth, mainImg.current.offsetHeight);

                                var new_pick = {
                                    x: xorig,
                                    y: yorig
                                }

                                if(debug.current == true)
                                    console.log('DEBUG> onMouseUp() setProps(pick): ', new_pick);

                                setProps({pick: new_pick});
                            }
                            else
                            {
                                if(debug.current == true)
                                    console.log('--> BOX');

                                var xmin = (boxXmin.current + mainBox.current.scrollLeft) / zoomfact.current;
                                var ymin = (mainBox.current.scrollHeight - (boxYmax.current + mainBox.current.scrollTop))/ zoomfact.current;

                                var dx   = deltaX / zoomfact.current;
                                var dy   = deltaY / zoomfact.current;

                                var new_box = {
                                    xmin:       xmin,
                                    ymin:       ymin,
                                    dx:         dx,
                                    dy:         dy,
                                    screenXmin: screenXmin.current,
                                    screenYmin: screenYmin.current,
                                    screenDx:   screenDx.current,
                                    screenDy:   screenDy.current
                                };

                                if(debug.current == true)
                                    console.log('DEBUG> onMouseUp() setProps(box):', new_box);

                                setProps({box: new_box});
                            }
                        }

                        if(debug.current == true)
                            console.log('------------------------------------------------------');
                    }

                    mouseIsDown.current   = false;
                    mouseHasMoved.current = false;
                }}


                onScroll={(event) => {

                    var xmin = mainBox.current.scrollLeft   / zoomfact.current;
                    var ymin = (mainImg.current.offsetHeight - (mainBox.current.scrollTop + mainBox.current.offsetHeight)) / zoomfact.current;

                    var dx   = mainBox.current.offsetWidth  / zoomfact.current;
                    var dy   = mainBox.current.offsetHeight / zoomfact.current;

                    var new_zoombox = {
                        zoom:   zoomfact.current,

                        width:  mainBox.current.offsetWidth,
                        height: mainBox.current.offsetHeight,

                        xmin:   xmin,
                        ymin:   ymin,

                        dx:     dx,
                        dy:     dy
                    };

                    if(debug.current == true)
                        console.log('DEBUG> Scroll() setProps(zoombox): ', new_zoombox);

                    setProps({zoombox: new_zoombox});


                   {/* Draw the region box on the inset image */}

                    var insetXmin = insetWidth  * mainBox.current.scrollLeft   / mainImg.current.offsetWidth;
                    var insetYmin = insetHeight * mainBox.current.scrollTop    / mainImg.current.offsetHeight;
                    var deltaX    = insetWidth  * mainBox.current.offsetWidth  / mainImg.current.offsetWidth;
                    var deltaY    = insetHeight * mainBox.current.offsetHeight / mainImg.current.offsetHeight;

                    var ctx      =  mainCanvas.current.getContext('2d');
                    var insetCtx = insetCanvas.current.getContext('2d');

                    if(debug.current == true)
                        console.log('DEBUG> onScroll() Clear ctx, clear and redraw insetCtx.');

                    ctx.clearRect(0, 0, mainImg.current.offsetWidth, mainImg.current.offsetHeight);

                    insetCtx.clearRect(0, 0, insetWidth, insetHeight);

                    insetCtx.beginPath();
                    insetCtx.rect(insetXmin, insetYmin, deltaX, deltaY);
                    insetCtx.lineWidth = 2;
                    insetCtx.strokeStyle = '#8080ff';
                    insetCtx.stroke();
                }}
            >

                <img ref={mainImg} id='mainImg' className='mainImg' src={img}
                    style={{
                        width:   zWidth + 'px',
                        height:  zHeight + 'px'
                    }} />
  
                <canvas ref={mainCanvas} id='mainCanvas' className='mainCanvas'
                    style={{
                        width:    zWidth + 'px',
                        height:   zHeight + 'px',
                        position: 'absolute'
                    }} />
 

                <div
                    ref = {insetBox}
                    id = 'insetBox'
                    className = 'insetBox'

                    style={{
                        width:    insetWidth + 'px',
                        height:   insetHeight + 'px',
                        position: 'absolute'
                    }}
                >

                    <img ref={insetImg} id='insetImg' className='insetImg'
                         src={inset} style={{width: insetWidth + 'px', height: insetHeight + 'px', border: '1px solid cca70'}} />

                    <canvas ref={insetCanvas} id='insetCanvas' className='insetCanvas'
                            width={insetWidth} height={insetHeight} style={{width: insetWidth + 'px', height: insetHeight + 'px'}} />

                </div>

            </section>


            {/* Div for monitoring incidental information (viewport into data image, etc.) */}

            <div
                ref={infoBox}
                id='info'
                className='info'

                style={{
                   width: '100%',
                   height: infoHeight + 'px',
                   border: '1px solid gray', 
                   color: '#000000',
                   backgroundColor: '#ffffff',
                   fontSize: '12px'
                }}
            >
            </div>

            <div
                ref={cutoutInfo}
                id='cutout'
                style={{visibility: 'hidden'}}
            >
            </div>

        </div>
    );
};

Mviewer.defaultProps = {};


Mviewer.propTypes = {

    /**
     * The ID used to identify this component in Dash callbacks.
     */
    id: PropTypes.string,

    /**
     * Main image.
     */
    img: PropTypes.string.isRequired,

    /**
     * Main image width.
     */
    imgWidth: PropTypes.number.isRequired,

    /**
     * Main image height.
     */
    imgHeight: PropTypes.number.isRequired,

    /**
     * Inset image (to show region of main image)
     * currently being displayed.
     */
    inset: PropTypes.string.isRequired,

    /**
     * Inset image width.
     */
    insetWidth: PropTypes.number.isRequired,

    /**
     * Inset image height.
     */
    insetHeight: PropTypes.number.isRequired,

    /**
     * App window width.
     */
    width: PropTypes.number,

    /**
     * App window height.
     */
    height: PropTypes.number,

     /**
     * Command string.
     */
    cutoutDesc: PropTypes.string,



    /**
     * Visible window box and zoom factor.
     */
    zoombox:   PropTypes.shape({
       zoom:   PropTypes.number.isRequired,
       width:  PropTypes.number.isRequired,
       height: PropTypes.number.isRequired,
       xmin:   PropTypes.number.isRequired,
       ymin:   PropTypes.number.isRequired,
       dx:     PropTypes.number.isRequired,
       dy:     PropTypes.number.isRequired
    }),


    /**
     * Box region selected (returned).
     */
    box: PropTypes.shape({
       xmin:       PropTypes.number.isRequired,
       ymin:       PropTypes.number.isRequired,
       dx:         PropTypes.number.isRequired,
       dy:         PropTypes.number.isRequired,
       screenXmin: PropTypes.number.isRequired,
       screenYmin: PropTypes.number.isRequired,
       screenDx:   PropTypes.number.isRequired,
       screenDy:   PropTypes.number.isRequired
    }),


    /**
     * Location of a location pick (returned).
     */
    pick: PropTypes.shape({
       x: PropTypes.number.isRequired,
       y: PropTypes.number.isRequired
    }),


     /**
     * Command string (returned).
     */
    cmdStr: PropTypes.string,


    /**
     * Dash-assigned callback that should be called to report property changes
     * to Dash, to make them available for callbacks.
     */
    setProps: PropTypes.func
};

export default Mviewer;
