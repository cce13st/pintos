#include "frame.h"
#include <bitmap.h>
#include "threads/vaddr.h"

void *eviction ();
struct frame_entry *find_victim ();

/* Initialize Frame Table */
void frame_init ()
{
	int i;
	frame_alloc = bitmap_create (1024);
	for (i=0; i<0x28b; i++)
		bitmap_set (frame_alloc, i, true);
	lock_init (&frame_lock);
	list_init (&frame_list);
}

/* Insert frame table entry
 * argument
 * 	upage : user page address,
 * 	kpage : kernel page address,
 * 	t : current thread */
void frame_insert (void *upage, void *kpage, struct thread *t)
{
//	lock_acquire (&frame_lock);
	struct frame_entry *fte = (struct frame_entry *)malloc (sizeof (struct frame_entry));
	fte->upage = upage;
	fte->kpage = kpage;
	fte->t = t;

	bitmap_set (frame_alloc, ((int)kpage)/PGSIZE, true);
	list_push_back (&frame_list, &fte->list_elem);
//	lock_release (&frame_lock);
}

/* Remove entry from frame table */
void frame_remove (void *kpage)
{
//	lock_acquire (&frame_lock);
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

//	lock_release (&frame_lock);
}

/* Find free frame */
void *frame_get ()
{
//	lock_acquire (&frame_lock);
	void *kpage;
	kpage = (void *) bitmap_scan (frame_alloc, 0, 1, false);
	if (kpage == BITMAP_ERROR)
		kpage = eviction();
	else{
		kpage = (unsigned)kpage * PGSIZE;
		kpage = (unsigned)kpage + 0xc028b000;
	}
//	lock_release (&frame_lock);
	return kpage;
}

static void *
eviction ()
{
	struct frame_entry *fte = find_victim ();
	void *empty_page = fte->kpage + 0xc0000000;
	swap_out (empty_page);
	pagedir_clear_page (fte->t->pagedir, fte->upage);
	frame_remove (fte->kpage);
	return empty_page;
}

struct frame_entry *
find_victim ()
{
	struct frame_entry *victim;
	struct list_elem *e;

	e = list_front (&frame_list);
	victim = list_entry (e, struct frame_entry, list_elem);
	return victim;
}
