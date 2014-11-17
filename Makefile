
CC=g++

SOURCES=etisnoop.cpp dabplussnoop.cpp lib_crc.c firecode.c faad_decoder.cpp wavfile.c
HEADERS=dabplussnoop.h lib_crc.h firecode.h faad_decoder.h wavfile.h

all: etisnoop

etisnoop: $(SOURCES) $(HEADERS)
	$(CC) -Wall -ggdb $(SOURCES) $(HEADERS) -lfaad -o etisnoop

etisnoop-static: libfaad $(SOURCES) $(HEADERS)
	$(CC) -Wall -ggdb $(SOURCES) $(HEADERS) faad2-2.7/libfaad/.libs/libfaad.a -o etisnoop

libfaad:
	make -C ./faad2-2.7


clean:
	rm -f etisnoop *.o
