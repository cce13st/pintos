#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* Define Disk cache buffer size */
#define CACHE_SIZE 128

/* On-disk inode.
   Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
		uint16_t index[252];
  };

static int cache_find (disk_sector_t);
static void cache_read (disk_sector_t, off_t, char *, int);
static void cache_write (disk_sector_t, off_t, char *, int);
static int cache_get (void);
static void cache_out (int);

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    disk_sector_t sector;               /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

/* Returns the disk sector that contains byte offset POS within
   INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static disk_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length)
		return inode->data.index[pos/DISK_SECTOR_SIZE];
    //return inode->data.start + pos / DISK_SECTOR_SIZE;
  else
    return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
	cache_init ();
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   disk.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (disk_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;
	
  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == DISK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      /*if (free_map_allocate (sectors, &disk_inode->start))
        {
          disk_write (filesys_disk, sector, disk_inode);
          if (sectors > 0) 
            {
              static char zeros[DISK_SECTOR_SIZE];
              size_t i;
              
              for (i = 0; i < sectors; i++) 
                disk_write (filesys_disk, disk_inode->start + i, zeros); 
            }
          success = true; 
        } */

			int i;
			static char zeros[DISK_SECTOR_SIZE];
			for (i=0; i<sectors; i++){
				free_map_allocate (1, disk_inode->index+i);
				disk_write (filesys_disk, disk_inode->index[i], zeros);
			}
			disk_write (filesys_disk, sector, disk_inode);
			success = true;
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (disk_sector_t sector) 
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  disk_read (filesys_disk, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
disk_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;
	
  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
			int i;

      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

			for (i=0; i<bytes_to_sectors (inode->data.length); i++){
				int exist = cache_find (inode->data.index[i]);
				if (exist != -1)
					cache_out (exist);
			}

      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
					for (i=0; i<bytes_to_sectors (inode->data.length); i++)
						free_map_release (inode->data.index[i], 1);
					//free_map_release (inode->data.index,
          //                  bytes_to_sectors (inode->data.length)); 
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;
  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      disk_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % DISK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

			cache_read (sector_idx, sector_ofs, buffer + bytes_read, chunk_size);
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

	int a = size;
  if (inode->deny_write_cnt)
    return 0;

	if (inode->data.length < offset+size)
	{
		/* File extensinon condition */
		int i, length = inode->data.length, sectors = bytes_to_sectors (inode->data.length);
		int extend = offset + size - length;
		static char zeros[DISK_SECTOR_SIZE];

		int remain = 0;
		if ((length % DISK_SECTOR_SIZE) != 0)
			remain = DISK_SECTOR_SIZE - (length % DISK_SECTOR_SIZE);

		if (remain >= extend)
			inode->data.length += extend;
		else
		{
			for (i=0; i<bytes_to_sectors (extend); i++){
				free_map_allocate (1, inode->data.index+sectors+i);
				disk_write (filesys_disk, inode->data.index[sectors+i], zeros);
			}
			inode->data.length += extend;
		}
		disk_write (filesys_disk, inode->sector, &inode->data);
	}

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      disk_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % DISK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
			
			printf ("write on sector %d\n", sector_idx);
			cache_write (sector_idx, sector_ofs, buffer + bytes_written, chunk_size);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

	//printf ("written %d, size %d\n", bytes_written, a);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

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

static int
cache_find (disk_sector_t sector_idx)
{
	int i;
	for (i=0; i<CACHE_SIZE; i++)
		if (cidx[i] == sector_idx && cvalid[i])
			return i;

	return -1;
}

static void
cache_read (disk_sector_t sector_idx, off_t ofs, char *buf, int size)
{
	int target = cache_find (sector_idx);
	printf ("cache_read %x %d %x %d\n", filesys_disk, sector_idx, cdata, target);
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
	
	memcpy (buf, cdata + target * DISK_SECTOR_SIZE + ofs, size);
	//hex_dump (buf, buf, 512, true);
}

static void
cache_write (disk_sector_t sector_idx, off_t ofs, char *buf, int size)
{
	int target = cache_find (sector_idx);
	printf ("cache_write %x %d %x %d\n", filesys_disk, sector_idx, cdata, target);
	printf ("ofs %d, size %d buf %s\n", ofs, size, buf);
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
	
	memcpy (cdata + target * DISK_SECTOR_SIZE + ofs, buf, size);
	//hex_dump (cdata+target*DISK_SECTOR_SIZE, cdata+target*DISK_SECTOR_SIZE, 512, true);
}

static int
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

static void
cache_out (int target)
{
	printf ("cache_out %d\n", cidx[target]);
	cvalid[target] = false;

	if (cdirty[target])
		disk_write (filesys_disk, cidx[target], cdata + target * DISK_SECTOR_SIZE);
}
