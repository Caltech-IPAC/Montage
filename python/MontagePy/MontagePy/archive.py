#!/bin/env python

import os
import sys
import requests
import urllib.parse
import urllib.request
import json
import bz2

def mArchiveDownload(survey, location, size, path):

    """
    .
    mArchiveDownload populates a directory with raw images from 
    one of several astronomical missions (2MASS, SDSS, WISE,
    DSS, IRAC, MIPS).  These images are all in FITS format 
    and suitable for reprojection, moaicking, etc.

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

    path: str
        Directory for output files.
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
    
    nimages = len(data)
    
    if debug:
        print("DEBUG> nimages = " + str(nimages))
    
    
    # We need to check the given directory, 
    # whether it exists, whether it is writeable,
    # etc.  We'll do it by trying to create it,
    # then trying to write the image data it.
    
    try:
    
        if not os.path.exists(path):
            os.makedirs(path)
    
    except:
    
        return("{'status': '1', 'msg': 'Cannot create output directory.'}")
    
    
    # Retrieve all the images into the data directory
    
    chunk_size = 4096

    try:
        for index in range(0,nimages):
    
            datafile = path +  "/" + data[index]['file']
            url      = data[index]['url']

            bzfile = False

            if len(datafile) > 4 and datafile[-4:] == '.bz2':
                datafile = datafile[:-4]
                bzfile = True
    
            r = requests.get(url, stream=True)

            decompressor = bz2.BZ2Decompressor()

            with open(datafile, 'wb') as fd:

                for chunk in r.iter_content(chunk_size):

                    if bzfile:
                        decompressed = decompressor.decompress(chunk)

                        if decompressed:
                            fd.write(decompressed)
                    else:
                        fd.write(chunk)
    
            fd.close()

            if debug:
                print(datafile)

    except:
    
        return("{'status': '1', 'msg': 'Error writing data'}")
    
    
    # Success
    
    return("{'status': '0', 'count': " + str(nimages) + "}")



def mArchiveList(survey, location, size, output):

    """
    .
    mArchiveList populates a directory with raw images from 
    one of several astronomical missions (2MASS, SDSS, WISE,
    DSS, IRAC, MIPS).  These images are all in FITS format 
    and suitable for reprojection, moaicking, etc.

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
