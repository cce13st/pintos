#include "threads/vaddr.h"
#include "vm/swap.h"
#include <bitmap.h>

void swap_init ()
{
	swap_alloc = bitmap_create (768);
	lock_init (&swap_lock);
}

/* swap out page from frame to disk */
void swap_out (void *kpage)
{
	lock_acquire (&swap_lock);
	int i;
	void *src = kpage;
	unsigned dst;
	struct spt_entry *spte;
	struct disk *swap_disk;
	swap_disk = disk_get (1,1);

	spte = spt_find_kpage (kpage, thread_current ());
	
	/* Find empty slot of swap disk */
	dst = (disk_sector_t) bitmap_scan_and_flip (swap_alloc, 0, 1, false);
	if (dst == BITMAP_ERROR)
		PANIC("Swap disk full");

	/* Write on swap disk */
	for (i=0; i<8; i++) 
		disk_write (swap_disk, dst*8 + i, src+512*i);
	bitmap_set (swap_alloc, dst, true);

	spte->swapped = true;
	spte->swap_idx = dst*8;

//	printf ("swap out %x\n", kpage);
	lock_release (&swap_lock);
}

/* swap in page from disk to frame */
void swap_in (void *kpage)
{
	lock_acquire (&swap_lock);
	int i;
	unsigned src;
	void *dst = kpage;
	struct spt_entry *spte;
	struct disk *swap_disk;
	swap_disk = disk_get (1,1);

  /* Find swap slot containing upage */
	spte = spt_find_kpage (kpage, thread_current ());
	src = spte->swap_idx;

	/* Find swap */
	for (i=0; i<8; i++) 
		disk_read (swap_disk, src*8+i , dst+i*512);
	bitmap_set (swap_alloc, src, false);

	spte->swapped = false;
	spte->kpage = src;
	frame_insert(spte->upage, spte->kpage, spte->t);

	printf ("swap in\n");

	lock_release (&swap_lock);
}
