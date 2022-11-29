// FSO 2022-2023

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "disk_driver.h"
#include "bfs_errno.h"

static struct {
    int dfd; // Only 1 disk at a time supported; if != -1, a disk is "open"
    unsigned int nblocks; // if != -1, number of blocks
} theDisk = {-1, -1};   // i.e., = 0xfffff...


static int disk_open_nc(const char *filename, unsigned int n) {
    struct stat s;
    unsigned char buf[DISK_BLOCK_SIZE];

    if (theDisk.dfd != -1) return -EBUSY;

    // open existing file
    if (!n) {
        if ((theDisk.dfd = open(filename, O_RDWR)) == -1) return -ENOENT;
        if (fstat(theDisk.dfd, &s) == -1) return -1;
        theDisk.nblocks = s.st_size / DISK_BLOCK_SIZE;
        return 0;
    }

    // create or re-create
    if ((theDisk.dfd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) return -1;

    memset(buf, 0, DISK_BLOCK_SIZE);
    for (int i = 0; i < n; i++)
        if (write(theDisk.dfd, buf, DISK_BLOCK_SIZE) == -1) return -1;

    theDisk.nblocks = n;

    return 0;
}


static int disk_stat_nc() {
    if (theDisk.dfd == -1) return -ENODEV;
    return theDisk.nblocks;
}


static int disk_read_nc(unsigned int blknmbr, unsigned char *buf, unsigned int reqblocks) {
    if (theDisk.dfd == -1) return -ENODEV;
    if (blknmbr + reqblocks >= theDisk.nblocks) return -ENOSPC;
    if (lseek(theDisk.dfd, blknmbr * DISK_BLOCK_SIZE, SEEK_SET) == -1) return -1;
    if (read(theDisk.dfd, buf, DISK_BLOCK_SIZE * reqblocks) < DISK_BLOCK_SIZE * reqblocks) return -1;

    return 0;
}


static int disk_write_nc(unsigned int blknmbr, const unsigned char *buf, unsigned int reqblocks) {
    if (theDisk.dfd == -1) return -ENODEV;
    if (blknmbr + reqblocks >= theDisk.nblocks) return -ENOSPC;
    if (lseek(theDisk.dfd, blknmbr * DISK_BLOCK_SIZE, SEEK_SET) == -1) return -1;
    if (write(theDisk.dfd, buf, DISK_BLOCK_SIZE * reqblocks) == -1) return -1;

    return 0;
}


static int disk_close_nc() {
    if (theDisk.dfd == -1) return -ENODEV;
    if (close(theDisk.dfd) == -1) return -1;
    theDisk.dfd = -1;
    return 0;
}


struct disk_operations disk_ops = {
        .open= disk_open_nc,
        .stat= disk_stat_nc,
        .read= disk_read_nc,
        .write= disk_write_nc,
        .close= disk_close_nc
};

