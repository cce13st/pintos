#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
static void syscall_exit (struct intr_frame *);
static void syscall_exec (struct intr_frame *);
static void syscall_wait (struct intr_frame *);
static void syscall_write (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_n = *(int *)(f->esp);
    
  /*  Switch-case for system call number */
  switch (syscall_n){
    case SYS_HALT:
      syscall_halt (f);
    case SYS_EXIT:
      syscall_exit (f);
    case SYS_EXEC:
      syscall_exec (f);
    case SYS_WAIT:
      syscall_wait (f);
    case SYS_WRITE:  
      syscall_write (f);
  }
}

static void
syscall_exit (struct intr_frame *f)
{
  int status;
  memcpy (&status, f->esp+4, sizeof (int));
  if (status < 0)
    status = -1;

  thread_current ()->ip->exit = true;
  thread_current ()->ip->exit_status = status;
  printf ("%s: exit(%d)\n", thread_name (), status);
  thread_exit ();
}

static void
syscall_exec (struct intr_frame *f)
{
  const char *cmd_line;
  int pid;
  memcpy (&cmd_line, f->esp+4, sizeof (char *));
  f->esp += 4;
  pid = process_execute (cmd_line);
  if (pid == TID_ERROR)
    f->eax = -1;
  else
    f->eax = pid;
}

static void
syscall_wait (struct intr_frame *f)
{
  int pid;
  int status;
  memcpy (&pid, f->esp+4, sizeof (int));
  status = process_wait (pid);
  f->eax = status;
}

static void
syscall_write (struct intr_frame *f)
{
  int fd;;
  void *buffer;
  unsigned size;
  memcpy (&fd, f->esp+4, sizeof (int));
  memcpy (&buffer, f->esp+8, sizeof (void *));
  memcpy (&size, f->esp+12, sizeof (unsigned));
  if (fd == 1)
    putbuf (buffer, size);
  f->eax = size;
}
