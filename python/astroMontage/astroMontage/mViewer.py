from threading import Thread

import tornado.ioloop
import tornado.web
import tornado.websocket
import tornado.template

import webbrowser
import subprocess
import os
import errno
import sys
import tempfile
import shutil
import random
import hashlib
import shlex

from pkg_resources import resource_filename


 
# This file contains the main mViewer object and a few support objects:
# 
#    mvStruct       --  Parser for Montage executable module return structures
#    mvViewOverlay  --  Data structure for image overlays info
#                        (grids, catalogs, etc.)
#    mvViewFile     --  Data structure for FITS file display info
#    mvView         --  Data structure for an mViewer view (includes instance 
#                       of the previous two objects)
#    mvMainHandler  --  Event handler for Tornado web service toolkit 
#                        (main index.html request)
#    mvWSHandler    --  Event handler for Tornado web service toolkit 
#                        (support file requestss; e.g., JS files)
#    mvThread       --  Extra thread for browser communications


# This simple class is used to parse return structures
# from the Montage C services

class mvStruct(object):

    def __init__(self, command, string):

        string = string[8:-1]

        strings = {}
        while True:
            try:
                p1 = string.index('"')
                p2 = string.index('"', p1 + 1)
                substring = string[p1 + 1:p2]
                key = hashlib.md5(substring.encode('ascii')).hexdigest()
                strings[key] = substring
                string = string[:p1] + key + string[p2 + 1:]
            except:
                break

        for pair in string.split(', '):
            key, value = pair.split('=')
            value = value.strip(' ')
            if value in strings:
                self.__dict__[key] = strings[value]
            else:
                self.__dict__[key] = self.simplify(value)


    def __repr__(self):

        string = ""
        for item in self.__dict__:
            substr = "%20s" % (item)
            string += substr + " : " + str(self.__dict__[item]) + "\n"
        return string[:-1]


    def simplify(self, value):
        try:
            return int(value)
        except:
            return float(value)


# These two classes here are pure data holders to provide 
# structure for an mViewer display.   The main one (mViewer.mvView) 
# describes the whole display and contains an arbitrary number 
# of mViewer.mvViewOverlay objects for the overlays on the main image.

# We also need information on image/canvas sizes and cutout
# offsets in order to map zoom regions and picks to original
# image coordinates and to shrink/expand the displayed region
# to fit the window.


class mvViewOverlay:
    type        =  ""
    visible     = True
    coordSys    =  ""
    color       =  ""
    dataFile    =  ""
    dataCol     =  ""
    dataRef     =  ""
    dataType    =  ""
    symSize     =  ""
    symType     =  ""
    symSides    =  ""
    symRotation =  ""
    lon         =  ""
    lat         =  ""
    text        =  ""
 
        
    def __str__(self):

        thisObj = dir(self)

        string = ""

        count = 0

        for item in thisObj:

            val = getattr(self, item)

            if item[0:2] == "__": 
                continue

            if val == "": 
                continue

            substr = "%40s" % (item)

            if isinstance(val, str):
                string += substr + ": '" + str(val) + "'\n"
            else:
                string += substr + ": " + str(val) + "\n"

            count += 1

        return string


    def __repr__(self):

        thisObj = dir(self)

        string = "{"

        count = 0

        for item in thisObj:

            val = getattr(self, item)

            if item[0:2] == "__": 
                continue

            if val == "": 
                continue

            if count > 0:
                string += ", "

            if isinstance(val, str):
                string += "'"+ item + "': '" + str(val) + "'"
            else:
                string += "'"+ item + "': " + str(val)

            count += 1

        string += "}"

        return string



class mvViewFile:
    fitsFile    = ""
    colorTable  = ""
    stretchMin  = ""
    stretchMax  = ""
    stretchMode = ""

    def __str__(self):

        thisObj = dir(self)

        string = ""

        for item in thisObj:

            val = getattr(self, item)

            if item[0:2] == "__": 
                continue

            if val == "": 
                continue

            substr = "%40s" % (item)

            if isinstance(val, str):
                string += substr + ": '" + str(val) + "'\n"
            else:
                string += substr + ": " + str(val) + "\n"

        return string


    def __repr__(self):

        thisObj = dir(self)

        string = "{"

        count = 0

        for item in thisObj:

            val = getattr(self, item)

            if item[0:2] == "__": 
                continue

            if val == "": 
                continue

            if count > 0:
                string += ", "

            if isinstance(val, str):
                string += "'"+ item + "': '" + str(val) + "'"
            else:
                string += "'"+ item + "': " + str(val)

            count += 1

        string += "}"

        return string



