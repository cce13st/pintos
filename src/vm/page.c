#include "vm/page.h"
#include "vm/frame.h"
#include "threads/malloc.h"

unsigned page_val (const struct hash_elem *e, void *aux UNUSED)
{
	struct spt_entry *fte = hash_entry (e, struct spt_entry, hash_elem);
	return fte->kpage;
}

bool page_cmp (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
	struct spt_entry *fte1 = hash_entry (a, struct spt_entry, hash_elem);
	struct spt_entry *fte2 = hash_entry (b, struct spt_entry, hash_elem);
	return fte1->kpage < fte2->kpage;
}

static struct spt_entry
*init_entry (uint8_t *upage, void *kpage, struct thread *t)
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
void spt_init (struct thread *t)
{
	hash_init (&t->spt_hash, &page_val, &page_cmp, NULL);
	list_init (&frame_stack);
}

/* Insert supplement page table entry */
void spt_insert (uint8_t *upage, void *kpage, struct thread *t)
{
	struct spt_entry *spte = init_entry (upage, kpage, t);
	hash_insert (&t->spt_hash, &spte->hash_elem);
	frame_insert (upage, kpage, t);
	/* add a frame_entry in the frame_stack */
	list_push_back (&frame_stack, &fte->list_elem);
}	

/* Remove supplement page table entry */
void spt_remove (void *kpage, struct thread *t)
{
	struct spt_entry *spte = spt_find_kpage (kpage, t);
	hash_delete (&t->spt_hash, &spte->hash_elem);
	free (spte);
	frame_remove (kpage);
	/* remove a frame_entry in the frame_stack */
	////
}

/* Find the spt_entry */
struct spt_entry
*spt_find_kpage (void *kpage, struct thread *t)
{
	struct spt_entry *spte, *aux;
	struct hash_elem *target;
	struct list_elem *target_frame;

	aux = (struct spt_entry *)malloc (sizeof (struct spt_entry));
	aux->kpage = kpage;

	target = hash_find (&t->spt_hash, &aux->hash_elem);
	spte = hash_entry (target, struct spt_entry, hash_elem);
/*
	//find the target frame
	target_frame = find_target_frame (kpage);
	//remove that frame and push_back into the frame_list
	list_remove (target_frame);
	list_push_back (&frame_list, target_frame);
*/

	//dirty_bit and accessed_bit check, and then set their value to false.
	//if dirty_bit is true, then remove the element in the frame_stack, and then push_back in the frame_stack.

	free(aux);
 	return spte;
}

void stack_growth()
{// Stack growth condition is satisfied
/*
	//Request one more page
	struct spt_entry *spte;
	void *kpage;
	
	kpage = palloc_get_page (PAL_USER);
	spt_insert (upage, kpage, thread_current ());

	
*/
}
