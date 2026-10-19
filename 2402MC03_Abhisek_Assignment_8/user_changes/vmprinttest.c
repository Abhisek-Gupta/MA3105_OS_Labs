#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("=== Question 3: Process Page Table Dump (vmprinttest) ===\n");

  printf("\n--- Test 1: Page table dump at start-up ---\n");
  vmprint();

  printf("\n--- Test 2: Growing heap with sbrk(8 * 4096) ---\n");
  char *p = sbrk(8 * 4096);
  if (p == SBRK_ERROR) {
    printf("sbrk(8 * 4096) failed\n");
    exit(1);
  }
  printf("Heap expanded by 8 pages (32768 bytes). New dump:\n");
  vmprint();

  printf("\n--- Test 3: Shrinking heap with sbrk(-4096) ---\n");
  if (sbrk(-4096) == SBRK_ERROR) {
    printf("sbrk(-4096) failed\n");
    exit(1);
  }
  printf("Heap shrunk by 1 page (4096 bytes). New dump:\n");
  vmprint();

  printf("\n=== vmprinttest completed successfully ===\n");
  exit(0);
}
