// xv6test.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_THREADS 3
#define LOOP_COUNT 5

int glob_var = 0;

int * pointer_to_a_threads_stack;


int test_threads_no_args_tid1;
int test_threads_no_args_tid2;

int test_thread_no_args_join_tid1;
int test_thread_no_args_join_tid2;

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
  // int tid = *(int *)args;
  for (int i = 0; i < LOOP_COUNT; i++) {
    printf( "fn3 (tid %d) (A: iteration %d\n" , *(int *)args, i);
  }
  thread_exit(0);
}

//test thread function that uses shared memory
void test_thread_fn4(void* args){
    glob_var++;
    printf("global variable: %d\n", glob_var);
    // thread_exit(0);
    while(1);//ending in this because I do not properly free shared memory yet
}

//test thread function that uses shared memory
void test_thread_fn5(void* args){
  int some_num = 943875243;
  printf("stack variable: %d at addr: %p\n", some_num, &some_num);
  pointer_to_a_threads_stack = &some_num;
  // thread_exit(0);
  while(1);//ending in this because I do not properly free shared memory yet
}

//test thread function that uses shared memory
void test_thread_fn6(void* args){
  sleep(10);//make sure the other thread has time to initialize their variable
  printf("stack variable from other thread: %d at addr: %p\n", *pointer_to_a_threads_stack, pointer_to_a_threads_stack);
  // thread_exit(0);
  while(1);//ending in this because I do not properly free shared memory yet
}

// Test: create two threads with no arguments.
void test_threads_with_shared_globals(void) {
  printf( "=== Testing threads with globals access @ M3 ===\n" );
  int tid1 = thread_create(0, test_thread_fn4);
  printf( "Created threads %d\n", tid1);
  sleep(10);//hopefully first one finishes in this amount of time
  int tid2 = thread_create(0, test_thread_fn4);
  printf( "Created threads %d\n", tid2);
  // while(1);//this ending would also cause the frees to happen I think and that might break things
  // thread_join(&tid1);
  // thread_join(&tid2);
}

// Test: create two threads with no arguments that have access to eachothers stacks
void test_threads_with_shared_stacks(void) {
  printf( "=== Testing threads with shared stacks access @ M3 ===\n" );
  int tid1 = thread_create(0, test_thread_fn5);
  printf( "Created threads %d\n", tid1);
  int tid2 = thread_create(0, test_thread_fn6);
  printf( "Created threads %d\n", tid2);
  while(1);//this ending would also cause the frees to happen I think and that might break things
  // thread_join(&tid1);
  // thread_join(&tid2);
}


void test_threads_with_args(void){
  //TODO: talk with team about how the final tid arg works
  printf("=== Testing threads with args @ M2 ===\n");
  
  int tids[NUM_THREADS];
  // void * stacks[NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++){
    tids[i] = thread_create(&tids[i], test_thread_fn3);
    printf("Created thread %d\n", tids[i], i);
  }
  // Optionally, join threads here:
  for(int i = 0;i < NUM_THREADS; i++){
    thread_join(&tids[i]);
  }
}


// Test: create two threads with no arguments.
void test_threads_no_args(void) {
  printf( "=== Testing threads with no args @ M2 ===\n" );
  // void *stack1 = malloc(PGSIZE);
  // void *stack2 = malloc(PGSIZE);
  test_threads_no_args_tid1 = thread_create(0, test_thread_fn1);//stack1
  test_threads_no_args_tid2 = thread_create(0, test_thread_fn2);//stack2
  printf( "Created threads %d and %d\n", test_threads_no_args_tid1, test_threads_no_args_tid2);
  thread_join(&test_threads_no_args_tid1);
  sleep(3);
  thread_join(&test_threads_no_args_tid2);
  printf( "Threads %d and %d Exited\n", test_threads_no_args_tid1, test_threads_no_args_tid2);
}

// Test: create one thread with no arguments for thread_join developement.
void test_thread_no_args_join(void) {
  printf( "=== Testing threads with no args and join @ M3 ===\n" );
  test_thread_no_args_join_tid1 = thread_create(0, test_thread_fn1);//stack1
  printf( "Created threads %d\n", test_thread_no_args_join_tid1);
  sleep(20);//this will make sure that it finishes after the mainthread has slept due to the thread_join
  thread_join(&test_thread_no_args_join_tid1);
  printf( "Threads %d Exited\n", test_thread_no_args_join_tid1);

  test_thread_no_args_join_tid2 = thread_create(0, test_thread_fn1);//stack1
  printf( "Created threads %d\n", test_thread_no_args_join_tid2);
  // sleep(20);//this will make sure that it finishes after the mainthread has slept due to the thread_join
  thread_join(&test_thread_no_args_join_tid2);
  printf( "Threads %d Exited\n", test_thread_no_args_join_tid2);
  return;
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
    printf( "test_threads_no_args Test Finished\n");
    sleep(30);
    test_threads_with_args();
    printf( "test_threads_with_args Test Finished\n");
    sleep(30);
    test_threads_with_shared_globals();
    printf( "test_threads_with_shared_globals Test Finished\n");
  } else {
    if (strcmp(argv[1], "noargs") == 0) {
      test_threads_no_args();
    } else if (strcmp(argv[1], "withargs") == 0) {
      test_threads_with_args();
    } else if(strcmp(argv[1], "withglobs") == 0){
      test_threads_with_shared_globals();
    } else if(strcmp(argv[1], "jointest") == 0){
      test_thread_no_args_join();
    } else if(strcmp(argv[1], "withstacks") == 0){
      test_threads_with_shared_stacks();
    } else {
      printf( "Unknown test: %s\n" , argv[1]);
    }
  }
  printf("All Finished\n");
  // while(1);
  sleep(10);
  exit(0);
  // return 0;
}
