#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//TODO: create thread methods (one that accepts args two that just print whatever)


int main(int argc, char *argv[]) {
  uint64 p = 0xdeadbeef;

  spoon((void*)p);
  thread_create(0, 0, 0);
  thread_join(0);
  thread_exit(0);
//TODO: Write code that creates the threads and gives them specified functions and arguments
//sidenote if I am adding args how will that work without having shared stacks yet? (maybe using a global will work because of copied stuff stack might also work because of copied page table)

  exit(0);
}
