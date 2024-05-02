#!/bin/env python

import os
import sys
import requests
import urllib.parse
import urllib.request
import ssl
import json
import bz2

def mArchiveList(survey, location, size, output):

    """
    .
    mArchiveList creates a list (ASCII table file) of the   
    images in a region for one of several astronomical missions
    (2MASS, SDSS, WISE, DSS, IRAC, MIPS).  These images are all
    in FITS format and suitable for reprojection, moaicking, etc.
    They can be downloaded using mArchiveDownload().

    Parameters
    ----------
    survey: str
        The survey and band information for the mission 
        (e.g. "2MASS J" or "SDSS g"). See

        http://montage.ipac.caltech.edu/applications/ArchiveList

        for a complete list.

    location: str
        Coordinates or name of an astronomical object
        (e.g. "4h23m11s -12d14m32.3s", "Messier 017").

    size: float
        Region size in degrees.

    output: str
        Output file for the image metadata.
    """

    debug = 0
    
    # Build the URL to get image metadata
    
    url = "http://montage.ipac.caltech.edu/cgi-bin/ArchiveList/nph-archivelist?survey=" \
        + urllib.parse.quote_plus(survey) \
        + "&location=" \
        + urllib.parse.quote_plus(location) \
        + "&size=" \
        + str(size) + "&units=deg&mode=JSON"
    
    if debug:
        print('DEBUG> url = "' + url + '"')
    
    
    # Retrieve the image metadata and convert
    # the JSON to a Python dictionary
    
    fjson = urllib.request.urlopen(url)
    
    data = json.load(fjson)
    
    if debug:
        print("DEBUG> data: ")
        print(data)
    

    # Write the data to the output file

    with open(output, 'w') as outfile:
        json.dump(data, outfile)

    return("{'status': '0'}")
