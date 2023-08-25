#include <stdio.h>

#include "mem_library.h"
#include "sfmm.h"

int main(int argc, char const *argv[]) {
  double* ptr = sf_malloc(sizeof(double));
  printf("pntr: %p\n", ptr);
  *ptr = 320320320e-320;

  sf_free(ptr);
  return EXIT_SUCCESS;
}
