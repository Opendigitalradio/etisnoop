ETISnoop analyser
=================

The ETSnoop analyser decodes and prints out a RAW ETI file in a
form that makes analysis easier.

It can show information about the signalling, details about the FIGs,
and extract a DAB+ subchannel into a file.

Build
-----

Make sure you have cmake installed and do

    mkdir build
    cd build
    cmake ..
    make


Build with old makefile
-----------------------

Alternatively, use the old Makefile. There are two
variants:

    make etisnoop

compiles the tool, dynamically linked against libfaad

Or you can extract the sources for libfaad2-2.7 into a subfolder of the
same name, configure it, and if required modify it. Then you can do

    make etisnoop-static

to compile a version of etisnoop compiled against your own copy of FAAD.

About
-----

This is a contribution by CSP.it, is now developed by opendigitalradio,
and is published under the terms of the GNU GPL v3 or later.
See LICENCE for more information.

