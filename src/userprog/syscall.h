#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void syscall_exit (int);
void syscall_munmap (int);

#endif /* userprog/syscall.h */

struct lock syscall_lock;
struct lock mmap_lock;
struct lock rw_lock;
