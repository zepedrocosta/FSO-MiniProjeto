#include <string.h>
#include <math.h>

#include "bfs_errno.h"

#ifndef DISK_DRIVER_H
#include "disk_driver.h"
extern struct disk_operations disk_ops;
#endif

#ifndef FFS_SUPER_H
#include "ffs_super.h"
extern struct super_operations super_ops;
extern struct IMsuper ffs_IMsb;
#endif

#ifndef FFS_INODE_H
#include "ffs_inode.h"
extern struct inode_operations inode_ops;
#endif

#ifndef FFS_BYTEMAP_H
#include "ffs_bytemap.h"
extern struct bytemap_operations bmap_ops;
#endif


#ifndef FFS_FILE_H
#include "ffs_file.h"
#include <stdio.h> //para prints
#endif

#define CLUSTER_SIZE		( super_ops.getClusterSize(&ffs_IMsb.sb) )
#define DATA_BLOCK_SIZE		( DISK_BLOCK_SIZE * CLUSTER_SIZE )
#define START_DATA_AREA		( super_ops.getStartDtArea(&ffs_IMsb.sb) )
#define NOT_BLKALIGN(filPtr)    ( (filPtr % DATA_BLOCK_SIZE) != 0 )
#define LBLK_NBR(filPtr)        ( filPtr / DATA_BLOCK_SIZE )
#define LBLK_OFF(filPtr)        ( filPtr % DATA_BLOCK_SIZE )
#define L2P(lBlk)		( (lBlk * CLUSTER_SIZE) + START_DATA_AREA )

#define MIN(x,y)		( (x) < (y) ? (x) : (y) )
#define DIVUP(X, Y)     ( (X+Y-1)/(Y) )

struct ffs_file openFT[MAX_OPEN_FILES];

static void ffs_file_mount(){
  memset(openFT, 0, sizeof(openFT));
}

/***
  create: creates a file (just the inode!)
    parameters:
      @in: file type: one of REG_FILE, DIR_FILE
     @out: nbr of inode allocated
    errors:
     from bytemap, inode operations
     errors from disk driver						***/

static int ffs_file_create( unsigned char type ) { //NÃO MUDAR (FEITO PELOS STORES)
  int ercode;

  // get the inode entry for this file
  ercode=bmap_ops.getfree(INODE_BMAP);

  if (ercode < 0) return ercode;
  unsigned int inode2set=ercode;

  // mark the inode entry for this file
  ercode=bmap_ops.set(INODE_BMAP, inode2set, 1);

  if (ercode < 0) return ercode; // This would be a bug!

  // create the data in an "empty" i-node
  struct inode ino; memset(&ino, 0, sizeof(struct inode) );
  ino.nlinks= 1;
  ino.type= type;

  // save it to disk without disturbing other i-nodes
  ercode=inode_ops.update(inode2set, &ino);
  if (ercode < 0) return ercode; // This would be a bug!
  return inode2set;
 }


/***
  unlink: unlinks a file, deleting if links==0
    parameters:
      @in: inode number
     @out: return OK
    errors:
     from bytemap, inode						***/

static int ffs_file_unlink(const unsigned int inoNbr) {
  int ercode;
  struct inode ino;

  // Read the inode
  ercode= inode_ops.read(inoNbr, &ino);
  if (ercode < 0) return ercode;

  ino.nlinks--;
  if (ino.nlinks) {
    ercode=  inode_ops.update(inoNbr, &ino);	   // Update the inode
    if (ercode < 0) return ercode; // This would be a bug!
    return 0;
  }

  // FIRST, "delete" all the data blocks
  // NOTE: just the direct pointers, no need to support for the indirect
  
  for(int i = 0; i < DIVUP(ino.size, DISK_BLOCK_SIZE); i++){
    memset(&ino.dptr[i], 0, sizeof(struct inode)); //duvidaaa
  }

  // NOW, we can clear the inode
  memset(&ino, 0, sizeof(struct inode) );
  ercode=inode_ops.update(inoNbr, &ino);
  if (ercode < 0) return ercode; // This would be a bug!

  // and remember to free the bmap...
 /*** TODO ***/ 
  if (ercode < 0) return ercode; // This would be a bug!

  return 0;
}

/***
  open: opens a file, if there's a free entry in the file table
	loads the inode, sets the file pointer and dirty both to 0/false
    parameters:
      @in: inode number
     @out: return  fd (FT entry index)
    errors:
     -ENFILE if no free entries in the file table
     errors from inode ops						***/

static int ffs_file_open(const unsigned int inNbr) {

  int found= -ENFILE;
  for (int i= 0; i < MAX_OPEN_FILES; i++) 
    if (!openFT[i].valid) {
      found= i;
      openFT[i].valid= 1; openFT[i].fpos= 0; openFT[i].IMino.inNbr= inNbr;
      inode_ops.read(inNbr, &openFT[i].IMino.ino);
      openFT[i].IMino.dirty= 0;
      break;
    }

  return found;
}

/***
  close: close a file, flushes the inode if dirty
	 clear the file table entry
    parameters:
      @in: fd (FT entry index)
     @out: return 0 (OK)
    errors:
     -EBADF  bad fd (< 0 or > MAX_OPEN_FILES)
     -ENOENT fd does not refer to an open file
     errors from inode ops						***/

