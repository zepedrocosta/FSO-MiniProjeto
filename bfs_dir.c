#include <string.h>
#include <math.h>

#include <malloc.h>

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
extern struct ffs_file_operations ffs_file_ops;
extern struct ffs_file openFT[MAX_OPEN_FILES];
#endif

#ifndef BFS_DIR_H
#include "bfs_dir.h"
#endif

#define CLUSTER_SIZE (super_ops.getClusterSize(&ffs_IMsb.sb))
#define DATA_BLOCK_SIZE (DISK_BLOCK_SIZE * CLUSTER_SIZE)

#define MATCH 1
#define FREE 0

struct IMdirectory cwd;

/***
  mkEmptydir: creates an empty directory  on disk; creates the . and ..
    (allocates the i-node, data block, sets the bmaps)
    handles the special case of creation of the root directory
    parameters:
      @in: root? (yes=1, no=0); parent inode
     @out: nbr of inode allocated
    errors:
     from bytemap, inode, flat file operations				***/

int bfs_dir_mkEmptydir(unsigned int root, unsigned int pIno)
{
  int ercode, inode, dbs = DATA_BLOCK_SIZE, szE = sizeof(struct dentry);

  union u_dir dirBuf[dbs / szE];
  memset(dirBuf, 0, dbs);

  inode = ffs_file_ops.create(DIR_FILE);
  if (inode < 0)
    return inode;

  if ((root) && (inode > 0)){
    return -EINVAL; // BUG, should be zero
  }
  ercode = ffs_file_ops.open(inode); /////////////////////
  if (ercode < 0)
    return ercode;
  int fd = ercode;

  strcpy(dirBuf[0].dirE.name, ".");
  dirBuf[0].dirE.inoNbr = inode;
  strcpy(dirBuf[1].dirE.name, "..");
  dirBuf[0].dirE.inoNbr = pIno;
  ercode = ffs_file_ops.write(fd, dirBuf, DATA_BLOCK_SIZE);
  if (ercode < 0)
    return ercode;
  // if the write is made to return the number of the allocated block,
  // we can test if the root dir block == 0

  ercode = ffs_file_ops.close(fd);
  if (ercode < 0)
    return ercode;

  return inode;
}

/***
  opendirI: Open a dir specified by its inoNbr
  Brings in the 1st dir block
    parameters:
     @in: pointer to IMdirectory (cwd), inode number
    errors:
     those from ffs_file_open						***/

int bfs_dir_opendirI(struct IMdirectory *cwd, unsigned int inoNbr)
{
  int ercode, fd;

  ercode = ffs_file_ops.open(inoNbr); // i-node of the directory
  if (ercode < 0)
    return ercode;

  fd = ercode;
  ercode = ffs_file_ops.read(fd, cwd->dirBuf, DATA_BLOCK_SIZE);
  if (ercode < 0)
    return ercode;

  // Just checking :-)
  if (strcmp(cwd->dirBuf[0].dirE.name, "."))
    return -EINVAL;
  if (strcmp(cwd->dirBuf[1].dirE.name, ".."))
    return -EINVAL;

  cwd->fd = fd;

  // fpos must be reset to the beggining
  ercode = ffs_file_ops.lseek(BFS_SEEK_SET, 0, 0);
  if (ercode < 0)
    return ercode;

  return 0;
}

/***
  mount: brings in the root dir of the disk on mount
    parameters:
     @in: pointer to IMdirectory (cwd)
    errors:
     those from bfs_dir_open                                           ***/

int bfs_dir_mount(struct IMdirectory *cwd)
{
  int ercode;

  memset(cwd, 0, sizeof(struct IMdirectory));

  // REMEMBER to free on Umount
  union u_dir *dirBuf = malloc(DATA_BLOCK_SIZE);
  cwd->dirBuf = dirBuf;

  ercode = bfs_dir_opendirI(cwd, 0);
  if (ercode < 0)
    return ercode;

  strcpy(cwd->dirent.name, "/");
  cwd->dirent.inoNbr = 0;

  return 0;
}

/***
  umount: cleans the data structures
    parameters:
     @in: pointer to IMdirectory (cwd)
    errors:
                  ***/

int bfs_dir_umount(struct IMdirectory *cwd)
{

  free(cwd->dirBuf);
  memset(cwd, 0, sizeof(struct IMdirectory));

  return 0;
}

/***
  readdir: reads the next directory entry
     can be used to look for free or valid entries...
    parameters:
     @in: pointer to IMdirectory (cwd)
    errors:
     -1 when directory pointer at end                                          ***/

int bfs_dir_readdir(struct IMdirectory *cwd, struct dentry *dirent)
{

  if (cwd->dirPos < 0)
    return (cwd->dirPos); // Already at end

  /*** TODO ***/

  return (cwd->dirPos);
}

#define REWIND(x) (x->dirPos = 0)

/***
  Helper:
    if MATCH, returns match pos, -1 if none
    if FREE,  returns free pos,  -1 if none				***/

int matchAndFree(struct IMdirectory *cwd, int op, char *name)
{
  int ercode, pos = -1;
  struct dentry dirent;
  memset(&dirent, 0, sizeof(struct dentry));

  for (;;)
  {
    ercode = bfs_dir_readdir(cwd, &dirent);
    if (ercode < 0)
      break;
    if ((op == MATCH) && (!strcmp(name, dirent.name)))
    {
      pos = cwd->dirPos - 1;
      REWIND(cwd);
      return pos;
    }
    if ((op == FREE) && (strlen(dirent.name) == 0) && (pos == -1))
      pos = cwd->dirPos - 1;
  }
  REWIND(cwd);

  return pos;
}

