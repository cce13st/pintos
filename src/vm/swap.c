#include "threads/vaddr.h"
#include "threads/thread.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"
#include <bitmap.h>

void swap_init ()
{
	swap_alloc = bitmap_create (1024);
	lock_init (&swap_lock);
}

/* swap out page from frame to disk */
void swap_out (void *upage)
{
	lock_acquire (&swap_lock);
	int i;
	void *src;
	unsigned dst;
	struct spt_entry *spte;
	struct disk *swap_disk;
	swap_disk = disk_get (1,1);

	spte = spt_find_upage (upage, thread_current ());
	src = spte->kpage;

	/* Find empty slot of swap disk */
	dst = (disk_sector_t) bitmap_scan (swap_alloc, 0, 1, false);
	if (dst == BITMAP_ERROR)
		PANIC("Swap disk full");

	/* Write on swap disk */
	for (i=0; i<8; i++) 
		disk_write (swap_disk, dst*8 + i, src+512*i);

	memset (src, 0, PGSIZE);
	bitmap_set (swap_alloc, dst, true);

	spte->swapped = true;
	spte->swap_idx = dst;

	lock_release (&swap_lock);
}

/* swap in page from disk to frame */
void swap_in (struct spt_entry *spte, void *kpage)
{
	lock_acquire (&swap_lock);
	int i;
	unsigned src;
	void *dst = kpage;
	struct disk *swap_disk;
	swap_disk = disk_get (1,1);

  /* Find swap slot containing upage */
	src = spte->swap_idx;

	/* Find swap */
	for (i=0; i<8; i++) 
		disk_read (swap_disk, src*8+i , dst+i*512);
	bitmap_set (swap_alloc, src, false);

	spte->swapped = false;
	spte->kpage = dst;
//	printf ("swap_in %x %x %u\n", spte->upage, spte->kpage, src*8);
	frame_insert(spte->upage, spte->kpage-PHYS_BASE, spte->t);
  pagedir_get_page (spte->t->pagedir, spte->upage);
  pagedir_set_page (spte->t->pagedir, spte->upage, spte->kpage, true);

	lock_release (&swap_lock);
}
