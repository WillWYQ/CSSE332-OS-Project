#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[]) {
  uint64 p = 0xdeadbeef;

  spoon((void*)p);
  thread_create(0, 0, 0);
  thread_join(0);
  thread_exit(0);

  exit(0);
}
