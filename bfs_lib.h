#ifndef BFS_LIB
#define BFS_LIB


#define BFS_SEEK_SET    0
#define BFS_SEEK_CUR    1
#define BFS_SEEK_END    2

int bfs_create(char *name);

int bfs_unlink(char *name);

int bfs_mkdir(char *name);

int bfs_open(char *name);

int bfs_close(int fd);

int bfs_cd(char *name);

int bfs_read(unsigned int fd, void *buf, unsigned int length);

int bfs_write(unsigned int fd, void *buf, unsigned int length);

int bfs_lseek(const unsigned int fd, const int offset, \
                const unsigned int whence);

void bfs_format(char *diskname, unsigned int sizeInArea,\
                            unsigned int clusterSize);

int bfs_mount(char *diskname, int debug);

int bfs_umount();

int disk_create(char *diskname, int nblocks);

#endif
