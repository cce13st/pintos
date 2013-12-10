#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void syscall_exit (int);
void syscall_munmap (int);

int path_cut (char *, char *);
int path_parse (char *, int, char *);
//bool path_abs (char *);

#endif /* userprog/syscall.h */

struct lock syscall_lock;
struct lock mmap_lock;
