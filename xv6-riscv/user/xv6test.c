#include "kernel/types.h"
#include "user/user.h"
#include "kernel/defs.h"
int main(int argc, char *argv[]) {
  uint64 p = 0xdeadbeef;

  spoon((void*)p);

  exit(0);
}
