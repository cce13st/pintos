#include "devices/disk.h"
#include "vm/page.h"
#include "threads/synch.h"

void swap_init (void);
void swap_out (struct frame_entry *);
void swap_in (struct spt_entry *, void *);
void swap_clear (unsigned);

struct bitmap *swap_alloc;
