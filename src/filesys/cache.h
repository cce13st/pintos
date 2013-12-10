#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/disk.h"
#endif /* filesys/inode.h */

char *cdata;
disk_sector_t *cidx;
bool *cvalid;
bool *cdirty;
int rucnt;
int *cused;

void cache_init (void);
int cache_find (disk_sector_t);
void cache_read (disk_sector_t, off_t, char *, int);
void cache_write (disk_sector_t, off_t, char *, int);
int cache_get (void);
void cache_out (int);
