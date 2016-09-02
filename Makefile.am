ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

if IS_GIT_REPO
GITVERSION_FLAGS = -DGITVERSION="\"`git describe --dirty`\""
else
GITVERSION_FLAGS =
endif

AM_CPPFLAGS = -Wall $(GITVERSION_FLAGS)
AM_CFLAGS = -Wall

etisnoop_SOURCES     = src/dabplussnoop.cpp src/dabplussnoop.hpp \
					   src/etiinput.cpp src/etiinput.hpp \
					   src/etisnoop.cpp \
					   src/faad_decoder.cpp src/faad_decoder.hpp \
					   src/fig0_0.cpp \
					   src/fig0_10.cpp \
					   src/fig0_11.cpp \
					   src/fig0_13.cpp \
					   src/fig0_14.cpp \
					   src/fig0_16.cpp \
					   src/fig0_17.cpp \
					   src/fig0_18.cpp \
					   src/fig0_19.cpp \
					   src/fig0_1.cpp \
					   src/fig0_21.cpp \
					   src/fig0_22.cpp \
					   src/fig0_24.cpp \
					   src/fig0_25.cpp \
					   src/fig0_26.cpp \
					   src/fig0_27.cpp \
					   src/fig0_28.cpp \
					   src/fig0_2.cpp \
					   src/fig0_31.cpp \
					   src/fig0_3.cpp \
					   src/fig0_5.cpp \
					   src/fig0_6.cpp \
					   src/fig0_8.cpp \
					   src/fig0_9.cpp \
					   src/fig1.cpp \
					   src/figs.cpp src/figs.hpp \
					   src/firecode.c src/firecode.h \
					   src/lib_crc.c src/lib_crc.h \
					   src/repetitionrate.cpp src/repetitionrate.hpp \
					   src/rsdecoder.cpp src/rsdecoder.hpp \
					   src/tables.cpp src/tables.hpp \
					   src/utils.cpp src/utils.hpp \
					   src/watermarkdecoder.hpp \
					   src/wavfile.c src/wavfile.h

bin_PROGRAMS =  etisnoop$(EXEEXT)

EXTRA_DIST = \
    $(top_srcdir)/bootstrap.sh \
    $(top_srcdir)/LICENCE \
    $(top_srcdir)/README.md \
    $(top_srcdir)/remove-duplicate-frames.py

