#include "threads/thread.h"
#include "threads/synch.h"

struct frame_entry
{
	void *upage;
	void *kpage;
	struct thread *t;
	struct list_elem list_elem;
};

struct list frame_list;
struct lock frame_lock;
struct bitmap *frame_alloc;

void frame_init (void);
void frame_insert (void *, void *, struct thread *);
void frame_remove (void *);
void *frame_get (void);
