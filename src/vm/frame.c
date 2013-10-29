#include "frame.h"

unsigned page_val (const struct hash_elem *e, void *aux UNUSED)
{
	struct frame_entry *fte = hash_entry (e, struct frame_entry, hash_elem);
	return fte->kpage;
}

bool page_cmp (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
	struct frame_entry *fte1 = hash_entry (a, struct frame_entry, hash_elem);
	struct frame_entry *fte2 = hash_entry (b, struct frame_entry, hash_elem);
	
	return fte1->kpage < fte2->kpage;
}

/* Initialize Frame Table */
void frame_init ()
{
	lock_init (&frame_lock);
	hash_init (&frame_hash, &page_val, &page_cmp, NULL);
}

/* Insert frame table entry
 * argument
 * 	upage : user page address,
 * 	kpage : kernel page address,
 * 	t : current thread */
void frame_insert (uint8_t *upage, uint8_t *kpage, struct thread *t)
{
	struct frame_entry *fte = (struct frame_entry *)malloc (sizeof (struct frame_entry));
	fte->upage = upage;
	fte->kpage = kpage;
	fte->t = t;

	hash_insert (&frame_hash, &fte->hash_elem);
}

/* Remove entry from frame table */
void frame_remove (uint8_t *kpage)
{
	struct frame_entry *fte, *aux;
	struct hash_elem *target;

	aux = (struct frame_entry *)malloc (sizeof (struct frame_entry));
	aux->kpage = kpage;
	target = hash_find (&frame_hash, aux);
	fte = hash_entry (target, struct frame_entry, hash_elem);

	hash_delete (&frame_hash, &fte->hash_elem);
	free (fte);
	free (aux);
}

void eviction ()
{
	/* Calculate LRU */

}
