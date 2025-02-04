#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// added by Yueqiao Wang on Feb 3, 2025 based on instruction from https://www.rose-hulman.edu/class/csse/csse332/2425b/labs/milestone1/
uint64
sys_spoon(void)
{
  // obtain the argument from the stack, we need some special handling
  uint64 addr;
  argaddr(0, &addr);
  return spoon((void*)addr);
}

uint64
sys_thread_create(void) {
    uint64 args, start_func, tid;
    argaddr(0, &args);
    argaddr(1, &start_func);
    argaddr(2, &tid);
    
    printf("sys_thread_create(%p, %p, %p) - Not implemented yet!\n", (void*)args, (void*)start_func, (void*)tid);
    return thread_create((void*)args,(void*)start_func,(void*)tid);
}

uint64
sys_thread_join(void) {
    uint64 tid;
    argaddr(0, &tid);

    printf("sys_thread_join(%p) - Not implemented yet!\n", (void*)tid);
    return thread_join((void*)tid);
}

uint64
sys_thread_exit(void) {
    uint64 tid;
    argaddr(0, &tid);
    
    printf("sys_thread_exit(%p) - Not implemented yet!\n", (void*)tid);
    return thread_exit((void*)tid);
}

