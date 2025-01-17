#include "memlib.h"
#include "mm.h"
#include <stdio.h>

int main() {
  mem_init();
  mm_init();
  int *a = (int *)mm_malloc(sizeof(int));
  *a = 4;
  printf("%d", *a);
  return 0;
}
