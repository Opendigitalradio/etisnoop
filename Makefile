
CC=g++

SOURCES=etisnoop.cpp dabplussnoop.cpp lib_crc.c firecode.c
HEADERS=dabplussnoop.h lib_crc.h firecode.h

all: etisnoop

etisnoop: $(SOURCES) $(HEADERS)
	$(CC) -Wall -ggdb $(SOURCES) $(HEADERS) -o etisnoop

clean:
	rm -f etisnoop *.o
