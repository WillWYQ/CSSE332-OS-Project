#include "kernel/types.h"
#include "user/defs.h"

int main(int argc, char *argv[]) {
  uint64 p = 0xdeadbeef;

  spoon((void*)p);

  exit(0);
}