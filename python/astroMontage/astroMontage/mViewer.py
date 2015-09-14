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
    type         =  ""
    visible      = True
    coord_sys    =  ""
    color        =  ""
    data_file    =  ""
    data_col     =  ""
    data_ref     =  ""
    data_type    =  ""
    sym_size     =  ""
    sym_type     =  ""
    sym_sides    =  ""
    sym_rotation =  ""
    lon          =  ""
    lat          =  ""
    text         =  ""
 
        
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
    fits_file    = ""
    color_table  = ""
    stretch_min  = ""
    stretch_max  = ""
    stretch_mode = ""

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

    viewer          = ""

    image_file      = "viewer.jpg"
    image_type      = "jpg"
    image_width     = "1000"
    image_height    = "1000"

    display_mode    = ""

    cutout_x_offset = ""
    cutout_y_offset = ""
    canvas_height   = 1000
    canvas_width    = 1000
    xmin            = ""
    ymin            = ""
    xmax            = ""
    ymax            = ""
    factor          = ""

    current_color           = "black"

    current_symbol_type     = "circle"
    current_symbol_size     =  1.0
    current_symbol_sides    =  3
    current_symbol_rotation =  0.0

    current_coord_sys       = "Equ J2000"

    gray_file  = mvViewFile()
    red_file   = mvViewFile()
    green_file = mvViewFile()
    blue_file  = mvViewFile()

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

mvHandle     = 0
mv_workspace = ""
mvViewData   = ""


# The next two objects are used by Tornado as
# document type handlers for its web server
# functionality.

class mvMainHandler(tornado.web.RequestHandler):

    # The initialization GET returns the index.html
    # file we put in the workspace

    def get(self):
    
        global mv_workspace

        self.workspace = mv_workspace

        loader = tornado.template.Loader(".")

        self.write(loader.load(self.workspace + "/index.html").generate())