/***
  create: creates an empty file or directory on disk;
          (f file, allocates the i-node, sets the bmap)
          if creating a dir, creates the . and ..
          (and allocates the i-node, data block, sets the bmaps)
          handles the special case of creation of the root directory
    parameters:
      @in: root? (yes=1, no=0); parent inode
     @out: nbr of inode allocated
    errors:
     from bytemap, inode, flat file operations				***/

int bfs_dir_create(struct IMdirectory *cwd, char *name, unsigned int type)
{
  int ercode, free = -1;
  struct dentry dirent;
  memset(&dirent, 0, sizeof(struct dentry));

  if (matchAndFree(cwd, MATCH, name) > 0)
    return -EEXIST;
  if ((free = matchAndFree(cwd, FREE, name)) < 0)
    return -ENOSPC;

  if (type == REG_FILE)
  {
    ercode = ffs_file_ops.create(type);
    if (ercode < 0)
      return ercode;
  }
  else
  { // create file and block with empty dir, etc. 0=NOT ROOT DIR
    ercode = bfs_dir_mkEmptydir(0, cwd->dirent.inoNbr);
    if (ercode < 0)
      return ercode;
  }

  // write the dirent to the current dir buffer
  dirent.inoNbr = ercode;
  strcpy(dirent.name, name);
  memcpy(&(cwd->dirBuf[free].dirE), &dirent, sizeof(struct dentry));

  // flush it to disk
  unsigned int dbsz = DATA_BLOCK_SIZE;
  ercode = ffs_file_ops.write(cwd->fd, cwd->dirBuf, dbsz);
  if (ercode < 0)
    return ercode;

  // fpos must be reset to the beggining
  ercode = ffs_file_ops.lseek(BFS_SEEK_SET, 0, 0);
  if (ercode < 0)
    return ercode;

  return 0;
}

/***
  cd: changes to subdirectory. Closes cur dir, opens new		***/

int bfs_dir_cd(struct IMdirectory *cwd, char *name)
{
  int ercode, pos, fdNEW, fdOLD, inoNbr;
  struct dentry dirent;

  if (strcmp(name, "/") != 0)
    if ((pos = matchAndFree(cwd, MATCH, name)) < 0)
      return -ENOENT;

  fdOLD = cwd->fd;
  ercode = ffs_file_ops.close(fdOLD);
  if (ercode < 0)
    return ercode;

  if (strcmp(name, "/") != 0)
  {
    memcpy(&dirent, &(cwd->dirBuf[pos].dirE), sizeof(struct dentry));
    inoNbr = dirent.inoNbr;
  }
  else
    inoNbr = 0;

  // cleaning OLD not mandatory, but...
  memset(&(cwd->dirent), 0, sizeof(struct dentry));
  cwd->fd = 0;     // well.... not really
  cwd->dirPos = 0; // idem...
  memset(cwd->dirBuf, 0, DATA_BLOCK_SIZE);

  fdNEW = bfs_dir_opendirI(cwd, inoNbr);

  // A check should be made to see if it's a DIR_FILE

  // if (fdNEW != fdOLD != 0) BUG because the close will free slot 0 and we reuse it
  return fdNEW;
}

/***
  rewinddir: resets the current dir file pointer			***/

void bfs_dir_rewinddir(struct IMdirectory *cwd)
{
  cwd->dirPos = 0;
}

/***
  openF: opens a file							***/

int bfs_dir_openF(struct IMdirectory *cwd, char *name)
{
  int pos, fd;
  struct dentry dirent;

  if ((pos = matchAndFree(cwd, MATCH, name)) < 0)
    return -ENOENT;

  memcpy(&dirent, &(cwd->dirBuf[pos].dirE), sizeof(struct dentry));
  fd = ffs_file_ops.open(dirent.inoNbr);

  // A check should be made to see if it's a REG_FILE
  return fd;
}

/***
  unlink: unlink (deletes) a file					***/

int bfs_dir_unlink(struct IMdirectory *cwd, char *name)
{
  int ercode, pos;
  struct dentry dirent;

  if ((pos = matchAndFree(cwd, MATCH, name)) < 0)
    return -ENOENT;

  memcpy(&dirent, &(cwd->dirBuf[pos].dirE), sizeof(struct dentry));
  // A check should be made to see if it's a REG_FILE

  ercode = ffs_file_ops.unlink(dirent.inoNbr);
  // Now delete the name (dentry)
  memset(&(cwd->dirBuf[pos].dirE), 0, sizeof(struct dentry));

  // flush it to disk
  unsigned int dbsz = DATA_BLOCK_SIZE;
  ercode = ffs_file_ops.write(cwd->fd, cwd->dirBuf, dbsz);
  if (ercode < 0)
    return ercode;

  // fpos must be reset to the beggining
  ercode = ffs_file_ops.lseek(BFS_SEEK_SET, 0, 0);

  return ercode;
}

/***
  closeF: closes a file							***/

int bfs_dir_closeF(int fd)
{
  int ercode;

  ercode = ffs_file_ops.close(fd);
  return ercode;
}

struct dir_operations bfs_dir_ops = {
    .mount = bfs_dir_mount,
    .umount = bfs_dir_umount,
    .opendirI = bfs_dir_opendirI,
    .openF = bfs_dir_openF,
    .closeF = bfs_dir_closeF,
    .cd = bfs_dir_cd,
    .readdir = bfs_dir_readdir,
    .rewinddir = bfs_dir_rewinddir,
    .create = bfs_dir_create,
    .unlink = bfs_dir_unlink,
    .mkEmptydir = bfs_dir_mkEmptydir};
