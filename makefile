CC=gcc
CFLAGS=-std=c99 -g
LIBS=-lpthread
#EXEC=main
#OBJS=somemain.o hashtable.o OS_list.o

main: hashtable.c hashtable.h OS_list.c OS_list.h test.c
	$(CC) $(CFLAGS) $(LIBS) test.c hashtable.c OS_list.c -o test

#$(EXEC): $(OBJS)
#	$(CC) $(CFLAGS) $(LIBS) $(OBJS) -o $(EXEC)

clean:
#	rm -f $(OBJS)
