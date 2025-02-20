//These includes come from list.c 
#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"


//Helper Debuger methods I need to print the list
//inorder
void print_list_inord(struct proc *head){
  if(!head){
    printf("List is currently empty.\n");
    return;
  }

  struct proc *current = head;
  printf("In-Order List: ");

  do{
    printf("%d->",current->tid);
    current = current->next_thread;
  }while(current!=head);
    printf("(head)\n");
}

//Helper Debuger methods I need to print the list
//reverse
void print_list_rev(struct proc *head){
  if(!head){
    printf("List is currently empty.\n");
    return;
  }

  struct proc *current = head->last_thread;
  printf("Reverse-Order List: ");
 
  do{
    printf("%d->",current->tid);
    current = current->last_thread;
  }while(current!=head->last_thread);
    printf("(head)\n");
}





//This is when the main thread is created
void init_list(struct proc *list)
{
  list->next_thread = list;
  list->last_thread = list;
  printf("Initial list with thread %d\n",list->tid);
  print_list_inord(list);
  print_list_rev(list);
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
  printf("Added thread %d to the list\n",new->tid);
  print_list_inord(head);
  print_list_rev(head);
}




void list_del(struct proc *entry)
{ 
  //I guess I am sure if this is necessaey but I think
  //I need to check to see if the proc is null
  if(!entry){
    return;
  }

  //this is when theres only one child in the list
  if(entry->next_thread==entry && entry->last_thread==entry){
    //
    entry->last_thread = entry->next_thread = entry;
    
  }else{
    
    __list_del(entry->last_thread, entry->next_thread);
    entry->last_thread = entry->next_thread = entry;}
    printf("Deleted thread %d from the list\n",entry->tid);

  
}
