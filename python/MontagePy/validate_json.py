# This script just ensures that all the JSON files can be parsed correctly

from __future__ import print_function

import os
import glob
import json

MONTAGELIB = os.path.join('..', '..', 'MontageLib')
    
for json_file in glob.glob(os.path.join(MONTAGELIB, '*', '*.json')):
    print("Validating {0}...".format(json_file))
    with open(json_file, 'r') as fjson:
        data = json.load(fjson)
