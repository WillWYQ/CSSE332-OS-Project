//These includes come from list.c 
#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"

//This is when the main thread is created
void init_list(struct proc *list)
{
  list->next_thread = list;
  list->last_thread = list;
}

//Internally adds a thread 
static inline void __list_add(struct proc *new, struct proc *last_thread,
                              struct proc *next_thread)
{
  next_thread->last_thread = new;
  new->next_thread = next_thread;
  new->last_thread = last_thread;
  last_thread->next_thread = new;
}

static inline void __list_del(struct proc *last_thread, struct proc *next_thread)
{
  next_thread->last_thread = last_thread;
  last_thread->next_thread = next_thread;
}


void list_add_tail(struct proc *head, struct proc *new)
{
  __list_add(new, head->last_thread, head);
}

void list_del(struct proc *entry)
{
  __list_del(entry->last_thread, entry->next_thread);
  entry->last_thread = entry->next_thread = entry;
}
