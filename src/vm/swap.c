#include "swap.h"

void swap_init ()
{
	unsigned size;
	swap_disk = disk_get (1,1);
	size = disk_size (swap_disk);
	swap_alloc = bitmap_create (size / PG_SIZE);
	lock_init (&swap_lock);
}

/* swap out page from frame to disk */
void swap_out (uint8_t *kpage)
{
	lock_acquire (&swap_lock);
	int i;
	uint8_t dst, *src;

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
	frame_remove (src);

	lock_release (&swap_lock);
}

/* swap in page from disk to frame */
void swap_in (uint8_t *kpage)
{
	lock_acquire (&swap_lock);
	int i;
	uint8_t src, *dst;

  /* Find swap slot containing upage */
	spte = spt_find_upage (upage);
	src = spte->swp_idx;

	/* Find swap */
	for (i=0; i<8; i++) 
		disk_read (swap_disk, dst*8 + i , src+ i*512);
	bitmap_set (swap_alloc, src, false);

	spte->swapped = false;
	spte->kpage = src;
	frame_insert(spte->upage, spte->kpage, spte->t);

	lock_release (&swap_lock);
}
