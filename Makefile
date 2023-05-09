ROOTDIR:=$(shell pwd)

CFLAGS:=-DNUM_OF_SAMPLES=20000 -pthread
LFLAGS=-lpthread -lm

%.o : %.c
	gcc -c -o $@ -c $(CFLAGS) -DLIB_VERSION $<

all:
	gcc -o log-tc2c -O3  -DLOG_CSV_MODE   -DNUM_OF_SAMPLES=20000 -pthread c2c-threads.c -lpthread -lm
	gcc -o tc2c -O3                       -DNUM_OF_SAMPLES=20000 -pthread c2c-threads.c -lpthread -lm
	gcc -o tc2c-debug -O0 -g              -DNUM_OF_SAMPLES=1024  -pthread c2c-threads.c -lpthread -lm
alltoall: c2c-threads.o
	gcc -o alltoall -I$(ROOTDIR) $(ROOTDIR)/usecases/alltoall.c c2c-threads.o -lm -lpthread
clean:
	rm -f tc2c tc2c-debug log-tc2c alltoall *.o
