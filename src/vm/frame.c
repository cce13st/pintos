#include "frame.h"

/* Initialize Frame Table */
void frame_init ()
{
	lock_init (&frame_lock);
	list_init (&frame_list);
}

/* Insert frame table entry
 * argument
 * 	upage : user page address,
 * 	kpage : kernel page address,
 * 	t : current thread */
void frame_insert (uint8_t *upage, uint8_t *kpage, struct thread *t)
{
	lock_acquire (&frame_lock);
	struct frame_entry *fte = (struct frame_entry *)malloc (sizeof (struct frame_entry));
	fte->upage = upage;
	fte->kpage = kpage;
	fte->t = t;
	
	list_insert (&frame_list, &fte->list_elem);
	lock_release (&frame_lock);
}

/* Remove entry from frame table */
void frame_remove (uint8_t *kpage)
{
	lock_acquire (&frame_lock);
	struct frame_entry *aux;
	struct list_elem *target;

	for (target = list_front (&frame_list); target != list_end (&frame_list); target = list_next (&target))
	{
		aux = list_entry (target, struct frame_entry, list_elem);
		if (aux->kpage == kpage)
		{
			list_remove (&aux->list_elem);
			free (aux);
			break;
		}
	}

	lock_release (&frame_lock);
}

void eviction ()
{
	/* Find victim */
	struct frame_entry *victim = find_victim ();
	// Swap Out
	// Free this entry
	list_remove (&frame_list, *fte->list_elem);
	free (victim);
}
