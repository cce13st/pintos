#include "threads/synch.h"
#include <hash.h>

void spt_init (struct thread *);
void spt_insert (uint8_t *, void *, struct thread *);
void spt_remove (void *, struct thread *);
struct spt_entry *spt_find_kpage (void *, struct thread *);

struct spt_entry
{
	bool pinned;
	bool swapped;
	uint8_t *upage;
	uint8_t swap_idx;
	uint8_t *kpage;
	struct thread *t;

	struct hash_elem hash_elem;
};

struct list frame_stack;
