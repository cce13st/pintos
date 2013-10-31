#include "frame.h"
#include <bitmap.h>
#include "threads/vaddr.h"

uint8_t eviction ();
struct frame_entry *find_victim ();

/* Initialize Frame Table */
void frame_init ()
{
	frame_alloc = bitmap_create (1000);
	lock_init (&frame_lock);
	list_init (&frame_list);
}

/* Insert frame table entry
 * argument
 * 	upage : user page address,
 * 	kpage : kernel page address,
 * 	t : current thread */
void frame_insert (uint8_t *upage, void *kpage, struct thread *t)
{
	lock_acquire (&frame_lock);
	struct frame_entry *fte = (struct frame_entry *)malloc (sizeof (struct frame_entry));
	fte->upage = upage;
	fte->kpage = kpage;
	fte->t = t;
	
	bitmap_set (frame_alloc, ((int)kpage)/PGSIZE, true);
	list_push_back (&frame_list, &fte->list_elem);
	lock_release (&frame_lock);
}

/* Remove entry from frame table */
void frame_remove (void *kpage)
{
	lock_acquire (&frame_lock);
	struct frame_entry *aux;
	struct list_elem *target;

	for (target = list_begin (&frame_list); target != list_end (&frame_list); target = list_next (target))
	{
		aux = list_entry (target, struct frame_entry, list_elem);
		if (aux->kpage == kpage)
		{
			bitmap_set (frame_alloc, (int)kpage/PGSIZE, false);
			list_remove (target);
			free (aux);
			break;
		}
	}

	lock_release (&frame_lock);
}

/* Find free frame */
uint8_t frame_get ()
{
	lock_acquire (&frame_lock);
	uint8_t kpage;
	kpage = (uint8_t) bitmap_scan_and_flip (frame_alloc, 0, 1, false);
	if (kpage == BITMAP_ERROR)
		kpage = eviction();
	else
		kpage /= PGSIZE;

	lock_release (&frame_lock);
	return kpage;
}

uint8_t eviction ()
{
	struct frame_entry *fte = find_victim ();
	uint8_t empty_page = fte->kpage;
	swap_out (empty_page);

	return empty_page;
}

struct frame_entry *
find_victim ()
{
	struct frame_entry *victim;
	
	victim = list_entry (list_begin (&frame_list), struct frame_entry, list_elem);
	
	return victim;
}
