#ifndef BFS_DIR_H
#define BFS_DIR_H

#ifndef DISK_DRIVER_H
#include "disk_driver.h"
extern struct disk_operations disk_ops;
#endif

// need struct super
#ifndef FFS_SUPER_H
#include "ffs_super.h"
#endif

#define NAME_SIZE	14

struct dentry {
  char name[NAME_SIZE];
  unsigned short inoNbr;
};

union u_dir {
  struct dentry dirE;
  unsigned char buf[sizeof(struct dentry)];
};

/***
  In-memory directory structure used to define the CWD variable
   dirent: To have the name and inode of the cwd easily "available"
   fd:     fd of the cwd on the open files Table; get the rest from there
   dirPos: next dentry pointer
   dirBuf: the currently loaded directory data block			***/

struct IMdirectory {
  struct dentry dirent;
  unsigned int fd;
  unsigned int dirPos;
  union u_dir *dirBuf;
};


/* operations on directory structures

  RATIONALE:
    There is only one dir opened at a time, and that's the one sitting
    on the cwd structure. When the disk is mounted, cwd is filled with
    the disk's root info. Directories only use ONE CLUSTER for their data.
    - cd only descends into the next level. There is no cd . or cd ..,
      but there is cd / and cd <dir>
    - there is no opendir(), and readdir() operates on the cwd using
      dirpos as pointer; rewind sets dirpos to zero.
    - NOTE: dirpos is NOT the same as fpos (of the open file Table)

  mkEmptydir: creates an empty directory  on disk; creates the . and ..
          (allocates the i-node, data block, sets the bmaps)
          handles the special case of creation of the root directory
    parameters:
      @in: root? (yes=1, no=0); parent inode
     @out: nbr of inode allocated
    errors:
     from bytemap, inode, flat file operations

  opendirI: Open a dir specified by its inoNbr
        Brings in the 1st dir block
    parameters:
     @in: pointer to IMdirectory (cwd), inode number
    errors:
     those from ffs_file_open

  mount: brings in the root dir of the disk on mount
    parameters:
     @in: pointer to IMdirectory (cwd)
    errors:
     those from bfs_dir_open

  umount: cleans the data structures
    parameters:
     @in: pointer to IMdirectory (cwd)

  create: creates an empty file or directory on disk; 
          (f file, allocates the i-node, sets the bmap)
	  if creating a dir, creates the . and ..
          (and allocates the i-node, data block, sets the bmaps)
          handles the special case of creation of the root directory
    parameters:
      @in: root? (yes=1, no=0); parent inode
     @out: nbr of inode allocated
    errors:
     from bytemap, inode, flat file operations

  readdir: reads the next directory entry
           can be used to look for free or valid entries...
    parameters:
     @in: pointer to IMdirectory (cwd)
    errors:
     -1 when directory pointer at end

  cd: changes to subdirectory. Closes cur dir, opens new

  rewinddir: resets the current dir file pointer

  openF: opens a file

  unlink: unlink (deletes) a file

  closeF: closes a file
*/


/* Helper function prototypes 

*/



struct dir_operations {
  int (*mount)(struct IMdirectory *cwd);
  int (*umount)(struct IMdirectory *cwd);
  int (*opendirI)(struct IMdirectory *cwd, unsigned int inoNbr);
  int (*openF)(struct IMdirectory *cwd, char *name);
  int (*closeF)(int fd);
  int (*cd)(struct IMdirectory *cwd, char *name);
  int (*readdir)(struct IMdirectory *cwd, struct dentry *dirent);
  void (*rewinddir)(struct IMdirectory *cwd);
  int (*create)(struct IMdirectory *cwd, char *name, \
		unsigned int type);
  int (*unlink)(struct IMdirectory *cwd, char *name);
  int (*mkEmptydir)(unsigned int root, unsigned int pIno);
};

#endif
