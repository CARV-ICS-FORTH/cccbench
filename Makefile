all:
	gcc -o log-tc2c -O3  -DLOG_CSV_MODE   -DNUM_OF_SAMPLES=20000 -pthread c2c-threads.c -lpthread -lm
	gcc -o tc2c -O3                       -DNUM_OF_SAMPLES=20000 -pthread c2c-threads.c -lpthread -lm
	gcc -c -o libtc2c.o -O3 -DLIB_VERSION -DNUM_OF_SAMPLES=20000 -pthread c2c-threads.c -lpthread -lm
	gcc -o tc2c-debug -O0 -g              -DNUM_OF_SAMPLES=1024  -pthread c2c-threads.c -lpthread -lm
clean:
	rm -f tc2c tc2c-debug log-tc2c *.o
