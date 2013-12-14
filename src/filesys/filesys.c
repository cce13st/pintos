#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"
#include "threads/thread.h"

/* The disk that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  filesys_disk = disk_get (0, 1);
  if (filesys_disk == NULL)
    PANIC ("hd0:1 (hdb) not present, file system initialization failed");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  disk_sector_t inode_sector = 0;
	char buf[128];
	int pos = path_cut (name, buf);
	struct dir *dir;
	if (pos == 0) {
		//dir = dir_open(inode_open (thread_current()->cur_dir));
		if (name[0] == '/')
			dir = dir_open_root ();
		else {
			dir = dir_open (inode_open (thread_current ()-> cur_dir));
			pos = -1;
		}
	} else {	
		dir = get_directory (buf,*name == '/');
	}

	bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name+pos+1, inode_sector));

//printf("create inode : %x\n", dir_get_inode(dir));
	if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
 // struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

	char buf[128];
	int pos = path_cut (name, buf);
	struct dir *dir;
//	printf ("filesys_open name %s, buf %s\n", name, buf);	
	if (pos == 0) {
		if (name[0] == '/') { 
			dir = dir_open_root();
	//		printf("root\n");
		}
		else {
		//	printf("not root\n");
			dir = dir_open(inode_open (thread_current()->cur_dir));
			pos = -1;
		}
	}
	else {
//		printf("get_directory in open\n");
		dir = get_directory (buf, name[0] == '/');
	}
	if (dir != NULL) {
    if (name[0] =='/' && name[1] == 0) 
			inode = dir_get_inode(dir);
		else{
			dir_lookup (dir, name+pos+1, &inode);
		}
	}else {
		dir_close (dir);
		return NULL;
	}
	dir_close (dir);
//printf("is dir : %d\n", inode_is_dir(inode));
//printf("inode : %x\n", inode);
  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
//  struct dir *dir = dir_open_root ();
 	//printf ("filesys_remove\n"); 
	char buf[128];
	int pos = path_cut (name, buf);
	struct dir *dir;


	if (pos == 0) {
		if (name[0] == '/')
			dir = dir_open_root();
		else {
			dir = dir_open(inode_open (thread_current()->cur_dir));
			//dir = dir_open_root ();
			pos = -1;
		}
	} else {	
		dir = get_directory (buf,*name == '/');
	}
	//TODO
	if (strlen (name) == 2 && name[0] == '/')
		pos = 0;
	
	bool success = dir != NULL && dir_remove (dir, name+pos+1);
  dir_close (dir); 
  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
