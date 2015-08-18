CC = gcc
SWIG = swig3.0

CFLAGS = -Wall -lm -std=gnu99 -O3


all: delta

.PHONY: clean swig wrap so

delta: swig wrap so

swig:
	$(SWIG) -python delta.i

wrap:
	$(CC) $(CFLAGS) -c -fpic delta.c delta_wrap.c -I/usr/include/python3.4m

so:
	$(CC) -shared delta.o delta_wrap.o -o _delta.so

clean:
	rm -rf _delta.so delta_wrap.o delta.o


