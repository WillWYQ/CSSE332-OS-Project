#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[]) {
  uint64 p = 0xdeadbeef;

  spoon((void*)p);

  exit(0);
}
