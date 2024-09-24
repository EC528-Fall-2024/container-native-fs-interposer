CC = g++
CFLAGS = -c -g -Werror 

.PHONY : FUSE_ll

FUSE_ll: FUSE_ll.o 
	$(CC) FUSE_ll.o -lfuse3 -o FUSE_ll

FUSE_ll.o: FUSE_ll.cpp
	$(CC) $(CFLAGS) FUSE_ll.cpp 

clean: 
	rm -f *.o FUSE_ll
