#include "threads/thread.h"
#include "threads/synch.h"

void frame_init (void);
void frame_insert (uint8_t *, uint8_t *, struct thread *);
void frame_remove (uint8_t *);
uint8_t frame_get (void);

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
