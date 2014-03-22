
CC=gcc

all: etisnoop

etisnoop: etisnoop.c lib_crc.c lib_crc.h
	$(CC) -Wall -ggdb etisnoop.c lib_crc.c -o etisnoop

clean:
	rm -f etisnoop *.o
