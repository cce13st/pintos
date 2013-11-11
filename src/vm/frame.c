#include "vm/frame.h"
#include "vm/page.h"
#include <bitmap.h>
#include "threads/vaddr.h"
#include "threads/thread.h"

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
	struct frame_entry *fte = (struct frame_entry *)malloc (sizeof (struct frame_entry));
	fte->upage = upage;
	fte->kpage = kpage;
	fte->t = t;

//	printf ("frame_insert %x %x %d\n", upage, kpage, t->tid);
	bitmap_set (frame_alloc, ((int)kpage)/PGSIZE, true);
	list_push_back (&frame_list, &fte->list_elem);
}

/* Remove entry from frame table */
void frame_remove (void *kpage)
{
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
}

/* Find free frame */
void *frame_get ()
{
	void *kpage;
	kpage = (void *) bitmap_scan (frame_alloc, 0, 1, false);
	if (kpage == BITMAP_ERROR){
		kpage = eviction();
		bitmap_set (frame_alloc, (unsigned)(kpage-0xc0000000)/PGSIZE, true);
	}
	else{
		kpage = (unsigned)kpage * PGSIZE;
		kpage = (unsigned)kpage + 0xc0000000;
	}

	return kpage;
}

static void *
eviction ()
{
	struct frame_entry *fte = find_victim ();
	void *empty_page;
	empty_page = fte->kpage;
	swap_out (fte);

	empty_page = (unsigned)empty_page + 0xc0000000;
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

void
frame_clear (struct thread *t)
{
	struct frame_entry *aux;
	struct list_elem *target, *garbage;
	for (target = list_begin (&frame_list); target != list_end (&frame_list);)
	{
		aux = list_entry (target, struct frame_entry, list_elem);
		if (aux->t == t)
		{
		//	printf ("frame_clear %x %x %d - %d\n", aux->upage, aux->kpage, aux->t->tid, (int)aux->kpage/PGSIZE);
			bitmap_set (frame_alloc, (int)aux->kpage/PGSIZE, false);
			garbage = target;
			target = list_next (target);
			list_remove (garbage);
			free (aux);
		}
		else
			target = list_next (target);
	}
}
