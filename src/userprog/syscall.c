#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
static void syscall_halt (struct intr_frame *);
static void syscall_exit (int);
static void syscall_exec (struct intr_frame *);
static void syscall_wait (struct intr_frame *);
static void syscall_write (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

bool
validate_address(void *pointer)
{
  if (is_user_vaddr (pointer) && pagedir_get_page (thread_current ()->pagedir, pointer) != NULL)
    return true;
  return false;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if (!validate_address (f->esp))
    syscall_exit (-1);

  int syscall_n = *(int *)(f->esp);
    
  /*  Switch-case for system call number */
  switch (syscall_n){
    case SYS_HALT:
      syscall_halt (f);
      break;
    case SYS_EXIT:
      syscall_exit (*(int *)(f->esp+4));
      break;
    case SYS_EXEC:
      syscall_exec (f);
      break;
    case SYS_WAIT:
      syscall_wait (f);
      break;
    case SYS_WRITE:  
      syscall_write (f);
      break;
  }
}

static void
syscall_halt (struct intr_frame *f)
{
  power_off ();
}

static void
syscall_exit (int status)
{
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
  if (!validate_address (cmd_line))
    syscall_exit (-1);

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
