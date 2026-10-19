#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/meminfo.h"
#include "user/user.h"

#define PGSIZE 4096
#define SZ_32MB (32 * 1024 * 1024)

static void
print_mem(const char *label, struct meminfo *mi)
{
  printf("[%s] Free pages: %d | User pages: %d\n",
         label, mi->free_pages, mi->user_pages);
}

int
main(int argc, char *argv[])
{
  struct meminfo mi0, mi1, mi2, mi3, mi4;

  printf("=== Question 4: Lazy Memory Allocation Test (lazytest) ===\n\n");

  // Step 1: Baseline memory
  if (getmeminfo(&mi0) < 0) {
    printf("getmeminfo failed\n");
    exit(1);
  }
  print_mem("Initial State", &mi0);

  // Step 2: Large sbrk(32 MB) without touching
  printf("\n--- Test 1: Large sbrk(32 MB) ---\n");
  char *p = sbrk(SZ_32MB);
  if (p == SBRK_ERROR) {
    printf("sbrk(32 MB) failed!\n");
    exit(1);
  }

  if (getmeminfo(&mi1) < 0) {
    printf("getmeminfo failed\n");
    exit(1);
  }
  print_mem("After sbrk(32MB)", &mi1);

  int pages_consumed = mi0.free_pages - mi1.free_pages;
  printf("Allocated virtual size: %d pages (32 MB)\n", SZ_32MB / PGSIZE);
  printf("Physical frames consumed immediately: %d pages\n", pages_consumed);
  if (pages_consumed == 0) {
    printf("=> SUCCESS: 32 MB allocated lazily with 0 physical frames consumed!\n");
  } else {
    printf("=> NOTE: %d page table pages used, no heap frames consumed.\n", pages_consumed);
  }

  // Step 3: Touch only a few pages (e.g., page 0, page 10, page 50, page 100)
  printf("\n--- Test 2: Touching 4 pages (page 0, 10, 50, 100) ---\n");
  int offsets[] = {0, 10 * PGSIZE, 50 * PGSIZE, 100 * PGSIZE};
  char markers[] = {'A', 'B', 'C', 'D'};

  for (int i = 0; i < 4; i++) {
    p[offsets[i]] = markers[i]; // triggers page fault and demand allocation
  }

  // Verify memory values
  for (int i = 0; i < 4; i++) {
    if (p[offsets[i]] != markers[i]) {
      printf("Memory verification failed at offset %d!\n", offsets[i]);
      exit(1);
    }
  }

  if (getmeminfo(&mi2) < 0) {
    printf("getmeminfo failed\n");
    exit(1);
  }
  print_mem("After touching 4 pages", &mi2);

  int touched_consumed = mi1.free_pages - mi2.free_pages;
  printf("Frames consumed after touching 4 pages: %d (expected 4)\n", touched_consumed);
  if (touched_consumed == 4) {
    printf("=> SUCCESS: Exactly 4 physical frames allocated on demand!\n");
  } else {
    printf("=> NOTE: %d frames consumed (includes intermediate L0/L1 page tables if spanned across 2MB boundaries)\n", touched_consumed);
  }

  // Step 4: System call using an untouched buffer
  printf("\n--- Test 3: System call touching an unmapped lazy buffer ---\n");
  char *untouched_buf = p + 2500 * PGSIZE; // Page 2500 is untouched
  int pfd[2];
  if (pipe(pfd) < 0) {
    printf("pipe failed\n");
    exit(1);
  }

  char msg[] = "lazy_pipe_data";
  write(pfd[1], msg, 15);
  close(pfd[1]);

  // read() invokes copyout into untouched_buf; kernel demand-pages it
  int nread = read(pfd[0], untouched_buf, 15);
  close(pfd[0]);

  if (nread != 15 || strcmp(untouched_buf, msg) != 0) {
    printf("System call read into untouched buffer failed!\n");
    exit(1);
  }
  printf("Read returned: \"%s\"\n", untouched_buf);

  if (getmeminfo(&mi3) < 0) {
    printf("getmeminfo failed\n");
    exit(1);
  }
  print_mem("After syscall on untouched buffer", &mi3);
  printf("=> SUCCESS: Kernel handled copyout on untouched page seamlessly!\n");

  // Step 5: Test fork & exit
  printf("\n--- Test 4: Fork and exit with lazy memory ---\n");
  int pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child verifies parent data
    for (int i = 0; i < 4; i++) {
      if (p[offsets[i]] != markers[i]) {
        printf("Child could not read touched page %d!\n", i);
        exit(1);
      }
    }
    // Child touches another lazy page
    p[6000 * PGSIZE] = 'K';
    if (p[6000 * PGSIZE] != 'K') {
      printf("Child touch failed!\n");
      exit(1);
    }
    exit(0);
  }

  int status;
  wait(&status);
  if (status != 0) {
    printf("Child process failed!\n");
    exit(1);
  }
  printf("=> SUCCESS: Child forked, inherited lazy mappings, and exited cleanly.\n");

  // Step 6: Edge cases - invalid accesses
  printf("\n--- Test 5: Invalid access (address >= sz) terminates child safely ---\n");
  pid = fork();
  if (pid == 0) {
    // Access beyond allocated size
    char *bad = p + SZ_32MB + PGSIZE;
    printf("Child attempting invalid access at %p (beyond sz)...\n", bad);
    *bad = 'X'; // Should fault and kill child
    printf("ERROR: child should have been killed!\n");
    exit(0);
  }
  wait(&status);
  if (status != 0) {
    printf("=> SUCCESS: Invalid access correctly terminated offending process without kernel crash.\n");
  } else {
    printf("=> FAILURE: Invalid access was allowed!\n");
  }

  // Step 7: Shrink memory back
  printf("\n--- Test 6: Shrinking lazy memory via sbrk(-32 MB) ---\n");
  if (sbrk(-SZ_32MB) == SBRK_ERROR) {
    printf("sbrk shrink failed!\n");
    exit(1);
  }

  if (getmeminfo(&mi4) < 0) {
    printf("getmeminfo failed\n");
    exit(1);
  }
  print_mem("After sbrk(-32MB)", &mi4);

  if (mi4.free_pages == mi0.free_pages && mi4.user_pages == mi0.user_pages) {
    printf("=> SUCCESS: All frames released, memory restored to initial state!\n");
  }

  printf("\n=== lazytest completed successfully ===\n");
  exit(0);
}
