#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern void init_list(struct proc *list);
extern void list_add_tail(struct proc *head, struct proc *new);
extern void list_del(struct proc *entry);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
    initlock(&p->lock, "proc");
    p->state = UNUSED;
    p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid()
{
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

  found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
    (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
  return 0;
}

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
if(mappages(pagetable, TRAPFRAME, PGSIZE,
  (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
uvmfree(pagetable, 0);
return 0;
}

return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++){
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  }
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;//this is the return for the current proc
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  // TODO: Modify exit() in kernel/proc.c to terminate all threads when the main process exits.

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if(pp->state == ZOMBIE){
          // Found one.
          pid = pp->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
            sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
          release(&wait_lock);
          return -1;
        }
        freeproc(pp);
        release(&pp->lock);
        release(&wait_lock);
        return pid;
      }
      release(&pp->lock);
    }
  }

    // No point waiting if we don't have any children.
  if(!havekids || killed(p)){
    release(&wait_lock);
    return -1;
  }

    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void
setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int
killed(struct proc *p)
{
  int k;
  
  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
    [UNUSED]    "unused",
    [USED]      "used",
    [SLEEPING]  "sleep ",
    [RUNNABLE]  "runble",
    [RUNNING]   "run   ",
    [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

// added by Yueqiao Wang on Feb 3, 2025 based on instruction from https://www.rose-hulman.edu/class/csse/csse332/2425b/labs/milestone1/
uint64 
spoon(void *arg)
{
  // Add your code here...
  printf("In spoon system call with argument %p\n", arg);
  return -1;
}


// void *thread_fn(void* args, void (*start_routine)(void*), int *tid){
//   start_routine(args);
//   thread_exit(tid);
//   return 0;//doesn't matter bc thread_exit theoretically kills this process
// }


//I decided to make a helper function to set the parent right so that 
//when a thead is created the parent is set correctly

//should essentially take a thread and go up the links until it 
//finds the parent then returns the actual paren
struct proc* find_parent_thread(struct proc *p){
  while(p->is_thread){
    p = p->parent;
  }
  return p;
}


uint64 thread_create(void *args, void (*start_routine)(void*)) {

  //want for tid to be unique
  int i, tpid;
  //thread_process
  struct proc *tp;
  struct proc *p = myproc();

  // Allocate process.
  if((tp = allocproc()) == 0){
    return -1;
  }

  // map user memory from "parent" to child.
  if(uvmshareallthreadpages(p->pagetable, tp->pagetable, p->sz) < 0){//used to be uvmcopy
    freeproc(tp);
    release(&tp->lock);
    return -1;
  }
  tp->sz = p->sz;

  //this has the kernel create a stack page and add it to the pagetable and updates sz
  uint64 stack_pointer = uvmthreadstackmap(tp);


  // copy saved user registers.
  *(tp->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  // printf("original epc: %p\n",(int*) tp->trapframe->epc);
  tp->trapframe->epc = (uint64) start_routine;//thread_fn;
  tp->trapframe->sp = (uint64) stack_pointer;
  tp->trapframe->a0 = (uint64) args;//if the code I enter into isn't expecting a return value it won't use this as a return value it will use it as the normal arg register
  // printf("original ra: %p\n",(int*) tp->trapframe->ra);
  // tp->trapframe->a1 = (uint64) start_routine;
  // tp->trapframe->a2 = (uint64) tid;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++){
    if(p->ofile[i])
      tp->ofile[i] = filedup(p->ofile[i]);
  }
  tp->cwd = idup(p->cwd);

  safestrcpy(tp->name, p->name, sizeof(p->name));

  tpid = tp->pid;
  tp->tid = tp->pid;

  //TODO: make sure the parent is the real main thread, not the one who created it
  //if the thing that created me has isthread then I want it's parent 
  //because if isthread is true then that means its not the main thread
  //because it has a main thread above it
  
  //Now this should make sure that the new child will not 
  //be the parent but the actual parent that gave birth to
  //the child will be the main thread
  tp->parent = find_parent_thread(p);

  //This should be fine but need to figure out why is this not working
  //
  //Sets the isthread flag
  tp->is_thread = 1;

  // Update parent's thread list.
  acquire(&p->lock);
  if(p->any_child == 0){
    // No child exists yet.
    p->any_child = tp;
    // tp->next_thread = tp; // Circular list: next points to self.
    // tp->last_thread = tp; // Circular list: last points to self.
    
    //this initializes the new thread's list
    init_list(tp);
  } else {
    // // Parent already has at least one child.
    // // p->any_child points to the first child.
    //Adds a new thread to the tail of the list.
    list_add_tail(p->any_child,tp);
  }

  //needs to be after we setup the linked list
  uvmsharethreadpage(tp, stack_pointer); //should be good to just use once list implementation is good
  release(&p->lock);

  tp->state = RUNNABLE;
  
  release(&tp->lock);//once tp releases its lock as runnable it is free to runs

  return tpid;

}

uint64 thread_join(int * join_tid) {  
// if tid is null, wait for any one, if not, wait for that one

  struct proc *ct;//child thread
  int pid, havechild;//shouldn't need to worry about have kids bc threads shouldn't have kids
  struct proc *mt = myproc();//main thread
  int tid;// I need this for a thing to copy into

  //all page tables should be equivalent and the join tid should either be in globals or the main threads stack so this should work
  copyin(mt->pagetable, (char*) &tid, (uint64) join_tid, (uint64) sizeof(int));//get tid to match from user space

  printf("thread join called with %d as tid inside function\n", tid);

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    printf("thread join of mainthread: %d is checking for if child with tid:%d exited (inloop)\n", mt->pid, tid);
    havechild = 0;

    for(ct = proc; ct < &proc[NPROC]; ct++){
      if(ct->parent == mt){
        havechild = 1;
      }

      if((!tid || tid == 0 || ct->tid == tid) && ct->parent == mt && ct->is_thread) {//this if statement can probably be replaced wit  == ct->pid(itstid)//used to be ct->parent == mt
        // make sure the child isn't still in exit() or swtch().
        acquire(&ct->lock);

        if(ct->state == ZOMBIE){
          // Found one.
          pid = ct->pid;
          release(&ct->lock);
          release(&wait_lock);
          printf("reaches end of successful thread_join with %d as the tid\n", pid);
          return pid;
        }
        release(&ct->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havechild || killed(mt)){
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(mt, &wait_lock);  //DOC: wait-sleep
  }
  return 0;
}

// TODO are we have thread id or just kill the current runing thread
// implemented by Yueqiao Wang on Feb 9 
uint64 thread_exit(int *tid) {

  //----------------------------------------------------Yueqiao's Code
  struct proc *t = myproc();

  if (!t->is_thread)
  {
    panic("this is not a thread");
  }

  struct proc *p = t->parent;

  begin_op();
  iput(t->cwd);
  end_op();
  t->cwd = 0;
  
  acquire(&t->lock);
  acquire(&wait_lock);
  
  //this should handle the function to handle the removal of threads
  //while also updating the next and last thread pointers
  list_del(t);

  //   // Handle thread list updates
  // if (t->last_thread != t) {
  //   t->last_thread->next_thread = t->next_thread;
  //   t->next_thread->last_thread = t->last_thread;
  //   //see if I am just updating the list by adding a
  //   //tail to this but not completely sure
  // } else {
  //       // If this is the only thread, update parent process accordingly
  //   acquire(&p->lock);
  //   if (p->any_child == t) {
  //     p->any_child = (t->next_thread != t) ? t->next_thread : 0;
  //   }
  //   release(&p->lock);
  // }
    // Handle thread list updates
  if (t->next_thread == t) {
      // If this is the only thread, update parent process accordingly
      acquire(&p->lock);
      if (p->any_child == t) {
        p->any_child = 0;
      }
      release(&p->lock);
  }

  wakeup(p);  // Wake up any thread waiting in thread_join()


  t->xstate = t->state;
  t->state = ZOMBIE;

  release(&wait_lock);

  printf("SYS: Thread %d exited, waiting for sche()\n",t->tid);
  
  sched();

  panic("zombie thread exit");
  return -1;
}
