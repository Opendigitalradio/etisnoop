#!/usr/bin/env python
#
# This script removes duplicate frames from a RAW ETI file.
# It reads FCT from etisnoop output and only copies the
# frames if the FCT is increasing by one.
#
# usage:
# etisnoop -v -i in.eti | remove-duplicate-frames.py in.eti out.eti

import sys
import re

regex = re.compile(r'Frame Count \[([0-9]+)\]')

if len(sys.argv) < 3:
    print("usage: etisnoop -vi in.eti | {} in.eti out.eti".format(sys.argv[0]))
    sys.exit(1)


eti_in = open(sys.argv[1], "rb")
eti_out = open(sys.argv[2], "wb")

lastfct = -1
while True:
    line = sys.stdin.readline()
    if line == '':
        break

    fct = 0

    m = regex.search(line)
    if m is None:
        continue
    else:
        fct = int(m.groups()[0])

    if lastfct == 0 and fct == 249:
        print("Ignore {} duplicate rollover".format(fct))
        eti_in.read(6144)
    elif fct > lastfct:
        print("Take {}".format(fct))
        eti_out.write(eti_in.read(6144))
        lastfct = fct
    elif lastfct == 249 and fct == 0:
        print("Take {} because rollover".format(fct))
        eti_out.write(eti_in.read(6144))
        lastfct = fct
    else:
        print("Ignore {}".format(fct))
        eti_in.read(6144)

