#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"

#include "threads/thread.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* On-disk inode.
   Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
  	bool is_dir;
		uint16_t index[251];
  };

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
		//bool is_dir;
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
	{
		int i = pos / DISK_SECTOR_SIZE;

		// 250 for double indirect, <= 249 for direct
		if (i < 250)
			return inode->data.index[i];
		else
		{
			uint16_t *buf1 = malloc (512);
			uint16_t *buf2 = malloc (512);
			disk_read (filesys_disk, inode->data.index[250], buf1);
			i -= 250;
			disk_read (filesys_disk, buf1[i/256], buf2);
			uint16_t result = buf2[i%256];
			free (buf1);
			free (buf2);
			return result;
		}
	}
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

			int i;
			static char zeros[DISK_SECTOR_SIZE];
			/* Direct allocate */
			for (i=0; i<250 && i<sectors; i++){
				free_map_allocate (1, disk_inode->index+i);
				disk_write (filesys_disk, disk_inode->index[i], zeros);
			}
			uint16_t *buf1 = malloc (512);
			uint16_t *buf2 = malloc (512);;
			/* Indirect allocate */
			if (250 <= sectors)
				free_map_allocate (1, disk_inode->index+250);

			for (i=250; i<sectors; i++){
				if (sectors - i >= 256)
				{
					int j, k = (i-250)/256;
					free_map_allocate (1, buf1+k);
					for (j=0; j<256; j++){
						free_map_allocate (1, buf2+j);
						disk_write (filesys_disk, buf2[j], zeros);
					}
					disk_write (filesys_disk, buf1[k], buf2);
					i += 255;
				}
				else
				{	
					int j, k = (i-250)/256;
					free_map_allocate (1, buf1+k);
					for (j=0; j<sectors-i; j++){
						free_map_allocate (1, buf2+j);
						disk_write (filesys_disk, buf2[j], zeros);
					}
					disk_write (filesys_disk, buf1[k], buf2);
					break;
				}
			}

			if (250 <= sectors)
				disk_write (filesys_disk, disk_inode->index[250], buf1);
			disk_write (filesys_disk, sector, disk_inode);
			success = true;
			free (buf1);
			free (buf2);
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
	//inode->is_dir = false;
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

bool
inode_is_dir (struct inode *inode)
{
	return inode->data.is_dir;
	//return 
}

void
inode_set_is_dir (struct inode *inode, bool boolean)
{
	//inode_deny_write(inode);
	inode->data.is_dir = boolean;
	disk_write (filesys_disk, inode->sector, &inode->data);
}

int
inode_open_cnt (struct inode *inode)
{
	return inode->open_cnt;
}