class mvWSHandler(tornado.websocket.WebSocketHandler):

    def open(self):
        global mvHandle
        global mvViewData

        self.debug = False

        self.workspace = mv_workspace
        self.view      = mvViewData
        self.viewer    = self.view.viewer

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

        ref_file = self.view.gray_file.fits_file

        if self.view.display_mode == "":
            print "No images defined. Nothing to display."
            sys.stdout.write('\n>>> ')
            sys.stdout.flush()
            return

        if self.view.display_mode == "grayscale":
            ref_file = self.view.gray_file.fits_file

        if self.view.display_mode == "color":
            ref_file = self.view.red_file.fits_file

        command = "mExamine " + ref_file

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

        subimage_width  = retval.naxis1
        subimage_height = retval.naxis2

        if self.view.xmin == "":
            self.view.xmin = 0

        if self.view.xmax == "":
            self.view.xmax = retval.naxis1

        if self.view.ymin == "":
            self.view.ymin = 0

        if self.view.ymax == "":
            self.view.ymax = retval.naxis2

        self.view.image_width  = retval.naxis1
        self.view.image_height = retval.naxis2


        # Process browser request

        args = shlex.split(message)

        cmd = args[0]

        if cmd == 'update':
            self.update_display()


        if cmd == 'resize':

            self.view.canvas_width  = args[1]
            self.view.canvas_height = args[2]

            if self.view.factor == 0:

                self.view.xmin = 1
                self.view.xmax = self.view.image_width
                self.view.ymin = 1
                self.view.ymax = self.view.image_height

            self.update_display()


        if cmd == 'zoomReset':

            self.view.xmin = 1
            self.view.xmax = self.view.image_width
            self.view.ymin = 1
            self.view.ymax = self.view.image_height

            self.update_display()


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

            self.update_display()


        elif cmd == 'pick':

            boxx = float(args[1])
            boxy = float(args[2])

            if self.viewer.pick_callback is not None:
                self.viewer.pick_callback(boxx, boxy)


    def update_display(self):

        sys.stdout.write('\n>>> ')
        sys.stdout.flush()
        
        if self.view.display_mode == "":
            print "No images defined. Nothing to display."
            sys.stdout.write('\n>>> ')
            sys.stdout.flush()
            return

        if self.view.display_mode == "grayscale":

            # First cut out the part of the original grayscale image we want

            command = "mSubimage -p" 
            command += " " + self.view.gray_file.fits_file 
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
            command += " " + self.view.blue_file.fits_file 
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
            command += " " + self.view.green_file.fits_file 
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
            command += " " + self.view.red_file.fits_file 
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

        if self.view.display_mode == "grayscale":
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

        subimage_width  = retval.naxis1
        subimage_height = retval.naxis2



        xfactor = float(subimage_width)  / float(self.view.canvas_width)
        yfactor = float(subimage_height) / float(self.view.canvas_height)

        if float(yfactor) > float(xfactor):
            xfactor = yfactor

        self.view.factor = xfactor


        if self.view.display_mode == "grayscale":

            # Shrink/expand the grayscale cutout to the right size

            xfactor = float(subimage_width)  / float(self.view.canvas_width)
            yfactor = float(subimage_height) / float(self.view.canvas_height)

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
                coord_sys = self.view.overlay[i].coord_sys
                color    = self.view.overlay[i].color

                if visible == True:

                    if color != "":
                        command += " -color " + str(color)

                    command += " -grid "  + str(coord_sys)


            elif type == 'catalog':

                visible      = self.view.overlay[i].visible
                data_file    = self.view.overlay[i].data_file
                data_col     = self.view.overlay[i].data_col
                data_ref     = self.view.overlay[i].data_ref
                data_type    = self.view.overlay[i].data_type
                sym_size     = self.view.overlay[i].sym_size
                sym_type     = self.view.overlay[i].sym_type
                sym_sides    = self.view.overlay[i].sym_sides
                sym_rotation = self.view.overlay[i].sym_rotation
                color        = self.view.overlay[i].color

                if visible == True:

                    if color != "":
                        command += " -color " + str(color)

                    if sym_type != "" and sym_size != "":
                        command += " -symbol " + str(sym_size) + " " + str(sym_type) + " " + str(sym_sides) + " " + str(sym_rotation)

                    command += " -catalog "  + str(data_file) + " " + str(data_col) + " " + str(data_ref) + " " + str(data_type)


            elif type == 'imginfo':

                visible   = self.view.overlay[i].visible
                data_file = self.view.overlay[i].data_file
                color     = self.view.overlay[i].color

                if visible == True:

                    if color != "":
                        command += " -color " + str(color)

                    command += " -imginfo "  + str(data_file)


            elif type == 'mark':

                visible      = self.view.overlay[i].visible
                lon          = self.view.overlay[i].lon
                lat          = self.view.overlay[i].lat
                sym_size     = self.view.overlay[i].sym_size
                sym_type     = self.view.overlay[i].sym_type
                sym_sides    = self.view.overlay[i].sym_sides
                sym_rotation = self.view.overlay[i].sym_rotation
                color        = self.view.overlay[i].color

                if visible == True:

                    if color != "":
                        command += " -color " + str(color)

                    if sym_type != "" and sym_size != "":
                        command += " -symbol " + str(sym_size) + " " + str(sym_type) + " " + str(sym_sides) + " " + str(sym_rotation)

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


        if self.view.display_mode == "grayscale":

            fits_file    = self.workspace + "/shrunken.fits"
            color_table  = self.view.gray_file.color_table
            stretch_min  = self.view.gray_file.stretch_min
            stretch_max  = self.view.gray_file.stretch_max
            stretch_mode = self.view.gray_file.stretch_mode

            if color_table == "":
               color_table = 0

            if stretch_min == "":
               stretch_min = "-1s"

            if stretch_max == "":
               stretch_max = "max"

            if stretch_mode == "":
               stretch_mode = "gaussian-log"

            command += " -ct " + str(color_table)
            command += " -gray " + str(fits_file) + " " + str(stretch_min) + " " + str(stretch_max) + " " + str(stretch_mode)


        else:

            fits_file    = self.workspace + "/red_shrunken.fits"
            stretch_min  = self.view.red_file.stretch_min
            stretch_max  = self.view.red_file.stretch_max
            stretch_mode = self.view.red_file.stretch_mode
 
            if stretch_min == "":
               stretch_min = "-1s"

            if stretch_max == "":
               stretch_max = "max"

            if stretch_mode == "":
               stretch_mode = "gaussian-log"

            command += " -red " + str(fits_file) + " " + str(stretch_min) + " " + str(stretch_max) + " " + str(stretch_mode)
 
            fits_file    = self.workspace + "/green_shrunken.fits"
            stretch_min  = self.view.green_file.stretch_min
            stretch_max  = self.view.green_file.stretch_max
            stretch_mode = self.view.green_file.stretch_mode
 
            if stretch_min == "":
               stretch_min = "-1s"

            if stretch_max == "":
               stretch_max = "max"

            if stretch_mode == "":
               stretch_mode = "gaussian-log"

            command += " -green " + str(fits_file) + " " + str(stretch_min) + " " + str(stretch_max) + " " + str(stretch_mode)
 
            fits_file    = self.workspace + "/blue_shrunken.fits"
            stretch_min  = self.view.blue_file.stretch_min
            stretch_max  = self.view.blue_file.stretch_max
            stretch_mode = self.view.blue_file.stretch_mode

            if stretch_min == "":
               stretch_min = "-1s"

            if stretch_max == "":
               stretch_max = "max"

            if stretch_mode == "":
               stretch_mode = "gaussian-log"

            command += " -blue " + str(fits_file) + " " + str(stretch_min) + " " + str(stretch_max) + " " + str(stretch_mode)


        image_file = self.view.image_file

        command += " -jpeg " + self.workspace + "/" + str(image_file) 

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

        self.write_message("image " + image_file)


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
        global mv_workspace
        global mvViewData

        if self.workspace is None:
            print "Please set a workspace location first."
            return

        mv_workspace = self.workspace
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

        self.view.viewer = self;

        mv_workspace = workspace

        self.pick_callback = self.pick_location


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
            os.remove(self.workspace + "/" + str(self.view.image_file))
        except:
            pass

        try:
            os.rmdir(self.workspace)
        except:
            print "Workspace directory ('" + self.workspace + "') not deleted (not empty)"

        try:
            self.command("close")
            print "Browser connection closed."
        except:
            print "No active browser connection."


    # Utility function: set the display mode (grayscale / color)

    def set_display_mode(self, mode):

        mode = str(mode)

        if len(mode) == 0:
           mode = "grayscale"

        if mode[0] == 'g':
            self.view.display_mode = "grayscale"

        if mode[0] == 'G':
            self.view.display_mode = "grayscale"

        if mode[0] == 'b':
            self.view.display_mode = "grayscale"

        if mode[0] == 'B':
            self.view.display_mode = "grayscale"

        if mode[0] == 'r':
            self.view.display_mode = "color"

        if mode[0] == 'R':
            self.view.display_mode = "color"

        if mode[0] == 'c':
            self.view.display_mode = "color"

        if mode[0] == 'C':
            self.view.display_mode = "color"

        if mode[0] == 'f':
            self.view.display_mode = "color"

        if mode[0] == 'F':
            self.view.display_mode = "color"


    # Utility function: set the gray_file

    def set_gray_file(self, gray_file):

        self.view.gray_file.fits_file = gray_file

        if self.view.display_mode == "":
            self.view.display_mode = "grayscale"


    # Utility function: set the blue_file

    def set_blue_file(self, blue_file):

        self.view.blue_file.fits_file = blue_file

        if self.view.display_mode == "":
            if self.view.red_file.fits_file != "" and self.view.green_file.fits_file != "":
                self.view.display_mode = "color"


    # Utility function: set the green_file

    def set_green_file(self, green_file):

        self.view.green_file.fits_file = green_file

        if self.view.display_mode == "":
            if self.view.red_file.fits_file != "" and self.view.blue_file.fits_file != "":
                self.view.display_mode = "color"


    # Utility function: set the red_file

    def set_red_file(self, red_file):

        self.view.red_file.fits_file = red_file

        if self.view.display_mode == "":
            if self.view.green_file.fits_file != "" and self.view.blue_file.fits_file != "":
                self.view.display_mode = "color"


    # Utility function: set the current_color

    def set_current_color(self, current_color):

        self.view.current_color = current_color


    # Utility function: set the currentSymbol

    def set_current_symbol(self, *arg):
    
        nargs = len(arg)

        symbol_sides    = ""
        symbol_rotation = ""

        symbol_size     = arg[0]
        symbol_type     = arg[1]

        if nargs > 2:
            symbol_sides = arg[2]

        if nargs > 3:
            symbol_rotation = arg[3]

        self.view.current_symbol_size     = symbol_size
        self.view.current_symbol_type     = symbol_type
        self.view.current_symbol_sides    = symbol_sides
        self.view.current_symbol_rotation = symbol_rotation


    # Utility function: set the coord_sys

    def set_current_coord_sys(self, coord_sys):

        self.view.current_coord_sys = coord_sys


    # Utility function: set the color table (grayscale file)

    def set_color_table(self, color_table):

        self.view.gray_file.color_table = color_table


    # Utility function: set the grayscale color stretch

    def set_gray_stretch(self, stretch_min, stretch_max, stretch_mode):

        self.view.gray_file.stretch_min  = stretch_min
        self.view.gray_file.stretch_max  = stretch_max
        self.view.gray_file.stretch_mode = stretch_mode


    # Utility function: set the blue color stretch

    def set_blue_stretch(self, stretch_min, stretch_max, stretch_mode):

        self.view.blue_file.stretch_min  = stretch_min
        self.view.blue_file.stretch_max  = stretch_max
        self.view.blue_file.stretch_mode = stretch_mode


    # Utility function: set the green color stretch

    def set_green_stretch(self, stretch_min, stretch_max, stretch_mode):

        self.view.green_file.stretch_min  = stretch_min
        self.view.green_file.stretch_max  = stretch_max
        self.view.green_file.stretch_mode = stretch_mode


    # Utility function: set the red color stretch

    def set_red_stretch(self, stretch_min, stretch_max, stretch_mode):

        self.view.red_file.stretch_min  = stretch_min
        self.view.red_file.stretch_max  = stretch_max
        self.view.red_file.stretch_mode = stretch_mode


    # Utility function: add a grid overlay

    def add_grid(self, coord_sys):

        ovly = mvViewOverlay()

        ovly.type     = "grid"
        ovly.visible  =  True
        ovly.color    =  self.view.current_color
        ovly.coord_sys =  coord_sys

        self.view.overlay.append(ovly)

        return ovly


    # Utility function: add a catalog overlay

    def add_catalog(self, data_file, data_col, data_ref, data_type):

        ovly = mvViewOverlay()

        ovly.type         = "catalog"
        ovly.visible      =  True
        ovly.sym_size     =  self.view.current_symbol_size
        ovly.sym_type     =  self.view.current_symbol_type
        ovly.sym_sides    =  self.view.current_symbol_sides
        ovly.sym_rotation =  self.view.current_symbol_rotation
        ovly.coord_sys    =  self.view.current_coord_sys
        ovly.data_file    =  data_file
        ovly.data_col     =  data_col
        ovly.data_ref     =  data_ref
        ovly.data_type    =  data_type 
        ovly.color        =  self.view.current_color

        self.view.overlay.append(ovly)

        return ovly


    # Utility function: add an imginfo overlay

    def add_img_info(self, data_file):

        ovly = mvViewOverlay()

        ovly.type      = "imginfo"
        ovly.visible   = True
        ovly.data_file = data_file
        ovly.color     = self.view.current_color
        ovly.coord_sys = self.view.current_coord_sys

        self.view.overlay.append(ovly)

        return ovly


    # Utility function: add a marker overlay

    def add_marker(self, lon, lat):

        ovly = mvViewOverlay()

        ovly.type         = "mark"
        ovly.visible      =  True
        ovly.sym_size     =  self.view.current_symbol_size
        ovly.sym_type     =  self.view.current_symbol_type
        ovly.sym_sides    =  self.view.current_symbol_sides
        ovly.sym_rotation =  self.view.current_symbol_rotation
        ovly.lon          =  lon
        ovly.lat          =  lat
        ovly.coord_sys    =  self.view.current_coord_sys
        ovly.color        =  self.view.current_color

        self.view.overlay.append(ovly)

        return ovly


    # Utility function: add a label overlay

    def add_label(self, lon, lat, text):

        ovly = mvViewOverlay()

        ovly.type      = "label"
        ovly.visible   =  True
        ovly.lon       =  lon
        ovly.lat       =  lat
        ovly.text      =  text
        ovly.coord_sys =  self.view.current_coord_sys
        ovly.color     =  self.view.current_color

        self.view.overlay.append(ovly)

        return ovly


    # Start a second thread to interact with the browser.

    def init_browser_display(self):

        self.port = random.randint(10000,60000)

        template_file = resource_filename('astroMontage', 'web/index.html')
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


    # Default function to be used when the user picks a location
    # This can be overridden by the developer with a callback of 
    # their own.

    def pick_location(self, boxx, boxy):

        ref_file = []

        if self.view.display_mode == "grayscale":

            ref_file.append(self.view.gray_file.fits_file)


        if self.view.display_mode == "color":

            ref_file.append(self.view.blue_file.fits_file)
            ref_file.append(self.view.green_file.fits_file)
            ref_file.append(self.view.red_file.fits_file)

        radius = 31

        nfile = len(ref_file)

        for i in range(0, nfile):
            command = "mExamine -p " + repr(boxx) + "p " + repr(boxy) + "p " + repr(radius) + "p " + ref_file[i]

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

            
            print ""
            print "   File " + ref_file[i] + ":"
            print ""
            print "                 Flux    (sigma)                 (RA, Dec)         Pix Coord"
            print "                ------------------      -------------------------  ----------"
            print "      Center:   " + repr(retval.fluxref) + " (" + repr(retval.sigmaref) + ")  at  (" + repr(retval.raref) + ", " + repr(retval.decref) + ")  [" + repr(retval.xref) + ", " + repr(retval.yref) + "]"
            print "      Min:      " + repr(retval.fluxmin) + " (" + repr(retval.sigmamin) + ")  at  (" + repr(retval.ramin) + ", " + repr(retval.decmin) + ")  [" + repr(retval.xmin) + ", " + repr(retval.ymin) + "]"
            print "      Max:      " + repr(retval.fluxmax) + " (" + repr(retval.sigmamax) + ")  at  (" + repr(retval.ramax) + ", " + repr(retval.decmax) + ")  [" + repr(retval.xmax) + ", " + repr(retval.ymax) + "]"
            print ""
            print "      Average:  " + repr(retval.aveflux) + " +/- " + repr(retval.rmsflux)
            print ""
            print "      Radius:   " + repr(retval.radius) + " degrees (" + repr(retval.radpix) + " pixels) / Total area: " + repr(retval.npixel) + " pixels (" + repr(retval.nnull) + " nulls)"
            print ""

        sys.stdout.write('\n>>> ')
        sys.stdout.flush()


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
