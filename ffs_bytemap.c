// FSO 2022-2023

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "bfs_errno.h"
#include "disk_driver.h"

extern struct disk_operations disk_ops;

#include "ffs_super.h"

extern struct super_operations super_ops;
extern struct IMsuper ffs_IMsb;

#include "ffs_inode.h"
#include "ffs_bytemap.h"

/* Global variables */

struct bmapMData bmapMD[NBR_OF_BMAPS];

/* Helper functions */

#define MIN(x, y) (x <= y ? x : y)

/***
  bytemap_print_table: prints the full table, 16 entries per line
    parameters:
     @in: bmapIDX (which bmap to access) 0: INODE_BMAP 1: DATA_BMAP
       those resulting from disk operations
***/
int bytemap_print_table(unsigned int bmapIDX)
{
  int ercode;
  unsigned char bmap[DISK_BLOCK_SIZE];
  char msg[16];

  unsigned int blockStart, blockEnd, entriesLeft;
  int scan = 0;

  switch (bmapIDX)
  {
  case INODE_BMAP:
    blockStart = bmapMD[INODE_BMAP].BMbStart;
    blockEnd = bmapMD[INODE_BMAP].BMbEnd;
    entriesLeft = bmapMD[INODE_BMAP].BMentries;
    sprintf(msg, "%s", "inode");
    break;

  case DATA_BMAP:
    blockStart = bmapMD[DATA_BMAP].BMbStart;
    blockEnd = bmapMD[DATA_BMAP].BMbEnd;
    entriesLeft = bmapMD[DATA_BMAP].BMentries;
    sprintf(msg, "%s", "data blocks");
    break;
  }

  printf("Printing the %s bytemap ----------\n", msg);

  int lastNLneeded = entriesLeft; // 20221121

  for (int block = blockStart; block <= blockEnd; block++)
  {
    ercode = disk_ops.read(block, bmap, 1);
    if (ercode < 0)
      return ercode;

    // print 1 full block or the remaining entries
    for (int scan = 0; scan < MIN(entriesLeft, DISK_BLOCK_SIZE); scan++)
    {
      if ((scan + 1) % 16)
        printf("%u ", bmap[scan]);
      else
        printf("%u\n", bmap[scan]);
    }
    entriesLeft -= MIN(entriesLeft, DISK_BLOCK_SIZE);
    if (scan % 16)
      printf("\n"); // last NL for general case
  }

  // 20221121
  if (lastNLneeded % 16)
    printf("\n"); // last NL for a map (IDX or DATA)

  assert(entriesLeft == 0);

  return 0;
}

/* bytemap operations */

