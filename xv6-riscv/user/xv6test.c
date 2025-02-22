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

// Test thread function: Infinite loop (child threads that run indefinitely).
void test_infinite_loop(void *args) {
  while(1) {
    // Sleep to avoid busy-waiting.
    sleep(10);
  }
  // This should never be reached.
  thread_exit(0);
}

// Test thread function exited by child
void test_child_exit_all() {
  thread_all_exit(2);
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
    
    printf("Reader thread: read value %d from pointer %p\n", read_val, pointer_to_a_threads_stack);
  } else {
    printf("Reader thread: pointer_to_a_threads_stack is NULL\n");
  }
  thread_exit(0);
}

// Test thread function: Child-of-child thread.
void child_of_child_fn(void *args) {
  printf("Child-of-child thread: Hello from the child-of-child!\n");
  thread_exit(0);
}

// Test thread function: A child thread that creates its own child thread.
void test_child_creates_child_fn(void *args) {
  int tid_child;
  printf("Child thread: I'm creating my own child thread.\n");
  tid_child = thread_create(0, child_of_child_fn);
  if(tid_child < 0) {
    printf("Child thread: Failed to create child thread!\n");
  } else {
    printf("Child thread: Created child thread with tid %d.\n", tid_child);
  }
  // Optionally wait for the child-of-child to exit.
  thread_join(&tid_child);
  printf("Child thread: My child thread %d has terminated.\n", tid_child);
  thread_exit(0);
}

// Test create two threads with no arguments.
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

// Global pointer: initially invalid.
int *dyn_mem_ptr = (int *)0xdeadbeef;

// Test thread function: Receives a pointer to a variable on main's stack,
// prints its current value, modifies it, and then prints the new value.
void child_read_write(void *arg) {
  int *ptr = (int *)arg;
  printf("Child thread: received value %d from pointer %p\n", *ptr, ptr);
  // Modify the value pointed to.
  *ptr = 999;
  printf("Child thread: updated value to %d at pointer %p\n", *ptr, ptr);
  thread_exit(0);
}


void thread_dynamic_t1(void *arg) {
  int pages = 2;
  int *base = (int *) sbrk(4096 * pages);
  if (base == (int*)-1) {
    printf("thread_dynamic_t1: sbrk failed\n");
    thread_exit(1);
  }
  dyn_mem_ptr = base;

  // Write values into the first page.
  dyn_mem_ptr[0] = 3;
  dyn_mem_ptr[1] = 2;

  // Write values into the second page.
  int offset = 4096 / sizeof(int);
  dyn_mem_ptr[offset + 0] = 5;
  dyn_mem_ptr[offset + 1] = 7;

  thread_exit(0);
}

void thread_dynamic_t2(void *arg) {
  sleep(10);  // Wait for T1 to allocate and write memory

  if (dyn_mem_ptr == (int *)0xdeadbeef) {
    printf("thread_dynamic_t2: FAIL - dyn_mem_ptr not updated\n");
    thread_exit(1);
  }

  // Verify first page.
  if (dyn_mem_ptr[0] != 3 || dyn_mem_ptr[1] != 2) {
    printf("thread_dynamic_t2: FAIL - first page values incorrect: got %d, %d\n", 
     dyn_mem_ptr[0], dyn_mem_ptr[1]);
    thread_exit(1);
  }

  // Verify second page.
  int offset = 4096 / sizeof(int);
  if (dyn_mem_ptr[offset + 0] != 5 || dyn_mem_ptr[offset + 1] != 7) {
    printf("thread_dynamic_t2: FAIL - second page values incorrect: got %d, %d\n", 
     dyn_mem_ptr[offset + 0], dyn_mem_ptr[offset + 1]);
    thread_exit(1);
  }

  printf("thread_dynamic_t2: SUCCESS - dynamic memory changes visible across threads\n");
  thread_exit(0);
}


// ===========================Above is thread Function, Below is test case===============================


void test_spoon(void){
  printf("===============Spoon & M1==============\n");
  uint64 p = 0xdeadbeef;
  spoon((void*)p);
  // thread_create(0, 0, 0);
  // thread_join(0);
  //thread_exit(0);
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
  printf( "√√√ PASED: Testing threads with no args @ M2 √√√\n" );
}


