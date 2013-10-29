#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <hash.h>

void frame_init (void);
void frame_insert (uint8_t *, uint8_t *, struct thread *);
void frame_remove (uint8_t *);
void eviction (void);

struct frame_entry
{
	uint8_t *upage;
	uint8_t *kpage;
	struct thread *t;
	struct hash_elem hash_elem;
};

struct hash frame_hash;
struct lock frame_lock;
