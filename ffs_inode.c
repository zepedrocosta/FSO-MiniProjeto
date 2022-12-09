#include <stdio.h>
#include <string.h>

#include "ffs_inode.h"
#include "bfs_errno.h"
#include "ffs_super.h"
#include "disk_driver.h"

extern struct super_operations super_ops;
extern struct IMsuper ffs_IMsb;
extern struct disk_operations disk_ops;

/* Helper functions */

/* inode (global) number is decomposed into inode block number
   and offset within that block. The inode block number starts at 0
*/
static int inode_location(unsigned int numinode, unsigned int *numblock, unsigned int *offset)
{
  if (numinode >= super_ops.getTotalInodes(&ffs_IMsb.sb)){
    printf("%d       ararrra\n", super_ops.getTotalInodes(&ffs_IMsb.sb));
    return -EINVAL;
  }
  *numblock = (numinode / INODES_PER_BLK) + ffs_IMsb.sb.startInArea;
  *offset = numinode % INODES_PER_BLK;

  return 0;
}

/***
  debug: prints information about a single inode
    parameters:
     @in: inode number, pointer to inode, int to print all/valid only
  TBD: incomplete printout
***/
void inode_debug(unsigned int i, struct inode *ino, int validOnly)
{
  if ((!ino->nlinks) && (validOnly))
    return;

  printf("Inode: %d\n", i);
  printf("  nlinks/isvalid = %u\n", (unsigned short)(ino->nlinks));
  printf("  type   = %u\n", (unsigned short)(ino->type));
  printf("  size   = %u\n", ino->size);
  printf("  Direct ptrs:\n");
  for (int i = 0; i < DPOINTERS_PER_INODE; i++)
    printf("    [%d]= %u\n", i, ino->dptr[i]);
  printf("  Indirect ptrs:\n");
  printf("    [%d]= %u\n", 5, ino->iptr);

  fflush(stdout);
}

static int inode_read(unsigned int numinode, struct inode *in); // defined below

/***
  inode_print_table: prints inode information
    parameters:
     @in: int if 0 prints all inodes of the inode table,
          if 1 prints only valid inodes
    errors:
      those from inode_read
***/
int inode_print_table(int validOnly)
{
  int ercode;
  struct inode ino;

  unsigned int ninodesLeft = super_ops.getTotalInodes(&ffs_IMsb.sb);

  for (unsigned int i = 0; i < ninodesLeft; i++)
  {
    ercode = inode_read(i, &ino);
    if (ercode < 0)
      return ercode;

    inode_debug(i, &ino, validOnly);
  }

  return 0;
}

/* "Object" functions */

/***
  init: clears (zero) an inode in-memory
    parameters:
     @out: pointer to inode structure
***/
static void inode_init(struct inode *in)
{
  memset(in, 0, sizeof(struct inode));
}

/***
  update: reads the corresponding inode block, merges the new inode
    and writes back the block
    parameters:
     @in: (absolute) inode number; pointer to inode structure
    errors:
     those resulting from disk operations
***/
static int inode_update(const unsigned int numinode, const struct inode *in)
{
  int ercode;
  unsigned int block, offset;
  union u_inoBlk i_b;

  if (inode_location(numinode, &block, &offset) < 0)
    return -EINVAL;

  // read inode block from disk into local mem
  ercode = disk_ops.read(block, i_b.data, 1);
  if (ercode < 0)
    return ercode;

  // merge inode into block
  memcpy(&i_b.ino[offset], in, INODE_SIZE);

  // write inode block to disk
  ercode = disk_ops.write(block, i_b.data, 1);
  if (ercode < 0)
    return ercode;

  return 0;
}

/***
  read: fills in an inode with its disk image
    parameters:
     @in: (absolute) inode number
     @out: pointer to inode structure
    errors:
     those resulting from disk operations
***/
static int inode_read(unsigned int numinode, struct inode *in)
{
  int ercode;
  unsigned int block, offset;
  union u_inoBlk i_b;

  if (inode_location(numinode, &block, &offset) < 0)
    return -EINVAL;

  // read inode block from disk into local mem
  ercode = disk_ops.read(block, i_b.data, 1);
  if (ercode < 0)
    return ercode;

  // extract inode from block
  memcpy(in, &i_b.ino[offset], INODE_SIZE);

  return 0;
}

/***
  clear: clears the full inode zone blocks (used in format())
    parameters:
      @in: pointer to in-memory superblock
    returns:
      0 on success
    errors:
     those resulting from disk operations
***/
int inode_clear(struct super *sb)
{
  int ercode;

  unsigned char data[DISK_BLOCK_SIZE];
  memset(data, 0, DISK_BLOCK_SIZE);

  unsigned int start = super_ops.getStartInArea(sb);
  unsigned int end = (super_ops.getStartDtBmap(sb) - 1);
  for (int block = start; block < end; block++)
    if ((ercode = disk_ops.write(block, data ,1)) < 0)
      return ercode;

  return 0;
}

struct inode_operations inode_ops = {
    .init = inode_init,
    .update = inode_update,
    .read = inode_read,
    .clear = inode_clear};
