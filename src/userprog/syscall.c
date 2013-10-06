#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
static void syscall_exit (struct intr_frame *);
static void syscall_halt (struct intr_frame *);
static int syscall_write (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_n;
  memcpy (&syscall_n, f->esp, sizeof (int));
//  hex_dump ((int) f->esp, f->esp, 128, true);
  
  /*  Switch-case for system call number */
  switch (syscall_n){
    case SYS_HALT:
      syscall_halt (f);
    case SYS_EXIT:
      syscall_exit (f);
    case SYS_WRITE:  
      syscall_write (f);
  }
}

static void
syscall_halt (struct intr_frame *f)
{
  power_off ();
}

static void
syscall_exit (struct intr_frame *f)
{
  int status;
  memcpy (&status, f->esp, sizeof (int));
  f->esp += 4;
  printf ("%s: exit(%d)\n", thread_name (), status);
  thread_exit ();
}

static int
syscall_write (struct intr_frame *f)
{
  int fd;
  void *buffer;
  unsigned size;
  memcpy (&fd, f->esp + 20, sizeof (int));
  memcpy (&buffer, f->esp + 24, sizeof (void *));
  memcpy (&fd, f->esp + 28, sizeof (unsigned));
  if (fd == 1){
    putbuf (buffer, size);
    f->eax = size;
  }
}
