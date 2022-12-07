CFLAGS = -Wall -g
CC=cc

# programs to compile:
all: tstSB
#all: tstSB tstCopyin tstCopyout

tstCopyin: tstCopyin.c disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o ffs_file.o bfs_dir.o bfs_lib.o
	$(CC) $(CFLAGS) -o tstCopyin tstCopyin.c disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o ffs_file.o bfs_dir.o bfs_lib.o

tstCopyout: tstCopyout.c disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o ffs_file.o bfs_dir.o bfs_lib.o
	$(CC) $(CFLAGS) -o tstCopyout tstCopyin.c disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o ffs_file.o bfs_dir.o bfs_lib.o


tstBFS: tstBFS.o disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o ffs_file.o bfs_dir.o bfs_lib.o
	$(CC)  -g -o tstBFS tstBFS.o disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o ffs_file.o bfs_dir.o bfs_lib.o -lm

tstSB: tstSB.o disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o ffs_file.o bfs_dir.o bfs_lib.o
	$(CC)  -g -o tstSB tstSB.o disk_driver.o ffs_bytemap.o ffs_super.o ffs_inode.o ffs_file.o bfs_dir.o bfs_lib.o


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

bfs_dir.o: bfs_dir.c bfs_dir.h ffs_super.h disk_driver.h bfs_errno.h ffs_file.h
	$(CC) $(CFLAGS) -c bfs_dir.c

ffs_file.o: ffs_file.c ffs_file.h ffs_bytemap.h ffs_super.h bfs_errno.h
	$(CC) $(CFLAGS) -c ffs_file.c

bfs_lib.o: bfs_lib.c bfs_lib.h bfs_dir.h
	$(CC) $(CFLAGS) -c bfs_lib.c

clean:
	rm -f tstSB tstCopyin tstCopyout *.o


