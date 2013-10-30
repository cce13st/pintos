#include "threads/synch.h"
#include "vm/frame.h"
#include "lib/kernel/hash.h"

void spt_init (void);
void spt_insert (uint8_t *, uint8_t *, struct thread *);
void spt_remove (uint8_t *);
struct spt_entry *spt_find_kpage (uint8_t *);

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

struct lock spt_lock;
struct hash spt_hash;
