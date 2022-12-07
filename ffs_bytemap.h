// FSO 2022-2023

#ifndef FFS_BYTEMAP_H
#define FFS_BYTEMAP_H

#include "disk_driver.h"
extern struct disk_operations disk_ops;

// For choosing between the two bytemaps: inodes or data 
#define INODE_BMAP    0
#define DATA_BMAP     1

#define NBR_OF_BMAPS    2

#define FREE        0
#define IN_USE      1

// Declare as many entries of this structure as bytemaps you want.
// Current implementation in .c uses an array of these structures

struct bmapMData {
    unsigned int BMbStart;    // abs disk block number of first block
    unsigned int BMbEnd;        // abs disk block number of last block
    unsigned int BMentries;    // number of valid entries in this bmap
};


/* operations on bytemaps

   mount: initializes in-memory structure that holds bmap addresses
     disk address, in-memory start/end indexes

   ummount: clears the bytemap in-memory structure. Must be called by
       for multiple mount/umounts in the same run

   getfree: get the first free (zero) bytemap entry
     parameters:
       @in: bmapIDX (which bmap to access)
     returns:
       on success, the entry absolute address
     errors:
       -ENOSPC if no free entry
       those resulting from disk operations

   set: set a bytemap entry to a value and updates disk block
     parameters:
       @in: bmapIDX (which bmap to access), entry absolute address
                value
     returns:
       0 on success
     errors:
       -EINVAL entry address invalid
       those resulting from disk operations

   clear: clears the full  bytemap zone blocks (used in format())
     parameters:
       @in: bmapIDX (which bmap to access)
     returns:
       0 on success
     errors:
*/


/* Helper function prototypes

  bytemap_print_table: prints the full table, 16 entries per line
    parameters:
     @in: bmapIDX (which bmap to access) 0: INODE_BMAP 1: DATA_BMAP
       those resulting from disk operations

*/

int bytemap_print_table(unsigned int bmapIDX);


struct bytemap_operations {
    void (*mount)(struct super *sb);
    void (*umount)();
    int (*getfree)(unsigned int bmapIDX);
    int (*set)(unsigned int bmapIDX, unsigned int entry, unsigned char set);
    int (*clear)(unsigned int bmapIDX, struct super *sb);
};

#endif
