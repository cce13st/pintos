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
static void syscall_create (struct intr_frame *);
static void syscall_remove (struct intr_frame *);
static void syscall_open (struct intr_frame *);
static void syscall_read (struct intr_frame *);
static void syscall_write (struct intr_frame *);
static void syscall_seek (struct intr_frame *);
static void syscall_tell (struct intr_frame *);
static void syscall_close (struct intr_frame *);
static void syscall_filesize (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

bool
validate_fd (int fd)
{
  if (fd > thread_current ()->cur_fd)
	  return false;
	return true;
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
		case SYS_CREATE:
			syscall_create (f);
			break;
		case SYS_REMOVE:
			syscall_remove (f);
			break;
		case SYS_OPEN:
			syscall_open (f);
			break;
		case SYS_FILESIZE:
			syscall_filesize (f);
			break;
		case SYS_READ:
			syscall_read (f);
			break;
		case SYS_SEEK:
			syscall_seek (f);
			break;
		case SYS_TELL:
			syscall_tell (f);
			break;
		case SYS_CLOSE:
			syscall_close (f);
			break;
  }
}

static void
syscall_halt(struct intr_frame *f)
{
	power_off();
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

	struct thread *t;
	t = thread_current();

	if (fd ==0) {
		f->eax = -1;
	} else if (fd == 1) {
		putbuf(buffer,size);
		f->eax = size;
	} else {
		
		struct file *target_file;
		target_file = t->fd_table[fd];

		if(!target_file)
			f->eax = -1;
		else 
			f->eax = file_write(target_file, buffer, size);
	}
}

static void
syscall_create (struct intr_frame *f)
{
	const char *file;
	unsigned initial_size;
	memcpy (&file, f->esp+4, sizeof (char *));
	memcpy (&initial_size, f->esp+8, sizeof (unsigned));
	
  if (file == NULL){
    printf ("%s: exit(%d)\n", thread_name (), -1);
    thread_exit ();
		f->eax = false;
  } else
		f->eax = filesys_create(file, initial_size);
}

static void
syscall_remove (struct intr_frame *f)
{
	const char *file;
	memcpy (&file, f->esp+4, sizeof (char *));

	if (!file)
		//exit???
		f->eax = false;
	else 
		f->eax = filesys_remove(file);
}

static void
syscall_open (struct intr_frame *f)
{
	const char *file;
	memcpy (&file, f->esp+4, sizeof (char *));

	struct thread *t;
	struct file *target_file;
	t = thread_current();
	target_file = filesys_open(file);

	if (target_file == NULL){
		f->eax = -1;
		return;
	}
	
	f->eax = t->cur_fd;
	t->fd_table[t->cur_fd++] = target_file;
}

static void
syscall_filesize (struct intr_frame *f)
{
	int fd;
	memcpy (&fd, f->esp+4, sizeof(int));
	
	struct thread *t;
	struct file *target_file;

	t = thread_current();
	target_file = t->fd_table[fd];
	if (!target_file)
		f->eax = -1;
	else
		f->eax = file_length(target_file);
}

static void
syscall_read (struct intr_frame *f) {
	int fd;
	void *buffer;
	unsigned size;
	memcpy (&fd, f->esp+4 , sizeof(int));
	memcpy (&buffer, f->esp+8, sizeof(void *));
	memcpy (&size, f->esp+12, sizeof(unsigned));

	struct thread *t;
	int i;
	t = thread_current();

	if (fd ==0) {
		for (i=0; i<size; i++) {
			*(char *)(buffer + i) = (char)input_getc();
		}
	} else if (fd ==1) {
		f->eax = -1;
	} else {
		struct file *target_file;
		target_file = t->fd_table[fd];
		if (!target_file)
			f->eax = -1;
		else
			f->eax = file_read(target_file, buffer, size);
	}

}

static void
syscall_seek (struct intr_frame *f)
{
	int fd;
	unsigned position;
	memcpy(&fd, f->esp+4, sizeof(int));
	memcpy(&position, f->esp+8, sizeof(unsigned));

	struct thread *t;
	struct file *target_file;
	t = thread_current();
	target_file = t->fd_table[fd];

	if (target_file)
		file_seek(target_file, position);

}

static void
syscall_tell (struct intr_frame *f)
{
	int fd;
	memcpy(&fd, f->esp+4, sizeof(int));
	struct thread *t;
	struct file *target_file;
	t = thread_current();
	target_file = t->fd_table[fd];
	if (!target_file)
		f->eax = -1;
	else
		f->eax = file_tell(target_file);

}

static void
syscall_close(struct intr_frame *f) {
	int fd;
	memcpy(&fd, f->esp+4, sizeof(int));

	struct thread *t;
	struct file *target_file;
	t = thread_current();
	target_file = t->fd_table[fd];
	file_close(target_file);
	t->fd_table[fd] = NULL;
	// free fd_table
}
