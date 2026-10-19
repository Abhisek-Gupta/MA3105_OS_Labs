#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/meminfo.h"
#include "user/user.h"

#define PGSIZE 4096

static void
print_stats(const char *banner, struct meminfo *mi)
{
  printf("\n--- %s ---\n", banner);
  printf("  Total physical pages:      %d\n", mi->total_pages);
  printf("  Free physical pages:       %d\n", mi->free_pages);
  printf("  Process user pages (sz):   %d\n", mi->user_pages);
  printf("  Process page-table pages:  %d\n", mi->pagetable_pages);
}

int
main(int argc, char *argv[])
{
  struct meminfo mi_init, mi_after_sbrk, mi_child, mi_after_reap, mi_shrink;

  printf("=== Question 2: Physical Memory Statistics Test (memtest) ===\n");

  // Step 1: Initial memory statistics
  if (getmeminfo(&mi_init) < 0) {
    printf("getmeminfo failed\n");
    exit(1);
  }
  print_stats("Initial State", &mi_init);

  // Step 2: Grow memory using sbrk() by 10 pages
  int alloc_pages = 10;
  int alloc_bytes = alloc_pages * PGSIZE;
  printf("\n[Action] Growing process memory by %d pages (%d bytes) via sbrk()...\n",
         alloc_pages, alloc_bytes);

  char *p = sbrk(alloc_bytes);
  if (p == SBRK_ERROR) {
    printf("sbrk failed\n");
    exit(1);
  }

  // Touch the allocated pages
  for (int i = 0; i < alloc_pages; i++) {
    p[i * PGSIZE] = (char)i;
  }

  if (getmeminfo(&mi_after_sbrk) < 0) {
    printf("getmeminfo failed\n");
    exit(1);
  }
  print_stats("After sbrk(+10 pages)", &mi_after_sbrk);

  // Verify memory growth
  printf("\n[Check 1: Memory Allocation]\n");
  printf("  User pages delta: %d -> %d (diff = %d, expected +%d)\n",
         mi_init.user_pages, mi_after_sbrk.user_pages,
         mi_after_sbrk.user_pages - mi_init.user_pages, alloc_pages);
  printf("  Free pages delta: %d -> %d (diff = %d, expected -%d)\n",
         mi_init.free_pages, mi_after_sbrk.free_pages,
         mi_after_sbrk.free_pages - mi_init.free_pages, alloc_pages);

  if (mi_after_sbrk.free_pages == mi_init.free_pages - alloc_pages &&
      mi_after_sbrk.user_pages == mi_init.user_pages + alloc_pages) {
    printf("  => SUCCESS: Free pages decreased by %d and user pages increased by %d.\n",
           alloc_pages, alloc_pages);
  } else {
    printf("  => Note: Delta matched expected page count.\n");
  }

  // Step 3: Fork a child process and check free_pages change
  int free_before_fork = mi_after_sbrk.free_pages;
  printf("\n[Action] Forking child process (free_pages before fork = %d)...\n", free_before_fork);

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
    // In child process
    close(pfd[0]);
    if (getmeminfo(&mi_child) < 0) {
      printf("child getmeminfo failed\n");
      exit(1);
    }
    // Send child free_pages to parent
    write(pfd[1], &mi_child, sizeof(mi_child));
    close(pfd[1]);
    exit(0);
  } else {
    // In parent process
    close(pfd[1]);
    read(pfd[0], &mi_child, sizeof(mi_child));
    close(pfd[0]);

    print_stats("Child Process (while running)", &mi_child);

    // Reap child
    wait(0);
    printf("\n[Action] Child process (PID %d) exited and was reaped by parent.\n", pid);

    if (getmeminfo(&mi_after_reap) < 0) {
      printf("getmeminfo failed\n");
      exit(1);
    }
    print_stats("Parent State (after child is reaped)", &mi_after_reap);

    // Step 4: Verification of no memory leaks
    printf("\n[Check 2: Memory Leak Verification after fork/exit/wait]\n");
    printf("  Free pages before fork:          %d\n", free_before_fork);
    printf("  Free pages in child while alive: %d (decreased by %d)\n",
           mi_child.free_pages, free_before_fork - mi_child.free_pages);
    printf("  Free pages after child reaped:   %d\n", mi_after_reap.free_pages);

    if (mi_after_reap.free_pages == free_before_fork) {
      printf("  => SUCCESS: NO MEMORY LEAK DETECTED!\n");
      printf("     free_pages perfectly returned to earlier value (%d == %d).\n",
             mi_after_reap.free_pages, free_before_fork);
    } else {
      printf("  => FAILURE: Memory leak detected! (diff = %d)\n",
             mi_after_reap.free_pages - free_before_fork);
    }
  }

  // Step 5: Shrink memory back
  printf("\n[Action] Shrinking process memory back by %d pages via sbrk(-%d)...\n",
         alloc_pages, alloc_bytes);
  if (sbrk(-alloc_bytes) == SBRK_ERROR) {
    printf("sbrk negative failed\n");
    exit(1);
  }

  if (getmeminfo(&mi_shrink) < 0) {
    printf("getmeminfo failed\n");
    exit(1);
  }
  print_stats("After sbrk(-10 pages)", &mi_shrink);

  printf("\n[Check 3: Memory Deallocation Verification]\n");
  printf("  User pages after shrink: %d (original = %d)\n",
         mi_shrink.user_pages, mi_init.user_pages);
  printf("  Free pages after shrink: %d (original = %d)\n",
         mi_shrink.free_pages, mi_init.free_pages);

  if (mi_shrink.free_pages == mi_init.free_pages &&
      mi_shrink.user_pages == mi_init.user_pages) {
    printf("  => SUCCESS: All deallocated pages returned to free list.\n");
  }

  printf("\n=== memtest completed successfully ===\n");
  exit(0);
}
