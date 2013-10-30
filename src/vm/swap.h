#include "devices/disk.h"
#include "vm/page.h"
#include "threads/synch.h"

void swap_init (void);
void swap_in ();
void swap_out ();

struct disk *swap_disk;
struct lock swap_lock;
struct bitmap *swap_alloc;
