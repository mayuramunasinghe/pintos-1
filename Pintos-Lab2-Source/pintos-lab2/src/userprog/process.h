#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

/* Two references: parent ownership and child ownership. */
struct child_status
  {
    tid_t tid;
    int exit_status;
    bool load_ok;
    int references;
    struct semaphore loaded;
    struct semaphore exited;
    struct list_elem elem;
  };

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
