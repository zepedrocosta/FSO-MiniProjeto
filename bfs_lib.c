#include <stdio.h>
#include <string.h>


// File types duplicated from ffs_file.h
#define REG_FILE        0
#define DIR_FILE        1

#ifndef DISK_DRIVER_H
#include "disk_driver.h"
extern struct disk_operations disk_ops;
#endif

#ifndef FFS_SUPER_H
#include "ffs_super.h"
extern struct super_operations super_ops;
extern struct IMsuper ffs_IMsb;
#endif

#ifndef FFS_BYTEMAP_H
#include "ffs_bytemap.h"
extern struct bytemap_operations bmap_ops;
#endif

#ifndef FFS_INODE_H
#include "ffs_inode.h"
//extern struct inode_operations inode_ops;
#endif

#ifndef FFS_FILE_H
#include "ffs_file.h"
struct ffs_file_operations ffs_file_ops;
#endif

#ifndef BFS_DIR_H
#include "bfs_dir.h"
extern struct IMdirectory cwd;
extern struct dir_operations bfs_dir_ops;
#endif



int bfs_create(char *name) {
  int ercode;

  ercode= bfs_dir_ops.create(&cwd, name, REG_FILE); 
  return ercode;
}

int bfs_unlink(char *name) {
  int ercode;

  ercode= bfs_dir_ops.unlink(&cwd, name); 
  return ercode;
}

int bfs_mkdir(char *name) {
  int ercode;

  ercode= bfs_dir_ops.create(&cwd, name, DIR_FILE); 
  return ercode;
}

int bfs_open(char *name) {
  int ercode;

  ercode= bfs_dir_ops.openF(&cwd, name); 
  return ercode;
}

int bfs_close(int fd) {
  int ercode;

  ercode= bfs_dir_ops.closeF(fd); 
  return ercode;
}

int bfs_cd(char *name) {
  int ercode;

  ercode= bfs_dir_ops.cd(&cwd, name); 
  return ercode;
}

int bfs_read(unsigned int fd, void *buf, unsigned int length) {
  int ercode;

  ercode= ffs_file_ops.read(fd, buf, length); 
  return ercode;
}

int bfs_write(unsigned int fd, void *buf, unsigned int length) {
  int ercode;

  ercode= ffs_file_ops.write(fd, buf, length); 
  return ercode;
}

int bfs_lseek(const unsigned int fd, const int offset, \
                const unsigned int whence) {
  int ercode;

  ercode= ffs_file_ops.lseek(fd, offset, whence); 
  return ercode;
}

// For the hack below
int bfs_mount(char *diskname, int debug);
int bfs_umount();

void bfs_format(char *diskname, unsigned int sizeInArea,\
                            unsigned int clusterSize) {
  int ercode;

  ercode= disk_ops.open(diskname, 0);
  if (ercode < 0) {
    printf( "format():disk_ops.open %s\n", strerror(-ercode) );
    return;
  }

  ercode= disk_ops.stat(diskname);
  if (ercode < 0) {
    printf( "format():disk_ops.stat %s\n", strerror(-ercode) );
    return;
  }

  unsigned int nblocks= ercode;
  super_ops.create(&ffs_IMsb.sb, nblocks, sizeInArea, clusterSize);

  ercode= super_ops.write(&ffs_IMsb.sb);
  if (ercode < 0) {
    printf( "format():super_ops.write %s\n", strerror(-ercode) );
    return;
  }

  ercode= bmap_ops.clear(INODE_BMAP,&ffs_IMsb.sb);
  if (ercode < 0) {
    printf( "format():bmap_ops.clear INODE_BMAP %s\n", strerror(-ercode) );
    return;
  }
  ercode= bmap_ops.clear(DATA_BMAP,&ffs_IMsb.sb);
  if (ercode < 0) {
    printf( "format():bmap_ops.clear DATA_BMAP %s\n", strerror(-ercode) );
    return;
  }

/*
        MISSING MISSING MISSING MISSING MISSING MISSING MISSING
        MISSING clean of inodes and, optionally ?data?
  ercode= inode_ops.clear(&ffs_IMsb.sb);
  if (ercode < 0) {
    printf( "format():inode_ops.clear %s\n", strerror(-ercode) );
    return;
  }
  */

  // Format will now close the disk
  ercode= disk_ops.close();
  if (ercode < 0) {
    printf( "format():disk_ops.close %s\n", strerror(-ercode) );
  }

  /*** UGLY HACK: to create the root dir, the disk must be mounted, as
		  it's on the mount that in-core structures are init'ed	***/

  ercode= bfs_mount(diskname, 0);
  if (ercode < 0) {
    printf( "format():bfs_mount %s\n", strerror(-ercode) );
  }

  ercode= bfs_dir_ops.mkEmptydir(1, 0);
  if (ercode < 0) {
    printf( "format(): bfs_dir_ops.mkEmptydir %s\n", strerror(-ercode) );
    return;
  }

  ercode= bfs_umount();
  if (ercode < 0) {
    printf( "format():bfs_umount %s\n", strerror(-ercode) );
  }
}


int bfs_mount(char *diskname, int debug) {
  int ercode;

  ercode= super_ops.mount(diskname, &ffs_IMsb, debug);
  if (ercode < 0) return ercode;

  bmap_ops.mount(&ffs_IMsb.sb);
  ffs_file_ops.mount();
  bfs_dir_ops.mount(&cwd);

  return ercode;
}

int bfs_umount() {
  int ercode;

  ercode= super_ops.umount(&ffs_IMsb);
  if (ercode < 0) return ercode;

  bmap_ops.umount();
  // no umount for ffs_file_ops
  bfs_dir_ops.umount(&cwd);

  return ercode;
}

int disk_create(char *diskname, int nblocks) {
  int ercode;

  ercode= disk_ops.open(diskname, nblocks);
  if (ercode < 0) return ercode;

  ercode= disk_ops.close();
  return ercode;
}

