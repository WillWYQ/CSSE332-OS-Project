#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define NUM_THREADS 3
//TODO: create thread methods (one that accepts args two that just print whatever)
void test_thread_create_fn1(void * args){
  printf("This thread is running test_thread_create_fn1 at %p\n", test_thread_create_fn1);
  while(1);
  return;
}

void test_thread_create_fn2(void * args){
  printf("This thread is running test_thread_create_fn2 at %p\n", test_thread_create_fn2);
  while(1);
  return;
}

void test_thread_create_fn3(void * args){
  int tid = *(int *) args;
  printf("This is thread %d running test_thread_create_fn3 at %p\n", tid, test_thread_create_fn3);
  while(1);
  return;
}


void test_thread_start_at_pc_m2(void){
  //TODO: talk with team about how the final tid arg works
  //TODO: add join lower in this
  printf("===============Threads Starting at Arbitrary Function No Args==============\n");
  void * stack1 = malloc(PGSIZE);
  void * stack2 = malloc(PGSIZE);
  thread_create(0, test_thread_create_fn1, 0, stack1);//no Args
  thread_create(0, test_thread_create_fn2, 0, stack2);//no Args
}

void test_thread_with_args_m2(void){
  //TODO: talk with team about how the final tid arg works
  //TODO: add join lower in this
  printf("===============Threads With Args==============\n");
  int tids[NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++){
    tids[i] = i;
    thread_create(&tids[i], test_thread_create_fn3, 0);//with Args
  }
}

void test_spoon_m1(void){
  printf("===============Spoon & M1==============\n");
  uint64 p = 0xdeadbeef;
  spoon((void*)p);
  thread_create(0, 0, 0);
  thread_join(0);
  //thread_exit(0);
}

int main(int argc, char *argv[]) {
  //TODO: Write code to select what tests to run
  test_thread_start_at_pc_m2();

  test_thread_with_args_m2();

  test_spoon_m1();

  exit(0);
}
