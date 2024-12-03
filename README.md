ETISnoop analyser
=================

The **ETISnoop analyser** decodes a RAW ETI (see https://wiki.opendigitalradio.org/Ensemble_Transport_Interface ) file and prints out
its contents in [YAML](https://en.wikipedia.org/wiki/YAML) for easier analysis.

It can show information about the signalling, details about the FIGs,
and extract a DAB+ subchannel into a file.

Building
--------

**etisnoop** is using autotools. If you do not have a release containing a `./configure` script,
run `./bootstrap.sh` before.

Install prerequisites: A C++ compiler with complete C++11 support and `libfaad-dev`

Then run

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
   -d N    decode subchannel N into stream-N.dab file
           if DAB+: decode audio to stream-N.wav file and extract PAD to stream-N.dab
           (superframes with RS coding)
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

You can open the stream-N.dab file in https://www.basicmaster.de/xpadxpert/ 
(remark: in case of DAB please rename the .dab to .mp2)

Hint: subchannel N means the (N+1)th subchannel in a mux (including data subchannels!)

Example FIG carousel statistic output
-------------------------------------

Using `-r` parameter a FIG statistic will be printed at the end of a file.

```
CAROUSEL FIG T/EXT  AVG  (COUNT) -   AVG  (COUNT) -  LEN - LENGTH HISTOGRAM               IN FIB(S)
CAROUSEL FIG 0/ 0  10.42 (  237) -  10.42 (  237) -  5.0 [▁▁▁▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0
CAROUSEL FIG 0/ 1  31.34 (  714) -  10.42 (  237) - 23.7 [▁▁▁▁▁▁▁▁▁▁▁▁▁▄▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▇] -  0 1 2
CAROUSEL FIG 0/ 2  62.54 ( 1424) -  10.42 (  237) - 15.5 [▁▁▁▁▁▁▄▁▄▁▁▄▁▁▁▁▄▁▁▁▁▁▁▁▁▁▇▁▁▁] -  0 1 2
CAROUSEL FIG 0/ 3  10.42 (  237) -  10.42 (  236) -  6.0 [▁▁▁▁▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0
CAROUSEL FIG 0/ 5  16.07 (  365) -   1.01 (   22) -  3.2 [▁▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0 1 2
CAROUSEL FIG 0/ 6   3.12 (   69) -   1.27 (   28) -  7.0 [▁▁▁▇▁▁▁▇▄▁▂▁▁▁▁▁▁▁▁▁▂▁▁▁▁▁▁▁▁▁] -  0 1 2
CAROUSEL FIG 0/ 7  10.42 (  237) -  10.42 (  237) -  3.0 [▁▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0
CAROUSEL FIG 0/ 8  10.36 (  230) -   1.01 (   22) -  8.1 [▁▁▁▁▁▂▁▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0 2
CAROUSEL FIG 0/ 9   1.01 (   23) -   1.01 (   23) -  4.0 [▁▁▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0 1 2
CAROUSEL FIG 0/10   1.01 (   23) -   1.01 (   23) -  7.0 [▁▁▁▁▁▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0 1 2
CAROUSEL FIG 0/13  17.35 (  389) -   1.01 (   22) -  8.1 [▁▁▁▁▁▁▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0 1 2
CAROUSEL FIG 0/14   1.01 (   22) -   1.01 (   21) -  2.0 [▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0 2
CAROUSEL FIG 0/17   9.55 (  217) -   0.99 (   22) -  7.7 [▁▁▁▁▁▇▁▁▁▅▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0 1 2
CAROUSEL FIG 0/18  10.73 (  244) -   0.99 (   22) -  7.7 [▁▁▁▁▁▁▁▇▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁] -  0 1
CAROUSEL FIG 1/ 0   0.99 (   22) -   0.99 (   22) - 21.0 [▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▇▁▁▁▁▁▁▁▁] -  1 2
CAROUSEL FIG 1/ 1  15.93 (  362) -   0.99 (   22) - 21.0 [▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▇▁▁▁▁▁▁▁▁] -  1 2
CAROUSEL FIG 1/ 5   0.99 (   22) -   0.99 (   22) - 23.0 [▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▇▁▁▁▁▁▁] -  1 2
```

About
-----

This is a contribution from CSP.it, now developed by Opendigitalradio,
and is published under the terms of the GNU GPL v3 or later.
See LICENCE for more information.


Faadalyse
=========

**faadalyse** can extract the audio from `.dabp` files created with [ODR-AudioEnc](https://github.com/Opendigitalradio/ODR-AudioEnc),
and is designed to do more in-depth analysis of the AAC encoding using a modified
libfaad.

Installation
------------

To install, download (latest) [faad2-2.8.8](https://sourceforge.net/projects/faac/files/faad2-src/faad2-2.8.0/), extract it into the etisnoop-folder and run

    cd faad2-2.8.8/
    ./configure 
    make

You can patch that faad library to display additional information you need.

Remark: Do **not** run make install.

Then run

    make -f Makefile.faadalyse
    ./faadalyse

Example output
--------------

```
./faadalyse /tmp/file.dabp 96 | head -30
Faadalyse -- A .dabp file analyser
  www.opendigitalradio.org
 compiled at Dec  3 2024 21:23:42
Analysing /tmp/file.dabp 96kbps
DAB+ decode: Found valid FireCode at 0
DAB+ decode: We have 1024 bytes of data
DAB+ decode: Found valid FireCode at 0
DAB+ decode: We have 2048 bytes of data
		DAB+ decode:
			firecode           0x30b0
			rfa                  0
			dac_rate             1
			sbr_flag             0
			aac_channel_mode     1
			ps_flag              0
			mpeg_surround_config 0
			num_aus              6
		DAB+ decode: AU start
			AU[0] 11 0xb
			AU[1] 216 0xd8
			AU[2] 432 0x1b0
			AU[3] 648 0x288
			AU[4] 864 0x360
			AU[5] 1080 0x438
DAB+ decode:		Copy au 0 of size 203
DAB+ decode:		Copy au 1 of size 214
DAB+ decode:		Copy au 2 of size 214
DAB+ decode:		Copy au 3 of size 214
DAB+ decode:		Copy au 4 of size 214
DAB+ decode:		Copy au 5 of size 238
DAB+ decode: Found valid FireCode at 0
DAB+ decode: We have 608 bytes of data
DAB+ decode: Found valid FireCode at 0
DAB+ decode: We have 1632 bytes of data

```
