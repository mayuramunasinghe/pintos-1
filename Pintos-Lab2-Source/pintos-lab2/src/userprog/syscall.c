#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

struct lock filesys_lock;

struct file_descriptor
  {
    int number;
    struct file *file;
    struct list_elem elem;
  };

static void syscall_handler (struct intr_frame *);

static void
bad_user (void)
{
  thread_current ()->exit_status = -1;
  thread_exit ();
}

/* One inspection per intersected page, with overflow-safe bounds.
   No eviction or shared user threads exist in Project 2, so a validated
   mapping cannot disappear while this syscall is executing. */
static void
validate_buffer (const void *buffer, unsigned size, bool writable)
{
  uintptr_t first = (uintptr_t) buffer;
  uintptr_t last, page;
  uint32_t *pd = thread_current ()->pagedir;
  if (size == 0)
    return;
  if (first == 0 || first >= (uintptr_t) PHYS_BASE
      || size > (uintptr_t) PHYS_BASE - first)
    bad_user ();
  last = first + size - 1;
  for (page = (uintptr_t) pg_round_down (buffer); page <= last; page += PGSIZE)
    if (writable ? !pagedir_is_writable (pd, (void *) page)
                 : pagedir_get_page (pd, (void *) page) == NULL)
      bad_user ();
}

/* First locate the terminator, then allocate: bad pointers leak nothing.
   Each page is checked once. Commands/paths are limited to one page. */
static char *
copy_string (const char *user)
{
  size_t n;
  uintptr_t address = (uintptr_t) user;
  for (n = 0; n < PGSIZE; n++)
    {
      uintptr_t at = address + n;
      if (at < address || at >= (uintptr_t) PHYS_BASE || at == 0)
        bad_user ();
      if (n == 0 || pg_ofs ((void *) at) == 0)
        validate_buffer ((void *) at, 1, false);
      if (*(const char *) at == 0)
        {
          char *copy = malloc (n + 1);
          if (copy != NULL) memcpy (copy, user, n + 1);
          return copy;
        }
    }
  return NULL;
}

static struct file_descriptor *
find_fd (int number)
{
  struct list *files = &thread_current ()->open_files;
  struct list_elem *e;
  for (e = list_begin (files); e != list_end (files); e = list_next (e))
    {
      struct file_descriptor *fd = list_entry (e, struct file_descriptor, elem);
      if (fd->number == number) return fd;
    }
  return NULL;
}

static void
close_fd (struct file_descriptor *fd)
{
  if (fd == NULL) return;
  list_remove (&fd->elem);
  lock_acquire (&filesys_lock);
  file_close (fd->file);
  lock_release (&filesys_lock);
  free (fd);
}

void
syscall_close_all (void)
{
  struct list *files = &thread_current ()->open_files;
  while (!list_empty (files))
    close_fd (list_entry (list_front (files), struct file_descriptor, elem));
}

void
syscall_init (void)
{
  lock_init (&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  uint32_t call, args[3] = {0, 0, 0};
  unsigned count;
  struct file_descriptor *fd;
  struct thread *cur = thread_current ();
  validate_buffer (f->esp, sizeof call, false);
  memcpy (&call, f->esp, sizeof call);
  switch (call)
    {
    case SYS_HALT: count = 0; break;
    case SYS_EXIT: case SYS_EXEC: case SYS_WAIT: case SYS_REMOVE:
    case SYS_OPEN: case SYS_FILESIZE: case SYS_TELL: case SYS_CLOSE:
      count = 1; break;
    case SYS_CREATE: case SYS_SEEK: count = 2; break;
    case SYS_READ: case SYS_WRITE: count = 3; break;
    default: bad_user (); return;
    }
  if (count != 0)
    {
      const void *start = (const uint8_t *) f->esp + sizeof call;
      validate_buffer (start, count * sizeof *args, false);
      memcpy (args, start, count * sizeof *args);
    }
  f->eax = -1;
  switch (call)
    {
    case SYS_HALT:
      shutdown_power_off ();
      break;
    case SYS_EXIT:
      cur->exit_status = (int) args[0];
      thread_exit ();
      break;
    case SYS_WAIT:
      f->eax = process_wait ((tid_t) args[0]);
      break;
    case SYS_EXEC:
      {
        char *command = copy_string ((const char *) args[0]);
        if (command != NULL)
          {
            f->eax = process_execute (command);
            free (command);
          }
        break;
      }
    case SYS_CREATE: case SYS_REMOVE: case SYS_OPEN:
      {
        char *name = copy_string ((const char *) args[0]);
        if (call != SYS_OPEN) f->eax = false;
        if (name == NULL) break;
        if (call == SYS_OPEN)
          {
            fd = malloc (sizeof *fd);
            if (fd != NULL && cur->next_fd < 0x7fffffff)
              {
                lock_acquire (&filesys_lock);
                fd->file = filesys_open (name);
                lock_release (&filesys_lock);
                if (fd->file != NULL)
                  {
                    fd->number = cur->next_fd++;
                    list_push_back (&cur->open_files, &fd->elem);
                    f->eax = fd->number;
                  }
                else free (fd);
              }
            else free (fd);
          }
        else
          {
            lock_acquire (&filesys_lock);
            f->eax = call == SYS_CREATE ? filesys_create (name, args[1])
                                        : filesys_remove (name);
            lock_release (&filesys_lock);
          }
        free (name);
        break;
      }
    case SYS_READ: case SYS_WRITE:
      {
        void *buffer = (void *) args[1];
        unsigned size = args[2];
        validate_buffer (buffer, size, call == SYS_READ);
        fd = find_fd ((int) args[0]);
        if (call == SYS_READ && args[0] == 0)
          {
            unsigned i;
            for (i = 0; i < size; i++) ((uint8_t *) buffer)[i] = input_getc ();
            f->eax = size;
          }
        else if (call == SYS_WRITE && args[0] == 1)
          {
            unsigned offset = 0;
            while (offset < size)
              {
                unsigned chunk = size - offset;
                if (chunk > 512) chunk = 512;
                putbuf ((const char *) buffer + offset, chunk);
                offset += chunk;
              }
            f->eax = size;
          }
        else if (fd != NULL)
          {
            lock_acquire (&filesys_lock);
            f->eax = call == SYS_READ ? file_read (fd->file, buffer, size)
                                      : file_write (fd->file, buffer, size);
            lock_release (&filesys_lock);
          }
        break;
      }
    case SYS_CLOSE:
      close_fd (find_fd ((int) args[0]));
      break;
    case SYS_FILESIZE: case SYS_SEEK: case SYS_TELL:
      fd = find_fd ((int) args[0]);
      if (fd != NULL)
        {
          lock_acquire (&filesys_lock);
          if (call == SYS_FILESIZE) f->eax = file_length (fd->file);
          else if (call == SYS_TELL) f->eax = file_tell (fd->file);
          else file_seek (fd->file, args[1]);
          lock_release (&filesys_lock);
        }
      break;
    }
}