class mvView:

    imageFile     = "viewer.jpg"
    imageType     = "jpg"
    imageWidth    = "1000"
    imageHeight   = "1000"

    displayMode   = ""

    cutoutXoffset = ""
    cutoutYoffset = ""
    canvasHeight  = 1000
    canvasWidth   = 1000
    xmin          = ""
    ymin          = ""
    xmax          = ""
    ymax          = ""
    factor        = ""

    currentColor          = "black"

    currentSymbolType     = "circle"
    currentSymbolSize     =  1.0
    currentSymbolSides    =  3
    currentSymbolRotation =  0.0

    currentCoordSys       = "Equ J2000"

    grayFile  = mvViewFile()
    redFile   = mvViewFile()
    greenFile = mvViewFile()
    blueFile  = mvViewFile()

    overlay = []


    def __str__(self):

        thisObj = dir(self)

        string = "\n"

        count = 0

        for item in thisObj:

            val = getattr(self, item)

            if item[0:2] == "__": 
                continue

            if val == "": 
                continue

            substr = "%25s" % (item)

            objType = val.__class__.__name__

            if objType == 'str':
                string += substr + ": '" + str(val) + "'\n"

            elif objType == 'list':

                string += substr + ":\n"

                count = 0
                for ovly in val:
                   label = "%38s %d:\n" % ("Overlay", count)

                   string += label
                   string += ovly.__str__() 

                   count += 1

            elif objType == 'mvViewFile':

                string += substr + ":\n"

                string += val.__str__() 

            else:
                string += substr + ": " + str(val) + "\n"

            count += 1

        string += "\n"

        return string


    def __repr__(self):

        thisObj = dir(self)

        string = "{"

        count = 0

        for item in thisObj:

            val = getattr(self, item)

            if item[0:2] == "__": 
                continue

            if val == "": 
                continue

            if count > 0:
                string += ", "

            if isinstance(val, str):
                string += "'"+ item + "': '" + str(val) + "'"
            else:
                string += "'"+ item + "': " + str(val)

            count += 1

        string += "}"

        return string



# We want our mvThread object to have
# acess to the WebSocketHandler write_message()
# method.  This variable externalizes the 
# WebSocketHandler 'self' long enough to allow
# this.  We don't count on it lasting but a
# moment (which is all we need).  We do the
# same thing with the path to the "workspace"
# containing our working files and the "view"
# object containing the display parameters

mvHandle    = 0
mvWorkspace = ""
mvViewData  = ""


# The next two objects are used by Tornado as
# document type handlers for its web server
# functionality.

class mvMainHandler(tornado.web.RequestHandler):

    # The initialization GET returns the index.html
    # file we put in the workspace

    def get(self):
    
        global mvWorkspace

        self.workspace = mvWorkspace

        loader = tornado.template.Loader(".")

        self.write(loader.load(self.workspace + "/index.html").generate())



