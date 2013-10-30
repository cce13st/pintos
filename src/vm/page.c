#include "page.h"

unsigned page_val (const struct hash_elem *e, void *aux UNUSED)
{
	struct frame_entry *fte = hash_entry (e, struct frame_entry, hash_elem);
	return fte->upage;
}

bool page_cmp (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
	struct frame_entry *fte1 = hash_entry (a, struct frame_entry, hash_elem);
	struct frame_entry *fte2 = hash_entry (b, struct frame_entry, hash_elem);
	
	return fte1->upage < fte2->upage;
}

static struct spt_entry
*init_entry (upage, kpage, t)
{
	spte = (struct spt_entry *)malloc (sizeof (struct spt_entry));
	spte->pinned = false;
	spte->swapped = false;
	spte->upage = upage;
	spte->kpage = kpage;
	spte->t = t;

	return spte;
}

/* Initialize Supplemental Page Table */
void spt_init ()
{
	lock_init (&spage_lock);
	hash_init (&spt_hash, &page_val, &page_cmp, NULL);
}

/* Insert supplement page table entry */
void spt_insert (uint8_t *upage, uint8_t *kpage, struct thread *t)
{
	lock_acquire (&spage_lock);
	struct spt_entry *spte = init_entry (upage, kpage, t);
	hash_insert (&spt_hash, &spte->hash_elem);
	frame_insert (upage, kpage, t);
	lock_release (&spage_lock);
}	

/* Remove supplement page table entry */
void spt_remove (uint8_t *upage, uint8_t *kpage, struct thread *t)
{
	lock_acquire (&spage_lock);
	struct spt_entry *spte = spt_find_upage (upage);
	hash_remove (&spt_hash, &spte->hash_elem);
	free (spte);
	lock_release (&spage_lock);
}

/* Find the spt_entry */
struct spt_entry 
*spt_find_upage (uint8_t *upage)
{
	lock_acquire (&spage_lock);
	struct spt_entry *spte, *aux;
	aux = (struct spt_entry *)malloc (sizeof (struct spt_entry));
	aux->upage = upage;
	target = hash_find (&spt_hash, aux);
	spte = hash_entry (target, struct spt_entry, hash_elem);
 	return spte;
}
