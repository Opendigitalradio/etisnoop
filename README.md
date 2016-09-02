ETISnoop analyser
=================

The ETSnoop analyser decodes and prints out a RAW ETI file in a
form that makes analysis easier.

It can show information about the signalling, details about the FIGs,
and extract a DAB+ subchannel into a file.

Build
-----

etisnoop is using autotools. If you do not have a release containing a ./configure script,
run ./bootstrap.sh

Then do

    ./configure
    make
    sudo make install

About
-----

This is a contribution by CSP.it, is now developed by opendigitalradio,
and is published under the terms of the GNU GPL v3 or later.
See LICENCE for more information.

