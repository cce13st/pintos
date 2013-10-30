#include "threads/thread.h"
#include "threads/synch.h"

struct frame_entry
{
	uint8_t *upage;
	uint8_t *kpage;
	struct thread *t;
	struct list_elem list_elem;
};

struct list frame_list;
struct lock frame_lock;
struct bitmap *frame_alloc;

void frame_init (void);
void frame_insert (uint8_t *, void *, struct thread *);
void frame_remove (void *);
uint8_t frame_get (void);
