#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/page.h"
#include "lib/user/syscall.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);
static void syscall_halt (struct intr_frame *);
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
static void syscall_mmap (struct intr_frame *);
static void syscall_munmap (struct intr_frame *);

struct file_info * find_by_fd(int);
mapid_t mapid = 0;


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
	lock_init (&syscall_lock);
}

bool
validate_fd (int fd)
{
	if (fd < 0 || fd > 127) return false;
	if (fd > thread_current ()->cur_fd) return false;
	if (fd != 0 && fd!= 1 && find_by_fd(fd) == NULL) return false;
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
  	case SYS_MMAP:
			syscall_mmap (f);
			break;
		case SYS_MUNMAP:
			syscall_munmap (f);
			break;
	}
}

static void
syscall_halt(struct intr_frame *f)
{
	power_off();
}

void
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

	lock_acquire (&syscall_lock);
  pid = process_execute (cmd_line);
  if (pid == TID_ERROR)
    f->eax = -1;
  else
    f->eax = pid;
	lock_release (&syscall_lock);
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
	
	if (!validate_address (buffer))
	  syscall_exit (-1);
	
	//lock_acquire (&syscall_lock);
	if (! validate_fd (fd)){
		//lock_release (&syscall_lock);
	  f->eax = -1;
  	return;
	}
	struct thread *t;
	t = thread_current();

	if (fd ==0) {
		f->eax = -1;
	} else if (fd == 1) {
		putbuf(buffer,size);
		f->eax = size;
	} else {
		struct file *target_file;
		target_file = find_by_fd (fd)->f;

		if(!target_file)
			f->eax = -1;
		else 
			f->eax = file_write(target_file, buffer, size);
	}
	//lock_release (&syscall_lock);
}

static void
syscall_create (struct intr_frame *f)
{
	const char *file;
	unsigned initial_size;
	memcpy (&file, f->esp+4, sizeof (char *));
	memcpy (&initial_size, f->esp+8, sizeof (unsigned));
	if (!validate_address (file))
	  syscall_exit (-1);
	
	lock_acquire (&syscall_lock);
  if (file == NULL){
		lock_release (&syscall_lock);
	  syscall_exit (-1);
		f->eax = false;
  }
	else
		f->eax = filesys_create(file, initial_size);
	lock_release (&syscall_lock);
}

static void
syscall_remove (struct intr_frame *f)
{
	const char *file;
	memcpy (&file, f->esp+4, sizeof (char *));

	lock_acquire (&syscall_lock);
	if (!file)
		//exit???
		f->eax = false;
	else 
		f->eax = filesys_remove(file);
	lock_release (&syscall_lock);
}

static void
syscall_open (struct intr_frame *f)
{
	printf("start open\n");
	
	const char *file;
	memcpy (&file, f->esp+4, sizeof (char *));

	lock_acquire (&syscall_lock);
  if (!validate_address (file)){
		lock_release (&syscall_lock);
	  syscall_exit (-1);
	}
	struct thread *t;
	struct file *target_file;
	struct file_info *fip;
	t = thread_current();
	target_file = filesys_open(file);
	
	if (target_file == NULL){
		f->eax = -1;
		lock_release (&syscall_lock);
		return;
	}
	fip = malloc (sizeof (struct file_info));
	fip->fd = t->cur_fd; 
	fip->f = target_file;
	fip->mapped = false;
	list_push_back (&t->fd_table, &fip->elem);
	f->eax = t->cur_fd++;
	lock_release (&syscall_lock);

	printf("end open\n");
}

