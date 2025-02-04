#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[]) {
  uint64 p = 0xdeadbeef;

  spoon((void*)p);
  thread_create(NULL, NULL, NULL);
  thread_join(NULL);
  thread_exit(NULL);

  exit(0);
}
