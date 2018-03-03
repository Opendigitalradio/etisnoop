#!/usr/bin/env python
#
# An example on how to read the YAML output from etisnoop
# Pipe etisnoop to this script
#
# License: public domain

import sys
import yaml

for frame in yaml.load_all(sys.stdin):
    print("FIGs in frame {}".format(frame['Frame']))
    for fib in frame['LIDATA']['FIC']:
        if fib['FIGs']:
            for fig in fib['FIGs']:
                print(" FIG " + fig['FIG'])

