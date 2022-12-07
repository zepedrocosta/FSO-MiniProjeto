#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "bfs_lib.h"

/* copy from a real OS file to a virtual disk */
void copyIn(int fdDst, int fdSrc, char *buf, int length) {
  int bytesR, bytesW, acumR= 0, acumW= 0;

  for(;;) {
    memset(buf, 0, length);
    bytesR= read(fdSrc, buf, length);
    if (bytesR == 0) break;
    acumR += bytesR;
    bytesW= bfs_write(fdDst, buf, length);
    if (bytesW < 0) {
        printf("bfs_write error: %d\n", bytesW);
        break;
    }
    acumW += bytesW;
  }
  printf("Bytes lidos   : %d\n", acumR);
  printf("Bytes escritos: %d\n", acumW);
}

int main(int argc, char *argv[]) {
  char diskname[8];
  int nblocks, cluster, fd1, fd2, block_size;

  char buf[2048]; // MAX for cluster 4

  printf("Disk name (for create)? "); scanf("%s", diskname);
  printf("Disk size (for create)? "); scanf("%d", &nblocks);
  disk_create(diskname, nblocks);   // a new disk is created

  printf("I-node area size (in blocks)? "); scanf("%d", &nblocks);
  printf("Cluster size (1, 2 or 4)? "); scanf("%d", &cluster);
  bfs_format(diskname, nblocks, cluster);  // the new disk os formated

  bfs_mount(diskname, 1);

  block_size= cluster*512;

  fd1= open("lixo.disco3.in", O_RDONLY); // The size of the file must be
					 // a multiple of block_size or garbage can appear at file end
  bfs_create("File1");  // bfs_create doesn't open the file
  fd2= bfs_open("File1");

  // Copy a file from Linux FS into BFS
  copyIn(fd2, fd1, buf, block_size);

  close(fd1);
  bfs_close(fd2);

  return 0;
}
