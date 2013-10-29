#ifndef SPAGE_H_
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <hash.h>

struct spt_entry
{
	bool pinned;
	bool swapped;
	uint8_t *upage;
	uint8_t swap_idx;
	uint8_t *kpage;

	struct hash_elem hash_elem;
};

struct lock spage_lock;
struct hash spage_hash;

void spage_init (void);
void spage_insert ();
void spage_remove ();


#endif /* vm/page.h end */
