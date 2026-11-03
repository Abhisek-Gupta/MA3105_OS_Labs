#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

int
main(int argc, char *argv[])
{
  printf("==================================================\n");
  printf("    QUESTION 3: PAGE PROTECTION & PAGEINFO TEST    \n");
  printf("==================================================\n\n");

  printf("[Step 1] Allocating 1 memory page with sbrk(%d)...\n", PGSIZE);
  char *p = sbrk(PGSIZE);
  if (p == (char *)-1) {
    printf("sbrk failed!\n");
    exit(1);
  }

  // Initial write before protection
  p[0] = 'A';
  p[1] = 'B';
  printf("Successfully allocated page at virtual address: %p\n", p);
  printf("Initial content written: p[0]='%c', p[1]='%c'\n\n", p[0], p[1]);

  printf("[Step 2] Displaying page-table information (should have W=1):\n");
  if (pageinfo((uint64)p) < 0) {
    printf("pageinfo failed on allocated page!\n");
    exit(1);
  }

  printf("\n[Step 3] Forking child process to test read-only protection...\n");
  int pid = fork();
  if (pid < 0) {
    printf("fork failed!\n");
    exit(1);
  }

  if (pid == 0) {
    // Child process
    printf("[Child] Changing page to read-only using setreadonly()...\n");
    if (setreadonly((uint64)p) < 0) {
      printf("[Child] setreadonly failed!\n");
      exit(1);
    }
    printf("[Child] Page successfully set to read-only.\n\n");

    printf("[Child] Displaying page-table information after setreadonly (should have W=0):\n");
    pageinfo((uint64)p);

    printf("\n[Child] Reading from protected page: p[0] = '%c' (Read OK!)\n", p[0]);
    printf("[Child] Now attempting to WRITE to protected page (*p = 'Z')...\n");
    printf("[Child] (Expect kernel page fault and process termination!)\n");

    // This write attempt MUST trigger a hardware store page fault (scause 15 / 0xf)
    *p = 'Z';

    // Should never reach here
    printf("[Child] ERROR: Write succeeded! Protection failed!\n");
    exit(1);
  } else {
    // Parent process
    int status = 0;
    wait(&status);
    printf("\n[Parent] Child process terminated with status %d.\n", status);
    printf("[Parent] Page protection successfully triggered a page fault!\n\n");

    printf("[Step 4] Testing pageinfo on unmapped address:\n");
    pageinfo(0x3ffffff000ULL);

    printf("\n[Step 5] Testing setreadonly on unmapped address:\n");
    int res = setreadonly(0x3ffffff000ULL);
    printf("setreadonly returned: %d (expected -1)\n", res);

    printf("\n==================================================\n");
    printf("          PAGETEST COMPLETED SUCCESSFULLY         \n");
    printf("==================================================\n");
  }

  exit(0);
}
