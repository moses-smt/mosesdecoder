g++ srilm.cc -c -I/fs/clip-software/srilm-1.5.6-PIC/include -O2

make

g++  -g -O2  -L/fs/clip-software/libevent-1.4.8-stable/lib  -o memcached memcached-memcached.o memcached-slabs.o memcached-items.o memcached-assoc.o memcached-thread.o memcached-stats.o srilm.o  -levent -L/fs/clip-software/srilm-1.5.6-PIC/lib/i686 -loolm -ldstruct -lmisc

