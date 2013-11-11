#include "vm/page.h"
#include "vm/swap.h"
#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

unsigned page_val (const struct hash_elem *e, void *aux UNUSED)
{
	struct spt_entry *fte = hash_entry (e, struct spt_entry, hash_elem);
	return fte->upage;
}

bool page_cmp (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
	struct spt_entry *fte1 = hash_entry (a, struct spt_entry, hash_elem);
	struct spt_entry *fte2 = hash_entry (b, struct spt_entry, hash_elem);
	return fte1->upage < fte2->upage;
}

static struct spt_entry
*init_entry (void *upage, void *kpage, struct thread *t)
{
	struct spt_entry *spte;
	spte = (struct spt_entry *)malloc (sizeof (struct spt_entry));
	spte->swapped = false;
	spte->upage = upage;
	spte->kpage = kpage;
	spte->t = t;

	return spte;
}

/* Initialize Supplemental Page Table */
void spt_init (struct thread *t)
{
	hash_init (&t->spt_hash, &page_val, &page_cmp, NULL);
}

/* Insert supplement page table entry */
void spt_insert (void *upage, void *kpage, struct thread *t)
{
	struct spt_entry *spte = init_entry (upage, kpage, t);
	hash_insert (&t->spt_hash, &spte->hash_elem);
	frame_insert (upage, kpage-PHYS_BASE, t);
}	

/* Remove supplement page table entry */
void spt_remove (void *upage, struct thread *t)
{
	struct spt_entry *spte = spt_find_upage (upage, t);
	hash_delete (&t->spt_hash, &spte->hash_elem);
	free (spte);
	lock_acquire (&frame_lock);
	frame_remove (spte->kpage);
	lock_release (&frame_lock);
}

/* Find the spt_entry */
struct spt_entry
*spt_find_upage (void *upage, struct thread *t)
{
	struct spt_entry *spte, *aux;
	struct hash_elem *target;

	aux = (struct spt_entry *)malloc (sizeof (struct spt_entry));
	aux->upage = upage;

	target = hash_find (&t->spt_hash, &aux->hash_elem);
	if (target == NULL)
		return NULL;
	spte = hash_entry (target, struct spt_entry, hash_elem);
	
	free(aux);
 	return spte;
}

void stack_growth(void *upage, struct thread *t)
{
	struct spt_entry *spte;
	void *new;

	new = palloc_get_page (PAL_USER);
  pagedir_get_page (t->pagedir, upage);
  pagedir_set_page (t->pagedir, upage, new, true);
	spt_insert (upage, new, t);
}

void spt_destroy (struct hash_elem *elem, void *aux)
{
	struct spt_entry *spte = hash_entry (elem, struct spt_entry, hash_elem);
	if (spte->swapped){
		swap_clear (spte->swap_idx);
	}
	free (spte);
}
