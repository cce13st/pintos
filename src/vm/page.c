#include "vm/page.h"
#include "threads/malloc.h"

unsigned page_val (const struct hash_elem *e, void *aux UNUSED)
{
<<<<<<< HEAD
	struct frame_entry *fte = hash_entry (e, struct frame_entry, hash_elem);
=======
	struct spt_entry *fte = hash_entry (e, struct spt_entry, hash_elem);
>>>>>>> remotes/origin/a_pj31
	return fte->kpage;
}

bool page_cmp (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
<<<<<<< HEAD
	struct frame_entry *fte1 = hash_entry (a, struct frame_entry, hash_elem);
	struct frame_entry *fte2 = hash_entry (b, struct frame_entry, hash_elem);
	
=======
	struct spt_entry *fte1 = hash_entry (a, struct spt_entry, hash_elem);
	struct spt_entry *fte2 = hash_entry (b, struct spt_entry, hash_elem);
>>>>>>> remotes/origin/a_pj31
	return fte1->kpage < fte2->kpage;
}

static struct spt_entry
*init_entry (uint8_t *upage, uint8_t *kpage, struct thread *t)
{
	struct spt_entry *spte;
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
	lock_init (&spt_lock);
	hash_init (&spt_hash, &page_val, &page_cmp, NULL);
}

/* Insert supplement page table entry */
void spt_insert (uint8_t *upage, uint8_t *kpage, struct thread *t)
{
	lock_acquire (&spt_lock);
	struct spt_entry *spte = init_entry (upage, kpage, t);
	hash_insert (&spt_hash, &spte->hash_elem);
//	frame_insert (upage, kpage, t);
	lock_release (&spt_lock);
}	

/* Remove supplement page table entry */
void spt_remove (uint8_t *kpage)
{
<<<<<<< HEAD
	lock_acquire (&spage_lock);
	struct spt_entry *spte = spt_find_upage (kpage);
	hash_remove (&spt_hash, &spte->hash_elem);
=======
	lock_acquire (&spt_lock);
	struct spt_entry *spte = spt_find_kpage (kpage);
	hash_delete (&spt_hash, &spte->hash_elem);
>>>>>>> remotes/origin/a_pj31
	free (spte);
	//frame_remove (kpage);
	lock_release (&spt_lock);
}

/* Find the spt_entry */
<<<<<<< HEAD
struct spt_entry 
*spt_find_upage (uint8_t *kpage)
=======
struct spt_entry
*spt_find_kpage (uint8_t *kpage)
>>>>>>> remotes/origin/a_pj31
{
	lock_acquire (&spt_lock);
	struct spt_entry *spte, *aux;
	struct hash_elem *target;
	aux = (struct spt_entry *)malloc (sizeof (struct spt_entry));
	aux->kpage = kpage;
<<<<<<< HEAD
	target = hash_find (&spt_hash, aux);
=======
	target = hash_find (&spt_hash, &aux->hash_elem);
>>>>>>> remotes/origin/a_pj31
	spte = hash_entry (target, struct spt_entry, hash_elem);
	lock_release (&spt_lock);
 	return spte;
}
