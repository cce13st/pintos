#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"

void
cache_init ()
{
	int i;
	rucnt = 1;		// Recently used count
	cdata = malloc (DISK_SECTOR_SIZE * CACHE_SIZE);		// actual disk data
	cidx = calloc (sizeof (int), CACHE_SIZE);				// sector index
	cvalid = malloc (sizeof (bool) * CACHE_SIZE);
	cdirty = malloc (sizeof (bool) * CACHE_SIZE);			// dirty bit
	cused = malloc (sizeof (int) * CACHE_SIZE);			// store rucnt for LRU eviction policy
	for (i=0; i<CACHE_SIZE; i++)
		cvalid[i] = false;
}

int
cache_find (disk_sector_t sector_idx)
{
	int i;
	for (i=0; i<CACHE_SIZE; i++)
		if (cidx[i] == sector_idx && cvalid[i])
			return i;

	return -1;
}

void
cache_read (disk_sector_t sector_idx, off_t ofs, char *buf, int size)
{
	int target = cache_find (sector_idx);
//	printf ("cache_read %x %d %x %d\n", filesys_disk, sector_idx, cdata, target);
	if (target != -1)
	{
		cused[target] = rucnt++;
		memcpy (buf, cdata + target * DISK_SECTOR_SIZE + ofs, size);
		return;
	}
	
	/* else, find from disk */
	int empty = cache_get ();
	cused[empty] = rucnt++;
	cidx[empty] = sector_idx;
	cvalid[empty] = true;
	cdirty[empty] = false;
  disk_read (filesys_disk, sector_idx, cdata + empty * DISK_SECTOR_SIZE);
	
	memcpy (buf, cdata + empty * DISK_SECTOR_SIZE + ofs, size);
	//hex_dump (buf, buf, 512, true);
}

void
cache_write (disk_sector_t sector_idx, off_t ofs, char *buf, int size)
{
	int target = cache_find (sector_idx);
//	printf ("cache_write %x %d %x %d\n", filesys_disk, sector_idx, cdata, target);
//	printf ("ofs %d, size %d buf %s\n", ofs, size, buf);
	if (target != -1)
	{
		cused[target] = rucnt++;
		cdirty[target] = true;
		memcpy (cdata + target * DISK_SECTOR_SIZE + ofs, buf, size);
		//hex_dump (cdata+target*DISK_SECTOR_SIZE, cdata+target*DISK_SECTOR_SIZE, 512, true);
		return;
	}
	
	/* else, find from disk */
	int empty = cache_get ();
	cused[empty] = rucnt++;
	cidx[empty] = sector_idx;
	cvalid[empty] = true;
	cdirty[empty] = true;
  disk_read (filesys_disk, sector_idx, cdata + empty * DISK_SECTOR_SIZE);
	
	memcpy (cdata + empty * DISK_SECTOR_SIZE + ofs, buf, size);
	//hex_dump (cdata+target*DISK_SECTOR_SIZE, cdata+target*DISK_SECTOR_SIZE, 512, true);
}

int
cache_get ()
{
	int i;
	for (i=0; i<CACHE_SIZE; i++)
		if (!cvalid[i])
			return i;
	
	/* Eviction process */
	int max = 0, midx = 0;
	for (i=0; i<CACHE_SIZE; i++){
		if (max < cused[i]){
			midx = i;
			max = cused[i];
		}
	}

	cache_out (midx);
	return midx;
}

void
cache_out (int target)
{
	cvalid[target] = false;

	if (cdirty[target])
		disk_write (filesys_disk, cidx[target], cdata + target * DISK_SECTOR_SIZE);
}
