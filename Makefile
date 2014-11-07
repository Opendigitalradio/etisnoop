
CC=g++

all: etisnoop

etisnoop: etisnoop.cpp lib_crc.c lib_crc.h
	$(CC) -Wall -ggdb etisnoop.cpp lib_crc.c -o etisnoop

clean:
	rm -f etisnoop *.o
