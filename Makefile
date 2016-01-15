
CXX=g++

SOURCES=etisnoop.cpp dabplussnoop.cpp lib_crc.c firecode.c faad_decoder.cpp wavfile.c etiinput.cpp rsdecoder.cpp
HEADERS=dabplussnoop.h lib_crc.h firecode.h faad_decoder.h wavfile.h etiinput.h rsdecoder.h

all: etisnoop

etisnoop: $(SOURCES) $(HEADERS)
	$(CXX) -std=c++11 -Wall -ggdb $(SOURCES) -lfaad -lfec -o etisnoop

etisnoop-static: libfaad $(SOURCES) $(HEADERS)
	$(CXX) -std=c++11 -Wall -ggdb $(SOURCES) -lfec -Ifaad2-2.7/include faad2-2.7/libfaad/.libs/libfaad.a -o etisnoop

libfaad:
	make -C ./faad2-2.7

.PHONY: tags
tags:
	ctags -R .


clean:
	rm -f etisnoop *.o