class mvWSHandler(tornado.websocket.WebSocketHandler):

    def open(self):
        global mvHandle
        global mvViewData

        self.debug = False

        self.workspace = mvWorkspace
        self.view      = mvViewData

        self.write_message("mViewer python server connection accepted.")
        self.write_message("")

        mvHandle = self


    # This is where we process "commands" coming
    # from the browser.  These are things like 
    # resize, zoom and pick events.

    def on_message(self, message):

        if self.debug:
           print "mvWSHandler.on_message('" + message + "')"

        # Find the image size

        refFile = self.view.grayFile.fitsFile

        if self.view.displayMode == "":
            print "No images defined. Nothing to display."
            sys.stdout.write('\n>>> ')
            sys.stdout.flush()
            return

        if self.view.displayMode == "grayscale":
            refFile = self.view.grayFile.fitsFile

        if self.view.displayMode == "color":
            refFile = self.view.redFile.fitsFile

        command = "mExamine " + refFile

        if self.debug:
           print "\nMONTAGE Command:\n---------------\n" + command

        p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        stderr = p.stderr.read()

        if stderr:
            raise Exception(stderr)
            return

        retval = mvStruct("mExamine", p.stdout.read().strip())

        if self.debug:
            print "\nRETURN Struct:\n-------------\n"
            print retval
            sys.stdout.write('\n>>> ')
            sys.stdout.flush()

        subimageWidth  = retval.naxis1
        subimageHeight = retval.naxis2

        if self.view.xmin == "":
            self.view.xmin = 0

        if self.view.xmax == "":
            self.view.xmax = retval.naxis1

        if self.view.ymin == "":
            self.view.ymin = 0

        if self.view.ymax == "":
            self.view.ymax = retval.naxis2

        self.view.imageWidth  = retval.naxis1
        self.view.imageHeight = retval.naxis2


        # Process browser request

        args = shlex.split(message)

        cmd = args[0]

        if cmd == 'update':
            self.updateDisplay()


        if cmd == 'resize':

            self.view.canvasWidth  = args[1]
            self.view.canvasHeight = args[2]

            if self.view.factor == 0:

                self.view.xmin = 1
                self.view.xmax = self.view.imageWidth
                self.view.ymin = 1
                self.view.ymax = self.view.imageHeight

            self.updateDisplay()


        if cmd == 'zoomReset':

            self.view.xmin = 1
            self.view.xmax = self.view.imageWidth
            self.view.ymin = 1
            self.view.ymax = self.view.imageHeight

            self.updateDisplay()


        elif cmd == 'zoom':

            boxxmin = float(args[1])
            boxxmax = float(args[2])
            boxymin = float(args[3])
            boxymax = float(args[4])

            oldxmin = float(self.view.xmin)
            oldxmax = float(self.view.xmax)
            oldymin = float(self.view.ymin)
            oldymax = float(self.view.ymax)

            factor = float(self.view.factor)

            boxxmin = boxxmin * factor
            boxxmax = boxxmax * factor
            boxymin = boxymin * factor
            boxymax = boxymax * factor

            boxxmin = boxxmin + oldxmin
            boxxmax = boxxmax + oldxmin
            boxymin = boxymin + oldymin
            boxymax = boxymax + oldymin


            self.view.xmin = boxxmin
            self.view.xmax = boxxmax
            self.view.ymin = boxymin
            self.view.ymax = boxymax

            self.updateDisplay()


        elif cmd == 'pick':

           pass


    def updateDisplay(self):

        sys.stdout.write('\n>>> ')
        sys.stdout.flush()
        
        if self.view.displayMode == "":
            print "No images defined. Nothing to display."
            sys.stdout.write('\n>>> ')
            sys.stdout.flush()
            return

        if self.view.displayMode == "grayscale":

            # First cut out the part of the original grayscale image we want

            command = "mSubimage -p" 
            command += " " + self.view.grayFile.fitsFile 
            command += " " + self.workspace + "/subimage.fits" 
            command += " " + str(self.view.xmin)
            command += " " + str(self.view.ymin)
            command += " " + str(int(self.view.xmax) - int(self.view.xmin))
            command += " " + str(int(self.view.ymax) - int(self.view.ymin))

            if self.debug:
               print "\nMONTAGE Command:\n---------------\n" + command

            p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            stderr = p.stderr.read()

            if stderr:
                raise Exception(stderr)
                return

            retval = mvStruct("mSubimage", p.stdout.read().strip())

            if self.debug:
                print "\nRETURN Struct:\n-------------\n"
                print retval
                sys.stdout.write('\n>>> ')
                sys.stdout.flush()



        else:

            # Or if in color mode, cut out the three images

            # Blue

            command = "mSubimage -p" 
            command += " " + self.view.blueFile.fitsFile 
            command += " " + self.workspace + "/blue_subimage.fits" 
            command += " " + str(self.view.xmin)
            command += " " + str(self.view.ymin)
            command += " " + str(int(self.view.xmax) - int(self.view.xmin))
            command += " " + str(int(self.view.ymax) - int(self.view.ymin))

            if self.debug:
               print "\nMONTAGE Command:\n---------------\n" + command

            p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            stderr = p.stderr.read()

            if stderr:
                raise Exception(stderr)
                return

            retval = mvStruct("mSubimage", p.stdout.read().strip())

            if self.debug:
                print "\nRETURN Struct:\n-------------\n"
                print retval


            # Green

            command = "mSubimage -p" 
            command += " " + self.view.greenFile.fitsFile 
            command += " " + self.workspace + "/green_subimage.fits" 
            command += " " + str(self.view.xmin)
            command += " " + str(self.view.ymin)
            command += " " + str(int(self.view.xmax) - int(self.view.xmin))
            command += " " + str(int(self.view.ymax) - int(self.view.ymin))

            if self.debug:
               print "\nMONTAGE Command:\n---------------\n" + command

            p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            stderr = p.stderr.read()

            if stderr:
                raise Exception(stderr)
                return

            retval = mvStruct("mSubimage", p.stdout.read().strip())

            if self.debug:
                print "\nRETURN Struct:\n-------------\n"
                print retval


            # Red

            command = "mSubimage -p" 
            command += " " + self.view.redFile.fitsFile 
            command += " " + self.workspace + "/red_subimage.fits" 
            command += " " + str(self.view.xmin)
            command += " " + str(self.view.ymin)
            command += " " + str(int(self.view.xmax) - int(self.view.xmin))
            command += " " + str(int(self.view.ymax) - int(self.view.ymin))

            if self.debug:
               print "\nMONTAGE Command:\n---------------\n" + command

            p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            stderr = p.stderr.read()

            if stderr:
                raise Exception(stderr)
                return

            retval = mvStruct("mSubimage", p.stdout.read().strip())

            if self.debug:
                print "\nRETURN Struct:\n-------------\n"
                print retval
                sys.stdout.write('\n>>> ')
                sys.stdout.flush()


        # Get the size (all three are the same)

        if self.view.displayMode == "grayscale":
            command = "mExamine " + self.workspace + "/subimage.fits"
        else:
            command = "mExamine " + self.workspace + "/red_subimage.fits"

        if self.debug:
           print "\nMONTAGE Command:\n---------------\n" + command

        p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        stderr = p.stderr.read()

        if stderr:
            raise Exception(stderr)
            return

        retval = mvStruct("mExamine", p.stdout.read().strip())

        if self.debug:
            print "\nRETURN Struct:\n-------------\n"
            print retval
            sys.stdout.write('\n>>> ')
            sys.stdout.flush()

        subimageWidth  = retval.naxis1
        subimageHeight = retval.naxis2



        xfactor = float(subimageWidth)  / float(self.view.canvasWidth)
        yfactor = float(subimageHeight) / float(self.view.canvasHeight)

        if float(yfactor) > float(xfactor):
            xfactor = yfactor

        self.view.factor = xfactor


        if self.view.displayMode == "grayscale":

            # Shrink/expand the grayscale cutout to the right size

            xfactor = float(subimageWidth)  / float(self.view.canvasWidth)
            yfactor = float(subimageHeight) / float(self.view.canvasHeight)

            if float(yfactor) > float(xfactor):
                xfactor = yfactor

            self.view.factor = xfactor

            command = "mShrink" 
            command += " " + self.workspace + "/subimage.fits" 
            command += " " + self.workspace + "/shrunken.fits" 
            command += " " + str(xfactor)

            if self.debug:
               print "\nMONTAGE Command:\n---------------\n" + command

            p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            stderr = p.stderr.read()

            if stderr:
                raise Exception(stderr)
                return

            retval = mvStruct("mShrink", p.stdout.read().strip())

            if self.debug:
                print "\nRETURN Struct:\n-------------\n"
                print retval
                sys.stdout.write('\n>>> ')
                sys.stdout.flush()


        else:

            # Shrink/expand the three color cutouts to the right size

            # Blue

            command = "mShrink" 
            command += " " + self.workspace + "/blue_subimage.fits" 
            command += " " + self.workspace + "/blue_shrunken.fits" 
            command += " " + str(xfactor)

            if self.debug:
               print "\nMONTAGE Command:\n---------------\n" + command

            p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            stderr = p.stderr.read()

            if stderr:
                raise Exception(stderr)
                return

            retval = mvStruct("mShrink", p.stdout.read().strip())

            if self.debug:
                print "\nRETURN Struct:\n-------------\n"
                print retval


            # Green

            command = "mShrink" 
            command += " " + self.workspace + "/green_subimage.fits" 
            command += " " + self.workspace + "/green_shrunken.fits" 
            command += " " + str(xfactor)

            if self.debug:
               print "\nMONTAGE Command:\n---------------\n" + command

            p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            stderr = p.stderr.read()

            if stderr:
                raise Exception(stderr)
                return

            retval = mvStruct("mShrink", p.stdout.read().strip())

            if self.debug:
                print "\nRETURN Struct:\n-------------\n"
                print retval


            # Red

            command = "mShrink" 
            command += " " + self.workspace + "/red_subimage.fits" 
            command += " " + self.workspace + "/red_shrunken.fits" 
            command += " " + str(xfactor)

            if self.debug:
               print "\nMONTAGE Command:\n---------------\n" + command

            p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            stderr = p.stderr.read()

            if stderr:
                raise Exception(stderr)
                return

            retval = mvStruct("mShrink", p.stdout.read().strip())

            if self.debug:
                print "\nRETURN Struct:\n-------------\n"
                print retval
                sys.stdout.write('\n>>> ')
                sys.stdout.flush()



        # Finally, generate the JPEG

        command = "mViewer"

        noverlay = len(self.view.overlay)

        for i in range(0, noverlay):

            type = self.view.overlay[i].type

            if   type == 'grid':

                visible  = self.view.overlay[i].visible
                coordSys = self.view.overlay[i].coordSys
                color    = self.view.overlay[i].color

                if visible == True:

                    if color != "":
                        command += " -color " + str(color)

                    command += " -grid "  + str(coordSys)


            elif type == 'catalog':

                visible     = self.view.overlay[i].visible
                dataFile    = self.view.overlay[i].dataFile
                dataCol     = self.view.overlay[i].dataCol
                dataRef     = self.view.overlay[i].dataRef
                dataType    = self.view.overlay[i].dataType
                symSize     = self.view.overlay[i].symSize
                symType     = self.view.overlay[i].symType
                symSides    = self.view.overlay[i].symSides
                symRotation = self.view.overlay[i].symRotation
                color       = self.view.overlay[i].color

                if visible == True:

                    if color != "":
                        command += " -color " + str(color)

                    if symType != "" and symSize != "":
                        command += " -symbol " + str(symSize) + " " + str(symType) + " " + str(symSides) + " " + str(symRotation)

                    command += " -catalog "  + str(dataFile) + " " + str(dataCol) + " " + str(dataRef) + " " + str(dataType)


            elif type == 'imginfo':

                visible  = self.view.overlay[i].visible
                dataFile = self.view.overlay[i].dataFile
                color    = self.view.overlay[i].color

                if visible == True:

                    if color != "":
                        command += " -color " + str(color)

                    command += " -imginfo "  + str(dataFile)


            elif type == 'mark':

                visible     = self.view.overlay[i].visible
                lon         = self.view.overlay[i].lon
                lat         = self.view.overlay[i].lat
                symSize     = self.view.overlay[i].symSize
                symType     = self.view.overlay[i].symType
                symSides    = self.view.overlay[i].symSides
                symRotation = self.view.overlay[i].symRotation
                color       = self.view.overlay[i].color

                if visible == True:

                    if color != "":
                        command += " -color " + str(color)

                    if symType != "" and symSize != "":
                        command += " -symbol " + str(symSize) + " " + str(symType) + " " + str(symSides) + " " + str(symRotation)

                    command += " -mark "  + str(lon) + " " + str(lat)


            elif type == 'label':

                visible  = self.view.overlay[i].visible
                lon      = self.view.overlay[i].lon
                lat      = self.view.overlay[i].lat
                text     = self.view.overlay[i].text
                color    = self.view.overlay[i].color

                if visible == True:

                    if color != "":
                        command += " -color " + str(color)

                    command += " -label "  + str(lon) + " " + str(lat) + ' "' + str(text) + '"'


            else:
                print "Invalid overlay type '" + str(type) + "' in view specification."


        if self.view.displayMode == "grayscale":

            fitsFile    = self.workspace + "/shrunken.fits"
            colorTable  = self.view.grayFile.colorTable
            stretchMin  = self.view.grayFile.stretchMin
            stretchMax  = self.view.grayFile.stretchMax
            stretchMode = self.view.grayFile.stretchMode

            if colorTable == "":
               colorTable = 0

            if stretchMin == "":
               stretchMin = "-1s"

            if stretchMax == "":
               stretchMax = "max"

            if stretchMode == "":
               stretchMode = "gaussian-log"

            command += " -ct " + str(colorTable)
            command += " -gray " + str(fitsFile) + " " + str(stretchMin) + " " + str(stretchMax) + " " + str(stretchMode)


        else:

            fitsFile    = self.workspace + "/red_shrunken.fits"
            stretchMin  = self.view.redFile.stretchMin
            stretchMax  = self.view.redFile.stretchMax
            stretchMode = self.view.redFile.stretchMode
 
            if stretchMin == "":
               stretchMin = "-1s"

            if stretchMax == "":
               stretchMax = "max"

            if stretchMode == "":
               stretchMode = "gaussian-log"

            command += " -red " + str(fitsFile) + " " + str(stretchMin) + " " + str(stretchMax) + " " + str(stretchMode)
 
            fitsFile    = self.workspace + "/green_shrunken.fits"
            stretchMin  = self.view.greenFile.stretchMin
            stretchMax  = self.view.greenFile.stretchMax
            stretchMode = self.view.greenFile.stretchMode
 
            if stretchMin == "":
               stretchMin = "-1s"

            if stretchMax == "":
               stretchMax = "max"

            if stretchMode == "":
               stretchMode = "gaussian-log"

            command += " -green " + str(fitsFile) + " " + str(stretchMin) + " " + str(stretchMax) + " " + str(stretchMode)
 
            fitsFile    = self.workspace + "/blue_shrunken.fits"
            stretchMin  = self.view.blueFile.stretchMin
            stretchMax  = self.view.blueFile.stretchMax
            stretchMode = self.view.blueFile.stretchMode

            if stretchMin == "":
               stretchMin = "-1s"

            if stretchMax == "":
               stretchMax = "max"

            if stretchMode == "":
               stretchMode = "gaussian-log"

            command += " -blue " + str(fitsFile) + " " + str(stretchMin) + " " + str(stretchMax) + " " + str(stretchMode)


        imageFile = self.view.imageFile

        command += " -jpeg " + self.workspace + "/" + str(imageFile) 

        if self.debug:
           print "\nMONTAGE Command:\n---------------\n" + command

        p = subprocess.Popen(shlex.split(command), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        stderr = p.stderr.read()

        if stderr:
            raise Exception(stderr)
            return

        retval = mvStruct("mViewer", p.stdout.read().strip())

        if self.debug:
            print "\nRETURN Struct:\n-------------\n"
            print retval
            sys.stdout.write('\n>>> ')
            sys.stdout.flush()

        if stderr:
            raise Exception(stderr)
            return

        if retval.stat == "WARNING":
            print "\nWARNING: " + retval.msg
            sys.stdout.write('\n>>> ')
            sys.stdout.flush()
            return

        if retval.stat == "ERROR":
            print "\nERROR: " + retval.msg
            sys.stdout.write('\n>>> ')
            sys.stdout.flush()
            return

        self.write_message("image " + imageFile)


    def on_close(self):

        print '\nWeb connection closed by browser.'

        sys.stdout.write('\n>>> ')
        sys.stdout.flush()



# Since we want to have the web window containing
# mViewer open while still allowing interactive
# Python processing, we need to put the Tornado
# connection in a separate thread.

class mvThread(Thread):
 
    def __init__(self, port, workspace, view):
        
        Thread.__init__(self)

        self.remote    = None
        self.port      = port
        self.workspace = workspace
        self.view      = view

        self.handler   = mvWSHandler

        self.debug = False

        self.daemon    = True    # If we make the thread a daemon, 
                                 # it closes when the process finishes.
 
    def run(self):

        global mvHandle
        global mvWorkspace
        global mvViewData

        if self.workspace is None:
            print "Please set a workspace location first."
            return

        mvWorkspace = self.workspace
        mvViewData  = self.view
        
        application = tornado.web.Application([
            (r'/ws',   mvWSHandler),
            (r'/',     mvMainHandler),
            (r"/(.*)", tornado.web.StaticFileHandler, {"path": self.workspace}),
        ])


        # Here we would populate the workspace and configure
        # its specific index.html to use this specific port

        application.listen(self.port)

        webbrowser.open("localhost:" + str(self.port))

        self.remote = mvHandle

        tornado.ioloop.IOLoop.instance().start()


    def command(self, msg):

        if self.debug:
            print "DEBUG> mvThread.command('" + msg + "')"

        self.remote = mvHandle

        self.remote.write_message(msg)



# Here at last we implement the actual mViewer control code.
# It's __init__ method starts up the second thread (which in
# turn makes the websocket connection to a local browser)
# and keeps track of the view external object we use for
# defining the view.  Other methods control the other actions
# (like updating the display and initiating "pick" loop 
# processing.

class mViewer():

    # Initialization (mostly setting up the workspace)

    def __init__(self, *arg):

        self.debug = False

        nargs = len(arg)

        if nargs == 0:
            workspace = tempfile.mkdtemp(prefix="mvWork_", dir=".")
        else:
            workspace = arg[0]

        try:
            os.makedirs(workspace)

        except OSError as exception:

            if exception.errno != errno.EEXIST:
                raise
                return

        self.workspace = workspace
        self.view      = mvView()

        mvWorkspace = workspace


    # Shutdown (removing workspace)

    def close(self):

        try:
            os.remove(self.workspace + "/index.html")
        except:
            pass

        try:
            os.remove(self.workspace + "/WebClient.js")
        except:
            pass

        try:
            os.remove(self.workspace + "/mViewer.js")
        except:
            pass

        try:
            os.remove(self.workspace + "/iceGraphics.js")
        except:
            pass

        try:
            os.remove(self.workspace + "/favicon.ico")
        except:
            pass

        try:
            os.remove(self.workspace + "/waitClock.gif")
        except:
            pass

        try:
            os.remove(self.workspace + "/reload.png")
        except:
            pass

        try:
            os.remove(self.workspace + "/subimage.fits")
        except:
            pass

        try:
            os.remove(self.workspace + "/blue_subimage.fits")
        except:
            pass

        try:
            os.remove(self.workspace + "/green_subimage.fits")
        except:
            pass

        try:
            os.remove(self.workspace + "/red_subimage.fits")
        except:
            pass

        try:
            os.remove(self.workspace + "/shrunken.fits")
        except:
            pass

        try:
            os.remove(self.workspace + "/blue_shrunken.fits")
        except:
            pass

        try:
            os.remove(self.workspace + "/green_shrunken.fits")
        except:
            pass

        try:
            os.remove(self.workspace + "/red_shrunken.fits")
        except:
            pass

        try:
            os.remove(self.workspace + "/" + str(self.view.imageFile))
        except:
            pass

        try:
            os.rmdir(self.workspace)
        except:
            print "Workspace directory ('" + self.workspace + "') not deleted (not empty)"

        self.command("close")


    # Utility function: set the display mode (grayscale / color)

    def setDisplayMode(self, mode):

        mode = str(mode)

        if len(mode) == 0:
           mode = "grayscale"

        if mode[0] == 'g':
            self.view.displayMode = "grayscale"

        if mode[0] == 'G':
            self.view.displayMode = "grayscale"

        if mode[0] == 'b':
            self.view.displayMode = "grayscale"

        if mode[0] == 'B':
            self.view.displayMode = "grayscale"

        if mode[0] == 'r':
            self.view.displayMode = "color"

        if mode[0] == 'R':
            self.view.displayMode = "color"

        if mode[0] == 'c':
            self.view.displayMode = "color"

        if mode[0] == 'C':
            self.view.displayMode = "color"

        if mode[0] == 'f':
            self.view.displayMode = "color"

        if mode[0] == 'F':
            self.view.displayMode = "color"


    # Utility function: set the grayFile

    def setGrayFile(self, grayFile):

        self.view.grayFile.fitsFile = grayFile

        if self.view.displayMode == "":
            self.view.displayMode = "grayscale"


    # Utility function: set the blueFile

    def setBlueFile(self, blueFile):

        self.view.blueFile.fitsFile = blueFile

        if self.view.displayMode == "":
            if self.view.redFile.fitsFile != "" and self.view.greenFile.fitsFile != "":
                self.view.displayMode = "color"


    # Utility function: set the greenFile

    def setGreenFile(self, greenFile):

        self.view.greenFile.fitsFile = greenFile

        if self.view.displayMode == "":
            if self.view.redFile.fitsFile != "" and self.view.blueFile.fitsFile != "":
                self.view.displayMode = "color"


    # Utility function: set the redFile

    def setRedFile(self, redFile):

        self.view.redFile.fitsFile = redFile

        if self.view.displayMode == "":
            if self.view.greenFile.fitsFile != "" and self.view.blueFile.fitsFile != "":
                self.view.displayMode = "color"


    # Utility function: set the currentColor

    def setCurrentColor(self, currentColor):

        self.view.currentColor = currentColor


    # Utility function: set the currentSymbol

    def setCurrentSymbol(self, *arg):
    
        nargs = len(arg)

        symbolSides    = ""
        symbolRotation = ""

        symbolSize     = arg[0]
        symbolType     = arg[1]

        if nargs > 2:
            symbolSides = arg[2]

        if nargs > 3:
            symbolRotation = arg[3]

        self.view.currentSymbolSize     = symbolSize
        self.view.currentSymbolType     = symbolType
        self.view.currentSymbolSides    = symbolSides
        self.view.currentSymbolRotation = symbolRotation


    # Utility function: set the coordSys

    def setCurrentCoordSys(self, coordSys):

        self.view.currentCoordSys = coordSys


    # Utility function: set the color table (grayscale file)

    def setColorTable(self, colorTable):

        self.view.grayFile.colorTable = colorTable


    # Utility function: set the grayscale color stretch

    def setGrayStretch(self, stretchMin, stretchMax, stretchMode):

        self.view.grayFile.stretchMin  = stretchMin
        self.view.grayFile.stretchMax  = stretchMax
        self.view.grayFile.stretchMode = stretchMode


    # Utility function: set the blue color stretch

    def setBlueStretch(self, stretchMin, stretchMax, stretchMode):

        self.view.blueFile.stretchMin  = stretchMin
        self.view.blueFile.stretchMax  = stretchMax
        self.view.blueFile.stretchMode = stretchMode


    # Utility function: set the green color stretch

    def setGreenStretch(self, stretchMin, stretchMax, stretchMode):

        self.view.greenFile.stretchMin  = stretchMin
        self.view.greenFile.stretchMax  = stretchMax
        self.view.greenFile.stretchMode = stretchMode


    # Utility function: set the red color stretch

    def setRedStretch(self, stretchMin, stretchMax, stretchMode):

        self.view.redFile.stretchMin  = stretchMin
        self.view.redFile.stretchMax  = stretchMax
        self.view.redFile.stretchMode = stretchMode


    # Utility function: add a grid overlay

    def addGrid(self, coordSys):

        ovly = mvViewOverlay()

        ovly.type     = "grid"
        ovly.visible  =  True
        ovly.color    =  self.view.currentColor
        ovly.coordSys =  coordSys

        self.view.overlay.append(ovly)

        return ovly


    # Utility function: add a catalog overlay

    def addCatalog(self, dataFile, dataCol, dataRef, dataType):

        ovly = mvViewOverlay()

        ovly.type        = "catalog"
        ovly.visible     =  True
        ovly.symSize     =  self.view.currentSymbolSize
        ovly.symType     =  self.view.currentSymbolType
        ovly.symSides    =  self.view.currentSymbolSides
        ovly.symRotation =  self.view.currentSymbolRotation
        ovly.coordSys    =  self.view.currentCoordSys
        ovly.dataFile    =  dataFile
        ovly.dataCol     =  dataCol
        ovly.dataRef     =  dataRef
        ovly.dataType    =  dataType 
        ovly.color       =  self.view.currentColor

        self.view.overlay.append(ovly)

        return ovly


    # Utility function: add an imginfo overlay

    def addImgInfo(self, dataFile):

        ovly = mvViewOverlay()

        ovly.type     = "imginfo"
        ovly.visible  = True
        ovly.dataFile = dataFile
        ovly.color    = self.view.currentColor
        ovly.coordSys = self.view.currentCoordSys

        self.view.overlay.append(ovly)

        return ovly


    # Utility function: add a marker overlay

    def addMarker(self, lon, lat):

        ovly = mvViewOverlay()

        ovly.type        = "mark"
        ovly.visible     =  True
        ovly.symSize     =  self.view.currentSymbolSize
        ovly.symType     =  self.view.currentSymbolType
        ovly.symSides    =  self.view.currentSymbolSides
        ovly.symRotation =  self.view.currentSymbolRotation
        ovly.lon         =  lon
        ovly.lat         =  lat
        ovly.coordSys    =  self.view.currentCoordSys
        ovly.color       =  self.view.currentColor

        self.view.overlay.append(ovly)

        return ovly


    # Utility function: add a label overlay

    def addLabel(self, lon, lat, text):

        ovly = mvViewOverlay()

        ovly.type     = "label"
        ovly.visible  =  True
        ovly.lon      =  lon
        ovly.lat      =  lat
        ovly.text     =  text
        ovly.coordSys =  self.view.currentCoordSys
        ovly.color    =  self.view.currentColor

        self.view.overlay.append(ovly)

        return ovly


    # Start a second thread to interact with the browser.

    def initBrowserDisplay(self):

        self.port = random.randint(10000,60000)

        template_file  = resource_filename('astroMontage', 'web/index.html')
        index_file    = self.workspace + "/index.html"

        port_string   = str(self.port)

        with open(index_file,'w') as new_file:
           with open(template_file) as old_file:
              for line in old_file:
                 new_file.write(line.replace("\\PORT\\", port_string))


        in_file  = resource_filename('astroMontage', 'web/WebClient.js')
        out_file = self.workspace + '/WebClient.js'

        shutil.copy(in_file, out_file)


        in_file  = resource_filename('astroMontage', 'web/mViewer.js')
        out_file = self.workspace + '/mViewer.js'

        shutil.copy(in_file, out_file)


        in_file  = resource_filename('astroMontage', 'web/iceGraphics.js')
        out_file = self.workspace + '/iceGraphics.js'

        shutil.copy(in_file, out_file)


        in_file  = resource_filename('astroMontage', 'web/favicon.ico')
        out_file = self.workspace + '/favicon.ico'

        shutil.copy(in_file, out_file)


        in_file  = resource_filename('astroMontage', 'web/waitClock.gif')
        out_file = self.workspace + '/waitClock.gif'

        shutil.copy(in_file, out_file)


        in_file  = resource_filename('astroMontage', 'web/reload.png')
        out_file = self.workspace + '/reload.png'

        shutil.copy(in_file, out_file)


        self.thread = mvThread(self.port, self.workspace, self.view)

        self.thread.start()


    # Send a display update notification to the browser.

    def draw(self):

        if self.debug:
            print "DEBUG> mViewer.draw(): sending 'updateDisplay' to browser."

        self.thread.command("updateDisplay")


    # Send a command to the browser javascript object              

    def command(self, message):

        if self.debug:
            print "DEBUG> mViewer.command('" + message + "')"

        self.thread.command(message)