void test_threads_with_args(void){
  printf("=== Testing threads with args @ M2 ===\n");
  
  int tids[NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++){
    tids[i] = thread_create(&tids[i], test_thread_fn3);
    printf("Created thread %d\n", tids[i], i);
  }
  for(int i = 0;i < NUM_THREADS; i++){
    thread_join(&tids[i]);
  }
  printf( "Threads ");
  for(int i = 0;i < NUM_THREADS; i++){
    printf( "%d, ",tids[i]);
  }
  printf( "Exited!\n");
  printf( "√√√ PASED: Testing threads with args @ M2 √√√\n" );
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
  printf( "√√√ PASED:  Testing threads with no args and join @ M3 √√√\n" );
  return;
}


// Test: Main thread creates a child thread that in turn creates its own child.
void test_child_creates_child(void) {
  int tid;
  printf("=== Testing that a child thread can create a child thread ===\n");
  tid = thread_create(0, test_child_creates_child_fn);
  if(tid < 0) {
    printf("Main thread: Failed to create child thread for test!\n");
    return;
  } else {
    printf("Main thread: Created child thread with tid %d.\n", tid);
  }
  // Wait for the child thread (which created its own child) to finish.
  thread_join(&tid);
  printf("Main thread: Child thread %d has terminated.\n", tid);
  printf("√√√ PASED: Testing that a child thread can create a child thread √√√\n");
}

