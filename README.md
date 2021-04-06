ETISnoop analyser
=================

The ETISnoop analyser decodes a RAW ETI (see https://wiki.opendigitalradio.org/Ensemble_Transport_Interface ) file and prints out
its contents in YAML for easier analysis.

It can show information about the signalling, details about the FIGs,
and extract a DAB+ subchannel into a file.

Build
-----

etisnoop is using autotools. If you do not have a release containing a ./configure script,
run ./bootstrap.sh

Install prerequisites: A C++ compiler with complete C++11 support and `libfaad-dev`

Then do

    ./configure
    make
    sudo make install
    

Usage
-----

```
etisnoop [options] [(-i|-I) filename]

   -i      the file contains RAW ETI
   -I      the file contains FIC
   -v      increase verbosity (can be given more than once)
   -d N    decode subchannel N into stream-N.msc file
           if DAB+: decode audio to stream-N.wav file and extract PAD to stream-N.msc
   -s <filename.yaml>
           statistics mode: decode all subchannels and measure audio level, write statistics to file
   -n N    stop analysing after N ETI frames
   -f      analyse FIC carousel (no YAML output)
   -r      analyse FIG rates in FIGs per second
   -R      analyse FIG rates in frames per FIG
   -w      decode CRC-DABMUX and ODR-DabMux watermark.
   -e      decode frames with SYNC error and decode FIGs with invalid CRC
   -F <type>/<ext>
           add FIG type/ext to list of FIGs to display.
           if the option is not given, all FIGs are displayed.
```

You can open the stream-N.msc file in https://www.basicmaster.de/xpadxpert/ 
(remark: in case of DAB please rename the .msc to .mp2)

Hint: subchannel N means the (N+1)th subchannel in a mux (including data subchannels!)

About
-----

This is a contribution from CSP.it, now developed by Opendigitalradio,
and is published under the terms of the GNU GPL v3 or later.
See LICENCE for more information.


Faadalyse
=========

faadalyse can extract the audio from .dabp files created with ODR-AudioEnc,
and is designed to do more in-depth analysis of the AAC encoding using a modified
libfaad.

To install:

download and extract faad2-2.7 to a folder of the same name, and configure and compile it.
You can patch that faad library to display additional information you need.
Do not run make install.

Then run

    make -f Makefile.faadalyse
    ./faadalyse
