#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

static void
print_translation(const char *desc, uint64 va)
{
  uint64 pa = vtop(va);
  if (pa == (uint64)-1) {
    printf("  %s:\n    VA: 0x%lx -> [UNMAPPED / INVALID]\n", desc, va);
  } else {
    printf("  %s:\n    VA: 0x%lx -> PA: 0x%lx (Offset: 0x%x)\n",
           desc, va, pa, (uint)(va & (PGSIZE - 1)));
  }
}

int
main(int argc, char *argv[])
{
  int local_var = 1234;

  printf("==================================================\n");
  printf(" QUESTION 2: VIRTUAL-TO-PHYSICAL TRANSLATION TEST \n");
  printf("==================================================\n\n");

  printf("[Step 1] Translating Existing Process Segments:\n");
  print_translation("Code Segment (main)", (uint64)main);
  print_translation("Stack Variable (&local)", (uint64)&local_var);

  printf("\n[Step 2] Allocating 3 Pages with sbrk()...\n");
  char *p = sbrk(3 * PGSIZE);
  if (p == (char *)-1) {
    printf("sbrk failed!\n");
    exit(1);
  }

  // Touch the allocated pages
  p[0] = 'X';
  p[PGSIZE] = 'Y';
  p[2 * PGSIZE] = 'Z';

  printf("\n[Step 3] Translating Allocated Heap Pages:\n");
  print_translation("Heap Page 0 (Base)", (uint64)p);
  print_translation("Heap Page 0 (Offset 512)", (uint64)p + 512);
  print_translation("Heap Page 1 (Base)", (uint64)(p + PGSIZE));
  print_translation("Heap Page 1 (Offset 1024)", (uint64)(p + PGSIZE + 1024));
  print_translation("Heap Page 2 (Base)", (uint64)(p + 2 * PGSIZE));

  printf("\n[Step 4] Testing Invalid and Unmapped Virtual Addresses:\n");
  print_translation("Beyond Heap Break", (uint64)(p + 10 * PGSIZE));
  print_translation("Arbitrary High VA", 0x3ffffff000ULL);
  print_translation("VA >= MAXVA (0x8000000000)", 0x8000000000ULL);

  printf("\n[Step 5] Deallocating 3 Pages using sbrk(-%d)...\n", 3 * PGSIZE);
  sbrk(-3 * PGSIZE);

  printf("\n[Step 6] Testing Previously Allocated VA After Deallocation:\n");
  print_translation("Deallocated Heap Page 0", (uint64)p);

  printf("\n==================================================\n");
  printf("           VTTEST COMPLETED SUCCESSFULLY          \n");
  printf("==================================================\n");

  exit(0);
}
