#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include "bfs_lib.h"

/* copy from virtual disk to real OS file */
void copyOut(int fdDst, int fdSrc, char *buf, int length) {
  int bytesR, bytesW, acumR= 0, acumW= 0;
  for(;;) {
    memset(buf, 0, length);
    bytesR= bfs_read(fdSrc, buf, length);
    if (bytesR <= 0) {
        printf("bfs_read error: %d\n", bytesR);
        break;
    }
    acumR += bytesR;
    bytesW= write(fdDst, buf, length);
    acumW += bytesW;
  }
  printf("Bytes lidos   : %d\n", acumR);
  printf("Bytes escritos: %d\n", acumW);
}

int main(int argc, char *argv[]) {
  char diskname[8];
  int nblocks, cluster, fd, fd1, fd2, block_size;

  char buf[2048]; // MAX for cluster 4

  cluster = 1;   // testing with cluster of 1 block
                 // (change to the correct value of your virtual disk)
  block_size= cluster*512;

  bfs_mount("disco3", 1);

  fd1= bfs_open("File1"); // File1 must exist in virtual disco3 disk
  fd2= open("lixo.disco3.CopyOut", O_RDWR|O_CREAT|O_TRUNC, 0666);
  copyOut(fd2, fd1, buf, block_size);

  bfs_close(fd1);
  close(fd2);

  return 0;
}
