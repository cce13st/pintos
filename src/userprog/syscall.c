#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
static void syscall_exit (int);
static void syscall_halt (void);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_n;
  printf ("syscall\n");
/*  memcpy = (&syscall_n, f->esp, sizeof (int));
  f->esp += sizeof (int);

  printf ("system call! - %d\n", syscall_n);

   Switch-case for system call number 
  switch (syscall_n){
    case SYS_EXIT:
      syscall_halt ();
    case SYS_EXIT:
      int status;
      memcpy = (&status, f->esp, sizeof (int));
      syscall_exit (status);
    default :
      printf ("Syscall argument error\n");
  }*/

  thread_exit ();
}

static void
syscall_halt (void)
{
  shutdown_power_off ();
}

static void
syscall_exit (int status)
{
  printf ("%s: exit(%d)\n", thread_name (), status);
  thread_exit ();
}
