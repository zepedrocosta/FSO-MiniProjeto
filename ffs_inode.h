
#ifndef FFS_INODE_H
#define FFS_INODE_H



// need struct super
#include "ffs_super.h"

// Disk block where inode table starts
#define INODE_OFFSET(sb)    (super_ops.getStartInArea(sb))


// Data block addressing pointers for indexed files:
// the last pointer is 1-level indirect, the others are direct
#define DPOINTERS_PER_INODE 5
#define IPOINTERS_PER_INODE 1

// inodes for indexed files
struct inode {
    unsigned char nlinks;    // also dubs as isvalid, 'cause 0 is free
    unsigned char type;
    unsigned short size;
    unsigned short dptr[DPOINTERS_PER_INODE];
    unsigned short iptr;
};

// in-memory inode
struct IMinode {
    struct inode ino;
    unsigned int dirty;
};


// Note that inodes currently fit exactly in one block

#define INODE_SIZE        (sizeof(struct inode))
#define INODES_PER_BLK        (DISK_BLOCK_SIZE / INODE_SIZE)

union u_inoBlk {
    struct inode ino[INODES_PER_BLK];
    unsigned char data[DISK_BLOCK_SIZE];
};


/* operations on inode structures

  init: clears (zero) an inode in-memory
    parameters:
     @out: pointer to inode structure

  read: fills in an inode with its disk image
    parameters:
     @in: (absolute) inode number
     @out: pointer to inode structure
    errors:
     those resulting from disk operations

  update: reads the corresponding inode block, merges the new inode
          and writes back the block
    parameters:
     @in: (absolute) inode number; pointer to inode structure
    errors:
     those resulting from disk operations

  debug: prints information about a single inode
    parameters:
     @in: inode number, pointer to inode, int to print all/valid only
  TBD: incomplete printout

  clear: clears the full inode zone blocks (used in format())
    parameters:
      none
    returns:
      0 on success
    errors:

*/


/* Helper function prototypes (public)

  inode_print_table: prints inode information
    parameters:
     @in: int if 0 prints all inodes of the inode table, if 1
	  prints only valid inodes
    errors:
      those from inode_read
*/

int inode_print_table(int validOnly);


struct inode_operations {
    void (*init)(struct inode *in);
    int (*update)(const unsigned int number, const struct inode *in);
    int (*read)(const unsigned int number, struct inode *in);
    int (*clear)(struct super *sb);
};

#endif