static int ffs_file_close(const unsigned int fd) {
  int ercode;

  if (fd >= MAX_OPEN_FILES) return -EBADF;
  if (!openFT[fd].valid) return -ENOENT;

  if (openFT[fd].IMino.dirty) {
    ercode= inode_ops.update(openFT[fd].IMino.inNbr, &openFT[fd].IMino.ino);
    if (ercode < 0) return ercode;
    // flushes of cached file blocks would be triggered here...
  }

  memset( &openFT[fd], 0, sizeof(struct ffs_file) );

  return 0;
}


/***
  read: read a number of bytes from a file into a buffer
	updates the file pointer
    parameters:
      @in: fd (FT entry index), pointer to buffer, length to read
     @out: pointer to buffer, number of bytes read
    errors:
     -EBADF  bad fd (<0 or > MAX_OPEN_FILES)
     -EFBIG  file size exceeds maximum allowed
     -ENOENT fd does not refer to an open file				***/

/***
  THIS VERSION:
    len must be == DATA_BLOCK_SIZE
    file size <= DPOINTERS_PER_INODE*DATA_BLOCK_SIZE (no indirect ptr) ***/

static int ffs_file_read(const int fd, void *buffer, const unsigned int len) {
  int ercode;
  unsigned int fpos= openFT[fd].fpos, size= openFT[fd].IMino.ino.size;
  unsigned int fileBlkNbr;

  if ( (fd >= MAX_OPEN_FILES) || (fd < 0) ) return -EBADF;
  if (!(openFT[fd].valid)) return -ENOENT;
  if ( fpos >= size ) return 0;

  fileBlkNbr= LBLK_NBR( (fpos + len-1) );
  ercode=disk_ops.read( L2P( openFT[fd].IMino.ino.dptr[fileBlkNbr] ),\
				  buffer, CLUSTER_SIZE );
  if (ercode < 0) return ercode;

  openFT[fd].fpos= fpos + len;

  return len;
}


/***
  write: write a number of bytes from a buffer to a file 
	 updates the file pointer
    parameters:
      @in: fd (FT entry index), pointer to buffer, length to write
     @out: pointer to buffer, number of bytes written
    errors:
     -EBADF  bad fd (<0 or > MAX_OPEN_FILES)
     -EFBIG  file size exceeds maximum allowed
     -ENOENT fd does not refer to an open file				***/

/***
  THIS VERSION:
    len must be == DATA_BLOCK_SIZE
    file size <= DPOINTERS_PER_INODE*DATA_BLOCK_SIZE (no indirect ptr) ***/

static int ffs_file_write(const int fd, const void *buffer,\
					const unsigned int len) {
  int ercode;
  unsigned int fpos= openFT[fd].fpos, size= openFT[fd].IMino.ino.size;
  unsigned int fileBlkNbr, lDskBlkToWrite;

  if ( (fd >= MAX_OPEN_FILES) || (fd < 0) ) return -EBADF;
  if (!(openFT[fd].valid)) return -ENOENT;
  if ( (fpos + len) > DPOINTERS_PER_INODE*DATA_BLOCK_SIZE ) return -EFBIG;

  // Compute the logical block number of the file from fpos and len
  /*** TODO ***/
  fileBlkNbr = LBLK_NBR((fpos + len - 1));
 
  // Then, see if the dptr pointer for that block is already used
  // if not, allocate a new block (a cluster!) for use in dptr
  /*** TODO ***/
  
  // now write the block, but remember: you should convert the
  // logical block number into a physical block number 
  /*** TODO ***/

  // Write the Block!
  ercode=disk_ops.write(L2P(openFT[fd].IMino.ino.dptr[fileBlkNbr]), buffer, CLUSTER_SIZE );
  if (ercode < 0) return ercode; // This would be a bug!

  // Dont forget to update the bmap, if a new cluster was allocated
  ercode=bmap_ops.getfree(DATA_BMAP);

  if (ercode < 0) return ercode;
  unsigned int inode2set=ercode;

  ercode=bmap_ops.set(DATA_BMAP, inode2set, 1);

  // Dont forget to update the size, if it has increased :-)
  /*** TODO ***/

  // Dont forget to update the inode, if necessary, and write it back
  /*** TODO ***/

  return (len);

}


/***
  lseek: lseek to some position depending on whence and offset
	updates the file pointer
    parameters:
      @in: fd (FT entry index), offset and whence (see man lseek)
     @out: the new file pointer position
    errors:
     -EINVAL bad value for whence
     -ENXIO  the resulting position would be negative			***/

int ffs_file_lseek (const unsigned int fd, const int offset,\
			const unsigned int whence) {

  unsigned int size= openFT[fd].IMino.ino.size;
  unsigned int cur= openFT[fd].fpos;

  switch (whence) {
    case BFS_SEEK_SET:
      if ( (offset > size) || (offset < 0) ) return -ENXIO;
      openFT[fd].fpos= offset;
      break;
    case BFS_SEEK_CUR:
      if ( (cur+offset > size) || (cur+offset < 0) ) return -ENXIO;
      openFT[fd].fpos= cur+offset;
      break;
    case BFS_SEEK_END:
      if ( (size+offset > size) || (size+offset < 0) ) return -ENXIO;
      openFT[fd].fpos= size+offset;
      break;
    default:
      return -EINVAL;
  }

  return openFT[fd].fpos;
}


struct ffs_file_operations ffs_file_ops= {
        .mount= ffs_file_mount,
        .create= ffs_file_create,
        .unlink= ffs_file_unlink,
	.open= ffs_file_open,
	.close= ffs_file_close,
	.read= ffs_file_read,
	.write= ffs_file_write,
	.lseek= ffs_file_lseek
};
