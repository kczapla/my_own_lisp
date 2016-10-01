CC=clang
CFLAGS=-c -Wall

all: prompt

prompt: lispy.o mpc.o
	$(CC) -ledit lispy.o mpc.o -o prompt

lispy.o: lispy.c
	$(CC) $(CFLAGS) lispy.c

mpc.o: mpc.h mpc.c
	$(CC) $(CFLAGS) mpc.h mpc.c

clean:
	rm *o *gch prompt
