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
    thread_exit(0);
    // while(1);//ending in this because I do not properly free shared memory yet
}

// Test thread function: writer thread that stores a pointer to its local variable.
void test_thread_stack_share_writer(void *args) {
  int local_val = 12345;  // Local (stack) variable.
  printf("Writer thread: local_val = %d at address %p\n", local_val, &local_val);
  // Store the address in a global pointer.
  pointer_to_a_threads_stack = &local_val;
  // Sleep long enough for the reader thread to access the value.
  sleep(50);
  thread_exit(0);
}

// Test thread function: reader thread that uses the pointer set by the writer.
void test_thread_stack_share_reader(void *args) {
  // Wait a bit to ensure the writer has stored its pointer.
  sleep(20);
  if (pointer_to_a_threads_stack != 0) {
    int read_val = *pointer_to_a_threads_stack;
    printf("Reader thread: read value %d from pointer %p\n", read_val, pointer_to_a_threads_stack);
  } else {
    printf("Reader thread: pointer_to_a_threads_stack is NULL\n");
  }
  thread_exit(0);
}

// Test: create two threads with no arguments.
void test_threads_with_shared_globals(void) {
  printf( "=== Testing threads with globals access @ M3 ===\n" );
  int tid1 = thread_create(0, test_thread_fn4);
  printf( "Created threads %d\n", tid1);
  int tid2 = thread_create(0, test_thread_fn4);
  printf( "Created threads %d\n", tid2);
  // while(1);//this ending would also cause the frees to happen I think and that might break things
  // thread_join(&tid1);
  // thread_join(&tid2);
}

// Test: create two threads to check that a pointer to a stack variable in one thread
// can be used by another thread.
void test_threads_shared_stack_vars(void) {
  printf("=== Testing shared access to stack variables across threads ===\n");
  int tid_writer = thread_create(0, test_thread_stack_share_writer);
  int tid_reader = thread_create(0, test_thread_stack_share_reader);
  printf("Created writer thread %d and reader thread %d\n", tid_writer, tid_reader);
  
  thread_join(&tid_writer);
  thread_join(&tid_reader);
  printf("=== Shared stack variable test completed ===\n");
}


void test_threads_with_args(void){//
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
void test_threads_no_args(void) {//
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
    sleep(30);
    test_thread_no_args_join();
    sleep(30);
    test_threads_shared_stack_vars();

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
      test_threads_shared_stack_vars();
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