/***
  mount: initializes in-memory structure that holds bmap absolute
   start and end disk block addresses and
   number of valid entries in the bmap
***/
static void bytemap_mount(struct super *sb)
{
  unsigned int blockStart, blockEnd, entriesLeft;
  blockStart = super_ops.getStartInBmap(sb);
  blockEnd = blockStart + super_ops.getSizeInBmap(sb) - 1;
  entriesLeft = super_ops.getTotalInodes(sb);
  bmapMD[INODE_BMAP].BMbStart = blockStart;
  bmapMD[INODE_BMAP].BMbEnd = blockEnd;
  bmapMD[INODE_BMAP].BMentries = entriesLeft;

  blockStart = super_ops.getStartDtBmap(sb);
  blockEnd = blockStart + super_ops.getSizeDtBmap(sb);
  entriesLeft = super_ops.getNclusters(sb);
  bmapMD[DATA_BMAP].BMbStart = blockStart;
  bmapMD[DATA_BMAP].BMbEnd = blockEnd;
  bmapMD[DATA_BMAP].BMentries = entriesLeft;
}
  /***
  ummount: clears the bytemap in-memory structure. Must be called by
           for multiple mount/umounts in the same run                   ***/

  static void bytemap_umount()
  {
    memset(bmapMD, 0, sizeof(struct bmapMData) * NBR_OF_BMAPS);
  }

  /***
     getfree: first-fit get one free entry
       parameters:
         @in: bmapIDX (which bmap to access)
       returns:
         position of the 1st free entry
       errors:
         -ENOSPC there are no free entries
         those resulting from disk operations
  ***/
  static int bytemap_getfree(unsigned int bmapIDX)
  {
    int ercode;
    unsigned char bmap[DISK_BLOCK_SIZE];

    unsigned int block, blockStart, blockEnd, entriesLeft;
    unsigned int freeEntry = 0;

    if (bmapIDX == INODE_BMAP)
    {
      blockStart = bmapMD[INODE_BMAP].BMbStart;
      blockEnd = bmapMD[INODE_BMAP].BMbEnd;
      entriesLeft = bmapMD[INODE_BMAP].BMentries;
    }
    else
    {
      blockStart = bmapMD[DATA_BMAP].BMbStart;
      blockEnd = bmapMD[DATA_BMAP].BMbEnd;
      entriesLeft = bmapMD[DATA_BMAP].BMentries;
    }

    for (block = blockStart; block <= blockEnd; block++)
    {
      ercode = disk_ops.read(block, bmap, 1);
      if (ercode < 0)
        return ercode;

      /*** TODO find a free entry and, if found, return it ***/
      for (int i = 0; i <= entriesLeft; i++)
      {
        if (bmap[i] == FREE)
        {
          freeEntry = i;
          return freeEntry;
        }
      }
      entriesLeft -= MIN(entriesLeft, DISK_BLOCK_SIZE);
    }

    assert(entriesLeft == 0);
    return -ENOSPC;
  }

  /***
     set: set a bytemap entry to a value and updates disk block
       parameters:
         @in: bmapIDX (which bmap to access), entry absolute address
      value
       returns:
         0 on success
       errors:
         -EINVAL entry address invalid
         those resulting from disk operations
  ***/
  static int bytemap_set(unsigned int bmapIDX, unsigned int entry, unsigned char set)
  {
    int ercode;
    unsigned char bmap[DISK_BLOCK_SIZE];

    unsigned int blockStart, blockEnd, entriesLeft;

    if (bmapIDX == INODE_BMAP)
    {
      blockStart = bmapMD[INODE_BMAP].BMbStart;
      blockEnd = bmapMD[INODE_BMAP].BMbEnd;
      entriesLeft = bmapMD[INODE_BMAP].BMentries;
    }
    else
    {
      blockStart = bmapMD[DATA_BMAP].BMbStart;
      blockEnd = bmapMD[DATA_BMAP].BMbEnd;
      entriesLeft = bmapMD[DATA_BMAP].BMentries;
    }

    if (entry >= entriesLeft)
      return -EINVAL;
    
    // Locate the block where the entry is stored
    unsigned int block = entry / DISK_BLOCK_SIZE;
    if (block > (blockEnd - blockStart))
      return -EINVAL;

    // Read the block
    ercode = disk_ops.read((block + blockStart), bmap, 1);
    if (ercode < 0)
      return ercode;

    // Compute the entry's offset within the block and set its value
    unsigned int offset = (entry % DISK_BLOCK_SIZE);
    bmap[offset] = set;

    // Write back the changed value
    ercode = disk_ops.write((block + blockStart), bmap, 1);
    if (ercode < 0)
      return ercode;

    return 0;
  }

  /***
     clear: clears the full  bytemap zone blocks (used in format())
       parameters:
         @in: bmapIDX (which bmap to access)
       returns:
         0 on success
       errors:
         those resulting from disk operations
  ***/
  static int bytemap_clear(unsigned int bmapIDX, struct super *sb)
  {
    int ercode = 0;
    unsigned char bmap[DISK_BLOCK_SIZE];

    unsigned int block, blockStart, blockEnd;

    // NOTE: when format()ing, the disk is NOT mounted yet...
    //	   however, the in-mem superblock is already populated
    //	  CAVEAT: That's only true if super_ops.create is executed FIRST
    if (bmapIDX == INODE_BMAP)
    {
      blockStart = super_ops.getStartInBmap(sb);
      blockEnd = blockStart + super_ops.getSizeInBmap(sb) - 1;
    }
    else
    {
      blockStart = super_ops.getStartDtBmap(sb);
      blockEnd = blockStart + super_ops.getSizeDtBmap(sb) - 1;
    }

    memset(bmap, FREE, DISK_BLOCK_SIZE);
    for (block = blockStart; block <= blockEnd; block++)
    {
      ercode = disk_ops.write(block, bmap, 1);
      if (ercode < 0)
        return ercode;
    }

    return 0;
  }

  struct bytemap_operations bmap_ops = {
      .mount = bytemap_mount,
      .umount = bytemap_umount,
      .getfree = bytemap_getfree,
      .set = bytemap_set,
      .clear = bytemap_clear};
