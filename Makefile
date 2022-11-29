CFLAGS = -Wall -g
CC=cc

all: tstSB

tstSB: tstSB.o disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o
	$(CC) -g -o tstSB tstSB.o disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o -lm

tstSB.o: tstSB.c
	$(CC) $(CFLAGS) -c tstSB.c

disk_driver.o: disk_driver.c disk_driver.h bfs_errno.h
	$(CC) $(CFLAGS) -c disk_driver.c

ffs_bytemap.o: ffs_bytemap.c ffs_bytemap.h bfs_errno.h
	$(CC) $(CFLAGS) -c ffs_bytemap.c

ffs_super.o: ffs_super.c ffs_super.h disk_driver.h ffs_inode.h bfs_errno.h
	$(CC) $(CFLAGS) -c ffs_super.c

ffs_inode.o: ffs_inode.c ffs_super.h disk_driver.h ffs_inode.h bfs_errno.h
	$(CC) $(CFLAGS) -c ffs_inode.c

clean:
	rm tstSB tstSB.o disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o

