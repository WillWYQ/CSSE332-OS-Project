#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define NUM_THREADS 3

// ANSI color escape codes:
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"


// Test thread functions with no arguments.
void test_thread_fn1(void *args) {
  for (int i = 0; i < LOOP_COUNT; i++) {
    printf(1, GREEN "fn1: iteration %d\n" RESET, i);
  }
  thread_exit();
}

// Test thread functions with no arguments.
void test_thread_fn2(void *args) {
  for (int i = 0; i < LOOP_COUNT; i++) {
    printf(1, YELLOW "fn2: iteration %d\n" RESET, i);
  }
  thread_exit();
}

// Test thread function that accepts an argument.
void test_thread_fn3(void *args) {
  int tid = *(int *)args;
  for (int i = 0; i < LOOP_COUNT; i++) {
    printf(1, CYAN "fn3 (tid %d): iteration %d\n" RESET, tid, i);
  }
  thread_exit();
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
  // int* threads[NUM_THREADS];
  int tids[NUM_THREADS];
  void * stacks[NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++){
    tids[i] = i;
    stacks[i] = malloc(PGSIZE);
    thread_create(&tids[i], test_thread_create_fn3, 0, stacks[i]);//with Args
  }
}

// Test: create two threads with no arguments.
void test_threads_no_args(void) {
  printf(1, MAGENTA "=== Testing threads with no args ===\n" RESET);
  void *stack1 = malloc(PGSIZE);
  void *stack2 = malloc(PGSIZE);
  int tid1 = thread_create(0, test_thread_fn1, 0, stack1);
  int tid2 = thread_create(0, test_thread_fn2, 0, stack2);
  printf(1, "Created threads %d and %d\n", tid1, tid2);
  // thread_join();
  // thread_join();
}

void test_spoon_m1(void){
  printf("===============Spoon & M1==============\n");
  uint64 p = 0xdeadbeef;
  spoon((void*)p);
  // thread_create(0, 0, 0);
  // thread_join(0);
  //thread_exit(0);
}


int main(int argc, char *argv[]) {
  if (argc < 2) {
    test_threads_no_args();
    test_threads_with_args();
  } else {
    if (strcmp(argv[1], "noargs") == 0) {
      test_threads_no_args();
    } else if (strcmp(argv[1], "withargs") == 0) {
      test_threads_with_args();
    } else {
      printf(1, RED "Unknown test: %s\n" RESET, argv[1]);
    }
  }
  exit(0);
}
