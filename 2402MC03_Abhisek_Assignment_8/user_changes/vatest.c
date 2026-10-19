#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Global variable for testing
int global_var = 42;

int
main(int argc, char *argv[])
{
  // 1. Local variable on the stack
  int local_var = 100;

  // 2. Heap block allocated with malloc
  char *heap_ptr = (char *)malloc(128);
  if (!heap_ptr) {
    printf("malloc failed\n");
    exit(1);
  }
  heap_ptr[0] = 'H';

  printf("=== Question 1: Virtual to Physical Address Translation ===\n\n");

  // Part (c): Print VA and PA of global variable, local variable, and malloc'd block
  uint64 g_va = (uint64)&global_var;
  uint64 g_pa = va2pa(g_va);
  printf("Global variable: VA = 0x%lx, PA = 0x%lx\n", g_va, g_pa);

  uint64 l_va = (uint64)&local_var;
  uint64 l_pa = va2pa(l_va);
  printf("Local variable:  VA = 0x%lx, PA = 0x%lx\n", l_va, l_pa);

  uint64 h_va = (uint64)heap_ptr;
  uint64 h_pa = va2pa(h_va);
  printf("Malloc block:    VA = 0x%lx, PA = 0x%lx\n", h_va, h_pa);

  // Show that unmapped address (0x7FFFFFFF) returns -1
  uint64 unmapped_va = 0x7FFFFFFF;
  uint64 unmapped_pa = va2pa(unmapped_va);
  printf("\nTesting unmapped address 0x%lx:\n", unmapped_va);
  printf("Unmapped VA:     VA = 0x%lx, PA = %ld\n", unmapped_va, (long)unmapped_pa);
  if ((long)unmapped_pa == -1) {
    printf("Result: Correctly returned -1 for unmapped address.\n");
  } else {
    printf("Result: Unexpected return value %ld (expected -1).\n", (long)unmapped_pa);
  }

  // Part (d): fork() and demonstrate identical VA but different PA for global variable
  printf("\n=== Fork Test: Checking Parent vs Child Mappings ===\n");
  int pfd[2];
  if (pipe(pfd) < 0) {
    printf("pipe failed\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child process
    close(pfd[0]);
    uint64 child_va = (uint64)&global_var;
    uint64 child_pa = va2pa(child_va);
    printf("[Child  PID %d] Global var: VA = 0x%lx, PA = 0x%lx\n", getpid(), child_va, child_pa);

    // Send child PA to parent for verification
    write(pfd[1], &child_pa, sizeof(child_pa));
    close(pfd[1]);
    exit(0);
  } else {
    // Parent process
    close(pfd[1]);
    uint64 parent_va = (uint64)&global_var;
    uint64 parent_pa = va2pa(parent_va);

    uint64 child_pa = 0;
    read(pfd[0], &child_pa, sizeof(child_pa));
    close(pfd[0]);
    wait(0);

    printf("[Parent PID %d] Global var: VA = 0x%lx, PA = 0x%lx\n", getpid(), parent_va, parent_pa);
    printf("\nVerification:\n");
    printf("  Virtual addresses match:   %s (Parent VA = 0x%lx, Child VA = 0x%lx)\n",
           (parent_va == g_va) ? "YES" : "NO", parent_va, g_va);
    printf("  Physical addresses differ: %s (Parent PA = 0x%lx, Child PA = 0x%lx)\n",
           (parent_pa != child_pa) ? "YES" : "NO", parent_pa, child_pa);
  }

  free(heap_ptr);
  printf("\n=== vatest completed successfully ===\n");
  exit(0);
}
