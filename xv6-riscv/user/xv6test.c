// xv6test.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
// #include "kernel/riscv.h"

#define NUM_THREADS 3
#define LOOP_COUNT 5

// Test thread functions with no arguments.
void test_thread_fn1(void *args) {
  for (int i = 0; i < LOOP_COUNT; i++) {
    printf("fn1 (NO-A: iteration %d\n" , i);
  }
  thread_exit(0);
}

// Test thread functions with no arguments.
void test_thread_fn2(void *args) {
  for (int i = 0; i < LOOP_COUNT; i++) {
    printf( "fn2 (NO-A: iteration %d\n" , i);
  }
  thread_exit(0);
}

// Test thread function that accepts an argument.
void test_thread_fn3(void *args) {
  int tid = *(int *)args;
  for (int i = 0; i < LOOP_COUNT; i++) {
    printf( "fn3 (tid %d) (A: iteration %d\n" , tid, i);
  }
  thread_exit(0);
}

void test_threads_with_args(void){
  //TODO: talk with team about how the final tid arg works
  printf("=== Testing threads with args @ M2 ===\n");
  
  int tids[NUM_THREADS];
  // void * stacks[NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++){
    tids[i] = i;
    // stacks[i] = malloc(PGSIZE);
    int tid = thread_create(&tids[i], test_thread_fn3);
    printf("Created thread %d (expected id: %d)\n", tid, i);
  }
  // Optionally, join threads here:
  // for(int i = 0;i < NUM_THREADS; i++){
  //   thread_join();
  // }
}


// Test: create two threads with no arguments.
void test_threads_no_args(void) {
  printf( "=== Testing threads with no args @ M2 ===\n" );
  // void *stack1 = malloc(PGSIZE);
  // void *stack2 = malloc(PGSIZE);
  int tid1 = thread_create(0, test_thread_fn1);//stack1
  int tid2 = thread_create(0, test_thread_fn2);//stack2
  printf( "Created threads %d and %d\n", tid1, tid2);
  thread_join(&tid1);
  thread_join(&tid2);
  printf( "Threads %d and %d Exited\n", tid1, tid2);
}

void test_spoon(void){
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
    sleep(30);
    test_threads_with_args();
  } else {
    if (strcmp(argv[1], "noargs") == 0) {
      test_threads_no_args();
    } else if (strcmp(argv[1], "withargs") == 0) {
      test_threads_with_args();
    } else {
      printf( "Unknown test: %s\n" , argv[1]);
    }
  }
  exit(0);
}
