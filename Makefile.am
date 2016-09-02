ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

if IS_GIT_REPO
GITVERSION_FLAGS = -DGITVERSION="\"`git describe --dirty`\""
else
GITVERSION_FLAGS =
endif

AM_CPPFLAGS = -g -O2 $(GITVERSION_FLAGS)

etisnoop_SOURCES     = dabplussnoop.cpp dabplussnoop.hpp \
					   etiinput.cpp etiinput.hpp \
					   etisnoop.cpp \
					   faad_decoder.cpp faad_decoder.hpp \
					   fig0_0.cpp \
					   fig0_10.cpp \
					   fig0_11.cpp \
					   fig0_13.cpp \
					   fig0_14.cpp \
					   fig0_16.cpp \
					   fig0_17.cpp \
					   fig0_18.cpp \
					   fig0_19.cpp \
					   fig0_1.cpp \
					   fig0_21.cpp \
					   fig0_22.cpp \
					   fig0_24.cpp \
					   fig0_25.cpp \
					   fig0_26.cpp \
					   fig0_27.cpp \
					   fig0_28.cpp \
					   fig0_2.cpp \
					   fig0_31.cpp \
					   fig0_3.cpp \
					   fig0_5.cpp \
					   fig0_6.cpp \
					   fig0_8.cpp \
					   fig0_9.cpp \
					   fig1.cpp \
					   figs.cpp figs.hpp \
					   firecode.c firecode.h \
					   lib_crc.c lib_crc.h \
					   repetitionrate.cpp repetitionrate.hpp \
					   rsdecoder.cpp rsdecoder.hpp \
					   tables.cpp tables.hpp \
					   utils.cpp utils.hpp \
					   watermarkdecoder.hpp \
					   wavfile.c wavfile.h

bin_PROGRAMS =  etisnoop$(EXEEXT)

EXTRA_DIST = \
    $(top_srcdir)/bootstrap.sh \
    $(top_srcdir)/LICENCE \
    $(top_srcdir)/README.md \
    $(top_srcdir)/remove-duplicate-frames.py

