#ifndef FFS_FILE_H
#define FFS_FILE_H

// File types
#define REG_FILE	0
#define DIR_FILE	1


// Maximum open files (includes directories)
#define MAX_OPEN_FILES	8

// BFS defines should not mix with standard ones
#define	BFS_SEEK_SET	0
#define	BFS_SEEK_CUR	1
#define	BFS_SEEK_END	2

struct ffs_file {
  unsigned int valid;
  unsigned int fpos;
  struct IMinode IMino;
};


/* operations on file structures

  mount: initializes (clears) the in-memory structure openFT
	 Must be called by the library mount

  ummount: no need for it

  create: creates a file (just the inode!)
    parameters:
      @in: file type: one of REG_FILE, DIR_FILE
     @out: return OK
    errors:
     from bytemap, inode operations
     errors from disk driver

  unlink: unlinks a file, deleting if links==0
    parameters:
      @in: inode number
     @out: return OK
    errors:
     from bytemap, inode

  open: opens a file, if there's a free entry in the file table
        loads the inode, sets the file pointer and dirty both to 0/false
    parameters:
      @in: inode number
     @out: return  fd (FT entry index)
    errors:
     -ENFILE if no free entries in the file table
     errors from inode ops

  close: close a file, flushes the inode if dirty
         clear the file table entry
    parameters:
      @in: fd (FT entry index)
     @out: return 0 (OK)
    errors:
     -EBADF  bad fd (< 0 or > MAX_OPEN_FILES)
     -ENOENT fd does not refer to an open file
     errors from inode ops

  read: read a number of bytes from a file into a buffer
        updates the file pointer
    parameters:
      @in: fd (FT entry index), pointer to buffer, length to read
     @out: pointer to buffer, number of bytes read
    errors:
     -EBADF  bad fd (<0 or > MAX_OPEN_FILES)
     -EFBIG  file size exceeds maximum allowed
     -ENOENT fd does not refer to an open file

  write: write a number of bytes from a buffer to a file
         updates the file pointer
    parameters:
      @in: fd (FT entry index), pointer to buffer, length to write
     @out: pointer to buffer, number of bytes written
    errors:
     -EBADF  bad fd (<0 or > MAX_OPEN_FILES)
     -EFBIG  file size exceeds maximum allowed
     -ENOENT fd does not refer to an open file

  lseek: lseek to some position depending on whence and offset
        updates the file pointer
    parameters:
      @in: fd (FT entry index), offset and whence (see man lseek)
     @out: the new file pointer position
    errors:
     -EINVAL bad value for whence
     -ENXIO  the resulting position would be negative
*/

struct ffs_file_operations {
  void (*mount)();
  /* No need for umount() */
  int (*create)(unsigned char type);
  int (*unlink)(unsigned int inoNbr);
  int (*open)(const unsigned int inNbr);
  int (*close)(const unsigned int fd);
  int (*read)(const int fd, void *buffer, const unsigned int len);
  int (*write)(const int fd, const void *buffer, const unsigned int len);
  int (*lseek)(const unsigned int fd, const int offset,\
		const unsigned int whence);
};

#endif
