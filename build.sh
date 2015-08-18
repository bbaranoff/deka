swig3.0 -python delta.i
gcc -std=gnu99 -g -ggdb3 -Wall -lm -O0 -c -fpic delta.c delta_wrap.c -I/usr/include/python3.4m
gcc -shared delta.o delta_wrap.o -o _delta.so

