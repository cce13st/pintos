#ifndef PAGE_H
#define PAGE_H
#include "threads/synch.h"
#include "filesys/off_t.h"
#include <hash.h>

void spt_init (struct thread *);
void spt_insert (void *, void *, bool, struct thread *);
void spt_remove (void *, struct thread *);
void spt_destroy (struct hash_elem *, void *);
struct spt_entry *spt_find_upage (void *, struct thread *);
void stack_growth (void *, struct thread *);

struct spt_entry
{
	bool swapped;
	void *upage;
	uint32_t swap_idx;
	void *kpage;
	struct thread *t;

	/* Lazy Loading */
	bool lazy;
	bool zero;
	bool writable;
	off_t offset;
	size_t read_bytes;
	struct file *file;

	struct hash_elem hash_elem;
};
#endif 