static void
syscall_filesize (struct intr_frame *f)
{
	int fd;
	memcpy (&fd, f->esp+4, sizeof(int));
	
	struct file *target_file;

	target_file = find_by_fd(fd)->f;
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

  if (!validate_address (buffer) || buffer == NULL)
	  syscall_exit (-1);
	if (!validate_fd (fd)){
	  f->eax = -1;
		return;
  }
	int i;

	if (fd ==0) {
		for (i=0; i<size; i++) {
			*(char *)(buffer + i) = (char)input_getc();
		}
	}
	else if (fd ==1) {
		f->eax = -1;
	}
	else {
		struct file *target_file;
		target_file = find_by_fd(fd)->f;
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

	struct file *target_file;
	target_file = find_by_fd(fd)->f;

	if (target_file)
		file_seek(target_file, position);

}

static void
syscall_tell (struct intr_frame *f)
{
	int fd;
	memcpy(&fd, f->esp+4, sizeof(int));
	struct file *target_file;
	target_file = find_by_fd(fd)->f;
	if (!target_file)
		f->eax = -1;
	else
		f->eax = file_tell(target_file);

}

static void
syscall_close(struct intr_frame *f) {
	int fd;
	memcpy(&fd, f->esp+4, sizeof(int));

	lock_acquire (&syscall_lock);
  if (!validate_fd (fd)){
		lock_release (&syscall_lock);
	  syscall_exit (-1);
	}
	
	struct file *target_file;
	struct file_info *fip;
	fip = find_by_fd(fd);
	target_file = fip->f;
	list_remove (&fip->elem);
	file_close(target_file);
	free(fip);
	lock_release (&syscall_lock);
}

struct file_info*
find_by_fd (int fd)
{
  struct thread *t = thread_current ();
  struct file_info *fip;
  struct list_elem *ittr;

  ittr = list_begin (&t->fd_table);
	while (ittr != list_end (&t->fd_table))
	{
	  fip = list_entry (ittr, struct file_info, elem);
		if (fip->fd == fd)
		  return fip;
	  
		ittr = list_next (ittr);
	}

	return NULL;
}

static void
syscall_mmap (struct intr_frame *f)
{
	printf("hahaha\n");
	struct file_info *fip;

	int fd, fsize, pgsize;
	void *addr;
	memcpy (&fd, f->esp+4, sizeof(int));
	memcpy (&addr, f->esp+8, sizeof(void *));

	lock_acquire (&syscall_lock);
	
	if (!validate_fd (fd)) {
		lock_release (&syscall_lock);
		syscall_exit (-1);
	}
	if (!validate_address (addr)) {
		lock_release (&syscall_lock);
		syscall_exit (-1);
	}
	
	/*check whether fd is 0 or 1 */
	if (fd ==0 || fd ==1) {
		f->eax = -1;
		lock_release (&syscall_lock);
		return;
	}

	/*get filesize from the file as fd */
	fip = find_by_fd (fd);
	struct file *file = file_reopen(fip->f);
	fsize = file_length (file);

	if (fsize == 0)	{
		f->eax = -1;
		lock_release (&syscall_lock);
		return;
	}

	/*check whether addr is page-aligned or not. And also addr !=0 */
	if ((unsigned) addr % PGSIZE != 0 || addr == 0) {
		f->eax = -1;
		lock_release (&syscall_lock);
		return;
	}

	/*get the mmap's PGSIZE and check whether overlapped or not*/
	uint32_t *pd;
	struct thread *curr = thread_current(); 
	void *dst;
	int cnt = 0;
	
	pd = curr->pagedir;
	for (dst = addr; (dst - addr) < fsize; dst += PGSIZE) {
		if (pagedir_get_page (pd,dst) != NULL) {
			file_close(file);
			f->eax = -1;
			lock_release (&syscall_lock);
			return;
		}
		cnt++;
	}

	int ittr = cnt;
	off_t read_offs = 0;
	dst = addr;
	while (ittr>1) {
		spt_lazy (dst, false, file, read_offs, true, curr, mapid);	
		read_offs += PGSIZE;
		dst += PGSIZE;
		ittr--;
	}
	spt_lazy (dst, false, file, (unsigned) fsize - (cnt-1)*PGSIZE, true, curr, mapid);
	
	f->eax = mapid++;
	lock_release (&syscall_lock);
	return;
	
}

static void
syscall_munmap (struct intr_frame *f)
{
return;

}




