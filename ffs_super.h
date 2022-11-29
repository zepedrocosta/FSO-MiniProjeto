// FSO 2022-2023
#ifndef FFS_SUPER_H
#define FFS_SUPER_H

#include "disk_driver.h"

/* Disk layout
   +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
   | SuB | BMi | Ino | BMd...BMd | Db0 | Db1 | Db2 | ... | END |
   +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
*/

#define SB_OFFSET    0
#define BMi_OFFSET    1

#define FS_MAGIC    0xf0f03410

#define NOTMOUNTED    0
#define MOUNTED        1

struct super {
    unsigned int fsmagic;
    unsigned int nblocks;        // Get that from the driver
    unsigned int startInBmap;    // Always 1 in BFS
    unsigned int sizeInBmap;    // Always 1 in BFS
    unsigned int startInArea;
    unsigned int sizeInArea;    // User format parameter (in blocks)
    unsigned int ninodes;
    unsigned int startDtBmap;
    unsigned int sizeDtBmap;
    unsigned int clusterSize;    // User format parameter (in blocks)
    unsigned int startDtArea;
    unsigned int nclusters;
    unsigned int mounted;
};

union u_sbBlk {
    struct super sb;
    unsigned char data[DISK_BLOCK_SIZE];
};

/* structure for in-memory (IM) variable(s) */
struct IMsuper {
    unsigned int dirty;
    struct super sb;
};


/* operations on superblock structures
  create: To be called by format(), requires NOT mounted as it
          overwrites the in-memory SB structure.
          DOES NOT UPDATE the disk superblock, that must be an explicit
	  call to the disk driver write function
    parameters:
     @in:  pointer to superblock structure, disk size (nblocks),
	   number of blocks to allocate for inodes (sizeInArea),
	   size of a cluster (clusterSize)
     @out: none 

  read: reads in a SB from disk, overwrites the in-memory SB structure.
        Requires disk open at the driver level
    parameters:
     @out: pointer to in-mem superblock structure

  write: writes the in-mem SB structure to disk.
        Requires disk open at the driver level
    parameters:
     @in: pointer to superblock structure

  mount: mount the superblock and optionally print its info
	 NO other disk should be mounted
    parameters:
     @in: disk filename, pointer to in-mem superblock,
	  debug(1) or not (0)
    errors:
     those from disk driver

  umount: flush the superblock to disk and wipes the in-mem image
    parameters:
     @in: pointer to in-mem superblock
    errors:
     those from disk driver

  get*: gets the relevant BFS info from the in-mem SB
    parameters: none
    returns: unsigned int value or address of region

  NOTE: dirty "bit" of in-memory SB must be set/cleared separately
	NOT used when there is NO support for cache
*/


struct super_operations {
    void (*create)(struct super *sb, unsigned int nblocks, \
          unsigned int sizeInArea, unsigned int clusterSize);

    int (*read)(struct super *sb);
    int (*write)(struct super *sb);
    int (*mount)(char *diskname, struct IMsuper *imSB, int debug);
    int (*umount)(struct IMsuper *imSB);

    unsigned int (*getStartInBmap)(struct super *sb);
    unsigned int (*getSizeInBmap)(struct super *sb);
    unsigned int (*getStartInArea)(struct super *sb);
    unsigned int (*getSizeInArea)(struct super *sb);
    unsigned int (*getTotalInodes)(struct super *sb);
    unsigned int (*getStartDtBmap)(struct super *sb);
    unsigned int (*getSizeDtBmap)(struct super *sb);
    unsigned int (*getClusterSize)(struct super *sb);
    unsigned int (*getStartDtArea)(struct super *sb);
    unsigned int (*getNclusters)(struct super *sb);
    unsigned int (*getMounted)(struct super *sb);

/* Helper function prototypes */
    void (*debug)(struct IMsuper *imSB, unsigned int dbg);
};

#endif
