#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

int
main(int argc, char *argv[])
{
  printf("==================================================\n");
  printf("     QUESTION 1: PHYSICAL MEMORY ALLOCATION TEST   \n");
  printf("==================================================\n\n");

  printf("[Step 1] Initial Physical Memory Statistics:\n");
  memstats();

  printf("[Step 2] Allocating 10 pages (%d bytes) using sbrk()...\n", 10 * PGSIZE);
  char *p = sbrk(10 * PGSIZE);
  if (p == (char *)-1) {
    printf("sbrk failed!\n");
    exit(1);
  }

  // Touch pages to ensure they are active
  for (int i = 0; i < 10; i++) {
    p[i * PGSIZE] = 'A' + i;
  }
  printf("Memory allocated at virtual address %p\n", p);

  printf("\n[Step 3] Memory Statistics After Allocating 10 Pages:\n");
  memstats();

  printf("[Step 4] Deallocating 10 pages using sbrk(-%d)...\n", 10 * PGSIZE);
  sbrk(-10 * PGSIZE);

  printf("\n[Step 5] Memory Statistics After Deallocating 10 Pages:\n");
  memstats();

  printf("[Step 6] Allocating 5 pages (%d bytes)...\n", 5 * PGSIZE);
  p = sbrk(5 * PGSIZE);
  if (p == (char *)-1) {
    printf("sbrk failed!\n");
    exit(1);
  }
  for (int i = 0; i < 5; i++) {
    p[i * PGSIZE] = 'Z' - i;
  }

  printf("\n[Step 7] Memory Statistics After Allocating 5 Pages:\n");
  memstats();

  printf("[Step 8] Releasing the 5 pages...\n");
  sbrk(-5 * PGSIZE);

  printf("\n[Step 9] Final Memory Statistics:\n");
  memstats();

  printf("==================================================\n");
  printf("           MEMTEST COMPLETED SUCCESSFULLY         \n");
  printf("==================================================\n");

  exit(0);
}