// Test: Main thread passes pointer to its own stack variable to a child thread.
// The child thread modifies the value, and after joining, main verifies the change.
void test_pass_stack_ptr(void) {
  printf("=== Testing that read write between parent and child @ M3 ===\n");
  int tid;
  int stack_val = 123; // Local (stack) variable in main.
  
  printf("Main thread: initial stack_val = %d at address %p\n", stack_val, &stack_val);
  
  // Create a child thread, passing the address of stack_val.
  tid = thread_create(&stack_val, child_read_write);
  if(tid < 0) {
    printf("Main thread: Failed to create child thread!\n");
    exit(1);
  }
  
  // Wait for the child thread to finish.
  thread_join(&tid);
  
  // After joining, print the updated value.
  printf("Main thread: after join, stack_val = %d at address %p\n", stack_val, &stack_val);
  if (stack_val==999)
  {
    printf("√√√ PASED: Testing that read write between parent and child @ M3 √√√\n");
  }
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

// Test: Forcibly exit all child threads using thread_all_exit in main
void test_thread_all_exit_by_child(void) {
  printf("=== Testing thread_all_exit: Forcibly exiting all child threads by a Child ===\n");

  // Create a few threads that run an infinite loop.
  int tid1 = thread_create(0, test_infinite_loop);
  int tid2 = thread_create(0, test_infinite_loop);
  int tid3 = thread_create(0, test_infinite_loop);
  printf("Created threads %d, %d, %d\n", tid1, tid2, tid3);

  // Allow the threads to run for a while.
  sleep(10);

  // Call thread_all_exit from the main (or parent) process to terminate all child threads.
  printf("Calling thread_all_exit() from a child process.\n");
  test_child_exit_all();

  // Join each thread to ensure they have terminated.
  thread_join(&tid1);
  thread_join(&tid2);
  thread_join(&tid3);
  
  printf("All child threads terminated by thread_all_exit().\n");
  printf("√√√ PASED: Testing thread_all_exit: Forcibly exiting all child threads by a Child √√√\n");
}

// Test: Forcibly exit all child threads using thread_all_exit in main
void test_thread_all_exit_by_main(void) {
  printf("=== Testing thread_all_exit: Forcibly exiting all child threads by Main ===\n");

  // Create a few threads that run an infinite loop.
  int tid1 = thread_create(0, test_infinite_loop);
  int tid2 = thread_create(0, test_infinite_loop);
  int tid3 = thread_create(0, test_infinite_loop);
  printf("Created threads %d, %d, %d\n", tid1, tid2, tid3);

  // Allow the threads to run for a while.
  sleep(10);

  // Call thread_all_exit from the main (or parent) process to terminate all child threads.
  printf("Calling thread_all_exit() from the main process.\n");
  thread_all_exit(1);

  // Join each thread to ensure they have terminated.
  thread_join(&tid1);
  thread_join(&tid2);
  thread_join(&tid3);
  
  printf("All child threads terminated by thread_all_exit().\n");
  printf("√√√ PASED: Testing thread_all_exit: Forcibly exiting all child threads by Main √√√\n");
}


/* Test function: Create threads T1 and T2, then join them. */
void test_dynamic_memory_changes(void) {
  printf("=== Testing dynamic memory changes across threads ===\n");
  int tid1 = thread_create(0, thread_dynamic_t1);
  int tid2 = thread_create(0, thread_dynamic_t2);
  thread_join(&tid1);
  thread_join(&tid2);
  printf("=== Dynamic memory test completed ===\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    // Test 1: Threads with no arguments.
    printf("Testing test_threads_no_args now...\n");
    test_threads_no_args();
    printf("test_threads_no_args finished\n");
    sleep(10);

  // Test 2: Threads with arguments.
    printf("Testing test_threads_with_args now...\n");
    test_threads_with_args();
    printf("test_threads_with_args finished\n");
    sleep(10);

  // Test 3: Threads with shared globals.
    printf("Testing test_threads_with_shared_globals now...\n");
    test_threads_with_shared_globals();
    printf("test_threads_with_shared_globals finished\n");
    sleep(10);

  // Test 4: Threads with no arguments and join.
    printf("Testing test_thread_no_args_join now...\n");
    test_thread_no_args_join();
    printf("test_thread_no_args_join finished\n");
    sleep(10);

  // Test 5: Threads sharing stack variables.
    printf("Testing test_threads_shared_stack_vars now...\n");
    test_threads_shared_stack_vars();
    printf("test_threads_shared_stack_vars finished\n");
    sleep(10);

  // Test 6: Forcibly exit all child threads from main.
    printf("Testing test_thread_all_exit_by_main now...\n");
    test_thread_all_exit_by_main();
    printf("test_thread_all_exit_by_main finished\n");
    sleep(10);

  // Test 7: Forcibly exit all child threads from a child thread.
    printf("Testing test_thread_all_exit_by_child now...\n");
    test_thread_all_exit_by_child();
    printf("test_thread_all_exit_by_child finished\n");
    sleep(10);

  // Test 8: Passing a pointer to a stack variable.
    printf("Testing test_pass_stack_ptr now...\n");
    test_pass_stack_ptr();
    printf("test_pass_stack_ptr finished\n");
    sleep(10);

  // Test 9: Child thread creating its own child.
    printf("Testing test_child_creates_child now...\n");
    test_child_creates_child();
    printf("test_child_creates_child finished\n");
    sleep(10);

  // Test 10: Dynamic memory changes across threads.
    printf("Testing test_dynamic_memory_changes now...\n");
    test_dynamic_memory_changes();
    printf("test_dynamic_memory_changes finished\n");

  } else {
    if (strcmp(argv[1], "noargs") == 0) {
      printf("Testing test_threads_no_args now...\n");
      test_threads_no_args();
      printf("test_threads_no_args finished\n");
    } else if (strcmp(argv[1], "withargs") == 0) {
      printf("Testing test_threads_with_args now...\n");
      test_threads_with_args();
      printf("test_threads_with_args finished\n");
    } else if (strcmp(argv[1], "withglobs") == 0) {
      printf("Testing test_threads_with_shared_globals now...\n");
      test_threads_with_shared_globals();
      printf("test_threads_with_shared_globals finished\n");
    } else if (strcmp(argv[1], "jointest") == 0) {
      printf("Testing test_thread_no_args_join now...\n");
      test_thread_no_args_join();
      printf("test_thread_no_args_join finished\n");
    } else if (strcmp(argv[1], "mainexitall") == 0) {
      printf("Testing test_thread_all_exit_by_main now...\n");
      test_thread_all_exit_by_main();
      printf("test_thread_all_exit_by_main finished\n");
    } else if (strcmp(argv[1], "childexitall") == 0) {
      printf("Testing test_thread_all_exit_by_child now...\n");
      test_thread_all_exit_by_child();
      printf("test_thread_all_exit_by_child finished\n");
    } else if (strcmp(argv[1], "withstacks") == 0) {
      printf("Testing test_threads_shared_stack_vars now...\n");
      test_threads_shared_stack_vars();
      printf("test_threads_shared_stack_vars finished\n");
    } else if (strcmp(argv[1], "simplestacks") == 0) {
      printf("Testing test_pass_stack_ptr now...\n");
      test_pass_stack_ptr();
      printf("test_pass_stack_ptr finished\n");
    } else if (strcmp(argv[1], "ccc") == 0) {
      printf("Testing test_child_creates_child now...\n");
      test_child_creates_child();
      printf("test_child_creates_child finished\n");
    } else if (strcmp(argv[1], "dynmem") == 0) {
      printf("Testing test_dynamic_memory_changes now...\n");
      test_dynamic_memory_changes();
      printf("test_dynamic_memory_changes finished\n");
    } else {
      printf("Unknown test: %s\n", argv[1]);
    }
  }
  printf("All Finished\n");
  return 0;
}
