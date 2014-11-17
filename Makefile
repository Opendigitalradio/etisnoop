
CC=g++

SOURCES=etisnoop.cpp dabplussnoop.cpp lib_crc.c firecode.c faad_decoder.cpp
HEADERS=dabplussnoop.h lib_crc.h firecode.h faad_decoder.h

all: etisnoop

libfaad:
	make -C ./faad2-2.7

etisnoop: libfaad $(SOURCES) $(HEADERS)
	$(CC) -Wall -ggdb $(SOURCES) $(HEADERS) faad2-2.7/libfaad/.libs/libfaad.a -o etisnoop

clean:
	rm -f etisnoop *.o
